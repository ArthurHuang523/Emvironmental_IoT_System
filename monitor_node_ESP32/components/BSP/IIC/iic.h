#ifndef __IIC_H
#define __IIC_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"


/* IIC control block */
typedef struct _i2c_obj_t {
    i2c_port_t port;
    gpio_num_t scl;
    gpio_num_t sda;
    esp_err_t init_flag;
} i2c_obj_t;

/* Read/write data structure */
typedef struct _i2c_buf_t {
    size_t len;
    uint8_t *buf;
} i2c_buf_t;

extern i2c_obj_t iic_master[I2C_NUM_MAX];

/* Read/write flag bits */
#define I2C_FLAG_READ                   (0x01)          /* Read flag */
#define I2C_FLAG_STOP                   (0x02)          /* Stop flag */
#define I2C_FLAG_WRITE                 (0x04)           /* Write flag */

/* Pin and related parameter definitions */
#define IIC0_SDA_GPIO_PIN               GPIO_NUM_41     /* IIC0_SDA pin */
#define IIC0_SCL_GPIO_PIN               GPIO_NUM_42     /* IIC0_SCL pin */
#define IIC1_SDA_GPIO_PIN               GPIO_NUM_5      /* IIC1_SDA pin */
#define IIC1_SCL_GPIO_PIN               GPIO_NUM_4      /* IIC1_SCL pin */
#define IIC_FREQ                        400000          /* IIC communication frequency */
#define I2C_MASTER_TX_BUF_DISABLE       0               /* I2C master does not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE       0               /* I2C master does not need buffer */
#define ACK_CHECK_EN                    0x1             /* I2C master will check ACK from slave */

/* Function declarations */
i2c_obj_t iic_init(uint8_t iic_port);                                                                       /* Initialize IIC */
esp_err_t i2c_transfer(i2c_obj_t *self, uint16_t addr, size_t n, i2c_buf_t *bufs, unsigned int flags);      /* IIC read/write data */

#endif