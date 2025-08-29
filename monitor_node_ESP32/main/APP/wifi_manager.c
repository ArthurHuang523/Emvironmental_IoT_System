#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <string.h>

static const char *TAG = "WIFI_MANAGER";

/* WiFi connection status event group */
static EventGroupHandle_t s_wifi_event_group;

/* WiFi event bit definitions */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/* WiFi reconnect configuration */
#define WIFI_MAXIMUM_RETRY 10
#define WIFI_RECONNECT_DELAY_MS 5000

static int s_retry_num = 0;
static wifi_status_callback_t status_callback = NULL;

/* WiFi configuration*/
#define WIFI_SSID "HXH"
#define WIFI_PASSWORD "Hxh20010523"

/**
 * @brief WiFi event handler
 */
static void event_handler(void *arg, esp_event_base_t event_base,
						  int32_t event_id, void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
		ESP_LOGI(TAG, "WiFi started, connecting...");
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (s_retry_num < WIFI_MAXIMUM_RETRY)
		{
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "Retrying to connect to AP (attempt %d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
		}
		else
		{
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
			ESP_LOGE(TAG, "Failed to connect to WiFi after %d attempts", WIFI_MAXIMUM_RETRY);
		}

		// Notify application layer of connection status
		if (status_callback)
		{
			status_callback(WIFI_STATUS_DISCONNECTED);
		}
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

		// Notify application layer of connection status
		if (status_callback)
		{
			status_callback(WIFI_STATUS_CONNECTED);
		}
	}
}

/**
 * @brief Initialize WiFi
 * @param callback WiFi status callback
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_init(wifi_status_callback_t callback)
{
	status_callback = callback;

	// Create WiFi event group
	s_wifi_event_group = xEventGroupCreate();
	if (s_wifi_event_group == NULL)
	{
		// ESP_LOGE(TAG, "Failed to create event group");
		return ESP_FAIL;
	}

	// Initialize network interface
	ESP_ERROR_CHECK(esp_netif_init());

	// Create default event loop
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	// WiFi initialization configuration
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// Register WiFi event handler
	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&event_handler,
														NULL,
														&instance_any_id));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
														IP_EVENT_STA_GOT_IP,
														&event_handler,
														NULL,
														&instance_got_ip));

	// WiFi configuration
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD,
			.threshold.authmode = WIFI_AUTH_WPA2_PSK, // Minimum security mode
			.pmf_cfg = {
				.capable = true,
				.required = false},
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "WiFi manager initialized. Connecting to %s...", WIFI_SSID);

	return ESP_OK;
}

/**
 * @brief Connect to WiFi (blocking function, waits for result)
 * @param timeout_ms Timeout in milliseconds
 * @retval ESP_OK if connected, ESP_FAIL if failed, ESP_ERR_TIMEOUT if timeout
 */
esp_err_t wifi_manager_connect(uint32_t timeout_ms)
{
	/* Wait for connection success or failure */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
										   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
										   pdFALSE,
										   pdFALSE,
										   pdMS_TO_TICKS(timeout_ms));

	if (bits & WIFI_CONNECTED_BIT)
	{
		ESP_LOGI(TAG, "Connected to WiFi successfully");
		return ESP_OK;
	}
	else if (bits & WIFI_FAIL_BIT)
	{
		ESP_LOGE(TAG, "Failed to connect to WiFi");
		return ESP_FAIL;
	}
	else
	{
		ESP_LOGE(TAG, "WiFi connection timeout");
		return ESP_ERR_TIMEOUT;
	}
}

/**
 * @brief Disconnect from WiFi
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_disconnect(void)
{
	ESP_LOGI(TAG, "Disconnecting from WiFi...");
	return esp_wifi_disconnect();
}

/**
 * @brief Get WiFi connection status
 * @retval true if connected, false otherwise
 */
bool wifi_manager_is_connected(void)
{
	EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
	return (bits & WIFI_CONNECTED_BIT) != 0;
}

/**
 * @brief Get WiFi signal strength
 * @param rssi Pointer to signal strength
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_get_rssi(int8_t *rssi)
{
	wifi_ap_record_t ap_info;
	esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
	if (ret == ESP_OK)
	{
		*rssi = ap_info.rssi;
	}
	return ret;
}

/**
 * @brief Get local IP address as string
 * @param ip_str Buffer for IP address string
 * @param max_len Maximum buffer length
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_get_ip_string(char *ip_str, size_t max_len)
{
	if (!wifi_manager_is_connected())
	{
		return ESP_FAIL;
	}

	esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (netif == NULL)
	{
		return ESP_FAIL;
	}

	esp_netif_ip_info_t ip_info;
	esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
	if (ret == ESP_OK)
	{
		snprintf(ip_str, max_len, IPSTR, IP2STR(&ip_info.ip));
	}

	return ret;
}

/**
 * @brief WiFi auto-reconnect task
 */
static void wifi_reconnect_task(void *pvParameters)
{
	while (1)
	{
		if (!wifi_manager_is_connected())
		{
			ESP_LOGI(TAG, "WiFi disconnected, attempting to reconnect...");
			s_retry_num = 0; // Reset retry count
			esp_wifi_connect();
		}

		vTaskDelay(pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS));
	}
}

/**
 * @brief Start WiFi auto-reconnect task
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t wifi_manager_start_auto_reconnect(void)
{
	static TaskHandle_t reconnect_task_handle = NULL;

	if (reconnect_task_handle == NULL)
	{
		BaseType_t ret = xTaskCreate(wifi_reconnect_task,
									 "wifi_reconnect",
									 2048,
									 NULL,
									 2,
									 &reconnect_task_handle);

		if (ret != pdPASS)
		{
			ESP_LOGE(TAG, "Failed to create WiFi reconnect task");
			return ESP_FAIL;
		}

		ESP_LOGI(TAG, "WiFi auto-reconnect task started");
	}

	return ESP_OK;
}
