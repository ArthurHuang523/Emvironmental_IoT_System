#ifndef _LTDC_H
#define _LTDC_H

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "xl9555.h"
#include <math.h>

/* RGB_BL */
#define LCD_BL(x)                                                            \
	do                                                                       \
	{                                                                        \
		x ? xl9555_pin_write(LCD_BL_IO, 1) : xl9555_pin_write(LCD_BL_IO, 0); \
	} while (0)

/* RGB LCD pins */
#define GPIO_LCD_DE (GPIO_NUM_4)
#define GPIO_LCD_VSYNC (GPIO_NUM_NC)
#define GPIO_LCD_HSYNC (GPIO_NUM_NC)
#define GPIO_LCD_PCLK (GPIO_NUM_5)

#define GPIO_LCD_R3 (GPIO_NUM_45)
#define GPIO_LCD_R4 (GPIO_NUM_48)
#define GPIO_LCD_R5 (GPIO_NUM_47)
#define GPIO_LCD_R6 (GPIO_NUM_21)
#define GPIO_LCD_R7 (GPIO_NUM_14)

#define GPIO_LCD_G2 (GPIO_NUM_10)
#define GPIO_LCD_G3 (GPIO_NUM_9)
#define GPIO_LCD_G4 (GPIO_NUM_46)
#define GPIO_LCD_G5 (GPIO_NUM_3)
#define GPIO_LCD_G6 (GPIO_NUM_8)
#define GPIO_LCD_G7 (GPIO_NUM_18)

#define GPIO_LCD_B3 (GPIO_NUM_17)
#define GPIO_LCD_B4 (GPIO_NUM_16)
#define GPIO_LCD_B5 (GPIO_NUM_15)
#define GPIO_LCD_B6 (GPIO_NUM_7)
#define GPIO_LCD_B7 (GPIO_NUM_6)

/* Common drawing colors */
#define WHITE 0xFFFF   /* White */
#define BLACK 0x0000   /* Black */
#define RED 0xF800	   /* Red */
#define GREEN 0x07E0   /* Green */
#define BLUE 0x001F	   /* Blue */
#define MAGENTA 0xF81F /* Magenta/Purple = BLUE + RED */
#define YELLOW 0xFFE0  /* Yellow = GREEN + RED */
#define CYAN 0x07FF	   /* Cyan = GREEN + BLUE */

/* LCD LTDC important parameter set */
typedef struct
{
	uint32_t pwidth;	 /* LTDC panel width, fixed parameter, does not change with display direction, if 0, no RGB screen is connected */
	uint32_t pheight;	 /* LTDC panel height, fixed parameter, does not change with display direction */
	uint16_t hsw;		 /* Horizontal sync width */
	uint16_t vsw;		 /* Vertical sync width */
	uint16_t hbp;		 /* Horizontal back porch */
	uint16_t vbp;		 /* Vertical back porch */
	uint16_t hfp;		 /* Horizontal front porch */
	uint16_t vfp;		 /* Vertical front porch */
	uint8_t activelayer; /* Current layer number: 0/1 */
	uint8_t dir;		 /* 0, portrait; 1, landscape */
	uint16_t id;		 /* LTDC ID */
	uint32_t pclk_hz;	 /* Set pixel clock */
	uint16_t width;		 /* LTDC width */
	uint16_t height;	 /* LTDC height */
} _ltdc_dev;

/* LTDC parameters */
extern _ltdc_dev ltdcdev; /* Manage LTDC important parameters */

extern esp_lcd_panel_handle_t panel_handle;

/* ltdc related functions */
void ltdc_init(void);																													  /* Initialize ltdc */
void ltdc_clear(uint16_t color);																										  /* Clear screen */
uint16_t ltdc_rgb888_to_565(uint8_t r, uint8_t g, uint8_t b);																			  /* RGB888 to RGB565 conversion */
void ltdc_display_dir(uint8_t dir);																										  /* LTDC display direction setting */
void ltdc_draw_point(uint16_t x, uint16_t y, uint16_t color);																			  /* LTDC draw point function */
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);												  /* Fill specified color block in specified area */
void ltdc_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);												  /* Draw line */
void ltdc_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);																  /* Draw circle */
void ltdc_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);										  /* Display a character at specified position */
void ltdc_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);									  /* Display len digits */
void ltdc_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);						  /* Extended display of len digits (high bits are 0 also displayed) */
void ltdc_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);					  /* Display string */
void ltdc_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);											  /* Draw rectangle */
void ltdc_app_show_mono_icos(uint16_t x, uint16_t y, uint8_t width, uint8_t height, uint8_t *icosbase, uint16_t color, uint16_t bkcolor); /* Display monochrome icon */
#endif