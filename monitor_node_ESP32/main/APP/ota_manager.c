#include "ota_manager.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "wifi_manager.h"
#include <string.h>

static const char *TAG = "OTA_MANAGER";
static const char server_cert_pem[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDETCCAfmgAwIBAgIUcU1eT7lJfwGFXG6RUyVJfX2cSIowDQYJKoZIhvcNAQEL\n"
	"BQAwGDEWMBQGA1UEAwwNMTkyLjE2OC4xMzcuMTAeFw0yNTA4MTIwMDQ0NTBaFw0y\n"
	"NjA4MTIwMDQ0NTBaMBgxFjAUBgNVBAMMDTE5Mi4xNjguMTM3LjEwggEiMA0GCSqG\n"
	"SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCZRq0AM/2FHKaiji3o24M6JUkzz9fOG/Yc\n"
	"mFFtdtlq2s6lmtXQmUlDx9yRGfH62aOmAhasAvvrBQoosArcp9p0vcNUZ4uiQHN8\n"
	"/ybtO2fsxj3CHp4kU3EEQRx0rsYFS+tRDpO5ywZxnWv47zpw1ahv6o5TYmywxou/\n"
	"xaXoknEhaL1JMNnkCIR9wkRqSGSbfu5M9vy4ApgKgQBntz4zA7ioMEBzOOnljU4F\n"
	"+Rf4sJIS4N34KmHO+vVf2ErlPXB/0wpc8ZfLrv+2z/f4qUYcGVmWLNojbKzx6XVZ\n"
	"tu8M4mKHr/4CfuJYAvtDt0IqN/VpPk/8cfRjoVqy6prtiMMyKl11AgMBAAGjUzBR\n"
	"MB0GA1UdDgQWBBTaSvbijDY5gYcZAakR6VrdJet/dzAfBgNVHSMEGDAWgBTaSvbi\n"
	"jDY5gYcZAakR6VrdJet/dzAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUA\n"
	"A4IBAQCG8DGztUC1QKYUHzzKUe2jh8GM6YPZ9RSp0p8+ZJHUgc5/cv5HrIdE6sWX\n"
	"33PrWQ79hD7vxHp26rstm8wqoj8bQmZTJpwgPvFW4pQY+voXCns3GTp/u8u2c8G+\n"
	"YCszjT41b+f2raz/kXPIdNRscqUonBIm4bnqXixFxPKYprMZnsyousT3CfIefYbK\n"
	"xO9u9N+2Pn/fUS6NJ7T6C55aqdw5UZn1SVA0IUrDY+IS/iWW/3fGG9lyztb0HPsg\n"
	"FBn+jcrHYAQ/YpBVkrmKLgN6vD3wtzHjR+Urr9kYa02pSyfQEptr6NJgK6LmclX4\n"
	"EyQfiK0C33hNEqENochY+Wg0LWvG\n"
	"-----END CERTIFICATE-----\n";
/* OTA Configure */
#define OTA_FIRMWARE_URL "https://192.168.137.1:9443/firmware_new.bin"
#define OTA_RECV_TIMEOUT 5000
#define OTA_BUFFER_SIZE 1024

/* OTA State Management */
static ota_status_t current_ota_status = OTA_STATUS_IDLE;
static ota_progress_callback_t progress_callback = NULL;
static ota_status_callback_t status_callback = NULL;

/* OTA handle */
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *ota_partition = NULL;
static int total_bytes = 0;
static int received_bytes = 0;

/**
 * @brief print partition state
 */
void ota_manager_print_partition_status(void)
{
	const esp_partition_t *running = esp_ota_get_running_partition();
	const esp_partition_t *next_update = esp_ota_get_next_update_partition(NULL);
	const esp_partition_t *last_invalid = esp_ota_get_last_invalid_partition();

	ESP_LOGI(TAG, "=== Partition Status ===");
	ESP_LOGI(TAG, "Running Partition: %s", running ? running->label : "unknown");
	ESP_LOGI(TAG, "Update Partition: %s", next_update ? next_update->label : "unknown");
	ESP_LOGI(TAG, "Rollback Available: %s", last_invalid ? "yes" : "no");
}

/**
 * @brief notify status change
 */
static void notify_status_change(ota_status_t status)
{
	current_ota_status = status;
	if (status_callback)
	{
		status_callback(status);
	}
}

/**
 * @brief notify progress change
 */
static void notify_progress_change(int progress_percent)
{
	if (progress_callback)
	{
		progress_callback(progress_percent, received_bytes, total_bytes);
	}
}

/**
 * @brief HTTP event handler
 */
esp_err_t ota_http_event_handler(esp_http_client_event_t *evt)
{
	switch (evt->event_id)
	{
	case HTTP_EVENT_ERROR:
		ESP_LOGE(TAG, "HTTP error");
		break;

	case HTTP_EVENT_ON_CONNECTED:
		ESP_LOGI(TAG, "HTTP connected");
		break;

	case HTTP_EVENT_HEADER_SENT:
		ESP_LOGI(TAG, "HTTP header sent");
		break;

	case HTTP_EVENT_ON_HEADER:
		if (strcasecmp(evt->header_key, "Content-Length") == 0)
		{
			total_bytes = atoi(evt->header_value);
			ESP_LOGI(TAG, "Firmware size: %d bytes", total_bytes);
		}
		break;

	case HTTP_EVENT_ON_DATA:
		// Write OTA data
		if (ota_handle != 0 && evt->data_len > 0)
		{
			esp_err_t err = esp_ota_write(ota_handle, evt->data, evt->data_len);
			if (err != ESP_OK)
			{
				ESP_LOGE(TAG, "OTA Write Error: %s", esp_err_to_name(err));
				return err;
			}

			received_bytes += evt->data_len;

			// Calculate and notify progress
			if (total_bytes > 0)
			{
				int progress = (received_bytes * 100) / total_bytes;
				notify_progress_change(progress);

				if (received_bytes % 10240 == 0)
				{ // print every 10KB
				  // ESP_LOGI(TAG, "OTA progress rate: %d%% (%d/%d bytes)",
				  // 		 progress, received_bytes, total_bytes);
				}
			}
		}
		break;

	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(TAG, "HTTP Request completed");
		break;

	case HTTP_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "HTTP Disconnected");
		break;

	default:
		break;
	}
	return ESP_OK;
}

