#include "ltdc.h"
#include "ltdcfont.h"

static const char *TAG = "ltdc";
esp_lcd_panel_handle_t panel_handle = NULL;						/* RGB LCD handle */
static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED; /* Define portMUX_TYPE spinlock variable for critical section protection */
uint32_t g_back_color = 0xFFFF;									/* Background color */

/* Manage LTDC important parameters */
_ltdc_dev ltdcdev;

/**
 * @brief       Initialize ltdc
 * @param       None
 * @retval      None
 */
void ltdc_init(void)
{
	panel_handle = NULL;

	/* Directly configure for 800*480 screen parameters */
	ltdcdev.id = 0X4384;				/* Set to 800*480 screen ID */
	ltdcdev.pwidth = 800;				/* Panel width, unit: pixels */
	ltdcdev.pheight = 480;				/* Panel height, unit: pixels */
	ltdcdev.hbp = 88;					/* Horizontal back porch */
	ltdcdev.hfp = 40;					/* Horizontal front porch */
	ltdcdev.hsw = 48;					/* Horizontal sync width */
	ltdcdev.vbp = 32;					/* Vertical back porch */
	ltdcdev.vfp = 13;					/* Vertical front porch */
	ltdcdev.vsw = 3;					/* Vertical sync width */
	ltdcdev.pclk_hz = 18 * 1000 * 1000; /* Set pixel clock 18Mhz */

	/* Configure RGB parameters */
	esp_lcd_rgb_panel_config_t panel_config = {
		/* RGB LCD configuration structure */
		.data_width = 16,				/* Data width is 16 bits */
		.psram_trans_align = 64,		/* Buffer alignment allocated in PSRAM */
		.clk_src = LCD_CLK_SRC_PLL160M, /* RGB LCD peripheral clock source */
		.disp_gpio_num = GPIO_NUM_NC,	/* Display control signal, set to -1 if not used */
		.pclk_gpio_num = GPIO_LCD_PCLK, /* PCLK signal pin */
		.hsync_gpio_num = GPIO_NUM_NC,	/* HSYNC signal pin, not used in DE mode */
		.vsync_gpio_num = GPIO_NUM_NC,	/* VSYNC signal pin, not used in DE mode */
		.de_gpio_num = GPIO_LCD_DE,		/* DE signal pin */
		.data_gpio_nums = {
			/* Data line pins */
			GPIO_LCD_B3,
			GPIO_LCD_B4,
			GPIO_LCD_B5,
			GPIO_LCD_B6,
			GPIO_LCD_B7,
			GPIO_LCD_G2,
			GPIO_LCD_G3,
			GPIO_LCD_G4,
			GPIO_LCD_G5,
			GPIO_LCD_G6,
			GPIO_LCD_G7,
			GPIO_LCD_R3,
			GPIO_LCD_R4,
			GPIO_LCD_R5,
			GPIO_LCD_R6,
			GPIO_LCD_R7,
		},
		.timings = {
			/* RGB LCD timing parameters */
			.pclk_hz = ltdcdev.pclk_hz,		  /* Pixel clock frequency */
			.h_res = ltdcdev.pwidth,		  /* Horizontal resolution, i.e., number of pixels in a line */
			.v_res = ltdcdev.pheight,		  /* Vertical resolution, i.e., number of lines in a frame */
			.hsync_back_porch = ltdcdev.hbp,  /* Horizontal back porch, number of PCLK between hsync and line active data start */
			.hsync_front_porch = ltdcdev.hfp, /* Horizontal front porch, number of PCLK between active data end and next hsync */
			.hsync_pulse_width = ltdcdev.vsw, /* Vertical sync width, unit: lines */
			.vsync_back_porch = ltdcdev.vbp,  /* Vertical back porch, number of invalid lines between vsync and frame start */
			.vsync_front_porch = ltdcdev.vfp, /* Vertical front porch, number of invalid lines between frame end and next vsync */
			.vsync_pulse_width = ltdcdev.hsw, /* Horizontal sync width, unit: PCLK cycles */
			.flags.pclk_active_neg = true,	  /* RGB data is clocked on falling edge */
		},
		.flags.fb_in_psram = true,		   /* Allocate frame buffer in PSRAM */
		.bounce_buffer_size_px = 480 * 10, /* Solve jitter problem when writing spiflash */
	};

	/* Create RGB object */
	esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
	/* Reset RGB screen */
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	/* Initialize RGB */
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
	/* Set landscape mode */
	ltdc_display_dir(1);
	/* Clear screen to color */
	ltdc_clear(WHITE);
	/* Turn on backlight */
	LCD_BL(1);
}

