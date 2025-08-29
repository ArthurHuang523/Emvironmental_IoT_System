#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "ltdc.h"
#include "gt9xxx.h"

#define TP_PRES_DOWN 0x8000 /* Touch screen is pressed */
#define TP_CATH_PRES 0x4000 /* A key is pressed */
#define CT_MAX_TOUCH 10		/* Number of points supported by capacitive screen, fixed at 5 points */

/* Touch screen controller */
typedef struct
{
	uint8_t (*init)(void);	  /* Initialize touch screen controller */
	uint8_t (*scan)(uint8_t); /* Scan touch screen. 0, screen scan; 1, physical coordinates; */
	uint16_t x[CT_MAX_TOUCH]; /* Current coordinates */
	uint16_t y[CT_MAX_TOUCH]; /* Capacitive screen has up to 10 sets of coordinates, resistive screen uses x[0],y[0] to represent:
							   * coordinates of touch screen during this scan, use x[9],y[9] to store coordinates when first pressed.
							   */

	uint16_t sta; /* Pen status
				   * b15: pressed 1/released 0;
				   * b14: 0, no key pressed; 1, key pressed.
				   * b13~b10: reserved
				   * b9~b0: number of points pressed on capacitive touch screen (0, not pressed, 1 means pressed)
				   */

	/* 5-point calibration touch screen calibration parameters (capacitive screen does not need calibration) */
	float xfac; /* X direction scale factor for 5-point calibration */
	float yfac; /* Y direction scale factor for 5-point calibration */
	short xc;	/* Center X coordinate physical value (AD value) */
	short yc;	/* Center Y coordinate physical value (AD value) */

	/* New parameters, needed when the touch screen's left, right, up, down are completely reversed.
	 * b0: 0, portrait (suitable for TP with left-right as X coordinate, up-down as Y coordinate)
	 *     1, landscape (suitable for TP with left-right as Y coordinate, up-down as X coordinate)
	 * b1~6: reserved.
	 * b7: 0, resistive screen
	 *     1, capacitive screen
	 */
	uint8_t touchtype;
} _m_tp_dev;

extern _m_tp_dev tp_dev; /* Touch controller is defined in touch.c */

/* Function declarations */
uint8_t tp_init(void); /* Touch screen initialization */

#endif