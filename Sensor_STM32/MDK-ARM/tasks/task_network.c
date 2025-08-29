#include "main.h"
#include "tasks.h"
#include "hardware.h"
#include <stdio.h>

// WiFi configuration
#define WIFI_SSID       "HXH"
#define WIFI_PASSWORD   "Hxh20010523"

// MQTT configuration
#define MQTT_SERVER     "192.168.137.34"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "STM32_Sensor_001"
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""

// MQTT topics
#define MQTT_TOPIC_DATA     "sensor/data"
#define MQTT_TOPIC_STATUS   "sensor/status"
#define MQTT_TOPIC_CONTROL  "sensor/control"

// Network state Define
typedef enum {
    STATE_INIT = 0,
    STATE_ESP_INIT,
    STATE_WIFI_CONNECT,
    STATE_MQTT_CONNECT,
    STATE_RUNNING,
    STATE_ERROR
} simple_state_t;

static simple_state_t current_state = STATE_INIT;
static uint32_t last_data_send = 0;
static uint32_t error_time = 0;

void StartMQTTTask(void const * argument)
{
    //Wait for system to be stable
    osDelay(7000);

    for(;;)
    {
        uint32_t now = HAL_GetTick();
        // Watchdog Heartbeat Report
        Watchdog_Task_Heartbeat(TASK_ID_MQTT);

        if(osMutexWait(ESP8266MutexHandle, 100) == osOK)
        {
            // Excute FSM
            switch(current_state)
            {
            // Init
            case STATE_INIT:
            {
                current_state = STATE_ESP_INIT;
                break;
            }
            // ESP8266 Init
            case STATE_ESP_INIT:
            {
                if(ESP8266_Init() == ESP8266_OK)
                {
                    current_state = STATE_WIFI_CONNECT;
                }
                else
                {
                    current_state = STATE_ERROR;
                    error_time = now;
                }
                break;
            }
            // WiFi Connect
            case STATE_WIFI_CONNECT:
            {
                if(ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD) == ESP8266_OK)
                {
                    current_state = STATE_MQTT_CONNECT;
                    g_wifi_connected = 1;
                }
                else
                {
                    current_state = STATE_ERROR;
                    error_time = now;
                    g_wifi_connected = 0;
                }
                break;
            }
            // MQTT Connect
            case STATE_MQTT_CONNECT:
            {
                if(ESP8266_ConnectMQTT(MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD) == ESP8266_OK)
                {
                    current_state = STATE_RUNNING;
                    g_mqtt_connected = 1;
                    // Subscribe Topic and send status notification
                    ESP8266_SubscribeMQTT(MQTT_TOPIC_CONTROL, 0);
                    Network_SendStatusInfo();
                    last_data_send = now;
                }
                else
                {
                    current_state = STATE_ERROR;
                    error_time = now;
                    g_mqtt_connected = 0;
                }
                break;
            }
            // Running mode
            case STATE_RUNNING:
            {
                g_wifi_connected = 1;
                g_mqtt_connected = 1;

                // Send Interval Set, actually normal mode:10-12s, power save mode:20-22s
                uint32_t send_interval = g_power_save_mode ? 18500 : 8500;
                if(now - last_data_send >= send_interval)
                {
                    // send data to server
                    Network_SendStatusInfo();
                    Network_SendSensorData();
                    Network_CheckSensorFaults();
                    last_data_send = now;
                }

                static uint32_t last_check = 0;
                // Check WifI State every minute
                if(now - last_check >= 60000)
                {
                    if(ESP8266_GetWiFiStatus() != WIFI_CONNECTED)
                    {
                        current_state = STATE_WIFI_CONNECT;
                        error_time = now;
                        g_wifi_connected = 0;
                        g_mqtt_connected = 0;
                    }
                    else if(ESP8266_GetMQTTStatus() != MQTT_CONNECTED)
                    {
                        current_state = STATE_MQTT_CONNECT;  
                        g_mqtt_connected = 0;
                    }
                    last_check = now;
                }
                break;
            }
            // restart network connection
            case STATE_ERROR:
            {
                g_wifi_connected = 0;
                g_mqtt_connected = 0;
                if(now - error_time >= 10000)
                {
                    current_state = STATE_INIT;
                }
                break;
            }
            }
            osMutexRelease(ESP8266MutexHandle);
        }
        // osDelay
        switch(current_state)
        {
        case STATE_INIT:
        case STATE_ESP_INIT:
            osDelay(2000);
            break;

        case STATE_WIFI_CONNECT:
        case STATE_MQTT_CONNECT:
            osDelay(3000);
            break;

        case STATE_RUNNING:
            osDelay(5000);
            break;

        case STATE_ERROR:
            osDelay(10000);
            break;

        default:
            osDelay(2000);
            break;
        }
    }
}