/**
 * @brief Start OTA upgrade
 * @param firmware_url Firmware URL (if NULL, use default URL)
 * @retval ESP_OK success, otherwise failed
 */
esp_err_t ota_manager_start_upgrade(const char *firmware_url)
{
	esp_err_t ret = ESP_OK;

	// Check WiFi connection
	if (!wifi_manager_is_connected())
	{
		ESP_LOGE(TAG, "WiFi not connected, cannot perform OTA upgrade");
		notify_status_change(OTA_STATUS_FAILED);
		return ESP_FAIL;
	}

	// Check if OTA is already running
	if (current_ota_status == OTA_STATUS_DOWNLOADING ||
		current_ota_status == OTA_STATUS_WRITING)
	{
		ESP_LOGW(TAG, "OTA upgrade already in progress");
		return ESP_ERR_INVALID_STATE;
	}

	notify_status_change(OTA_STATUS_STARTING);
	ESP_LOGI(TAG, "Starting OTA upgrade...");

	// Reset progress counter
	total_bytes = 0;
	received_bytes = 0;

	// Get next OTA partition
	ota_partition = esp_ota_get_next_update_partition(NULL);
	if (ota_partition == NULL)
	{
		// ESP_LOGE(TAG, "Failed to get OTA partition");
		notify_status_change(OTA_STATUS_FAILED);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "OTA Partition: %s, Offset: 0x%lx, Size: %ld",
			 ota_partition->label, ota_partition->address, ota_partition->size);

	// Begin OTA operation
	ret = esp_ota_begin(ota_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(ret));
		notify_status_change(OTA_STATUS_FAILED);
		return ret;
	}

	// Configure HTTP client
	const char *url = firmware_url ? firmware_url : OTA_FIRMWARE_URL;
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = ota_http_event_handler,
		.timeout_ms = 10000,
		.buffer_size = OTA_BUFFER_SIZE,
		.buffer_size_tx = OTA_BUFFER_SIZE,
		.transport_type = HTTP_TRANSPORT_OVER_SSL,
		.cert_pem = server_cert_pem,		 // Use our certificate
		.skip_cert_common_name_check = true, // Skip CN check
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL)
	{
		ESP_LOGE(TAG, "Failed to initialize HTTP client");
		esp_ota_abort(ota_handle);
		ota_handle = 0;
		notify_status_change(OTA_STATUS_FAILED);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Start downloading firmware from URL: %s", url);
	notify_status_change(OTA_STATUS_DOWNLOADING);

	// Perform HTTP request
	ret = esp_http_client_perform(client);

	if (ret == ESP_OK)
	{
		int status_code = esp_http_client_get_status_code(client);
		if (status_code == 200)
		{
			ESP_LOGI(TAG, "Firmware downloaded, start verifying...");
			notify_status_change(OTA_STATUS_WRITING);

			// Finish OTA writing
			ret = esp_ota_end(ota_handle);
			if (ret != ESP_OK)
			{
				if (ret == ESP_ERR_OTA_VALIDATE_FAILED)
				{
					ESP_LOGE(TAG, "Firmware validation failed");
				}
				else
				{
					ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(ret));
				}
				esp_ota_abort(ota_handle);
				notify_status_change(OTA_STATUS_FAILED);
			}
			else
			{
				// Set boot partition
				ret = esp_ota_set_boot_partition(ota_partition);
				if (ret != ESP_OK)
				{
					ESP_LOGE(TAG, "Set boot partition failed: %s", esp_err_to_name(ret));
					notify_status_change(OTA_STATUS_FAILED);
				}
				else
				{
					// ESP_LOGI(TAG, "System will restart in 2 seconds...");
					// vTaskDelay(pdMS_TO_TICKS(2000));
					// esp_restart();
					esp_err_t mark_ret = esp_ota_mark_app_valid_cancel_rollback();
					if (mark_ret == ESP_OK)
					{
						ESP_LOGI(TAG, "App marked as valid, rollback canceled");
					}
					else
					{
						ESP_LOGE(TAG, "Failed to mark app as valid: %s", esp_err_to_name(mark_ret));
					}
					ESP_LOGI(TAG, "System will restart in 2 seconds...");
					vTaskDelay(pdMS_TO_TICKS(2000));
					esp_restart();
				}
			}
		}
		else
		{
			ESP_LOGE(TAG, "HTTP error code: %d", status_code);
			esp_ota_abort(ota_handle);
			notify_status_change(OTA_STATUS_FAILED);
			ret = ESP_FAIL;
		}
	}
	else
	{
		ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(ret));
		esp_ota_abort(ota_handle);
		notify_status_change(OTA_STATUS_FAILED);
	}

	// Clean up
	esp_http_client_cleanup(client);
	ota_handle = 0;

	return ret;
}

