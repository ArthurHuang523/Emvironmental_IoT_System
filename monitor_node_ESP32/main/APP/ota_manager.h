#ifndef __OTA_MANAGER_H
#define __OTA_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdbool.h>

/* OTA status enumeration */
typedef enum
{
	OTA_STATUS_IDLE = 0,	/* Idle */
	OTA_STATUS_STARTING,	/* Starting upgrade */
	OTA_STATUS_DOWNLOADING, /* Downloading firmware */
	OTA_STATUS_WRITING,		/* Writing firmware */
	OTA_STATUS_SUCCESS,		/* Upgrade successful */
	OTA_STATUS_FAILED		/* Upgrade failed */
} ota_status_t;

/* Version information structure */
typedef struct
{
	char version[32];		  /* Firmware version */
	char project_name[32];	  /* Project name */
	char compile_time[16];	  /* Compile time */
	char compile_date[16];	  /* Compile date */
	char partition_label[16]; /* Current partition label */
} ota_version_info_t;

/* Callback function type definitions */
typedef void (*ota_progress_callback_t)(int progress_percent, int received_bytes, int total_bytes);
typedef void (*ota_status_callback_t)(ota_status_t status);

/* Function declarations */

/**
 * @brief Start OTA upgrade
 * @param firmware_url Firmware URL (if NULL, use default URL)
 * @retval ESP_OK if started successfully, other values on failure
 */
esp_err_t ota_manager_start_upgrade(const char *firmware_url);

/**
 * @brief Restart the system to apply the new firmware
 */
void ota_manager_restart_system(void);

/**
 * @brief Get current OTA status
 * @retval OTA status
 */
ota_status_t ota_manager_get_status(void);

/**
 * @brief Register progress callback function
 * @param callback Pointer to progress callback function
 */
void ota_manager_register_progress_callback(ota_progress_callback_t callback);

/**
 * @brief Register status callback function
 * @param callback Pointer to status callback function
 */
void ota_manager_register_status_callback(ota_status_callback_t callback);

/**
 * @brief Get current firmware version information
 * @param version_info Pointer to version information structure
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t ota_manager_get_version_info(ota_version_info_t *version_info);

/**
 * @brief Check if a rollback version is available
 * @retval true if rollback is possible, false otherwise
 */
bool ota_manager_can_rollback(void);

/**
 * @brief Roll back to the previous firmware version
 * @retval ESP_OK on success, other values on failure
 */
esp_err_t ota_manager_rollback(void);

/**
 * @brief Print partition status information
 */
void ota_manager_print_partition_status(void);

#endif /* __OTA_MANAGER_H */
