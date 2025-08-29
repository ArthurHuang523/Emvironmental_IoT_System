#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "MQTT_MANAGER";
/* MQTT configuration - Please modify according to your MQTT server */
#define MQTT_BROKER_URI "mqtt://192.168.137.34:1883" // Modify to your MQTT server address
#define MQTT_CLIENT_ID "ESP32_Display"
#define MQTT_USERNAME "" // Fill in username if required
#define MQTT_PASSWORD "" // Fill in password if required

/* MQTT topic definitions - Match topics sent by STM32 */
#define TOPIC_SENSOR_DATA "sensor/data"
#define TOPIC_SENSOR_STATUS "sensor/status"
#define TOPIC_SENSOR_FAULT "sensor/fault"
#define TOPIC_SENSOR_CONTROL "sensor/control"
#define TOPIC_OTA_STATUS "ota/status"
/* MQTT client handle */
static esp_mqtt_client_handle_t mqtt_client = NULL;
static mqtt_data_callback_t data_callback = NULL;
static mqtt_status_callback_t status_callback = NULL;
static bool mqtt_connected = false;

/* Sensor data cache */
static sensor_data_t sensor_cache = {0};

/**
 * @brief Parse sensor data sent by STM32
 * STM32 data format: "Temp:26.5_Humidity:65.2_SmokePPM:80_AirPPM:300_Lightlux:950_Alarm:0_Updatetime:12345"
 * @param data Data string
 * @param data_len Data length
 */
static void parse_stm32_sensor_data(const char *data, int data_len)
{
	char data_str[512] = {0};

	// Ensure data is null-terminated
	if (data_len < sizeof(data_str) - 1)
	{
		memcpy(data_str, data, data_len);
		data_str[data_len] = '\0';
	}
	else
	{
		ESP_LOGW(TAG, "Sensor data too long, truncating");
		memcpy(data_str, data, sizeof(data_str) - 1);
		data_str[sizeof(data_str) - 1] = '\0';
	}

	// ESP_LOGI(TAG, "Received sensor data: %s", data_str);

	// Parse formatted string sent by STM32
	float temp = 0, humi = 0, smoke = 0;
	int air_ppm = 0, light_lux = 0, alarm = 0;
	unsigned long update_time = 0;

	// Use sscanf to parse formatted string
	int parsed = sscanf(data_str, "Temp:%f_Humidity:%f_SmokePPM:%f_AirPPM:%d_Lightlux:%d_Alarm:%d_Updatetime:%lu",
						&temp, &humi, &smoke, &air_ppm, &light_lux, &alarm, &update_time);

	if (parsed >= 6)
	{ // At least parse the first 6 parameters
		sensor_cache.temperature = temp;
		sensor_cache.humidity = humi;
		sensor_cache.smoke_level = (int)smoke;
		sensor_cache.air_quality = air_ppm;
		sensor_cache.light_intensity = light_lux;
		sensor_cache.device_alarm = alarm;
		sensor_cache.timestamp = esp_timer_get_time() / 1000; // Local timestamp

		// ESP_LOGI(TAG, "Parse successful - Temperature:%.1fC Humidity:%.1f%% Smoke:%.0fppm Air:%dppm Light:%dlux Alarm:%d",
		//		 temp, humi, smoke, air_ppm, light_lux, alarm);

		// Notify application layer of data update
		if (data_callback)
		{
			data_callback(&sensor_cache);
		}
	}
	else
	{
		// ESP_LOGE(TAG, "Sensor data parsing failed, parsed %d parameters", parsed);
	}
}

