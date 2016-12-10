#include "display.h"
#include "task.h"
#include "semphr.h"
#include "string.h"

static TaskHandle_t DISPLAY_TaskHandle;
static QueueHandle_t DISPLAY_TxFinishedSignal;

DISPLAY_Racepage_t DISPLAY_DATA_Racepage;
DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;
DISPLAY_GearACC_t DISPLAY_DATA_GearACC;
DISPLAY_GearControl_t DISPLAY_DATA_GearControl;
DISPLAY_ClutchACC_t DISPLAY_DATA_ClutchACC;
DISPLAY_POWER_FAN_t DISPLAY_DATA_PowerFan;
DISPLAY_POWER_CURRENT_t DISPLAY_DATA_PowerCurrent;

EventGroupHandle_t DISPLAY_NewDataEventHandle;

static inline HAL_StatusTypeDef DISPLAY_SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout);
static HAL_StatusTypeDef DISPLAY_ShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout);
static void DISPLAY_TxFinished(UART_Config_t *config);
static inline uint8_t DISPLAY_CALC_BCC(uint8_t *payload, uint32_t length);
static void DISPLAY_Task(void *arg);

HAL_StatusTypeDef DISPLAY_Init(DISPLAY_Config_t *config)
{
  DISPLAY_NewDataEventHandle = xEventGroupCreate();

  DISPLAY_TxFinishedSignal = xSemaphoreCreateBinary();
  xTaskCreate(DISPLAY_Task, "DISPLAY", DISPLAY_TASK_STACK_SIZE, config, tskIDLE_PRIORITY + 2, &DISPLAY_TaskHandle);

  return HAL_OK;
}

static void DISPLAY_Task(void *arg)
{
  DISPLAY_Config_t *config = (DISPLAY_Config_t *)arg;

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

static inline HAL_StatusTypeDef DISPLAY_SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout)
{
  uint8_t response = 0;

  // send message
  if(HAL_UART_Transmit_IT(config->uart_config->handle, message, length + 3) != HAL_OK)
    return HAL_ERROR;

  // wait for transmit complete
  if(xSemaphoreTake(DISPLAY_TxFinished, timeout) != pdTRUE)
    return HAL_ERROR;

  // read response
  if(HAL_UART_Receive(config->uart_config->handle, &response, sizeof(response), timeout) != HAL_OK)
    return HAL_ERROR;

  // check if command was acknowledged
  if(response != DISPLAY_ACK)
    return HAL_ERROR;

  return HAL_OK;
}

static HAL_StatusTypeDef DISPLAY_ShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout)
{
  static uint8_t buffer[] = {DISPLAY_DC_1, 4, DISPLAY_ESC, 'M', 'N', 0x00, 0x00};

  buffer[6] = macro_number;
  buffer[7] = DISPLAY_CALC_BCC(buffer + 2, 4);

  return DISPLAY_SendCommand(config, buffer, 4 , timeout);
}

static void DISPLAY_TxFinished(UART_Config_t *config)
{
  xSemaphoreGive(DISPLAY_TxFinishedSignal);
}

static inline uint8_t DISPLAY_CALC_BCC(uint8_t *payload, uint32_t length)
{
  uint32_t bcc;

  for(uint32_t i = 0; i < length; ++i)
    bcc += payload[i];

  return bcc;

}