/**
 * @brief       Clear screen
 * @param       color: Color to clear
 * @retval      None
 */
void ltdc_clear(uint16_t color)
{
	uint16_t *buffer = heap_caps_malloc(ltdcdev.width * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

	if (NULL == buffer)
	{
		ESP_LOGE(TAG, "Memory for bitmap is not enough");
	}
	else
	{
		for (uint16_t i = 0; i < ltdcdev.width; i++)
		{
			buffer[i] = color;
		}

		for (uint16_t y = 0; y < ltdcdev.height; y++)
		{									  /* Use taskENTER_CRITICAL() and taskEXIT_CRITICAL() to protect drawing process, disable task scheduling */
			taskENTER_CRITICAL(&my_spinlock); /* Mask interrupts */
			esp_lcd_panel_draw_bitmap(panel_handle, 0, y, ltdcdev.width, y + 1, buffer);
			taskEXIT_CRITICAL(&my_spinlock); /* Re-enable interrupts */
		}

		heap_caps_free(buffer);
	}
}

/**
 * @brief       RGB888 to RGB565 conversion
 * @param       r: Red
 * @param       g: Green
 * @param       b: Blue
 * @retval      Return RGB565 color value
 */
uint16_t ltdc_rgb888_to_565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief       LTDC display direction setting
 * @param       dir: 0, portrait; 1, landscape
 * @retval      None
 */
void ltdc_display_dir(uint8_t dir)
{
	ltdcdev.dir = dir; /* Display direction */

	if (ltdcdev.dir == 0) /* Portrait */
	{
		ltdcdev.width = ltdcdev.pheight;
		ltdcdev.height = ltdcdev.pwidth;
		esp_lcd_panel_swap_xy(panel_handle, true);		 /* Swap X and Y axes */
		esp_lcd_panel_mirror(panel_handle, false, true); /* Mirror Y axis of the screen */
	}
	else if (ltdcdev.dir == 1) /* Landscape */
	{
		ltdcdev.width = ltdcdev.pwidth;
		ltdcdev.height = ltdcdev.pheight;
		esp_lcd_panel_swap_xy(panel_handle, false);		  /* No need to swap X and Y axes */
		esp_lcd_panel_mirror(panel_handle, false, false); /* No mirroring of XY axes */
	}
}

/**
 * @brief       LTDC draw point function
 * @param       x,y     : Write coordinates
 * @param       color   : Color value
 * @retval      None
 */
void ltdc_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
	taskENTER_CRITICAL(&my_spinlock);
	esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, &color);
	taskEXIT_CRITICAL(&my_spinlock);
}

/**
 * @brief       Fill specified color block in specified area
 * @note        This function only supports uint16_t, RGB565 format color array filling.
 *              (sx,sy),(ex,ey): Fill rectangle diagonal coordinates, area size: (ex - sx + 1) * (ey - sy + 1)
 *              Note: sx,ex cannot be greater than ltdcdev.width - 1; sy,ey cannot be greater than ltdcdev.height - 1
 * @param       sx,sy: Start coordinates
 * @param       ex,ey: End coordinates
 * @param       color: Fill color array base address
 * @retval      None
 */
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
	/* Ensure coordinates are within LCD range */
	if (sx < 0 || sy < 0 || ex > ltdcdev.width || ey > ltdcdev.height)
	{
		return; /* Coordinates exceed LCD range, do not perform filling */
	}

	/* Ensure start coordinates are less than end coordinates */
	if (sx > ex || sy > ey)
	{
		return; /* Invalid fill area, do not perform filling */
	}

	/* Ensure fill area is completely within LCD range */
	sx = fmax(0, sx);
	sy = fmax(0, sy);
	ex = fmin(ltdcdev.width - 1, ex);
	ey = fmin(ltdcdev.height - 1, ey);

	/* Start filling color */
	for (int i = sx; i <= ex; i++)
	{
		for (int j = sy; j <= ey; j++)
		{
			/* Set RGB value to corresponding position on LCD */
			ltdc_draw_point(i, j, color);
		}
	}
}

/**
 * @brief       Display monochrome icon
 * @param       x,y,width,height: Coordinates and dimensions
 * @param       icosbase: Point array position
 * @param       color: Drawing point color
 * @param       bkcolor: Background color
 * @retval      None
 */
