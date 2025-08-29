#include "main.h"
#include "tasks.h"

/* USER CODE BEGIN Header_StartOTATask */
/**
* @brief Function implementing the OTA_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartOTATask */
void StartOTATask(void const * argument)
{
    /* USER CODE BEGIN StartOTATask */
    /* Infinite loop */
    for(;;)
    {
        osDelay(1);
    }
    /* USER CODE END StartOTATask */
}