/**
 * @brief MQTT event handler function
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;

	switch ((esp_mqtt_event_id_t)event_id)
	{
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT connection successful");
		mqtt_connected = true;

		// Subscribe to STM32 sensor topics
		esp_mqtt_client_subscribe(client, TOPIC_SENSOR_DATA, 1);
		esp_mqtt_client_subscribe(client, TOPIC_SENSOR_STATUS, 1);
		esp_mqtt_client_subscribe(client, TOPIC_SENSOR_FAULT, 1);
		esp_mqtt_client_subscribe(client, TOPIC_OTA_STATUS, 1);

		ESP_LOGI(TAG, "Subscribed to STM32 sensor topics");

		// Notify application layer of connection status
		if (status_callback)
		{
			status_callback(MQTT_STATUS_CONNECTED);
		}
		break;

	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT connection disconnected");
		mqtt_connected = false;

		// Notify application layer of connection status
		if (status_callback)
		{
			status_callback(MQTT_STATUS_DISCONNECTED);
		}
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "Subscription successful, msg_id=%d", event->msg_id);
		break;

	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "Unsubscription successful, msg_id=%d", event->msg_id);
		break;

	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "Publish successful, msg_id=%d", event->msg_id);
		break;

	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "Received MQTT data");

		// Handle received data
		char topic[64] = {0};
		if (event->topic_len < sizeof(topic))
		{
			memcpy(topic, event->topic, event->topic_len);
			topic[event->topic_len] = '\0';

			// Handle different types of data based on topic
			if (strcmp(topic, TOPIC_SENSOR_DATA) == 0)
			{
				// Handle sensor data
				parse_stm32_sensor_data(event->data, event->data_len);
			}
			else if (strcmp(topic, TOPIC_SENSOR_STATUS) == 0)
			{
				// Handle status information
				char status_str[256] = {0};
				int len = (event->data_len < sizeof(status_str) - 1) ? event->data_len : sizeof(status_str) - 1;
				memcpy(status_str, event->data, len);
				ESP_LOGI(TAG, "Received status information: %s", status_str);
			}
			else if (strcmp(topic, TOPIC_SENSOR_FAULT) == 0)
			{
				// Handle fault information
				char fault_str[256] = {0};
				int len = (event->data_len < sizeof(fault_str) - 1) ? event->data_len : sizeof(fault_str) - 1;
				memcpy(fault_str, event->data, len);
				ESP_LOGI(TAG, "Received fault information: %s", fault_str);
			}
			else if (strcmp(topic, TOPIC_OTA_STATUS) == 0)
			{
				char status_str[8] = {0};
				int len = (event->data_len < sizeof(status_str) - 1) ? event->data_len : sizeof(status_str) - 1;
				memcpy(status_str, event->data, len);

				int ota_available = atoi(status_str);
				ESP_LOGI(TAG, "Received OTA status: %d", ota_available);
				// Notify interface to update OTA notification
				extern void update_ota_notification(bool available);
				update_ota_notification(ota_available == 1);
			}
		}
		break;

	case MQTT_EVENT_ERROR:
		ESP_LOGE(TAG, "MQTT error event");
		if (status_callback)
		{
			status_callback(MQTT_STATUS_ERROR);
		}
		break;

	default:
		ESP_LOGI(TAG, "Other MQTT event: %ld", event_id);
		break;
	}
}

/**
 * @brief Initialize MQTT client
 * @param data_cb Data callback function
 * @param status_cb Status callback function
 * @retval ESP_OK Success, other values failure
 */
esp_err_t mqtt_manager_init(mqtt_data_callback_t data_cb, mqtt_status_callback_t status_cb)
{
	data_callback = data_cb;
	status_callback = status_cb;

	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = MQTT_BROKER_URI,
		.credentials.client_id = MQTT_CLIENT_ID,
		.credentials.username = MQTT_USERNAME,
		.credentials.authentication.password = MQTT_PASSWORD,
		.session.keepalive = 60,
		.session.disable_clean_session = false,
		.network.reconnect_timeout_ms = 5000,
		.network.timeout_ms = 10000,
	};

	mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
	if (mqtt_client == NULL)
	{
		ESP_LOGE(TAG, "MQTT client initialization failed");
		return ESP_FAIL;
	}

	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

	ESP_LOGI(TAG, "MQTT client initialization successful");
	return ESP_OK;
}

/**
 * @brief Start MQTT client
 * @retval ESP_OK Success, other values failure
 */
esp_err_t mqtt_manager_start(void)
{
	if (mqtt_client == NULL)
	{
		ESP_LOGE(TAG, "MQTT client not initialized");
		return ESP_FAIL;
	}

	esp_err_t ret = esp_mqtt_client_start(mqtt_client);
	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MQTT client started successfully");
	}
	else
	{
		ESP_LOGE(TAG, "MQTT client start failed: %s", esp_err_to_name(ret));
	}

	return ret;
}

/**
 * @brief Stop MQTT client
 * @retval ESP_OK Success, other values failure
 */
esp_err_t mqtt_manager_stop(void)
{
	if (mqtt_client == NULL)
	{
		return ESP_OK;
	}

	esp_err_t ret = esp_mqtt_client_stop(mqtt_client);
	if (ret == ESP_OK)
	{
		mqtt_connected = false;
		ESP_LOGI(TAG, "MQTT client stopped successfully");
	}

	return ret;
}

/**
 * @brief Get MQTT connection status
 * @retval true Connected, false Not connected
 */
bool mqtt_manager_is_connected(void)
{
	return mqtt_connected;
}

/**
 * @brief Publish message to MQTT server
 * @param topic Topic
 * @param data Data
 * @param data_len Data length
 * @retval Message ID, -1 indicates failure
 */
int mqtt_manager_publish(const char *topic, const char *data, int data_len)
{
	if (!mqtt_connected || mqtt_client == NULL)
	{
		ESP_LOGW(TAG, "MQTT not connected, cannot publish message");
		return -1;
	}

	return esp_mqtt_client_publish(mqtt_client, topic, data, data_len, 1, 0);
}

/**
 * @brief Send control command to STM32
 * @param command Control command (such as "1", "0", "reset", etc.)
 * @retval Message ID, -1 indicates failure
 */
int mqtt_manager_send_control_command(const char *command)
{
	if (!mqtt_connected || mqtt_client == NULL)
	{
		ESP_LOGW(TAG, "MQTT not connected, cannot send control command");
		return -1;
	}

	// ESP_LOGI(TAG, "Sending control command: %s", command);
	return esp_mqtt_client_publish(mqtt_client, TOPIC_SENSOR_CONTROL, command, strlen(command), 1, 0);
}

/**
 * @brief Get latest sensor data
 * @retval Sensor data pointer
 */
const sensor_data_t *mqtt_manager_get_sensor_data(void)
{
	return &sensor_cache;
}