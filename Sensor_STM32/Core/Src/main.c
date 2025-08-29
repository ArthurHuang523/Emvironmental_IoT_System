/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

#include "main.h"
#include "cmsis_os.h"
#include "hardware.h"
#include "tasks.h"

// Task and Mutex Defination
osThreadId DefaultTaskHandle;
osThreadId SensorTaskHandle;
osThreadId DisplayTaskHandle;
osThreadId MqttTaskHandle;
osThreadId OTA_TaskHandle;
osMutexId sensorDataMutexHandle;
osMutexId OledMutexHandle;
osMutexId ESP8266MutexHandle;

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    // system init
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_CRC_Init();
    OLED_Init();

    /* Create the mutex(es) */
    /* definition and creation of sensorDataMutex */
    osMutexDef(sensorDataMutex);
    sensorDataMutexHandle = osMutexCreate(osMutex(sensorDataMutex));

    /* definition and creation of OledMutex */
    osMutexDef(OledMutex);
    OledMutexHandle = osMutexCreate(osMutex(OledMutex));

    /* definition and creation of ESP8266Mutex */
    osMutexDef(ESP8266Mutex);
    ESP8266MutexHandle = osMutexCreate(osMutex(ESP8266Mutex));


    /* Create the thread(s) */
    /* definition and creation of DefaultTask */
    osThreadDef(DefaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 256);
    DefaultTaskHandle = osThreadCreate(osThread(DefaultTask), NULL);

    /* definition and creation of SensorTask */
    osThreadDef(SensorTask, StartSensorTask, osPriorityHigh, 0, 512);
    SensorTaskHandle = osThreadCreate(osThread(SensorTask), NULL);

    /* definition and creation of DisplayTask */
    osThreadDef(DisplayTask, StartDisplayTask, osPriorityNormal, 0, 512);
    DisplayTaskHandle = osThreadCreate(osThread(DisplayTask), NULL);

    /* definition and creation of MqttTask */
    osThreadDef(MqttTask, StartMQTTTask, osPriorityNormal, 0, 1024);
    MqttTaskHandle = osThreadCreate(osThread(MqttTask), NULL);

    /* definition and creation of OTA_Task */
    //osThreadDef(OTA_Task, StartOTATask, osPriorityRealtime, 0, 896);
    //OTA_TaskHandle = osThreadCreate(osThread(OTA_Task), NULL);

    /* Start scheduler */
    osKernelStart();
    while (1)
    {

    }
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
