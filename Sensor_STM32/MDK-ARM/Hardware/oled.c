#include "main.h"
#include "hardware.h"
#include <math.h>

I2C_HandleTypeDef hi2c1;

// OLED defination
#define OLED_ADDRESS    0x78    // OLED I2C address (0x3C << 1) 0x78 write mode 0x79 read mode
#define OLED_CMD        0x00    // Command
#define OLED_DATA       0x40    // Data

// 6x8 font array (extended character set)
static const uint8_t F6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // space (0)
    {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00},   // ! (1)
    {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E},   // 0 (2)
    {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00},   // 1 (3)
    {0x00, 0x42, 0x61, 0x51, 0x49, 0x46},   // 2 (4)
    {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31},   // 3 (5)
    {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10},   // 4 (6)
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39},   // 5 (7)
    {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30},   // 6 (8)
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03},   // 7 (9)
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36},   // 8 (10)
    {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E},   // 9 (11)
    {0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C},   // A (12)
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x36},   // B (13)
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22},   // C (14)
    {0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C},   // D (15)
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x41},   // E (16)
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x01},   // F (17)
    {0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A},   // G (18)
    {0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F},   // H (19)
    {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00},   // I (20)
    {0x00, 0x20, 0x40, 0x41, 0x3F, 0x01},   // J (21)
    {0x00, 0x7F, 0x08, 0x14, 0x22, 0x41},   // K (22)
    {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40},   // L (23)
    {0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F},   // M (24)
    {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F},   // N (25)
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E},   // O (26)
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06},   // P (27)
    {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E},   // Q (28)
    {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46},   // R (29)
    {0x00, 0x46, 0x49, 0x49, 0x49, 0x31},   // S (30)
    {0x00, 0x01, 0x01, 0x7F, 0x01, 0x01},   // T (31)
    {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F},   // U (32)
    {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F},   // V (33)
    {0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F},   // W (34)
    {0x00, 0x63, 0x14, 0x08, 0x14, 0x63},   // X (35)
    {0x00, 0x07, 0x08, 0x70, 0x08, 0x07},   // Y (36)
    {0x00, 0x61, 0x51, 0x49, 0x45, 0x43},   // Z (37)
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00},   // . (38)
    {0x00, 0x08, 0x08, 0x08, 0x08, 0x08},   // - (39)
    {0x00, 0x20, 0x54, 0x54, 0x54, 0x78},   // a (40)
    {0x00, 0x7F, 0x48, 0x44, 0x44, 0x38},   // b (41)
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x20},   // c (42)
    {0x00, 0x38, 0x44, 0x44, 0x48, 0x7F},   // d (43)
    {0x00, 0x38, 0x54, 0x54, 0x54, 0x18},   // e (44)
    {0x00, 0x08, 0x7E, 0x09, 0x01, 0x02},   // f (45)
    {0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C},   // g (46)
    {0x00, 0x7F, 0x08, 0x04, 0x04, 0x78},   // h (47)
    {0x00, 0x00, 0x44, 0x7D, 0x40, 0x00},   // i (48)
    {0x00, 0x40, 0x80, 0x84, 0x7D, 0x00},   // j (49)
    {0x00, 0x7F, 0x10, 0x28, 0x44, 0x00},   // k (50)
    {0x00, 0x00, 0x41, 0x7F, 0x40, 0x00},   // l (51)
    {0x00, 0x7C, 0x04, 0x18, 0x04, 0x78},   // m (52)
    {0x00, 0x7C, 0x08, 0x04, 0x04, 0x78},   // n (53)
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x38},   // o (54)
    {0x00, 0xFC, 0x24, 0x24, 0x24, 0x18},   // p (55)
    {0x00, 0x18, 0x24, 0x24, 0x18, 0xFC},   // q (56)
    {0x00, 0x7C, 0x08, 0x04, 0x04, 0x08},   // r (57)
    {0x00, 0x48, 0x54, 0x54, 0x54, 0x20},   // s (58)
    {0x00, 0x04, 0x3F, 0x44, 0x40, 0x20},   // t (59)
    {0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C},   // u (60)
    {0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C},   // v (61)
    {0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C},   // w (62)
    {0x00, 0x44, 0x28, 0x10, 0x28, 0x44},   // x (63)
    {0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C},   // y (64)
    {0x00, 0x44, 0x64, 0x54, 0x4C, 0x44},   // z (65)
    {0x00, 0x62, 0x64, 0x08, 0x13, 0x23},   // % (66)
};

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, W25Q64_CS_Pin | ESP8266_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = W25Q64_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W25Q64_CS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DHT11_DATA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = MQ2_DO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(MQ2_DO_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ESP8266_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ESP8266_RST_GPIO_Port, &GPIO_InitStruct);
}
/**
  * @brief Write a byte to OLED
  * @param dat Data
  * @param cmd 0 - command; 1 - data
  * @retval None
  */
static void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    uint8_t data[2];
    data[0] = cmd ? OLED_DATA : OLED_CMD;
    data[1] = dat;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, data, 2, 100);
}

/**
  * @brief Set OLED position
  * @param x Horizontal coordinate (0-127)
  * @param y Vertical coordinate (0-7)
  * @retval None
  */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    OLED_WR_Byte(0xb0 + y, 0);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, 0);
    OLED_WR_Byte((x & 0x0f), 0);
}

/**
  * @brief Clear the OLED display
  * @param None
  * @retval None
  */
void OLED_Clear(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, 0);
        OLED_WR_Byte(0x00, 0);
        OLED_WR_Byte(0x10, 0);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(0, 1);
    }
}

