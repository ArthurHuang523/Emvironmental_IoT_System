#include "touch.h"

_m_tp_dev tp_dev =
	{{
		tp_init,
		0,
		0,
		0,
		0,
		0,
	}};

/**
 * @brief       Touch screen initialization
 * @param       None
 * @retval      0, Touch screen initialization successful
 *              1, Touch screen has problems
 */
uint8_t tp_init(void)
{
	tp_dev.touchtype = 0;					/* Default setting (resistive screen & portrait) */
	tp_dev.touchtype |= ltdcdev.dir & 0X01; /* Determine landscape or portrait based on LCD */

	if (ltdcdev.id == 0X4342 || ltdcdev.id == 0X4384) /* Capacitive touch screen, 4.3 inch screen */
	{
		gt9xxx_init();
		tp_dev.scan = gt9xxx_scan; /* Scan function points to GT touch screen scan */
		tp_dev.touchtype |= 0X80;  /* Capacitive screen */
		return 0;
	}

	return 1;
}