void ltdc_app_show_mono_icos(uint16_t x, uint16_t y, uint8_t width, uint8_t height, uint8_t *icosbase, uint16_t color, uint16_t bkcolor)
{
	uint16_t rsize;
	uint16_t i, j;
	uint8_t temp;
	uint8_t t = 0;
	uint16_t x0 = x;						   // Save x position
	rsize = width / 8 + ((width % 8) ? 1 : 0); // Number of bytes per line

	for (i = 0; i < rsize * height; i++)
	{
		temp = icosbase[i];

		for (j = 0; j < 8; j++)
		{
			if (temp & 0x80)
			{
				ltdc_draw_point(x, y, color);
			}
			else
			{
				ltdc_draw_point(x, y, bkcolor);
			}

			temp <<= 1;
			x++;
			t++; // Width counter

			if (t == width)
			{
				t = 0;
				x = x0;
				y++;
				break;
			}
		}
	}
}

/**
 * @brief       Draw line
 * @param       x1,y1: Start point coordinates
 * @param       x2,y2: End point coordinates
 * @param       color: Line color
 * @retval      None
 */
void ltdc_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	uint16_t t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, row, col;
	delta_x = x2 - x1; /* Calculate coordinate increment */
	delta_y = y2 - y1;
	row = x1;
	col = y1;

	if (delta_x > 0)
	{
		incx = 1; /* Set single step direction */
	}
	else if (delta_x == 0)
	{
		incx = 0; /* Vertical line */
	}
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}

	if (delta_y > 0)
	{
		incy = 1;
	}
	else if (delta_y == 0)
	{
		incy = 0; /* Horizontal line */
	}
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}

	if (delta_x > delta_y)
	{
		distance = delta_x; /* Select basic increment coordinate axis */
	}
	else
	{
		distance = delta_y;
	}

	for (t = 0; t <= distance + 1; t++) /* Line drawing output */
	{
		ltdc_draw_point(row, col, color); /* Draw point */
		xerr += delta_x;
		yerr += delta_y;

		if (xerr > distance)
		{
			xerr -= distance;
			row += incx;
		}

		if (yerr > distance)
		{
			yerr -= distance;
			col += incy;
		}
	}
}

/**
 * @brief       Draw a rectangle
 * @param       x1,y1   Start point coordinates
 * @param       x2,y2   End point coordinates
 * @param       color   Fill color
 * @retval      None
 */
void ltdc_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	ltdc_draw_line(x0, y0, x1, y0, color);
	ltdc_draw_line(x0, y0, x0, y1, color);
	ltdc_draw_line(x0, y1, x1, y1, color);
	ltdc_draw_line(x1, y0, x1, y1, color);
}

/**
 * @brief       Draw circle
 * @param       x0,y0: Circle center coordinates
 * @param       r    : Radius
 * @param       color: Circle color
 * @retval      None
 */
void ltdc_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1); /* Flag to determine next point position */

	while (a <= b)
	{
		ltdc_draw_point(x0 + a, y0 - b, color); /* 5 */
		ltdc_draw_point(x0 + b, y0 - a, color); /* 0 */
		ltdc_draw_point(x0 + b, y0 + a, color); /* 4 */
		ltdc_draw_point(x0 + a, y0 + b, color); /* 6 */
		ltdc_draw_point(x0 - a, y0 + b, color); /* 1 */
		ltdc_draw_point(x0 - b, y0 + a, color);
		ltdc_draw_point(x0 - a, y0 - b, color); /* 2 */
		ltdc_draw_point(x0 - b, y0 - a, color); /* 7 */
		a++;

		/* Use Bresenham algorithm to draw circle */
		if (di < 0)
		{
			di += 4 * a + 6;
		}
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}

/**
 * @brief       Display a character at specified position
 * @param       x,y  : Coordinates
 * @param       chr  : Character to display: " "--->"~"
 * @param       size : Font size 12/16/24/32
 * @param       mode : Overlay mode(1); Non-overlay mode(0);
 * @param       color: Font color
 * @retval      None
 */
