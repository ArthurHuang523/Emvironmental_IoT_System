#include "main.h"
#include "tasks.h"
#include "hardware.h"

// External global variable declarations - added new sensor variables
extern float g_temperature;
extern float g_humidity;
extern float g_smoke_voltage;
extern uint32_t g_smoke_adc;
extern uint8_t g_smoke_alarm;
extern uint8_t g_dht11_ok;
extern float g_smoke_ppm;
extern float g_air_quality_voltage;
extern uint32_t g_air_quality_adc;
extern uint8_t g_air_quality_alarm;
extern uint16_t g_air_quality_ppm;
extern float g_light_voltage;
extern uint32_t g_light_adc;
extern uint8_t g_light_level;
extern uint16_t g_light_lux;

uint8_t startflag;

/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
void StartDisplayTask(void const * argument)
{
    // OLED Init
		if(osMutexWait(OledMutexHandle, 1000) == osOK)
    {
        OLED_Init();
        OLED_Clear();
        OLED_ShowString(15, 5, (uint8_t*)"System Starting...");
        OLED_ShowString(0, 1, (uint8_t*)"Please Wait...");
        osMutexRelease(OledMutexHandle);
    }
		// Display warm-up time
    osDelay(5000);

    for(;;)
    {
        float temp, humi, smoke_ppm;
        uint8_t dht11_ok;
        uint16_t air_ppm, light_lux;
        uint8_t wifi_status, mqtt_status, power_save;
        uint8_t smoke_alarm, air_alarm;
        char* state_string;
				
				// Watchdog Heartbeat Report
        Watchdog_Task_Heartbeat(TASK_ID_DISPLAY);
				
				// extract sensor data
        if(osMutexWait(sensorDataMutexHandle, 100) == osOK)
        {
            temp = g_temperature;
            humi = g_humidity;
            smoke_ppm = g_smoke_ppm;
            dht11_ok = g_dht11_ok;
            air_ppm = g_air_quality_ppm;
            light_lux = g_light_lux;
            wifi_status = g_wifi_connected;
            mqtt_status = g_mqtt_connected;
            power_save = g_power_save_mode;
            smoke_alarm = g_smoke_alarm;
            air_alarm = g_air_quality_alarm;
            osMutexRelease(sensorDataMutexHandle);
        }
        else
        {
            osDelay(1000);
            continue;
        }

        if(osMutexWait(OledMutexHandle, 1000) == osOK)
        {
						// power save mode
            if(power_save)
            {
                OLED_Clear();
                OLED_ShowString(20, 4, (uint8_t*)"Power Save Mode");
                osDelay(2000);
                OLED_Display_Off();
            }
						// alarm mode
            else if(smoke_alarm || air_alarm)
            {
								OLED_Display_On();	
								OLED_Clear();
                if(smoke_alarm)
                {
                    OLED_ShowString(35, 2, (uint8_t*)"SMOKE ALARM!");
                }
                if(air_alarm)
                {
                    OLED_ShowString(40, 3, (uint8_t*)"AIR ALARM!");
                }
            }
						// normal mode
            else
            {
                OLED_Display_On();
                OLED_Clear_Line(0);
								// Row 0 show temperature
                OLED_ShowString(0, 0, (uint8_t*)"Temp:");
                if(dht11_ok)
                {
                    OLED_ShowFloat(50, 0, temp, 2, 1);
                    OLED_ShowChar(80, 0, 'C');
                }
                else
                {
                    OLED_ShowString(50, 0, (uint8_t*)"-.-");
                }
								// Row 1 show humidity
                OLED_Clear_Line(1);
                OLED_ShowString(0, 1, (uint8_t*)"Humi:");
                if(dht11_ok)
                {
                    OLED_ShowFloat(50, 1, humi, 2, 1);
                    OLED_ShowChar(80, 1, '%');
                }
                else
                {
                    OLED_ShowString(50, 1, (uint8_t*)"-.-%");
                }
								// Row 2 show smoke ppm
                OLED_Clear_Line(2);
                OLED_ShowString(0, 2, (uint8_t*)"Smoke:");
                OLED_ShowNum(50, 2, (uint32_t)smoke_ppm, 4);
                OLED_ShowString(80, 2, (uint8_t*)"ppm");
								// Row 3 show air ppm
                OLED_Clear_Line(3);
                OLED_ShowString(0, 3, (uint8_t*)"Air:");
                OLED_ShowNum(50, 3, air_ppm, 4);
                OLED_ShowString(80, 3, (uint8_t*)"ppm");
								// Row 4 show light lux
                OLED_Clear_Line(4);
                OLED_ShowString(0, 4, (uint8_t*)"Light:");
                OLED_ShowNum(50, 4, light_lux, 4);
                OLED_ShowString(80, 4, (uint8_t*)"lux");
								// Row 5 show WIFI state
                OLED_Clear_Line(5);
                OLED_ShowString(0, 5, (uint8_t*)"WiFi:");
                if(wifi_status)
                {
                    OLED_ShowString(50, 5, (uint8_t*)"Connected");
                } 
								else
                {
                    OLED_ShowString(50, 5, (uint8_t*)"Disconnected");
                }
								// Row 6 show MQTT state
                OLED_Clear_Line(6);
                OLED_ShowString(0, 6, (uint8_t*)"MQTT:");
                if(mqtt_status)
                {
                    OLED_ShowString(50, 6, (uint8_t*)"Connected");
                }
                else
                {
                    OLED_ShowString(50, 6, (uint8_t*)"Disconnected");
                }
								// Row 7 show Network state
                state_string = Network_GetStateString();
                OLED_Clear_Line(7);
                OLED_ShowString(0, 7, (uint8_t*)"STATE:");
                OLED_ShowString(50, 7, (uint8_t*)state_string);
            }
            osMutexRelease(OledMutexHandle);
        }
        osDelay(8000);
    }
}
