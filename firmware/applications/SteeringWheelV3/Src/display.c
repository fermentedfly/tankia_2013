#include "display.h"
#include "task.h"

static TaskHandle_t DISPLAY_TaskHandle;

DISPLAY_Racepage_t DISPLAY_DATA_Racepage;
DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;
DISPLAY_GearACC_t DISPLAY_DATA_GearACC;
DISPLAY_GearControl_t DISPLAY_DATA_GearControl;
DISPLAY_ClutchACC_t DISPLAY_DATA_ClutchACC;
DISPLAY_POWER_FAN_t DISPLAY_DATA_PowerFan;
DISPLAY_POWER_CURRENT_t DISPLAY_DATA_PowerCurrent;

EventGroupHandle_t DISPLAY_NewDataEventHandle;

void DISPLAY_Task(void *arg);

HAL_StatusTypeDef DISPLAY_Init(void)
{
  DISPLAY_NewDataEventHandle = xEventGroupCreate();

  xTaskCreate(DISPLAY_Task, "DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &DISPLAY_TaskHandle);

  return HAL_OK;
}

void DISPLAY_Task(void *arg)
{
  EventBits_t bits_set = 0;

  while(1)
  {
    bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_ALL, pdTRUE, pdFALSE, portMAX_DELAY);

    if(bits_set & DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL)
    {
      HAL_GPIO_TogglePin(LED_3_GPIO_Port, LED_3_Pin);
    }
  }
}
