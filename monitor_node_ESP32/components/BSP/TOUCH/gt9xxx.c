#include "gt9xxx.h"

/* Note: Except for GT9271 which supports 10-point touch, other touch chips only support 5-point touch */
uint8_t g_gt_tnum = 5; /* Default supported touch screen points (5-point touch) */

/**
 * @brief       Write data to gt9xxx
 * @param       reg : Start register address
 * @param       buf : Data buffer
 * @param       len : Write data length
 * @retval      esp_err_t : 0, success; 1, failure;
 */
esp_err_t gt9xxx_wr_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, GT9XXX_CMD_WR, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg >> 8, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg & 0XFF, ACK_CHECK_EN);
	i2c_master_write(cmd, buf, len, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(GT9XXX_IIC_PORT, cmd, 1000);
	i2c_cmd_link_delete(cmd);
	return ret;
}

/**
 * @brief       Read data from gt9xxx
 * @param       reg : Start register address
 * @param       buf : Data buffer
 * @param       len : Read data length
 * @retval      esp_err_t : 0, success; 1, failure;
 */
esp_err_t gt9xxx_rd_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
	if (len == 0)
	{
		return ESP_OK;
	}

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, GT9XXX_CMD_WR, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg >> 8, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg & 0XFF, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(GT9XXX_IIC_PORT, cmd, 1000);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, GT9XXX_CMD_RD, ACK_CHECK_EN);
	if (len > 1)
	{
		i2c_master_read(cmd, buf, len - 1, 0);
	}
	i2c_master_read_byte(cmd, buf + len - 1, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret |= i2c_master_cmd_begin(GT9XXX_IIC_PORT, cmd, 1000);
	i2c_cmd_link_delete(cmd);

	return ret;
}

/**
 * @brief       Initialize gt9xxx touch screen
 * @param       None
 * @retval      0, initialization successful; 1, initialization failed;
 */
uint8_t gt9xxx_init(void)
{
	uint8_t temp[5];
	gpio_config_t gpio_init_struct = {0};

	gpio_init_struct.intr_type = GPIO_INTR_DISABLE;				   /* Disable pin interrupt */
	gpio_init_struct.mode = GPIO_MODE_INPUT;					   /* Input mode */
	gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;			   /* Enable pull-up */
	gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;		   /* Disable pull-down */
	gpio_init_struct.pin_bit_mask = (1ull << GT9XXX_INT_GPIO_PIN); /* Pin bit mask to set */
	gpio_config(&gpio_init_struct);								   /* Configure GPIO */

	i2c_config_t iic_config_struct = {0};

	iic_config_struct.mode = I2C_MODE_MASTER;			   /* Set IIC mode - master mode */
	iic_config_struct.sda_io_num = GT9XXX_IIC_SDA;		   /* Set IIC_SDA pin */
	iic_config_struct.scl_io_num = GT9XXX_IIC_CLK;		   /* Set IIC_SCL pin */
	iic_config_struct.sda_pullup_en = GPIO_PULLUP_ENABLE;  /* Configure IIC_SDA pin pull-up enable */
	iic_config_struct.scl_pullup_en = GPIO_PULLUP_ENABLE;  /* Configure IIC_SCL pin pull-up enable */
	iic_config_struct.master.clk_speed = GT9XXX_IIC_FREQ;  /* Set IIC communication frequency */
	i2c_param_config(GT9XXX_IIC_PORT, &iic_config_struct); /* Set IIC initialization parameters */

	/* Activate I2C controller driver */
	i2c_driver_install(GT9XXX_IIC_PORT,			  /* Port number */
					   iic_config_struct.mode,	  /* Master mode */
					   I2C_MASTER_RX_BUF_DISABLE, /* Receive buffer size in slave mode (not used in master mode) */
					   I2C_MASTER_TX_BUF_DISABLE, /* Transmit buffer size in slave mode (not used in master mode) */
					   0);						  /* Flags for interrupt allocation (usually used in slave mode) */

	CT_RST(0);
	vTaskDelay(10);
	CT_RST(1);
	vTaskDelay(10);

	gt9xxx_rd_reg(GT9XXX_PID_REG, temp, 4); /* Read product ID */
	temp[4] = 0;
	/* Check if it is a specific touch screen */
	if (strcmp((char *)temp, "911") && strcmp((char *)temp, "9147") && strcmp((char *)temp, "1158") && strcmp((char *)temp, "9271"))
	{
		return 1; /* If not GT911/9147/1158/9271 used by touch screen, initialization fails, need to check touch IC model and timing functions */
	}
	printf("CTP ID:%s\r\n", temp); /* Print ID */

	if (strcmp((char *)temp, "9271") == 0) /* ID==9271, supports 10-point touch */
	{
		g_gt_tnum = 10; /* Support 10-point touch screen */
	}

	temp[0] = 0X02;
	gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1); /* Soft reset GT9XXX */

	vTaskDelay(10);

	temp[0] = 0X00;
	gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1); /* End reset, enter coordinate reading state */

	return 0;
}