void ltdc_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
	uint8_t temp, t1, t;
	uint16_t y0 = y;
	uint8_t csize = 0;
	uint8_t *pfont = 0;

	csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* Get number of bytes occupied by font character dot matrix */
	chr = (char)chr - ' ';									/* Get offset value (ASCII font starts from space, so -' ' is the corresponding character font) */

	switch (size)
	{
	case 12:
		pfont = (uint8_t *)asc2_1206[(uint8_t)chr]; /* Call 1206 font */
		break;

	case 16:
		pfont = (uint8_t *)asc2_1608[(uint8_t)chr]; /* Call 1608 font */
		break;

	case 24:
		pfont = (uint8_t *)asc2_2412[(uint8_t)chr]; /* Call 2412 font */
		break;

	case 32:
		pfont = (uint8_t *)asc2_3216[(uint8_t)chr]; /* Call 3216 font */
		break;

	default:
		return;
	}

	for (t = 0; t < csize; t++)
	{
		temp = pfont[t]; /* Get character dot matrix data */

		for (t1 = 0; t1 < 8; t1++) /* 8 points per byte */
		{
			if (temp & 0x80) /* Valid point, needs to be displayed */
			{
				ltdc_draw_point(x, y, color); /* Draw point, display this point */
			}
			else if (mode == 0) /* Invalid point, not displayed */
			{
				ltdc_draw_point(x, y, g_back_color); /* Draw background color, equivalent to not displaying this point (note background color is controlled by global variable) */
			}

			temp <<= 1; /* Shift to get next bit status */
			y++;

			if (y >= ltdcdev.height)
				return; /* Out of area */

			if ((y - y0) == size) /* Finished displaying one column? */
			{
				y = y0; /* Reset y coordinate */
				x++;	/* Increment x coordinate */

				if (x >= ltdcdev.width)
				{
					return; /* x coordinate out of area */
				}

				break;
			}
		}
	}
}

/**
 * @brief       Power function, m^n
 * @param       m: Base
 * @param       n: Exponent
 * @retval      m to the power of n
 */
static uint32_t ltdc_pow(uint8_t m, uint8_t n)
{
	uint32_t result = 1;

	while (n--)
	{
		result *= m;
	}

	return result;
}

/**
 * @brief       Display len digits
 * @param       x,y     : Start coordinates
 * @param       num     : Value (0 ~ 2^32)
 * @param       len     : Number of digits to display
 * @param       size    : Font selection 12/16/24/32
 * @param       color   : Font color
 * @retval      None
 */
void ltdc_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
	uint8_t t, temp;
	uint8_t enshow = 0;

	for (t = 0; t < len; t++) /* Loop by total display digits */
	{
		temp = (num / ltdc_pow(10, len - t - 1)) % 10; /* Get corresponding digit */

		if (enshow == 0 && t < (len - 1)) /* Not enabled for display, and there are still digits to display */
		{
			if (temp == 0)
			{
				ltdc_show_char(x + (size / 2) * t, y, ' ', size, 0, color); /* Display space, placeholder */
				continue;													/* Continue to next digit */
			}
			else
			{
				enshow = 1; /* Enable display */
			}
		}

		ltdc_show_char(x + (size / 2) * t, y, temp + '0', size, 0, color); /* Display character */
	}
}

/**
 * @brief       Extended display of len digits (high bits are 0 also displayed)
 * @param       x,y     : Start coordinates
 * @param       num     : Value (0 ~ 2^32)
 * @param       len     : Number of digits to display
 * @param       size    : Font selection 12/16/24/32
 * @param       mode    : Display mode
 *              [7]:0,no fill;1,fill with 0.
 *              [6:1]:reserved
 *              [0]:0,non-overlay display;1,overlay display.
 * @param       color   : Font color
 * @retval      None
 */
void ltdc_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
	uint8_t t, temp;
	uint8_t enshow = 0;

	for (t = 0; t < len; t++) /* Loop by total display digits */
	{
		temp = (num / ltdc_pow(10, len - t - 1)) % 10; /* Get corresponding digit */

		if (enshow == 0 && t < (len - 1)) /* Not enabled for display, and there are still digits to display */
		{
			if (temp == 0)
			{
				if (mode & 0x80) /* High bits need to be filled with 0 */
				{
					ltdc_show_char(x + (size / 2) * t, y, '0', size, mode & 0x01, color); /* Fill with 0 */
				}
				else
				{
					ltdc_show_char(x + (size / 2) * t, y, ' ', size, mode & 0x01, color); /* Fill with space */
				}

				continue;
			}
			else
			{
				enshow = 1; /* Enable display */
			}
		}

		ltdc_show_char(x + (size / 2) * t, y, temp + '0', size, mode & 0x01, color);
	}
}

/**
 * @brief       Display string
 * @param       x,y         : Start coordinates
 * @param       width,height: Area size
 * @param       size        : Font selection 12/16/24/32
 * @param       p           : String base address
 * @param       color       : Font color
 * @retval      None
 */
void ltdc_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
	uint8_t x0 = x;

	width += x;
	height += y;

	while ((*p <= '~') && (*p >= ' ')) /* Check if it's an illegal character! */
	{
		if (x >= width)
		{
			x = x0;
			y += size;
		}

		if (y >= height)
		{
			break; /* Exit */
		}

		ltdc_show_char(x, y, *p, size, 1, color);
		x += size / 2;
		p++;
	}
}