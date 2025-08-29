#ifndef __DATA_HANDLER_H
#define __DATA_HANDLER_H

#include "esp_err.h"
#include "mqtt_manager.h"
#include <stdint.h>
#include <stdbool.h>

/* Sensor data structure for display */
typedef struct {
    int temperature;        /* Temperature (°C) - integer */
    int humidity;           /* Humidity (%) - integer */
    int air_quality;        /* Air quality (ppm) */
    int smoke_level;        /* Smoke concentration (ppm) */
    int light_intensity;    /* Light intensity (lux) */
    int device_alarm;       /* Device alarm status */
    bool data_valid;        /* Data validity flag */
    uint64_t last_update;   /* Last update time (milliseconds) */
} sensor_display_data_t;

/* UI update callback function type */
typedef void (*ui_update_callback_t)(void);

/* Function declarations */

/**
 * @brief Data handler module initialization
 * @retval ESP_OK success, other values failure
 */
esp_err_t data_handler_init(void);

/**
 * @brief Register UI update callback function
 * @param callback Callback function pointer
 */
void data_handler_register_ui_callback(ui_update_callback_t callback);

/**
 * @brief Process new sensor data from MQTT
 * @param mqtt_data Sensor data received from MQTT
 */
void data_handler_process_mqtt_data(const sensor_data_t *mqtt_data);

/**
 * @brief Get current display data (thread-safe)
 * @param data Output data structure pointer
 * @retval true Get success, false Get failure
 */
bool data_handler_get_display_data(sensor_display_data_t *data);

/* Individual data access interfaces (for direct LVGL interface calls) */

/**
 * @brief Get temperature value
 * @retval Temperature value (°C)
 */
int data_handler_get_temperature(void);

/**
 * @brief Get humidity value
 * @retval Humidity value (%)
 */
int data_handler_get_humidity(void);

/**
 * @brief Get air quality value
 * @retval Air quality value (ppm)
 */
int data_handler_get_air_quality(void);

/**
 * @brief Get smoke concentration value
 * @retval Smoke concentration value (ppm)
 */
int data_handler_get_smoke_level(void);

/**
 * @brief Get light intensity value
 * @retval Light intensity value (lux)
 */
int data_handler_get_light_intensity(void);

/**
 * @brief Get device alarm status
 * @retval true Alarm present, false Normal
 */
bool data_handler_get_alarm_status(void);

/* Data status check interfaces */

/**
 * @brief Check if data is valid
 * @retval true Data valid, false Data invalid
 */
bool data_handler_is_data_valid(void);

/**
 * @brief Check if data has been updated (for LVGL periodic queries)
 * @retval true New data available, false No update
 */
bool data_handler_check_update(void);

/**
 * @brief Get data timeliness (time since last update)
 * @retval Data age (milliseconds)
 */
uint64_t data_handler_get_data_age(void);

/**
 * @brief Data validity check (check if data has expired)
 * @param timeout_ms Timeout (milliseconds)
 * @retval true Data valid, false Data expired
 */
bool data_handler_is_data_fresh(uint32_t timeout_ms);

#endif /* __DATA_HANDLER_H */