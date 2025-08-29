#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "xl9555.h"
#include "lvgl_demo.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "data_handler.h"
#include "ota_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

static const char *TAG = "MAIN";

i2c_obj_t i2c0_master;

/* Global status variables */
static bool g_wifi_connected = false;
static bool g_mqtt_connected = false;

/**
 * @brief MQTT data update callback function
 * @param data Sensor data
 */
void mqtt_data_received(const sensor_data_t *data)
{
	ESP_LOGI(TAG, "  Received sensor data:");
	ESP_LOGI(TAG, "  Temperature: %.1fC", data->temperature);
	ESP_LOGI(TAG, "  Humidity: %.1f%%", data->humidity);
	ESP_LOGI(TAG, "  Air quality: %d ppm", data->air_quality);
	ESP_LOGI(TAG, "  Smoke: %d ppm", data->smoke_level);
	ESP_LOGI(TAG, "  Light: %d lux", data->light_intensity);
	ESP_LOGI(TAG, "  Alarm: %d", data->device_alarm);
	// Pass MQTT data to data handler module
	data_handler_process_mqtt_data(data);
}

/**
 * @brief MQTT status change callback function
 * @param status MQTT connection status
 */
void mqtt_status_changed(mqtt_status_t status)
{
	switch (status)
	{
	case MQTT_STATUS_CONNECTED:
		ESP_LOGI(TAG, "MQTT connection successful!");
		g_mqtt_connected = true;
		break;

	case MQTT_STATUS_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT connection disconnected!");
		g_mqtt_connected = false;
		break;

	case MQTT_STATUS_ERROR:
		// ESP_LOGE(TAG, "MQTT connection error!");
		g_mqtt_connected = false;
		break;

	default:
		break;
	}
}
/**
 * @brief WiFi status change callback function
 * @param status WiFi connection status
 */
void wifi_status_changed(wifi_status_t status)
{
	switch (status)
	{
	case WIFI_STATUS_CONNECTED:
		ESP_LOGI(TAG, "WiFi connection successful!");
		g_wifi_connected = true;

		// Start MQTT after WiFi connection is successful
		ESP_LOGI(TAG, "Starting MQTT server connection...");
		if (mqtt_manager_start() != ESP_OK)
		{
			// ESP_LOGE(TAG, "MQTT startup failed");
		}
		break;

	case WIFI_STATUS_DISCONNECTED:
		ESP_LOGI(TAG, "WiFi connection disconnected!");
		g_wifi_connected = false;

		// Stop MQTT when WiFi is disconnected
		mqtt_manager_stop();
		g_mqtt_connected = false;
		break;

	case WIFI_STATUS_FAILED:
		ESP_LOGE(TAG, "WiFi connection failed!");
		g_wifi_connected = false;
		break;

	default:
		break;
	}
}

/**
 * @brief OTA status callback function
 */
void ota_status_callback(ota_status_t status)
{
	switch (status)
	{
	case OTA_STATUS_STARTING:
		ESP_LOGI(TAG, "OTA upgrade starting...");
		break;
	case OTA_STATUS_DOWNLOADING:
		ESP_LOGI(TAG, "Downloading firmware...");
		break;
	case OTA_STATUS_WRITING:
		ESP_LOGI(TAG, "Writing firmware...");
		break;
	case OTA_STATUS_SUCCESS:
		ESP_LOGI(TAG, "OTA upgrade successful!");
		break;
	case OTA_STATUS_FAILED:
		ESP_LOGE(TAG, "OTA upgrade failed!");
		break;
	default:
		break;
	}
}
/**
 * @brief Get system status (for other modules to call)
 */
bool get_wifi_status(void)
{
	return g_wifi_connected;
}

bool get_mqtt_status(void)
{
	return g_mqtt_connected;
}

/**
 * @brief Program entry point
 * @param None
 * @retval None
 */
void app_main(void)
{
	esp_err_t ret; // Store function return value
	ota_manager_print_partition_status();
	ESP_LOGI(TAG, "=== IoT System Startup ===");

	/* 1. NVS initialization */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "NVS initialization completed");

	/* 2. Hardware initialization */
	led_init();						   /* LED initialization */
	i2c0_master = iic_init(I2C_NUM_0); /* Initialize IIC0 */
	xl9555_init(i2c0_master);		   /* IO expansion chip initialization */
	ESP_LOGI(TAG, "Hardware initialization completed");

	/* 3. Data handler module initialization */
	ret = data_handler_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Data handler module initialization failed: %s", esp_err_to_name(ret));
	}
	else
	{
		ESP_LOGI(TAG, "Data handler module initialization completed");
	}

	/* 4. OTA module initialization */
	ota_manager_register_status_callback(ota_status_callback);
	ESP_LOGI(TAG, "OTA module initialization completed");

	/* 5. WiFi and MQTT initialization */
	ret = wifi_manager_init(wifi_status_changed);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "WiFi initialization failed: %s", esp_err_to_name(ret));
	}
	else
	{
		ESP_LOGI(TAG, "WiFi initialization completed");

		/* Start WiFi auto-reconnect task */
		wifi_manager_start_auto_reconnect();

		/* Attempt to connect to WiFi (non-blocking, background connection) */
		ESP_LOGI(TAG, "Starting WiFi connection...");
	}

	/* Initialize MQTT (will automatically start after WiFi connection is successful) */
	ret = mqtt_manager_init(mqtt_data_received, mqtt_status_changed);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "MQTT initialization failed: %s", esp_err_to_name(ret));
	}
	else
	{
		ESP_LOGI(TAG, "MQTT initialization completed");
	}

	esp_err_t mark_ret = esp_ota_mark_app_valid_cancel_rollback();
	/* 6. Start LVGL system */
	ESP_LOGI(TAG, "Starting LVGL display system...");
	lvgl_demo(); /* Run LVGL routine */

	ESP_LOGI(TAG, "=== System startup completed ===");
}