/**
 * @brief Restart system with new firmware
 */
void ota_manager_restart_system(void)
{
	ESP_LOGI(TAG, "System will restart in 3 seconds...");
	vTaskDelay(pdMS_TO_TICKS(3000));
	esp_restart();
}

/**
 * @brief Get current OTA status
 */
ota_status_t ota_manager_get_status(void)
{
	return current_ota_status;
}

/**
 * @brief Register progress callback
 */
void ota_manager_register_progress_callback(ota_progress_callback_t callback)
{
	progress_callback = callback;
}

/**
 * @brief Register status callback
 */
void ota_manager_register_status_callback(ota_status_callback_t callback)
{
	status_callback = callback;
}

/**
 * @brief Get current firmware version info
 */
esp_err_t ota_manager_get_version_info(ota_version_info_t *version_info)
{
	if (version_info == NULL)
	{
		return ESP_ERR_INVALID_ARG;
	}

	const esp_app_desc_t *app_desc = esp_ota_get_app_description();
	if (app_desc == NULL)
	{
		return ESP_FAIL;
	}

	strncpy(version_info->version, app_desc->version, sizeof(version_info->version) - 1);
	strncpy(version_info->project_name, app_desc->project_name, sizeof(version_info->project_name) - 1);
	strncpy(version_info->compile_time, app_desc->time, sizeof(version_info->compile_time) - 1);
	strncpy(version_info->compile_date, app_desc->date, sizeof(version_info->compile_date) - 1);

	// Get current running partition info
	const esp_partition_t *running_partition = esp_ota_get_running_partition();
	if (running_partition)
	{
		strncpy(version_info->partition_label, running_partition->label, sizeof(version_info->partition_label) - 1);
	}

	return ESP_OK;
}

/**
 * @brief Check if rollback is possible
 */
bool ota_manager_can_rollback(void)
{
	const esp_partition_t *running = esp_ota_get_running_partition();
	const esp_partition_t *other_ota = esp_ota_get_next_update_partition(NULL);

	if (!running || !other_ota)
	{
		ESP_LOGE(TAG, "Failed to get partition info");
		return false;
	}

	ESP_LOGI(TAG, "Rollback check: running=%s, other=%s", running->label, other_ota->label);

	// Check other OTA partition state
	esp_ota_img_states_t ota_state;
	esp_err_t ret = esp_ota_get_state_partition(other_ota, &ota_state);

	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get partition state: %s", esp_err_to_name(ret));
		return false;
	}

	ESP_LOGI(TAG, "Other partition state: %d", ota_state);

	// Rollback possible if other partition is not invalid
	bool can_rollback = (ota_state != ESP_OTA_IMG_INVALID);

	ESP_LOGI(TAG, "Rollback result: %s", can_rollback ? "possible" : "not possible");

	return can_rollback;
}

/**
 * @brief Rollback to previous version
 */
esp_err_t ota_manager_rollback(void)
{
	const esp_partition_t *running = esp_ota_get_running_partition();
	const esp_partition_t *other_ota = esp_ota_get_next_update_partition(NULL);

	if (!running || !other_ota)
	{
		ESP_LOGE(TAG, "Failed to get partition info");
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Preparing rollback: from %s to %s", running->label, other_ota->label);

	// Check target partition validity
	esp_ota_img_states_t ota_state;
	esp_err_t ret = esp_ota_get_state_partition(other_ota, &ota_state);

	if (ret != ESP_OK || ota_state == ESP_OTA_IMG_INVALID)
	{
		ESP_LOGE(TAG, "Target partition invalid, rollback not possible");
		return ESP_FAIL;
	}

	// Set boot partition
	ret = esp_ota_set_boot_partition(other_ota);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Rollback failed: %s", esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG, "Rollback successful, rebooting into partition: %s", other_ota->label);

	vTaskDelay(pdMS_TO_TICKS(3000));
	esp_restart();

	return ESP_OK;
}
