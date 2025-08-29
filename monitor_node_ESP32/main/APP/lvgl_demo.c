#include "lvgl_demo.h"
#include "led.h"
#include "ltdc.h"
#include "touch.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "lv_mainstart.h"

/* LV_DEMO_TASK task configuration
 * Includes: task priority, stack size, task handle, create task
 */
#define LV_DEMO_TASK_PRIO 3			   /* Task priority */
#define LV_DEMO_STK_SIZE 5 * 1024	   /* Task stack size */
TaskHandle_t LV_DEMOTask_Handler;	   /* Task handle */
void lv_demo_task(void *pvParameters); /* Task function */

/* LED_TASK task configuration
 * Includes: task priority, stack size, task handle, create task
 */
#define LED_TASK_PRIO 2			   /* Task priority */
#define LED_STK_SIZE 1 * 1024	   /* Task stack size */
TaskHandle_t LEDTask_Handler;	   /* Task handle */
void led_task(void *pvParameters); /* Task function */

/**
 * @brief       lvgl_demo entry function
 * @param       None
 * @retval      None
 */
void lvgl_demo(void)
{
	lv_init();			  /* Initialize LVGL graphics library */
	lv_port_disp_init();  /* lvgl display interface initialization*/
	lv_port_indev_init(); /* lvgl input interface initialization*/

	/* Provide time base unit for LVGL */
	const esp_timer_create_args_t lvgl_tick_timer_args = {
		.callback = &increase_lvgl_tick,
		.name = "lvgl_tick"};
	esp_timer_handle_t lvgl_tick_timer = NULL;
	ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1 * 1000));

	/* Create LVGL task */
	xTaskCreatePinnedToCore((TaskFunction_t)lv_demo_task,		  /* Task function */
							(const char *)"lv_demo_task",		  /* Task name */
							(uint16_t)LV_DEMO_STK_SIZE,			  /* Task stack size */
							(void *)NULL,						  /* Parameters passed to task function */
							(UBaseType_t)LV_DEMO_TASK_PRIO,		  /* Task priority */
							(TaskHandle_t *)&LV_DEMOTask_Handler, /* Task handle */
							(BaseType_t)0);						  /* Which core this task runs on */

	/* Create LED test task */
	xTaskCreatePinnedToCore((TaskFunction_t)led_task,		  /* Task function */
							(const char *)"led_task",		  /* Task name */
							(uint16_t)LED_STK_SIZE,			  /* Task stack size */
							(void *)NULL,					  /* Parameters passed to task function */
							(UBaseType_t)LED_TASK_PRIO,		  /* Task priority */
							(TaskHandle_t *)&LEDTask_Handler, /* Task handle */
							(BaseType_t)0);					  /* Which core this task runs on */
}

/**
 * @brief       LVGL running routine
 * @param       pvParameters : Input parameters (not used)
 * @retval      None
 */
void lv_demo_task(void *pvParameters)
{
	pvParameters = pvParameters;

	lv_mainstart(); /* Test demo */

	while (1)
	{
		lv_timer_handler();			   /* LVGL timer */
		vTaskDelay(pdMS_TO_TICKS(10)); /* Delay 10 milliseconds */
	}
}

/**
 * @brief       led_task, program running indicator LED
 * @param       pvParameters : Input parameters (not used)
 * @retval      None
 */
void led_task(void *pvParameters)
{
	pvParameters = pvParameters; // remove the compiler warning

	while (1)
	{
		LED_TOGGLE();
		vTaskDelay(pdMS_TO_TICKS(1000)); /* Delay 1000 milliseconds */
	}
}

/**
 * @brief       Initialize and register display device
 * @param       None
 * @retval      None
 */