/* GT9XXX 10 touch points (maximum) corresponding register table */
const uint16_t GT9XXX_TPX_TBL[10] =
	{
		GT9XXX_TP1_REG,
		GT9XXX_TP2_REG,
		GT9XXX_TP3_REG,
		GT9XXX_TP4_REG,
		GT9XXX_TP5_REG,
		GT9XXX_TP6_REG,
		GT9XXX_TP7_REG,
		GT9XXX_TP8_REG,
		GT9XXX_TP9_REG,
		GT9XXX_TP10_REG,
};

/**
 * @brief       Scan touch screen (using polling method)
 * @param       mode : Not used by capacitive screen, for compatibility with resistive screen
 * @retval      Current touch screen status
 *   @arg       0, no touch on touch screen;
 *   @arg       1, touch detected on touch screen;
 */
uint8_t gt9xxx_scan(uint8_t mode)
{
	uint8_t buf[4];
	uint8_t i = 0;
	uint8_t res = 0;
	uint16_t temp;
	uint16_t tempsta;
	static uint8_t t = 0; /* Control query interval to reduce CPU usage */
	t++;

	if ((t % 10) == 0 || t < 10) /* When idle, only detect once every 10 times entering CTP_Scan function to save CPU usage */
	{
		gt9xxx_rd_reg(GT9XXX_GSTID_REG, &mode, 1); /* Read touch point status */

		if ((mode & 0X80) && ((mode & 0XF) <= g_gt_tnum))
		{
			i = 0;
			gt9xxx_wr_reg(GT9XXX_GSTID_REG, &i, 1); /* Clear flag */
		}

		if ((mode & 0XF) && ((mode & 0XF) <= g_gt_tnum))
		{
			temp = 0XFFFF << (mode & 0XF); /* Convert number of points to number of 1 bits, matching tp_dev.sta definition */
			tempsta = tp_dev.sta;		   /* Save current tp_dev.sta value */
			tp_dev.sta = (~temp) | TP_PRES_DOWN | TP_CATH_PRES;
			tp_dev.x[g_gt_tnum - 1] = tp_dev.x[0]; /* Save touch point 0 data, save to the last one */
			tp_dev.y[g_gt_tnum - 1] = tp_dev.y[0];

			for (i = 0; i < g_gt_tnum; i++)
			{
				if (tp_dev.sta & (1 << i)) /* Touch valid? */
				{
					gt9xxx_rd_reg(GT9XXX_TPX_TBL[i], buf, 4); /* Read XY coordinate values */

					if (tp_dev.touchtype & 0X01) /* Landscape */
					{
						tp_dev.x[i] = ((uint16_t)buf[1] << 8) + buf[0];
						tp_dev.y[i] = ((uint16_t)buf[3] << 8) + buf[2];
					}
					else /* Portrait */
					{
						tp_dev.x[i] = ltdcdev.width - (((uint16_t)buf[3] << 8) + buf[2]);
						tp_dev.y[i] = ((uint16_t)buf[1] << 8) + buf[0];
					}
				}
			}

			res = 1;

			if (tp_dev.x[0] > ltdcdev.width || tp_dev.y[0] > ltdcdev.height) /* Invalid data (coordinates exceeded) */
			{
				if ((mode & 0XF) > 1) /* If other points have data, copy second touch point data to first touch point. */
				{
					tp_dev.x[0] = tp_dev.x[1];
					tp_dev.y[0] = tp_dev.y[1];
					t = 0; /* Trigger once, then monitor continuously for at least 10 times to improve hit rate */
				}
				else /* Invalid data, ignore this data (restore original) */
				{
					tp_dev.x[0] = tp_dev.x[g_gt_tnum - 1];
					tp_dev.y[0] = tp_dev.y[g_gt_tnum - 1];
					mode = 0X80;
					tp_dev.sta = tempsta; /* Restore tp_dev.sta */
				}
			}
			else
			{
				t = 0; /* Trigger once, then monitor continuously for at least 10 times to improve hit rate */
			}
		}
	}

	if ((mode & 0X8F) == 0X80) /* No touch point pressed */
	{
		if (tp_dev.sta & TP_PRES_DOWN) /* Previously was pressed */
		{
			tp_dev.sta &= ~TP_PRES_DOWN; /* Mark key released */
		}
		else /* Previously was not pressed */
		{
			tp_dev.x[0] = 0xffff;
			tp_dev.y[0] = 0xffff;
			tp_dev.sta &= 0XE000; /* Clear point valid mark */
		}
	}

	if (t > 240)
	{
		t = 10; /* Restart counting from 10 */
	}

	return res;
}