/**
  * @brief Display a character
  * @param x Horizontal coordinate
  * @param y Vertical coordinate
  * @param chr Character
  * @retval None
  */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr)
{
    uint8_t c = 0, i = 0;

    // Full character mapping
    if (chr == ' ') c = 0;
    else if (chr == '!') c = 1;
    else if (chr >= '0' && chr <= '9') c = chr - '0' + 2;        // Numbers 0-9
    else if (chr >= 'A' && chr <= 'Z') c = chr - 'A' + 12;       // Uppercase letters A-Z
    else if (chr == '.') c = 38;
    else if (chr == '-') c = 39;
    else if (chr >= 'a' && chr <= 'z') c = chr - 'a' + 40;       // Lowercase letters a-z
    else if (chr == '%') c = 66;                                 // Percent sign
    else c = 0; // Unsupported characters are shown as space

    OLED_Set_Pos(x, y);
    for (i = 0; i < 6; i++)
        OLED_WR_Byte(F6x8[c][i], 1);
}

/**
  * @brief Display a string
  * @param x Horizontal coordinate
  * @param y Vertical coordinate
  * @param str String
  * @retval None
  */
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *str)
{
    uint8_t j = 0;
    while (str[j] != '\0')
    {
        OLED_ShowChar(x, y, str[j]);
        x += 6;
        if (x > 120) {
            x = 0;
            y += 1;
        }
        j++;
    }
}

/**
  * @brief Power function
  * @param base Base
  * @param exp Exponent
  * @retval uint32_t Result
  */
static uint32_t oled_pow(uint32_t base, uint8_t exp)
{
    uint32_t result = 1;
    while (exp--)
        result *= base;
    return result;
}

/**
  * @brief Display a number
  * @param x Horizontal coordinate
  * @param y Vertical coordinate
  * @param num Number
  * @param len Number of digits
  * @retval None
  */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len)
{
    uint8_t t, temp;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        OLED_ShowChar(x + (6 * t), y, temp + '0');
    }
}

/**
  * @brief Display a float number
  * @param x Horizontal coordinate
  * @param y Vertical coordinate
  * @param num Float number
  * @param len Integer digit count
  * @param size Decimal digit count
  * @retval None
  */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size)
{
    uint32_t int_part = (uint32_t)num;
    uint32_t dec_part = (uint32_t)((num - int_part) * oled_pow(10, size));

    OLED_ShowNum(x, y, int_part, len);
    OLED_ShowChar(x + len * 6, y, '.');
    OLED_ShowNum(x + (len + 1) * 6, y, dec_part, size);
}

/**
  * @brief Turn on OLED display
  * @param None
  * @retval None
  */
void OLED_Display_On(void)
{
    OLED_WR_Byte(0X8D, 0);
    OLED_WR_Byte(0X14, 0);
    OLED_WR_Byte(0XAF, 0);
}

/**
  * @brief Turn off OLED display
  * @param None
  * @retval None
  */
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0X8D, 0);
    OLED_WR_Byte(0X10, 0);
    OLED_WR_Byte(0XAE, 0);
}

/**
  * @brief Clear a specific OLED line
  * @param line Line number (0-7)
  * @retval None
  */
void OLED_Clear_Line(uint8_t line)
{
    if(line > 7) return;  // Prevent out-of-bounds

    OLED_Set_Pos(0, line);
    for(uint8_t i = 0; i < 128; i++) {
        OLED_WR_Byte(0x00, 1);  // Clear all pixels in the line
    }
}

/**
  * @brief Initialize OLED
  * @param None
  * @retval None
  */
void OLED_Init(void)
{
    HAL_Delay(200);

    OLED_WR_Byte(0xAE, 0); // Display off
    OLED_WR_Byte(0x20, 0); // Set memory addressing mode
    OLED_WR_Byte(0x10, 0); // Page addressing mode
    OLED_WR_Byte(0xb0, 0); // Set page start address
    OLED_WR_Byte(0xc8, 0); // Set COM output scan direction
    OLED_WR_Byte(0x00, 0); // Set low column address
    OLED_WR_Byte(0x10, 0); // Set high column address
    OLED_WR_Byte(0x40, 0); // Set start line address
    OLED_WR_Byte(0x81, 0); // Set contrast control
    OLED_WR_Byte(0xFF, 0); // Contrast value
    OLED_WR_Byte(0xa1, 0); // Set segment re-map
    OLED_WR_Byte(0xa6, 0); // Set normal display
    OLED_WR_Byte(0xa8, 0); // Set multiplex ratio
    OLED_WR_Byte(0x3F, 0); // 1/64 duty
    OLED_WR_Byte(0xa4, 0); // Enable entire display
    OLED_WR_Byte(0xd3, 0); // Set display offset
    OLED_WR_Byte(0x00, 0); // No offset
    OLED_WR_Byte(0xd5, 0); // Set display clock divide ratio/oscillator frequency
    OLED_WR_Byte(0xf0, 0); // Divide ratio
    OLED_WR_Byte(0xd9, 0); // Set pre-charge period
    OLED_WR_Byte(0x22, 0);
    OLED_WR_Byte(0xda, 0); // Set COM pins hardware configuration
    OLED_WR_Byte(0x12, 0);
    OLED_WR_Byte(0xdb, 0); // Set VCOMH
    OLED_WR_Byte(0x20, 0);
    OLED_WR_Byte(0x8d, 0); // Set DC-DC enable
    OLED_WR_Byte(0x14, 0);
    OLED_WR_Byte(0xaf, 0); // Display on

    OLED_Clear();
}
