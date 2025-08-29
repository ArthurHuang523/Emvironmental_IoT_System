#include "main.h"
#include "hardware.h"

ADC_HandleTypeDef hadc1;

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE; //Single-channel
    hadc1.Init.ContinuousConvMode = DISABLE;	//single conversion
    hadc1.Init.DiscontinuousConvMode = DISABLE;	//normmal mode
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure Channel 0 - MQ-2 (PA0)
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure Channel 1 - MQ-135 (PA1)
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure Channel 5 - photoresistor (PA5)
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief ADC channel selection function
  * @param channel channel number
  * @retval None
  */
void ADC_SelectChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

// ==================== MQ-2 Smoke Sensor Functions ====================
/**
  * @brief Read raw ADC value of MQ2 sensor
  * @param None
  * @retval uint32_t raw ADC value (0-4095)
  */
uint32_t MQ2_Read_Raw(void)
{
    uint32_t adc_value = 0;
    // MQ-2 ADC channel 0 (PA0)
    ADC_SelectChannel(ADC_CHANNEL_0);
    HAL_Delay(10);
    // Start ADC conversion
    if (HAL_ADC_Start(&hadc1) == HAL_OK)
    {
        // Wait for conversion to complete
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
        {
            // Read conversion result
            adc_value = HAL_ADC_GetValue(&hadc1);
        }
        // Stop ADC
        HAL_ADC_Stop(&hadc1);
    }

    return adc_value;
}
/**
  * @brief Read MQ2 sensor voltage
  * @param None
  * @retval float voltage value (V)
  */
float MQ2_Read_Voltage(void)
{
    uint32_t adc_raw = MQ2_Read_Raw();
    float voltage = (adc_raw * 3.3f) / 4095.0f;
    return voltage;
}

/**
  * @brief Read average voltage of MQ2 sensor over multiple samples
  * @param samples number of samples
  * @retval float average voltage (V)
  */
float MQ2_Read_Average(uint8_t samples)
{
    if (samples == 0) samples = 1;

    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += MQ2_Read_Raw();
        // Sampling interval: 10ms
        HAL_Delay(10);
    }

    float average_voltage = ((float)sum / samples * 3.3f) / 4095.0f;
    return average_voltage;
}

/**
  * @brief Read MQ2 gas concentration (simplified algorithm)
  * @param None
  * @retval float gas concentration (ppm)
  * @note This is a simplified linear conversion. Actual application requires calibration based on sensor characteristics.
  */
float MQ2_Read_PPM(void)
{
    float voltage = MQ2_Read_Average(5);

    // 0.1V = 300ppm, each additional 0.1V adds 200ppm
    float ppm = 300.0f + (voltage - 0.1f) * 2000.0f;

    // Clamp to valid range
    if (ppm < 300) ppm = 300;
    if (ppm > 6000) ppm = 6000;

    return ppm;
}

/**
  * @brief Check for smoke alarm condition
  * @param threshold_voltage threshold voltage for alarm
  * @retval uint8_t 1=Smoke detected, 0=Normal
  */
uint8_t MQ2_Smoke_Detect(float threshold_voltage)
{
    float current_voltage = MQ2_Read_Average(3);
    return (current_voltage > threshold_voltage) ? 1 : 0;
}

/**
  * @brief Get MQ2 digital output state
  * @param None
  * @retval uint8_t 1=Smoke detected, 0=No smoke
  */
uint8_t MQ2_Digital_Read(void)
{
    return HAL_GPIO_ReadPin(MQ2_DO_GPIO_Port, MQ2_DO_Pin);
}

// ==================== MQ-135 Air Quality Sensor Functions ====================

/**
  * @brief Read raw ADC value from MQ-135 sensor
  * @param None
  * @retval uint32_t raw ADC value (0-4095)
  */
uint32_t MQ135_Read_Raw(void)
{
    uint32_t adc_value = 0;

    // Select ADC channel 1 (PA1)
    ADC_SelectChannel(ADC_CHANNEL_1);
    HAL_Delay(10);  
    // Start ADC conversion
    if (HAL_ADC_Start(&hadc1) == HAL_OK)
    {
        // Wait for conversion to complete
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
        {
            // Read conversion result
            adc_value = HAL_ADC_GetValue(&hadc1);
        }
        // Stop ADC
        HAL_ADC_Stop(&hadc1);
    }
    return adc_value;
}

/**
  * @brief Read MQ-135 sensor voltage
  * @param None
  * @retval float voltage (V)
  */
