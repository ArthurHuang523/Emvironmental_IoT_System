#ifndef HARDWARE_H
#define HARDWARE_H

#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_iwdg.h"

#include <stdio.h>   // For snprintf function

// ADC channel
#define ADC_CHANNEL_MQ2         ADC_CHANNEL_0    // PA0
#define ADC_CHANNEL_MQ135       ADC_CHANNEL_1    // PA1  
#define ADC_CHANNEL_LDR         ADC_CHANNEL_5    // PA5

// Hardware handles
extern ADC_HandleTypeDef hadc1;
extern CRC_HandleTypeDef hcrc;
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern IWDG_HandleTypeDef hiwdg;

// Function prototypes
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_I2C1_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_CRC_Init(void);

// ADC channel selection function
void ADC_SelectChannel(uint32_t channel);

// MQ2 sensor functions
uint32_t MQ2_Read_Raw(void);
float MQ2_Read_Voltage(void);
float MQ2_Read_Average(uint8_t samples);
float MQ2_Read_PPM(void);
uint8_t MQ2_Smoke_Detect(float threshold_voltage);
uint8_t MQ2_Digital_Read(void);

// MQ-135 air quality sensor functions
uint32_t MQ135_Read_Raw(void);
float MQ135_Read_Voltage(void);
float MQ135_Read_Average(uint8_t samples);
uint8_t MQ135_Air_Quality_Detect(float threshold_voltage);
uint16_t MQ135_Calculate_PPM(float voltage);
uint8_t MQ135_Digital_Read(void);

// Photoresistor sensor functions
uint32_t LDR_Read_Raw(void);
float LDR_Read_Voltage(void);
float LDR_Read_Average(uint8_t samples);
uint8_t LDR_Get_Light_Level(float voltage);
uint16_t LDR_Calculate_Lux(float voltage);
uint8_t LDR_Digital_Read(void);

// Light level enum
typedef enum {
    LIGHT_DARK = 0,     // Dark
    LIGHT_MEDIUM = 1,   // Medium light
    LIGHT_BRIGHT = 2    // Bright
} LightLevel_t;

// DHT11 sensor functions
uint8_t DHT11_Read_Data(float *temperature, float *humidity);
float DHT11_Read_Temperature(void);
float DHT11_Read_Humidity(void);
uint8_t DHT11_Check_Sensor(void);

// OLED display functions
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size);
void OLED_Clear_Line(uint8_t line);

// ESP8266 WiFi and MQTT functions
typedef enum {
    ESP8266_OK = 0,
    ESP8266_ERROR,
    ESP8266_TIMEOUT_ERROR,
    ESP8266_BUSY
} ESP8266_Status_t;

typedef enum {
    WIFI_DISCONNECTED = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED
} WiFi_Status_t;

typedef enum {
    MQTT_DISCONNECTED = 0,
    MQTT_CONNECTING,
    MQTT_CONNECTED
} MQTT_Status_t;

// ESP8266 basic functions
void ESP8266_Reset(void);
void ESP8266_SendCommand(char* command);
ESP8266_Status_t ESP8266_ReceiveResponse(uint32_t timeout);
ESP8266_Status_t ESP8266_SendCommandWithResponse(char* command, uint32_t timeout);
ESP8266_Status_t ESP8266_Init(void);
char* ESP8266_GetBuffer(void);
void ESP8266_ClearBuffer(void);

// WiFi related functions
ESP8266_Status_t ESP8266_ConnectWiFi(char* ssid, char* password);
ESP8266_Status_t ESP8266_DisconnectWiFi(void);
WiFi_Status_t ESP8266_GetWiFiStatus(void);
ESP8266_Status_t ESP8266_GetWiFiInfo(char* ssid_buffer, char* ip_buffer);
ESP8266_Status_t ESP8266_ScanWiFi(void);

// MQTT related functions
ESP8266_Status_t ESP8266_ConnectMQTT(char* server, uint16_t port, char* client_id, char* username, char* password);
ESP8266_Status_t ESP8266_PublishMQTT(char* topic, char* message, uint8_t qos, uint8_t retain);
ESP8266_Status_t ESP8266_SubscribeMQTT(char* topic, uint8_t qos);
ESP8266_Status_t ESP8266_UnsubscribeMQTT(char* topic);
ESP8266_Status_t ESP8266_DisconnectMQTT(void);
MQTT_Status_t ESP8266_GetMQTTStatus(void);
ESP8266_Status_t ESP8266_ReceiveMQTT(char* topic_buffer, char* message_buffer);

// Application layer function
ESP8266_Status_t ESP8266_SendSensorData(float temperature, float humidity, 
                                       float smoke_ppm, uint16_t air_quality_ppm, 
                                       uint16_t light_level, uint8_t device_alarm);

// ===================  Watchdog-related definitions  ===================
// Watchdog handle declaration

// Task ID Defination
#define TASK_ID_SENSOR    0
#define TASK_ID_DISPLAY   1
#define TASK_ID_MQTT      2
#define TASK_ID_DEFAULT   3
#define MAX_TASKS         4

// watchdog defination
HAL_StatusTypeDef Watchdog_Init(uint32_t timeout_ms);
void Watchdog_Feed(void);
void Watchdog_Task_Heartbeat(uint8_t task_id);
uint8_t Watchdog_Check_All_Tasks(void);
void Watchdog_Get_Status(char* buffer, uint16_t size);

#endif /* HARDWARE_H */
