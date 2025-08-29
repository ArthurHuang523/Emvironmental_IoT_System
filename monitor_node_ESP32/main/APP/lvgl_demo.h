#ifndef __LVGL_DEMO_H
#define __LVGL_DEMO_H

#include "lvgl.h"

/* Function declarations */
void lvgl_demo(void);																			  /* lvgl_demo entry function */
void lv_port_disp_init(void);																	  /* Initialize and register display device */
void lv_port_indev_init(void);																	  /* Initialize and register input device */
void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);							  /* Graphics library touch screen read callback function */
static void increase_lvgl_tick(void *arg);														  /* Tell LVGL the running time */
static void lvgl_disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map); /* Refresh the content of the internal buffer to a specific area on the display */

#endif