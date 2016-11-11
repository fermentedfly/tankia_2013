#include "display.h"
#include "task.h"
#include "cmsis_os.h"

static TaskHandle_t DISPLAY_TaskHandle;

DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;


void DISPLAY_Task(void *arg);

HAL_StatusTypeDef DISPLAY_Init(void)
{
  xTaskCreate(DISPLAY_Task, "display", DISPLAY_TASK_STACK_SIZE, NULL, osPriorityNormal, &DISPLAY_TaskHandle);

  return HAL_OK;
}

void DISPLAY_Task(void *arg)
{
  while(1)
  {
    __NOP();
  }
}
