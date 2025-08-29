#ifndef TASKS_H
#define TASKS_H

#include "main.h"
#include "cmsis_os.h"

// Task handles
extern osThreadId DefaultTaskHandle;
extern osThreadId SensorTaskHandle;
extern osThreadId DisplayTaskHandle;
extern osThreadId MqttTaskHandle;
extern osThreadId OTA_TaskHandle;

// Mutex handles
extern osMutexId sensorDataMutexHandle;
extern osMutexId OledMutexHandle;
extern osMutexId ESP8266MutexHandle;

// 传感器错误计数器
extern uint8_t g_dht11_error_count;
extern uint8_t g_mq2_error_count;
extern uint8_t g_mq135_error_count;
extern uint8_t g_ldr_error_count;

// Task function prototypes
void StartDefaultTask(void const * argument);
void StartSensorTask(void const * argument);
void StartDisplayTask(void const * argument);
void StartMQTTTask(void const * argument);
void StartOTATask(void const * argument);

extern uint8_t g_power_save_mode;

extern uint8_t g_wifi_connected;
extern uint8_t g_mqtt_connected;

// Global sensor data variables
extern float g_temperature;
extern float g_humidity;
extern uint8_t g_dht11_ok;

extern float g_smoke_voltage;
extern uint32_t g_smoke_adc;
extern uint8_t g_smoke_alarm;
extern float g_smoke_ppm;

extern float g_air_quality_voltage;
extern uint32_t g_air_quality_adc;
extern uint8_t g_air_quality_alarm;
extern uint16_t g_air_quality_ppm;

extern float g_light_voltage;
extern uint32_t g_light_adc;
extern uint8_t g_light_level;
extern uint16_t g_light_lux;

// Network state enum
typedef enum {
    NET_INIT ,
    NET_ESP_INIT,
    NET_WIFI_CONNECTING,
    NET_WIFI_CONNECTED,
    NET_MQTT_CONNECTING,
    NET_MQTT_CONNECTED,
    NET_ERROR
} Network_State_t;

// Network functions
void Network_HandleOperation(uint32_t current_time);
void Network_SendSensorData(void);
void Network_SendStatusInfo(void);
char* Network_GetStateString(void);
void Network_SendAlarm(char* alarm_type, char* message);
void Network_CheckSensorFaults(void);
	
#endif /* TASKS_H */
