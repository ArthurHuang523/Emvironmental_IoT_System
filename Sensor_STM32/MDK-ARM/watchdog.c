#include "hardware.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;
static IWDG_HandleTypeDef hiwdg;

typedef struct {
    uint32_t last_heartbeat;
    uint8_t is_alive;
    const char* name;
} TaskMonitor_t;

static TaskMonitor_t tasks[MAX_TASKS] = {
    {0, 0, "Sensor"},
    {0, 0, "Display"},
    {0, 0, "MQTT"},
    {0, 0, "Default"}
};

static uint32_t system_start_time = 0;
static uint8_t watchdog_enabled = 0;

/**
 * @brief init watchdog
 */
HAL_StatusTypeDef Watchdog_Init(uint32_t timeout_ms)
{
    hiwdg.Instance = IWDG;

    // 256 Prescaler
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	
    //  LSI = 40 kHz timeout = reload * prescaler / LSI
    uint32_t reload = (timeout_ms * 40000) / 256 / 1000;

    // Reload range£º0x0001 ~ 0x0FFF
    if (reload > 4095) reload = 4095;
    if (reload < 1) reload = 1;

    hiwdg.Init.Reload = reload;

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        return HAL_ERROR;
    }

    system_start_time = HAL_GetTick();
    watchdog_enabled = 1;

    return HAL_OK;
}


void Watchdog_Feed(void)
{
    if(watchdog_enabled) {
        HAL_IWDG_Refresh(&hiwdg);
    }
}

void Watchdog_Task_Heartbeat(uint8_t task_id)
{
    if(task_id < MAX_TASKS) {
        tasks[task_id].last_heartbeat = HAL_GetTick();
        tasks[task_id].is_alive = 1;
    }
}

/**
 * @brief Check_All_Tasks state
 */
uint8_t Watchdog_Check_All_Tasks(void)
{
    uint32_t current_time = HAL_GetTick();
    uint8_t all_healthy = 1;
	
		//start 30s protect
    if(current_time - system_start_time < 30000) {
        return 1;
    }

    for(uint8_t i = 0; i < MAX_TASKS; i++) {
        uint32_t silence_time = current_time - tasks[i].last_heartbeat;

        if(silence_time > 30000) {
            tasks[i].is_alive = 0;
            all_healthy = 0;
        }
    }

    return all_healthy;
}

void Watchdog_Get_Status(char* buffer, uint16_t size)
{
    if(!buffer) return;
    uint32_t uptime = (HAL_GetTick() - system_start_time) / 1000;
    snprintf(buffer, size,
             "UP:%lus S:%s D:%s M:%s Def:%s",
             uptime,
             tasks[0].is_alive ? "OK" : "ERR",
             tasks[1].is_alive ? "OK" : "ERR",
             tasks[2].is_alive ? "OK" : "ERR",
             tasks[3].is_alive ? "OK" : "ERR");
}
