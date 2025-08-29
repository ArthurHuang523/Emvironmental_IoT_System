#ifndef __MQTT_MANAGER_H
#define __MQTT_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* MQTT connection status enumeration */
typedef enum
{
	MQTT_STATUS_DISCONNECTED = 0, /* Disconnected */
	MQTT_STATUS_CONNECTING,		  /* Connecting */
	MQTT_STATUS_CONNECTED,		  /* Connected */
	MQTT_STATUS_ERROR			  /* Connection error */
} mqtt_status_t;

/* Sensor data structure - Match data sent by STM32 */
typedef struct
{
	float temperature;	 /* Temperature (°C) */
	float humidity;		 /* Humidity (%) */
	int smoke_level;	 /* Smoke concentration (ppm) */
	int air_quality;	 /* Air quality (ppm) */
	int light_intensity; /* Light intensity (lux) */
	int device_alarm;	 /* Device alarm status (0/1) */
	uint64_t timestamp;	 /* Timestamp (milliseconds) */
} sensor_data_t;

/* Callback function type definitions */
typedef void (*mqtt_data_callback_t)(const sensor_data_t *data);
typedef void (*mqtt_status_callback_t)(mqtt_status_t status);

/* Function declarations */

/**
 * @brief Initialize MQTT client
 * @param data_cb Data callback function (can be NULL)
 * @param status_cb Status callback function (can be NULL)
 * @retval ESP_OK Success, other values failure
 */
esp_err_t mqtt_manager_init(mqtt_data_callback_t data_cb, mqtt_status_callback_t status_cb);

/**
 * @brief Start MQTT client
 * @retval ESP_OK Success, other values failure
 */
esp_err_t mqtt_manager_start(void);

/**
 * @brief Stop MQTT client
 * @retval ESP_OK Success, other values failure
 */
esp_err_t mqtt_manager_stop(void);

/**
 * @brief Get MQTT connection status
 * @retval true Connected, false Not connected
 */
bool mqtt_manager_is_connected(void);

/**
 * @brief Publish message to MQTT server
 * @param topic Topic
 * @param data Data
 * @param data_len Data length
 * @retval Message ID, -1 indicates failure
 */
int mqtt_manager_publish(const char *topic, const char *data, int data_len);

/**
 * @brief Send control command to STM32
 * @param command Control command string
 * @retval Message ID, -1 indicates failure
 */
int mqtt_manager_send_control_command(const char *command);

/**
 * @brief Get latest sensor data
 * @retval Sensor data pointer
 */
const sensor_data_t *mqtt_manager_get_sensor_data(void);

#endif /* __MQTT_MANAGER_H */