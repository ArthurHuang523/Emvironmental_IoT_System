#ifndef __GT9XXX_H
#define __GT9XXX_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "touch.h"
#include "string.h"
#include "iic.h"
#include "xl9555.h"


/******************************************************************************************/
/* GT9XXX INT pin definition */
#define GT9XXX_INT_GPIO_PIN             GPIO_NUM_40
#define GT9XXX_IIC_PORT                 I2C_NUM_1
#define GT9XXX_IIC_SDA                  GPIO_NUM_39
#define GT9XXX_IIC_CLK                  GPIO_NUM_38
#define GT9XXX_IIC_FREQ                 400000                                  /* IIC FREQ */
#define GT9XXX_INT                      gpio_get_level(GT9XXX_INT_GPIO_PIN)     /* Interrupt pin */

/* RGB_BL */
#define CT_RST(x)       do { x ?                              \
                            xl9555_pin_write(CT_RST_IO, 1):   \
                            xl9555_pin_write(CT_RST_IO, 0);   \
                        } while(0)

/* IIC read/write commands */
#define GT9XXX_CMD_WR                   0X28        /* Write command */
#define GT9XXX_CMD_RD                   0X29        /* Read command */

/* GT9XXX partial register definitions */
#define GT9XXX_CTRL_REG                 0X8040      /* GT9XXX control register */
#define GT9XXX_CFGS_REG                 0X8047      /* GT9XXX configuration start address register */
#define GT9XXX_CHECK_REG                0X80FF      /* GT9XXX checksum register */
#define GT9XXX_PID_REG                  0X8140      /* GT9XXX product ID register */

#define GT9XXX_GSTID_REG                0X814E      /* GT9XXX currently detected touch status */
#define GT9XXX_TP1_REG                  0X8150      /* First touch point data address */
#define GT9XXX_TP2_REG                  0X8158      /* Second touch point data address */
#define GT9XXX_TP3_REG                  0X8160      /* Third touch point data address */
#define GT9XXX_TP4_REG                  0X8168      /* Fourth touch point data address */
#define GT9XXX_TP5_REG                  0X8170      /* Fifth touch point data address */
#define GT9XXX_TP6_REG                  0X8178      /* Sixth touch point data address */
#define GT9XXX_TP7_REG                  0X8180      /* Seventh touch point data address */
#define GT9XXX_TP8_REG                  0X8188      /* Eighth touch point data address */
#define GT9XXX_TP9_REG                  0X8190      /* Ninth touch point data address */
#define GT9XXX_TP10_REG                 0X8198      /* Tenth touch point data address */

/* Function declarations */
uint8_t gt9xxx_init(void);                                          /* Initialize gt9xxx touch screen */
uint8_t gt9xxx_scan(uint8_t mode);                                  /* Scan touch screen (using polling method) */
esp_err_t gt9xxx_wr_reg(uint16_t reg, uint8_t *buf, uint8_t len);   /* Write data to gt9xxx */
esp_err_t gt9xxx_rd_reg(uint16_t reg, uint8_t *buf, uint8_t len);   /* Read data from gt9xxx */

#endif