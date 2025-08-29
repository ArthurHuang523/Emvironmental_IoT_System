#include "main.h"
#include "tasks.h"
#include "hardware.h"
#include <string.h>

void StartDefaultTask(void const * argument)
{
    uint32_t last_feed = 0;
    uint32_t feed_count = 0;
		// Wait for system to be stable
    osDelay(10000);
		// init watchdig and configure 25s timeout
    if(Watchdog_Init(25000) != HAL_OK) {
        while(1) osDelay(1000);
    }
		
    Watchdog_Feed();
    last_feed = HAL_GetTick();
    feed_count = 1;

    for(;;)
    {
        uint32_t now = HAL_GetTick();
        Watchdog_Task_Heartbeat(TASK_ID_DEFAULT);
				// check all task heartbeat
        uint8_t all_ok = Watchdog_Check_All_Tasks();
        if(all_ok) {
            // feed dog every 6s
            if(now - last_feed >= 6000) {
                feed_count++;
                Watchdog_Feed();
                last_feed = now;
            }
        }
        osDelay(2000);
    }
}