/**
  * @brief Send sensor data
  */
void Network_SendSensorData(void)
{
    float temp, humi, smoke_ppm;
    uint16_t air_ppm, light_lux;
    uint8_t smoke_alarm, air_alarm, device_alarm;

    // extract sensor data
    if(osMutexWait(sensorDataMutexHandle, 300) == osOK)
    {
        temp = g_temperature;
        humi = g_humidity;
        smoke_ppm = g_smoke_ppm;
        smoke_alarm = g_smoke_alarm;
        air_ppm = g_air_quality_ppm;
        air_alarm = g_air_quality_alarm;
        light_lux = g_light_lux;
        // overall device alarm status
        device_alarm = smoke_alarm || air_alarm;
        osMutexRelease(sensorDataMutexHandle);

        // Send data to server
        ESP8266_SendSensorData(temp, humi, smoke_ppm, air_ppm, light_lux, device_alarm);
    }
}

/**
  * @brief Send status data
  */
void Network_SendStatusInfo(void)
{
    char status_msg[100];
    unsigned long uptime = HAL_GetTick() / 1000;
    sprintf(status_msg, "AT+MQTTPUB=0,\"sensor/status\",\"Status:online_Device:%s_Updatetime:%lu\",0,0", MQTT_CLIENT_ID, uptime);
    ESP8266_SendCommandWithResponse(status_msg, 5000);
}

/**
  * @brief Check all sensor fault states and send report
  */
void Network_CheckSensorFaults(void)
{
    char fault_msg[200];
    uint8_t fault_count = 0;

    // Count faulty sensors
    if(g_dht11_error_count >= 5) fault_count++;
    if(g_mq2_error_count >= 5) fault_count++;
    if(g_mq135_error_count >= 5) fault_count++;
    if(g_ldr_error_count >= 5) fault_count++;
    if(fault_count > 0)
    {
        sprintf(fault_msg, "FAULTS:%d_DHT11:%s_MQ2:%s_MQ135:%s_LDR:%s",
                fault_count,
                (g_dht11_error_count >= 5) ? "FAULT" : "OK",
                (g_mq2_error_count >= 5) ? "FAULT" : "OK",
                (g_mq135_error_count >= 5) ? "FAULT" : "OK",
                (g_ldr_error_count >= 5) ? "FAULT" : "OK");
    }
    else
    {
        sprintf(fault_msg, "ALL_OK_DHT11:%d_MQ2:%d_MQ135:%d_LDR:%d",
                g_dht11_error_count, g_mq2_error_count,
                g_mq135_error_count, g_ldr_error_count);
    }
    // Send fault report
    ESP8266_PublishMQTT("sensor/fault", fault_msg, 0, 0);
}

/**
  * @brief Convert network state to string format
  */
char* Network_GetStateString(void)
{
    switch(current_state)
    {
    case STATE_INIT:
        return "INIT";
    case STATE_ESP_INIT:
        return "ESP_INIT";
    case STATE_WIFI_CONNECT:
        return "WIFI_CONN";
    case STATE_MQTT_CONNECT:
        return "MQTT_CONN";
    case STATE_RUNNING:
        return "RUNNING";
    case STATE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
