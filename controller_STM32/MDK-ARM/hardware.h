/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : hardware.h
  * @brief          : Hardware initialization and ESP8266 configuration
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __HARDWARE_H__
#define __HARDWARE_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief ESP8266 Status enumeration
 */
typedef enum {
    ESP8266_OK = 0,                 /*!< Operation completed successfully */
    ESP8266_ERROR,                  /*!< General error occurred */
    ESP8266_TIMEOUT_ERROR,          /*!< Timeout error occurred */
    ESP8266_BUSY,                   /*!< Device is busy */
    ESP8266_INVALID_PARAM,          /*!< Invalid parameter */
    ESP8266_NOT_CONNECTED,          /*!< Device not connected */
    ESP8266_MEMORY_ERROR            /*!< Memory allocation error */
} ESP8266_Status_t;

/**
 * @brief WiFi connection status
 */
typedef enum {
    WIFI_DISCONNECTED = 0,          /*!< WiFi is disconnected */
    WIFI_CONNECTING,                /*!< WiFi is connecting */
    WIFI_CONNECTED,                 /*!< WiFi is connected */
    WIFI_ERROR                      /*!< WiFi connection error */
} WiFi_Status_t;

/**
 * @brief MQTT connection status
 */
typedef enum {
    MQTT_DISCONNECTED = 0,          /*!< MQTT is disconnected */
    MQTT_CONNECTING,                /*!< MQTT is connecting */
    MQTT_CONNECTED,                 /*!< MQTT is connected */
    MQTT_ERROR                      /*!< MQTT connection error */
} MQTT_Status_t;

/* Exported constants --------------------------------------------------------*/

// GPIO Pin Definitions - Motor Control
#define AIN1_Pin                    GPIO_PIN_0
#define AIN2_Pin                    GPIO_PIN_1  
#define STBY_Pin                    GPIO_PIN_4


// UART Configuration
#define DEBUG_UART                  huart1          /*!< Debug UART interface */
#define ESP8266_UART                huart2          /*!< ESP8266 UART interface */

// Buffer Sizes
#define DEBUG_BUFFER_SIZE           256             /*!< Debug buffer size */
#define ESP_MAX_SSID_LEN            32              /*!< Maximum SSID length */
#define ESP_MAX_PASS_LEN            64              /*!< Maximum password length */
#define ESP_MAX_HOST_LEN            64              /*!< Maximum hostname length */
#define ESP_MAX_CLIENT_ID_LEN       32              /*!< Maximum client ID length */
#define ESP_MAX_TOPIC_LEN           64              /*!< Maximum MQTT topic length */
#define ESP_MAX_MESSAGE_LEN         256             /*!< Maximum MQTT message length */

// Default Configuration Values
#define ESP_DEFAULT_BAUDRATE        115200          /*!< Default ESP8266 baud rate */
#define MQTT_DEFAULT_PORT           1883            /*!< Default MQTT port */
#define MQTT_DEFAULT_QOS            0               /*!< Default MQTT QoS */
#define MQTT_DEFAULT_RETAIN         0               /*!< Default MQTT retain flag */

/* Exported variables --------------------------------------------------------*/
extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* Exported functions prototypes ---------------------------------------------*/

// Hardware initialization functions
void Hardware_Init(void);
void SystemClock_Config(void);
void Error_Handler(void);

// GPIO functions
void MX_GPIO_Init(void);

// UART functions  
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

// Timer functions
void MX_TIM4_Init(void);

// Watchdog functions
void MX_IWDG_Init(void);

// Debug functions
void debug_printf(const char* format, ...);

// ESP8266 Core Functions
ESP8266_Status_t ESP8266_Init(void);
ESP8266_Status_t ESP8266_SendCommand(const char* cmd, uint32_t timeout);
char* ESP8266_GetBuffer(void);
void ESP8266_ClearBuffer(void);

// ESP8266 WiFi Functions
ESP8266_Status_t ESP8266_ConnectWiFi(char* ssid, char* password);
ESP8266_Status_t ESP8266_DisconnectWiFi(void);
WiFi_Status_t ESP8266_GetWiFiStatus(void);

// ESP8266 MQTT Functions
ESP8266_Status_t ESP8266_ConnectMQTT(char* server, uint16_t port, char* client_id, char* username, char* password);
ESP8266_Status_t ESP8266_DisconnectMQTT(void);
ESP8266_Status_t ESP8266_PublishMQTT(char* topic, char* message, uint8_t qos, uint8_t retain);
ESP8266_Status_t ESP8266_SubscribeMQTT(char* topic, uint8_t qos);
MQTT_Status_t ESP8266_GetMQTTStatus(void);


#endif /* __HARDWARE_H__ */
