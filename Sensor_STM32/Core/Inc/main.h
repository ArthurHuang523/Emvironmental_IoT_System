/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ESP8266_UART_TX_Pin GPIO_PIN_2
#define ESP8266_UART_TX_GPIO_Port GPIOA
#define ESP8266_UART_RX_Pin GPIO_PIN_3
#define ESP8266_UART_RX_GPIO_Port GPIOA
#define W25Q64_CS_Pin GPIO_PIN_4
#define W25Q64_CS_GPIO_Port GPIOA
#define DHT11_DATA_Pin GPIO_PIN_0
#define DHT11_DATA_GPIO_Port GPIOB
#define MQ2_DO_Pin GPIO_PIN_1
#define MQ2_DO_GPIO_Port GPIOB
#define ESP8266_RST_Pin GPIO_PIN_8
#define ESP8266_RST_GPIO_Port GPIOA
#define MQ135_DO_Pin GPIO_PIN_3
#define MQ135_DO_GPIO_Port GPIOB
#define LDR_DO_Pin GPIO_PIN_4
#define LDR_DO_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */
void Error_Handler(void);
void SystemClock_Config(void);
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
