#ifndef __XL9555_H
#define __XL9555_H

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"


/* Pin and related parameter definitions */
#define XL9555_INT_IO               GPIO_NUM_40                     /* XL9555_INT pin */
#define XL9555_INT                  gpio_get_level(XL9555_INT_IO)   /* Read XL9555_INT level */

/* XL9555 command macros */
#define XL9555_INPUT_PORT0_REG      0                               /* Input register 0 address */
#define XL9555_INPUT_PORT1_REG      1                               /* Input register 1 address */
#define XL9555_OUTPUT_PORT0_REG     2                               /* Output register 0 address */
#define XL9555_OUTPUT_PORT1_REG     3                               /* Output register 1 address */
#define XL9555_INVERSION_PORT0_REG  4                               /* Polarity inversion register 0 address */
#define XL9555_INVERSION_PORT1_REG  5                               /* Polarity inversion register 1 address */
#define XL9555_CONFIG_PORT0_REG     6                               /* Direction configuration register 0 address */
#define XL9555_CONFIG_PORT1_REG     7                               /* Direction configuration register 1 address */

#define XL9555_ADDR                 0X20                            /* XL9555 address (left shifted by one bit) --> see manual (9.1. Device Address) */

/* XL9555 IO function definitions */
#define AP_INT_IO                   0x0001
#define QMA_INT_IO                  0x0002
#define SPK_EN_IO                   0x0004
#define BEEP_IO                     0x0008
#define OV_PWDN_IO                  0x0010
#define OV_RESET_IO                 0x0020
#define GBC_LED_IO                  0x0040
#define GBC_KEY_IO                  0x0080
#define LCD_BL_IO                   0x0100
#define CT_RST_IO                   0x0200
#define SLCD_RST_IO                 0x0400
#define SLCD_PWR_IO                 0x0800
#define KEY3_IO                     0x1000
#define KEY2_IO                     0x2000
#define KEY1_IO                     0x4000
#define KEY0_IO                     0x8000

#define KEY0                        xl9555_pin_read(KEY0_IO)        /* Read KEY0 pin */
#define KEY1                        xl9555_pin_read(KEY1_IO)        /* Read KEY1 pin */
#define KEY2                        xl9555_pin_read(KEY2_IO)        /* Read KEY2 pin */
#define KEY3                        xl9555_pin_read(KEY3_IO)        /* Read KEY3 pin */

#define KEY0_PRES                   1                               /* KEY0 pressed */
#define KEY1_PRES                   2                               /* KEY1 pressed */
#define KEY2_PRES                   3                               /* KEY2 pressed */
#define KEY3_PRES                   4                               /* KEY3 pressed */

/* Function declarations */
void xl9555_init(i2c_obj_t self);                                   /* Initialize XL9555 */
int xl9555_pin_read(uint16_t pin);                                  /* Get the status of a specific IO */
uint16_t xl9555_pin_write(uint16_t pin, int val);                   /* Control the level of a specific IO */
esp_err_t xl9555_read_byte(uint8_t* data, size_t len);              /* Read 16-bit IO value from XL9555 */
uint8_t xl9555_key_scan(uint8_t mode);                              /* Scan key values */

#endif