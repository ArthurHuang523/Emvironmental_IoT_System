#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* WiFi connection status enumeration */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,   /* Disconnected */
    WIFI_STATUS_CONNECTING,         /* Connecting */
    WIFI_STATUS_CONNECTED,          /* Connected */
    WIFI_STATUS_FAILED              /* Connection failed */
} wifi_status_t;

/* WiFi status callback function type */
typedef void (*wifi_status_callback_t)(wifi_status_t status);

/* Function declarations */

/**
 * @brief Initialize WiFi
 * @param callback WiFi status callback function (can be NULL)
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_init(wifi_status_callback_t callback);

/**
 * @brief Connect to WiFi (blocking function, waits for connection result)
 * @param timeout_ms Timeout in milliseconds, 0 means wait indefinitely
 * @retval ESP_OK if connection successful, ESP_FAIL if failed, ESP_ERR_TIMEOUT if timeout
 */
esp_err_t wifi_manager_connect(uint32_t timeout_ms);

/**
 * @brief Disconnect from WiFi
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get WiFi connection status
 * @retval true if connected, false if not connected
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get WiFi signal strength
 * @param rssi Pointer to store signal strength (dBm)
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_get_rssi(int8_t *rssi);

/**
 * @brief Get local IP address string
 * @param ip_str Buffer to store IP address string
 * @param max_len Maximum buffer length
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_get_ip_string(char *ip_str, size_t max_len);

/**
 * @brief Start WiFi auto-reconnect task
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_start_auto_reconnect(void);

#endif /* __WIFI_MANAGER_H */
