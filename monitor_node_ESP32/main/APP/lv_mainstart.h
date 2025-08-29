#ifndef __LV_MAINSTART_H
#define __LV_MAINSTART_H

/* Function declarations */

/**
 * @brief LVGL main interface startup function
 */
void lv_mainstart(void);

/**
 * @brief UI update callback function (called by data_handler)
 */
void ui_update_callback(void);

#endif