float MQ135_Read_Voltage(void)
{
    uint32_t adc_raw = MQ135_Read_Raw();
    float voltage = (adc_raw * 3.3f) / 4095.0f;
    return voltage;
}

/**
  * @brief Read average voltage of MQ-135 sensor
  * @param samples number of samples
  * @retval float average voltage (V)
  */
float MQ135_Read_Average(uint8_t samples)
{
    if (samples == 0) samples = 1;

    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += MQ135_Read_Raw();
        HAL_Delay(10);
    }

    float average_voltage = ((float)sum / samples * 3.3f) / 4095.0f;
    return average_voltage;
}

/**
  * @brief Check if air quality exceeds threshold
  * @param threshold_voltage voltage threshold for alarm
  * @retval uint8_t 1=Poor air quality, 0=Good air quality
  */
uint8_t MQ135_Air_Quality_Detect(float threshold_voltage)
{
    float current_voltage = MQ135_Read_Average(3);
    return (current_voltage > threshold_voltage) ? 1 : 0;
}

/**
  * @brief Estimate air quality in PPM
  * @param voltage sensor voltage
  * @retval uint16_t PPM (approximate)
  */
uint16_t MQ135_Calculate_PPM(float voltage)
{
    if(voltage < 0.3f) return 10;  
    if(voltage > 3.0f) return 1000; 

    // Linear mapping: 0.3V = 10ppm, 3.0V = 1000ppm
    uint16_t ppm = (uint16_t)(((voltage - 0.3f) / 2.7f) * 990.0f + 10.0f);

    // Clamp within datasheet range
    if(ppm < 10) ppm = 10;
    if(ppm > 1000) ppm = 1000;

    return ppm;
}

/**
  * @brief Get MQ-135 digital output state
  * @param None
  * @retval uint8_t 1=Pollution detected, 0=Air is clean
  */
uint8_t MQ135_Digital_Read(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
}

// ==================== Photoresistor Sensor Functions ====================

/**
  * @brief Read raw ADC value from photoresistor
  * @param None
  * @retval uint32_t raw ADC value (0-4095)
  */
uint32_t LDR_Read_Raw(void)
{
    uint32_t adc_value = 0;

    // Select ADC channel 5 (PA5)
    ADC_SelectChannel(ADC_CHANNEL_5);
    HAL_Delay(10);  
    // Start ADC conversion
    if (HAL_ADC_Start(&hadc1) == HAL_OK)
    {
        // Wait for conversion to complete
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
        {
            // Read conversion result
            adc_value = HAL_ADC_GetValue(&hadc1);
        }
        // Stop ADC
        HAL_ADC_Stop(&hadc1);
    }

    return adc_value;
}

/**
  * @brief Read voltage of photoresistor
  * @param None
  * @retval float voltage (V)
  */
float LDR_Read_Voltage(void)
{
    uint32_t adc_raw = LDR_Read_Raw();
    float voltage = (adc_raw * 3.3f) / 4095.0f;
    return voltage;
}

/**
  * @brief Read average voltage from photoresistor
  * @param samples number of samples
  * @retval float average voltage (V)
  */
float LDR_Read_Average(uint8_t samples)
{
    if (samples == 0) samples = 1;

    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += LDR_Read_Raw();
        HAL_Delay(10); 
    }

    float average_voltage = ((float)sum / samples * 3.3f) / 4095.0f;
    return average_voltage;
}

/**
  * @brief Get light level category
  * @param voltage photoresistor voltage
  * @retval uint8_t light level 0=Dark, 1=Moderate, 2=Bright
  */
uint8_t LDR_Get_Light_Level(float voltage)
{
    if(voltage < 1.0f) return 0;        // Dark
    else if(voltage < 2.5f) return 1;   // Moderate
    else return 2;                      // Bright
}

/**
  * @brief Estimate light intensity in lux
  * @param voltage photoresistor voltage
  * @retval uint16_t light intensity (lux)
  */
uint16_t LDR_Calculate_Lux(float voltage)
{
    // Reverse mapping: 3.3V = 0 lux, 0V = 1000 lux
    if(voltage > 3.3f) voltage = 3.3f;
    uint16_t lux = (uint16_t)((3.3f - voltage) * 303.0f);  
    return lux;
}

/**
  * @brief Get digital output from photoresistor
  * @param None
  * @retval uint8_t 1=Bright, 0=Dark
  */
uint8_t LDR_Digital_Read(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
}
