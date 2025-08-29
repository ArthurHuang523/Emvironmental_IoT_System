#include "lv_mainstart.h"
#include "data_handler.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "ota_manager.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

LV_FONT_DECLARE(myFont14);

static const char *TAG = "LV_MAINSTART"; // store feedback

typedef struct
{
	const char *name;
	const char *unit;
	int min;
	int max;
	int (*get_value_func)(void);
	lv_obj_t *bar;	 // Progress Bar Control
	lv_obj_t *label; // label control
} SensorWidget;

static SensorWidget sensors_page1[5];
/* Rollback button reference */
static lv_obj_t *rollback_button_ref = NULL;
/* Control button related */
static lv_obj_t *control_status_label = NULL;
static lv_timer_t *on_btn_reset_timer = NULL;
static lv_timer_t *off_btn_reset_timer = NULL;
/* Status display controls */
static lv_obj_t *wifi_status_label = NULL;
static lv_obj_t *mqtt_status_label = NULL;
static lv_obj_t *data_status_label = NULL;
/* OTA related */
static lv_obj_t *ota_button_ref = NULL;
static TaskHandle_t ota_task_handle = NULL;
static lv_obj_t *ota_notification_label = NULL;
/* OTA dedicated interface */
static lv_obj_t *ota_screen = NULL;
static lv_obj_t *ota_full_progress_bar = NULL;
static lv_obj_t *ota_full_status_label = NULL;
static lv_obj_t *main_screen = NULL;
/* Timer */
static lv_timer_t *update_timer = NULL;

/* Return to Main Screen*/
static void return_to_main_timer_cb(lv_timer_t *timer)
{
	if (main_screen && ota_screen)
	{
		lv_scr_load(main_screen);
		lv_obj_del(ota_screen);
		ota_screen = NULL;
		ota_full_progress_bar = NULL;
		ota_full_status_label = NULL;
		ESP_LOGI(TAG, "Returned to main interface");
		if (update_timer)
		{
			lv_timer_resume(update_timer);
			ESP_LOGI(TAG, "Main interface update timer resumed");
		}
	}
}

/**
 * @brief Update OTA notification display
 */