void lv_port_disp_init(void)
{
	void *buf1 = NULL;
	void *buf2 = NULL;

	/* Initialize display device RGB LCD */
	ltdc_init();		 /* RGB screen initialization */
	ltdc_display_dir(1); /* Set landscape mode */
	/**
	 * LVGL requires a buffer to draw widgets
	 * Double buffer:
	 *      LVGL will draw the display device content to one of the buffers and write it to the display device.
	 *      DMA needs to be used to write the content to be displayed on the display device to the buffer.
	 *      When data is sent from the first buffer, it will enable LVGL to draw the next part of the screen to another buffer.
	 *      This allows rendering and refreshing to be executed in parallel.*/
	buf1 = heap_caps_malloc(ltdcdev.width * 60 * sizeof(lv_color_t), MALLOC_CAP_DMA);
	buf2 = heap_caps_malloc(ltdcdev.width * 60 * sizeof(lv_color_t), MALLOC_CAP_DMA);
	/* Initialize display buffer */
	static lv_disp_draw_buf_t disp_buf;								  /* Structure that saves display buffer information */
	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, ltdcdev.width * 60); /* Initialize display buffer */

	/* Register display device in LVGL */
	static lv_disp_drv_t disp_drv; /* Display device descriptor (display driver to be registered by HAL, structure and callback functions that interact with display and handle graphics-related operations) */
	lv_disp_drv_init(&disp_drv);   /* Initialize display device */

	/* Set display device resolution
	 */
	disp_drv.hor_res = ltdcdev.width;
	disp_drv.ver_res = ltdcdev.height;

	/* Used to copy buffer content to display device */
	disp_drv.flush_cb = lvgl_disp_flush_cb;

	/* Set display buffer */
	disp_drv.draw_buf = &disp_buf;

	disp_drv.user_data = panel_handle;

	/* Register display device */
	lv_disp_drv_register(&disp_drv);
}

/**
 * @brief       Initialize and register input device
 * @param       None
 * @retval      None
 */
void lv_port_indev_init(void)
{
	/* Initialize touch screen */
	tp_dev.init();

	/* Initialize input device */
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);

	/* Configure input device type */
	indev_drv.type = LV_INDEV_TYPE_POINTER;

	/* Set input device read callback function */
	indev_drv.read_cb = touchpad_read;

	/* Register driver in LVGL and save the created input device object */
	lv_indev_t *indev_touchpad;
	indev_touchpad = lv_indev_drv_register(&indev_drv);
}

/**
 * @brief    Refresh the content of the internal buffer to a specific area on the display
 * @note     You can use DMA or any hardware to accelerate this operation in the background
 *           However, you need to call the function 'lv_disp_flush_ready()' after the refresh is complete
 * @param    disp_drv : Display device
 * @param    area : Area to refresh, contains diagonal coordinates of the fill rectangle
 * @param    color_map : Color array
 * @retval   None
 */
static void lvgl_disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
	esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

	/* Draw pixels in specific area */
	esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);

	/* Important!!! Notify graphics library that refresh is complete */
	lv_disp_flush_ready(drv);
}

/**
 * @brief       Tell LVGL the running time
 * @param       arg : Input parameter (not used)
 * @retval      None
 */
static void increase_lvgl_tick(void *arg)
{
	/* Tell LVGL how many milliseconds have passed */
	lv_tick_inc(1);
}

/**
 * @brief       Get the status of the touch screen device
 * @param       None
 * @retval      Return whether the touch screen device is pressed
 */
static bool touchpad_is_pressed(void)
{
	tp_dev.scan(0); /* Touch key scan */

	if (tp_dev.sta & TP_PRES_DOWN)
	{
		return true;
	}

	return false;
}

/**
 * @brief       Read x, y coordinates when touch screen is pressed
 * @param       x   : Pointer to x coordinate
 * @param       y   : Pointer to y coordinate
 * @retval      None
 */
static void touchpad_get_xy(lv_coord_t *x, lv_coord_t *y)
{
	(*x) = tp_dev.x[0];
	(*y) = tp_dev.y[0];
}

/**
 * @brief       Graphics library touch screen read callback function
 * @param       indev_drv   : Touch screen device
 * @param       data        : Input device data structure
 * @retval      None
 */
void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
	static lv_coord_t last_x = 0;
	static lv_coord_t last_y = 0;

	/* Save pressed coordinates and status */
	if (touchpad_is_pressed())
	{
		touchpad_get_xy(&last_x, &last_y); /* Read x, y coordinates when touch screen is pressed */
		data->state = LV_INDEV_STATE_PR;
	}
	else
	{
		data->state = LV_INDEV_STATE_REL;
	}

	/* Set last pressed coordinates */
	data->point.x = last_x;
	data->point.y = last_y;
}