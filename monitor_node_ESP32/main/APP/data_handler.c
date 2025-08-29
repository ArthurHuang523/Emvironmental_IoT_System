#include "esp_timer.h"
#include "data_handler.h"
#include "mqtt_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "DATA_HANDLER";

/* Data caching and synchronization */
static sensor_display_data_t display_data = {0};
static SemaphoreHandle_t data_mutex = NULL;
static bool data_updated = false;

/* LVGL update callback function pointer */
static ui_update_callback_t ui_update_callback = NULL;

/**
 * @brief Data handler module initialization
 */
esp_err_t data_handler_init(void)
{
	// Create data protection mutex
	data_mutex = xSemaphoreCreateMutex();
	if (data_mutex == NULL)
	{
		// ESP_LOGE(TAG, "Failed to create data mutex");
		return ESP_FAIL;
	}

	// Initialize display data to default values
	display_data.temperature = 0;
	display_data.humidity = 0;
	display_data.air_quality = 0;
	display_data.smoke_level = 0;
	display_data.light_intensity = 0;
	display_data.device_alarm = 0;
	display_data.data_valid = false;
	display_data.last_update = 0;

	ESP_LOGI(TAG, "Data handler module initialization completed");
	return ESP_OK;
}

/**
 * @brief Register UI update callback function
 * @param callback Callback function pointer
 */
void data_handler_register_ui_callback(ui_update_callback_t callback)
{
	ui_update_callback = callback;
	ESP_LOGI(TAG, "UI update callback registered");
}

/**
 * @brief Process new sensor data from MQTT
 * @param mqtt_data Sensor data received from MQTT
 */
void data_handler_process_mqtt_data(const sensor_data_t *mqtt_data)
{
	if (mqtt_data == NULL)
	{
		return;
	}

	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
	{
		// Update display data
		display_data.temperature = (int)(mqtt_data->temperature + 0.5f); // Round to nearest integer
		display_data.humidity = (int)(mqtt_data->humidity + 0.5f);
		display_data.air_quality = mqtt_data->air_quality;
		display_data.smoke_level = mqtt_data->smoke_level;
		display_data.light_intensity = mqtt_data->light_intensity;
		display_data.device_alarm = mqtt_data->device_alarm;
		display_data.data_valid = true;
		display_data.last_update = esp_timer_get_time() / 1000; // Convert to milliseconds

		data_updated = true;

		xSemaphoreGive(data_mutex);

		ESP_LOGI(TAG, "Data updated - T:%d°C H:%d%% Air:%dppm Smoke:%dppm Light:%dlux Alarm:%d",
				 display_data.temperature, display_data.humidity,
				 display_data.air_quality, display_data.smoke_level,
				 display_data.light_intensity, display_data.device_alarm);

		// Notify UI update
		if (ui_update_callback)
		{
			ui_update_callback();
		}
	}
	else
	{
		ESP_LOGW(TAG, "Cannot acquire data lock, data update skipped");
	}
}

/**
 * @brief Get current display data (thread-safe)
 * @param data Output data structure pointer
 * @retval true Get success, false Get failure
 */
bool data_handler_get_display_data(sensor_display_data_t *data)
{
	if (data == NULL)
	{
		return false;
	}

	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
	{
		memcpy(data, &display_data, sizeof(sensor_display_data_t));
		xSemaphoreGive(data_mutex);
		return true;
	}

	return false;
}

/**
 * @brief Get temperature value (for LVGL interface calls)
 */
int data_handler_get_temperature(void)
{
	int value = 25; // Default value
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		value = display_data.temperature;
		xSemaphoreGive(data_mutex);
	}
	return value;
}

/**
 * @brief Get humidity value (for LVGL interface calls)
 */
int data_handler_get_humidity(void)
{
	int value = 50; // Default value
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		value = display_data.humidity;
		xSemaphoreGive(data_mutex);
	}
	return value;
}

/**
 * @brief Get air quality value (for LVGL interface calls)
 */
int data_handler_get_air_quality(void)
{
	int value = 200; // Default value
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		value = display_data.air_quality;
		xSemaphoreGive(data_mutex);
	}
	return value;
}

/**
 * @brief Get smoke concentration value (for LVGL interface calls)
 */
int data_handler_get_smoke_level(void)
{
	int value = 10; // Default value
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		value = display_data.smoke_level;
		xSemaphoreGive(data_mutex);
	}
	return value;
}

/**
 * @brief Get light intensity value (for LVGL interface calls)
 */
int data_handler_get_light_intensity(void)
{
	int value = 500; // Default value
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		value = display_data.light_intensity;
		xSemaphoreGive(data_mutex);
	}
	return value;
}

/**
 * @brief Get device alarm status (for LVGL interface calls)
 */
bool data_handler_get_alarm_status(void)
{
	bool alarm = false;
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		alarm = (display_data.device_alarm != 0);
		xSemaphoreGive(data_mutex);
	}
	return alarm;
}

/**
 * @brief Check if data is valid
 */
bool data_handler_is_data_valid(void)
{
	bool valid = false;
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		valid = display_data.data_valid;
		xSemaphoreGive(data_mutex);
	}
	return valid;
}

/**
 * @brief Check if data has been updated (for LVGL periodic queries)
 */
bool data_handler_check_update(void)
{
	bool updated = false;
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		updated = data_updated;
		if (updated)
		{
			data_updated = false; // Clear update flag
		}
		xSemaphoreGive(data_mutex);
	}
	return updated;
}

/**
 * @brief Get data timeliness (time since last update, milliseconds)
 */
uint64_t data_handler_get_data_age(void)
{
	uint64_t age = 0;
	if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{
		uint64_t now = esp_timer_get_time() / 1000;
		if (display_data.last_update > 0)
		{
			age = now - display_data.last_update;
		}
		xSemaphoreGive(data_mutex);
	}
	return age;
}

/**
 * @brief Data validity check (check if data has expired)
 * @param timeout_ms Timeout (milliseconds)
 * @retval true Data valid, false Data expired
 */
bool data_handler_is_data_fresh(uint32_t timeout_ms)
{
	return (data_handler_get_data_age() < timeout_ms) && data_handler_is_data_valid();
}