void update_ota_notification(bool available)
{
	if (ota_notification_label && lv_obj_is_valid(ota_notification_label))
	{
		if (available)
		{
			lv_label_set_text(ota_notification_label, "Firmware updates are available.");
			lv_obj_set_style_text_color(ota_notification_label, lv_palette_main(LV_PALETTE_ORANGE), 0);
			lv_obj_clear_flag(ota_notification_label, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(ota_notification_label, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

/* OTA interface functions */
static void create_ota_screen(void)
{
	ota_screen = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(ota_screen, lv_palette_main(LV_PALETTE_BLUE_GREY), LV_PART_MAIN);

	// Title
	lv_obj_t *title = lv_label_create(ota_screen);
	lv_obj_set_style_text_font(title, &myFont14, LV_PART_MAIN);
	lv_label_set_text(title, "OTA Firmware Update");
	lv_obj_set_style_text_color(title, lv_color_white(), 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 80);

	// Large progress bar
	ota_full_progress_bar = lv_bar_create(ota_screen);
	lv_obj_set_size(ota_full_progress_bar, 500, 40);
	lv_obj_align(ota_full_progress_bar, LV_ALIGN_CENTER, 0, 0);
	lv_bar_set_range(ota_full_progress_bar, 0, 100);
	lv_bar_set_value(ota_full_progress_bar, 0, LV_ANIM_OFF);
	lv_obj_set_style_bg_color(ota_full_progress_bar, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);
	lv_obj_set_style_bg_color(ota_full_progress_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

	// Status label
	ota_full_status_label = lv_label_create(ota_screen);
	lv_obj_set_style_text_font(ota_full_status_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(ota_full_status_label, "Starting OTA...");
	lv_obj_set_style_text_color(ota_full_status_label, lv_color_white(), 0);
	lv_obj_align(ota_full_status_label, LV_ALIGN_CENTER, 0, 60);

	// Warning text
	lv_obj_t *warning = lv_label_create(ota_screen);
	lv_obj_set_style_text_font(warning, &myFont14, LV_PART_MAIN);
	lv_label_set_text(warning, "Please do not power off the device!");
	lv_obj_set_style_text_color(warning, lv_palette_main(LV_PALETTE_ORANGE), 0);
	lv_obj_align(warning, LV_ALIGN_BOTTOM_MID, 0, -80);
}

static void switch_to_ota_screen(void)
{
	main_screen = lv_scr_act(); // Save Current Screen:
	if (ota_screen == NULL)
	{
		create_ota_screen();
	}
	if (update_timer) // Pause Update Timer
	{
		lv_timer_pause(update_timer);
	}
	lv_scr_load(ota_screen); // Switch to the OTA screen
	ESP_LOGI(TAG, "Switched to OTA dedicated interface");
}

/* Button state reset timer callback */
static void reset_rollback_btn_timer_cb(lv_timer_t *timer)
{
	if (rollback_button_ref && lv_obj_is_valid(rollback_button_ref))
	{
		lv_label_set_text_static(lv_obj_get_child(rollback_button_ref, 0), "ROLLBACK");
		lv_obj_set_style_bg_color(rollback_button_ref, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN);
	}
	lv_timer_del(timer);
}

/* Rollback countdown timer callback */
static void rollback_countdown_timer_cb(lv_timer_t *timer)
{
	static int countdown = 3;

	if (countdown > 0)
	{
		// ESP_LOGI(TAG, "Rollback countdown: %d seconds", countdown);

		// Update button text to show countdown
		if (rollback_button_ref && lv_obj_is_valid(rollback_button_ref))
		{
			char countdown_text[32];
			snprintf(countdown_text, sizeof(countdown_text), "ROLLBACK %d", countdown);
			lv_label_set_text(lv_obj_get_child(rollback_button_ref, 0), countdown_text);
		}
		countdown--;
	}
	else
	{
		// Countdown finished, execute rollback
		// ESP_LOGI(TAG, "Countdown finished, executing rollback!");
		lv_timer_del(timer);
		countdown = 3; // Reset countdown

		if (rollback_button_ref && lv_obj_is_valid(rollback_button_ref))
		{
			lv_label_set_text_static(lv_obj_get_child(rollback_button_ref, 0), "ROLLING BACK");
		}

		// Execute rollback
		ota_manager_rollback();
	}
}

/* rollback button event handler */
static void rollback_btn_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
	{

		if (ota_manager_can_rollback())
		{
			// Update button display
			lv_label_set_text_static(lv_obj_get_child(lv_event_get_target(e), 0), "ROLLBACK 3");
			lv_obj_set_style_bg_color(lv_event_get_target(e), lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);

			// Start countdown timer
			lv_timer_create(rollback_countdown_timer_cb, 1000, NULL);
		}
		else
		{
			// Display failure state
			lv_label_set_text_static(lv_obj_get_child(lv_event_get_target(e), 0), "NO ROLLBACK");
			lv_obj_set_style_bg_color(lv_event_get_target(e), lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

			// Restore button state after 2 seconds
			lv_timer_create(reset_rollback_btn_timer_cb, 2000, NULL);
		}
	}
}

/* OTA task function */
static void ota_task(void *pvParameters)
{
	ESP_LOGI(TAG, "OTA task started");
	esp_err_t ret = ota_manager_start_upgrade(NULL);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "OTA task execution failed: %s", esp_err_to_name(ret));
	}
	ESP_LOGI(TAG, "OTA task ended");
	ota_task_handle = NULL;
	vTaskDelete(NULL);
}

static esp_err_t start_ota_task(void)
{
	if (ota_task_handle != NULL)
	{
		ESP_LOGW(TAG, "OTA task is already running");
		return ESP_ERR_INVALID_STATE;
	}

	BaseType_t ret = xTaskCreatePinnedToCore(
		ota_task, "ota_task", 8192, NULL, 1, &ota_task_handle, 0);

	if (ret != pdPASS)
	{
		ESP_LOGE(TAG, "Failed to create OTA task");
		ota_task_handle = NULL;
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "OTA task created");
	return ESP_OK;
}

/* OTA button event */
static void ota_btn_event_cb(lv_event_t *e)
{
	lv_obj_t *btn = lv_event_get_target(e);
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
	{
		ESP_LOGI(TAG, "=== Starting OTA upgrade test ===");

		if (!wifi_manager_is_connected())
		{
			ESP_LOGE(TAG, "WiFi not connected, cannot perform OTA upgrade");
			return;
		}

		ota_status_t current_status = ota_manager_get_status();
		if (current_status == OTA_STATUS_DOWNLOADING ||
			current_status == OTA_STATUS_WRITING ||
			current_status == OTA_STATUS_STARTING ||
			ota_task_handle != NULL)
		{
			ESP_LOGW(TAG, "OTA is in progress, ignoring click");
			return;
		}
		switch_to_ota_screen();
		esp_err_t ret = start_ota_task();
		if (ret != ESP_OK)
		{
			ESP_LOGE(TAG, "Failed to start OTA task: %s", esp_err_to_name(ret));

			// If startup failed, return to main interface and restore timer
			if (main_screen && ota_screen)
			{
				lv_scr_load(main_screen);
				lv_obj_del(ota_screen);
				ota_screen = NULL; // clear pointer

				//  Restore main interface timer
				if (update_timer)
				{
					lv_timer_resume(update_timer);
				}
			}
		}
		else
		{
			ESP_LOGI(TAG, "OTA task started, interface remains responsive");
		}
	}
}

/* OTA callback functions */
void ota_progress_callback_debug(int progress_percent, int received_bytes, int total_bytes)
{
	ESP_LOGI(TAG, "=== OTA progress: %d%% (%d/%d KB) ===", progress_percent, received_bytes / 1024, total_bytes / 1024);
	// Update full screen progress bar
	if (ota_full_progress_bar)
	{
		lv_bar_set_value(ota_full_progress_bar, progress_percent, LV_ANIM_ON);
	}

	// Update full screen status label
	if (ota_full_status_label)
	{
		char progress_text[64];
		snprintf(progress_text, sizeof(progress_text), "Downloading: %d%% (%d/%d KB)",
				 progress_percent, received_bytes / 1024, total_bytes / 1024);
		lv_label_set_text(ota_full_status_label, progress_text);
	}
}

void ota_status_callback_debug(ota_status_t status)
{
	ESP_LOGI(TAG, "=== OTA status update: %d ===", status);
	switch (status)
	{
	case OTA_STATUS_STARTING:
		break;

	case OTA_STATUS_DOWNLOADING:
		if (ota_full_status_label)
		{
			lv_label_set_text(ota_full_status_label, "Downloading firmware...");
		}
		break;

	case OTA_STATUS_WRITING:
		if (ota_full_status_label)
		{
			lv_label_set_text(ota_full_status_label, "Writing firmware...");
		}
		if (ota_full_progress_bar)
		{
			lv_obj_set_style_bg_color(ota_full_progress_bar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
		}
		break;

	case OTA_STATUS_SUCCESS:
		if (ota_full_status_label)
		{
			lv_label_set_text(ota_full_status_label, "Update Complete! Restarting...");
		}
		if (ota_full_progress_bar)
		{
			lv_bar_set_value(ota_full_progress_bar, 100, LV_ANIM_ON);
			lv_obj_set_style_bg_color(ota_full_progress_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
		}
		break;

	case OTA_STATUS_FAILED:
		if (ota_full_status_label)
		{
			lv_label_set_text(ota_full_status_label, "Update Failed! Returning to main...");
		}
		if (ota_full_progress_bar)
		{
			lv_obj_set_style_bg_color(ota_full_progress_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
		}
		// Return to main interface after 5 seconds
		lv_timer_create(return_to_main_timer_cb, 5000, NULL);
		break;

	case OTA_STATUS_IDLE:
		if (ota_button_ref)
		{
			lv_label_set_text_static(lv_obj_get_child(ota_button_ref, 0), "START OTA");
			lv_obj_set_style_bg_color(ota_button_ref, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
		}
		break;

	default:
		break;
	}
}

/* Sensor related functions */
static lv_color_t get_sensor_color(const char *sensor_name, int value)
{
	if (strcmp(sensor_name, "Temp") == 0)
	{
		return (value > 20) ? lv_palette_main(LV_PALETTE_RED) : lv_palette_main(LV_PALETTE_BLUE);
	}
	else if (strcmp(sensor_name, "Humidity") == 0)
	{
		return (value > 50) ? lv_palette_main(LV_PALETTE_ORANGE) : lv_palette_main(LV_PALETTE_BLUE);
	}
	else if (strcmp(sensor_name, "Air") == 0)
	{
		return (value > 500) ? lv_palette_main(LV_PALETTE_PURPLE) : lv_palette_main(LV_PALETTE_BLUE);
	}
	else if (strcmp(sensor_name, "Smoke") == 0)
	{
		return (value > 1000) ? lv_palette_main(LV_PALETTE_RED) : lv_palette_main(LV_PALETTE_BLUE);
	}
	else if (strcmp(sensor_name, "Light") == 0)
	{
		return (value > 500) ? lv_palette_main(LV_PALETTE_YELLOW) : lv_palette_main(LV_PALETTE_BLUE);
	}
	return lv_palette_main(LV_PALETTE_BLUE);
}

// Update sensor widgets
void update_sensor_widgets(SensorWidget *sensors, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (sensors[i].get_value_func && sensors[i].bar && sensors[i].label)
		{
			int current_value = sensors[i].get_value_func();
			lv_bar_set_value(sensors[i].bar, current_value, LV_ANIM_ON);

			char buf[64];
			snprintf(buf, sizeof(buf), "%s: %d %s",
					 sensors[i].name, current_value, sensors[i].unit);
			lv_label_set_text(sensors[i].label, buf);

			lv_color_t bar_color = get_sensor_color(sensors[i].name, current_value);
			lv_obj_set_style_bg_color(sensors[i].bar, bar_color, LV_PART_INDICATOR);

			if (data_handler_get_alarm_status())
			{
				lv_obj_set_style_border_color(sensors[i].bar, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
				lv_obj_set_style_border_width(sensors[i].bar, 2, LV_PART_MAIN);
			}
			else
			{
				lv_obj_set_style_border_width(sensors[i].bar, 0, LV_PART_MAIN);
			}
		}
	}
}

// update status tab page display
void update_status_display(void)
{
	if (wifi_status_label)
	{
		bool wifi_connected = wifi_manager_is_connected();
		lv_label_set_text(wifi_status_label, wifi_connected ? "WiFi: Connected" : "WiFi: Disconnected");
		lv_obj_set_style_text_color(wifi_status_label,
									wifi_connected ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);
	}

	if (mqtt_status_label)
	{
		bool mqtt_connected = mqtt_manager_is_connected();
		lv_label_set_text(mqtt_status_label, mqtt_connected ? "MQTT: Connected" : "MQTT: Disconnected");
		lv_obj_set_style_text_color(mqtt_status_label,
									mqtt_connected ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);
	}

	if (data_status_label)
	{
		bool data_fresh = data_handler_is_data_fresh(30000);
		uint64_t data_age_sec = data_handler_get_data_age() / 1000;

		char data_text[64];
		if (data_handler_is_data_valid())
		{
			snprintf(data_text, sizeof(data_text), "Data: %llu sec ago", data_age_sec);
		}
		else
		{
			snprintf(data_text, sizeof(data_text), "Data: No data");
		}

		lv_label_set_text(data_status_label, data_text);
		lv_obj_set_style_text_color(data_status_label,
									data_fresh ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_ORANGE), 0);
	}
}

/* Control Page Button Turn On State Reset Timer*/
static void on_btn_reset_timer_cb(lv_timer_t *timer)
{
	lv_obj_t *btn = (lv_obj_t *)timer->user_data;
	if (btn && lv_obj_is_valid(btn))
	{
		lv_obj_t *label = lv_obj_get_child(btn, 0);
		if (label && lv_obj_is_valid(label))
		{
			lv_label_set_text_static(label, "TURN ON");
		}
	}
	on_btn_reset_timer = NULL;
}

/* Control button ON events */
static void control_on_btn_event_cb(lv_event_t *e)
{
	lv_obj_t *btn = lv_event_get_target(e);
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
	{
		if (on_btn_reset_timer != NULL)
		{
			ESP_LOGW(TAG, "ON button clicked too fast, ignoring this click");
			return;
		}

		int msg_id = mqtt_manager_send_control_command("1");

		if (msg_id != -1)
		{
			ESP_LOGI(TAG, "ON command sent, msg_id=%d", msg_id);
			lv_label_set_text_static(lv_obj_get_child(btn, 0), "ON SENT");

			if (control_status_label)
			{
				lv_label_set_text(control_status_label, "Status: Command 'ON' sent successfully");
				lv_obj_set_style_text_color(control_status_label, lv_palette_main(LV_PALETTE_GREEN), 0);
			}
		}
		else
		{
			// ESP_LOGE(TAG, "Failed to send ON command");
			lv_label_set_text_static(lv_obj_get_child(btn, 0), "SEND FAILED");

			if (control_status_label)
			{
				lv_label_set_text(control_status_label, "Status: Failed to send ON command");
				lv_obj_set_style_text_color(control_status_label, lv_palette_main(LV_PALETTE_RED), 0);
			}
		}

		on_btn_reset_timer = lv_timer_create(on_btn_reset_timer_cb, 1000, btn);
	}
}

/* Control Page Button Turn OFF State Reset Timer*/
static void off_btn_reset_timer_cb(lv_timer_t *timer)
{
	lv_obj_t *btn = (lv_obj_t *)timer->user_data;
	if (btn && lv_obj_is_valid(btn))
	{
		lv_obj_t *label = lv_obj_get_child(btn, 0);
		if (label && lv_obj_is_valid(label))
		{
			lv_label_set_text_static(label, "TURN OFF");
		}
	}
	off_btn_reset_timer = NULL;
}

/* Control button OFF events */
static void control_off_btn_event_cb(lv_event_t *e)
{
	lv_obj_t *btn = lv_event_get_target(e);
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
	{
		if (off_btn_reset_timer != NULL)
		{
			ESP_LOGW(TAG, "OFF button clicked too fast, ignoring this click");
			return;
		}

		int msg_id = mqtt_manager_send_control_command("0");

		if (msg_id != -1)
		{
			ESP_LOGI(TAG, "OFF command sent, msg_id=%d", msg_id);
			lv_label_set_text_static(lv_obj_get_child(btn, 0), "OFF SENT");

			if (control_status_label)
			{
				lv_label_set_text(control_status_label, "Status: Command 'OFF' sent successfully");
				lv_obj_set_style_text_color(control_status_label, lv_palette_main(LV_PALETTE_GREEN), 0);
			}
		}
		else
		{
			// ESP_LOGE(TAG, "Failed to send OFF command");
			lv_label_set_text_static(lv_obj_get_child(btn, 0), "SEND FAILED");

			if (control_status_label)
			{
				lv_label_set_text(control_status_label, "Status: Failed to send OFF command");
				lv_obj_set_style_text_color(control_status_label, lv_palette_main(LV_PALETTE_RED), 0);
			}
		}

		off_btn_reset_timer = lv_timer_create(off_btn_reset_timer_cb, 1000, btn);
	}
}

/* Page creation functions */
void create_control_page(lv_obj_t *parent)
{
	lv_obj_t *title = lv_label_create(parent);
	lv_obj_set_style_text_font(title, &myFont14, LV_PART_MAIN);
	lv_label_set_text(title, "MQTT Control Panel");
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

	lv_obj_t *desc = lv_label_create(parent);
	lv_obj_set_style_text_font(desc, &myFont14, LV_PART_MAIN);
	lv_label_set_text(desc, "Send control commands to STM32 via MQTT");
	lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 50);

	lv_obj_t *on_btn = lv_btn_create(parent);
	lv_obj_set_size(on_btn, 200, 80);
	lv_obj_align(on_btn, LV_ALIGN_CENTER, -120, -20);
	lv_obj_add_event_cb(on_btn, control_on_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_set_style_bg_color(on_btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
	lv_obj_set_style_bg_color(on_btn, lv_palette_darken(LV_PALETTE_GREEN, 2), LV_STATE_PRESSED);

	lv_obj_t *on_btn_label = lv_label_create(on_btn);
	lv_obj_set_style_text_font(on_btn_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(on_btn_label, "TURN ON");
	lv_obj_center(on_btn_label);

	lv_obj_t *off_btn = lv_btn_create(parent);
	lv_obj_set_size(off_btn, 200, 80);
	lv_obj_align(off_btn, LV_ALIGN_CENTER, 120, -20);
	lv_obj_add_event_cb(off_btn, control_off_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_set_style_bg_color(off_btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
	lv_obj_set_style_bg_color(off_btn, lv_palette_darken(LV_PALETTE_RED, 2), LV_STATE_PRESSED);

	lv_obj_t *off_btn_label = lv_label_create(off_btn);
	lv_obj_set_style_text_font(off_btn_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(off_btn_label, "TURN OFF");
	lv_obj_center(off_btn_label);

	control_status_label = lv_label_create(parent);
	lv_obj_set_style_text_font(control_status_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(control_status_label, "Status: Ready to send commands");
	lv_obj_align(control_status_label, LV_ALIGN_CENTER, 0, 60);
}

void create_status_area(lv_obj_t *parent)
{
	wifi_status_label = lv_label_create(parent);
	lv_obj_set_style_text_font(wifi_status_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(wifi_status_label, "WiFi: Checking...");
	lv_obj_align(wifi_status_label, LV_ALIGN_TOP_LEFT, 20, 10);

	mqtt_status_label = lv_label_create(parent);
	lv_obj_set_style_text_font(mqtt_status_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(mqtt_status_label, "MQTT: Checking...");
	lv_obj_align(mqtt_status_label, LV_ALIGN_TOP_LEFT, 200, 10);

	data_status_label = lv_label_create(parent);
	lv_obj_set_style_text_font(data_status_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(data_status_label, "Data: Waiting...");
	lv_obj_align(data_status_label, LV_ALIGN_TOP_LEFT, 400, 10);

	ota_notification_label = lv_label_create(parent);
	lv_obj_set_style_text_font(ota_notification_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(ota_notification_label, "Firmware updates are available.");
	lv_obj_set_style_text_color(ota_notification_label, lv_palette_main(LV_PALETTE_ORANGE), 0);
	lv_obj_align(ota_notification_label, LV_ALIGN_TOP_MID, 0, 40);
	lv_obj_add_flag(ota_notification_label, LV_OBJ_FLAG_HIDDEN);
}

void create_sensor_widgets(lv_obj_t *parent, SensorWidget *sensors)
{
	for (int i = 0; i < 5; i++)
	{
		int y_offset = 20 + i * 80;

		sensors[i].bar = lv_bar_create(parent);
		lv_obj_set_size(sensors[i].bar, 350, 25);
		lv_obj_align(sensors[i].bar, LV_ALIGN_TOP_LEFT, 20, y_offset);
		lv_bar_set_range(sensors[i].bar, sensors[i].min, sensors[i].max);
		// set initial value
		int initial_value = sensors[i].get_value_func ? sensors[i].get_value_func() : sensors[i].min;
		lv_bar_set_value(sensors[i].bar, initial_value, LV_ANIM_OFF);
		// creat sensor lable
		sensors[i].label = lv_label_create(parent);
		lv_obj_set_style_text_font(sensors[i].label, &myFont14, LV_PART_MAIN);

		char buf[64];
		snprintf(buf, sizeof(buf), "%s: %d %s", sensors[i].name, initial_value, sensors[i].unit);
		lv_label_set_text(sensors[i].label, buf);
		lv_obj_align_to(sensors[i].label, sensors[i].bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
	}
}

static void update_timer_cb(lv_timer_t *timer)
{
	if (data_handler_check_update() || (lv_tick_get() % 5000 == 0))
	{
		ESP_LOGI(TAG, "Updating sensor display");
		update_sensor_widgets(sensors_page1, 5);
	}
	update_status_display();
}

void ui_update_callback(void) // for debug
{
	ESP_LOGI(TAG, "Received data update notification");
}

void lv_mainstart(void)
{
	ESP_LOGI(TAG, "Starting LVGL main interface");

	// Register callback functions
	data_handler_register_ui_callback(ui_update_callback);
	ota_manager_register_status_callback(ota_status_callback_debug);
	ota_manager_register_progress_callback(ota_progress_callback_debug);

	lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_lighten(LV_PALETTE_BLUE, 3), LV_PART_MAIN);

	lv_obj_t *title = lv_label_create(lv_scr_act());
	lv_label_set_text(title, "Environmental Monitoring System v1.0");
	lv_obj_set_style_text_font(title, &myFont14, LV_PART_MAIN);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

	// Configure sensor controls
	sensors_page1[0] = (SensorWidget){"Temp", "C", 0, 40, data_handler_get_temperature, NULL, NULL};
	sensors_page1[1] = (SensorWidget){"Humidity", "%", 0, 100, data_handler_get_humidity, NULL, NULL};
	sensors_page1[2] = (SensorWidget){"Air", "ppm", 0, 1000, data_handler_get_air_quality, NULL, NULL};
	sensors_page1[3] = (SensorWidget){"Smoke", "ppm", 0, 5000, data_handler_get_smoke_level, NULL, NULL};
	sensors_page1[4] = (SensorWidget){"Light", "lux", 0, 1000, data_handler_get_light_intensity, NULL, NULL};

	lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);
	lv_obj_set_size(tabview, 600, 480);
	lv_obj_align(tabview, LV_ALIGN_TOP_MID, 0, 50);

	lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Sensors");
	lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "Control");
	lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "Status");

	create_sensor_widgets(tab1, sensors_page1);
	create_control_page(tab2);
	create_status_area(tab3); // Now only creates status display

	// Create OTA button (left side)
	lv_obj_t *ota_btn = lv_btn_create(tab3);
	ota_button_ref = ota_btn;
	lv_obj_set_size(ota_btn, 200, 80);
	lv_obj_align(ota_btn, LV_ALIGN_CENTER, -120, 0); // Left position
	lv_obj_add_event_cb(ota_btn, ota_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_set_style_bg_color(ota_btn, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
	lv_obj_set_style_bg_color(ota_btn, lv_palette_darken(LV_PALETTE_ORANGE, 2), LV_STATE_PRESSED);

	lv_obj_t *ota_btn_label = lv_label_create(ota_btn);
	lv_obj_set_style_text_font(ota_btn_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(ota_btn_label, "START OTA");
	lv_obj_center(ota_btn_label);

	// Create rollback button (right side)
	lv_obj_t *rollback_btn = lv_btn_create(tab3);
	rollback_button_ref = rollback_btn;
	lv_obj_set_size(rollback_btn, 200, 80);
	lv_obj_align(rollback_btn, LV_ALIGN_CENTER, 120, 0); // Right position
	lv_obj_add_event_cb(rollback_btn, rollback_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_set_style_bg_color(rollback_btn, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN);
	lv_obj_set_style_bg_color(rollback_btn, lv_palette_darken(LV_PALETTE_PURPLE, 2), LV_STATE_PRESSED);

	lv_obj_t *rollback_btn_label = lv_label_create(rollback_btn);
	lv_obj_set_style_text_font(rollback_btn_label, &myFont14, LV_PART_MAIN);
	lv_label_set_text(rollback_btn_label, "ROLLBACK");
	lv_obj_center(rollback_btn_label);

	// Create timer
	update_timer = lv_timer_create(update_timer_cb, 1000, NULL);
	ESP_LOGI(TAG, "LVGL main interface initialization completed");
}