#include "display.h"
#include "task.h"
#include "semphr.h"
#include "string.h"

DISPLAY_Racepage_t DISPLAY_DATA_Racepage;
DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;
DISPLAY_GearACC_t DISPLAY_DATA_GearACC;
DISPLAY_GearControl_t DISPLAY_DATA_GearControl;
DISPLAY_ClutchACC_t DISPLAY_DATA_ClutchACC;
DISPLAY_POWER_FAN_t DISPLAY_DATA_PowerFan;
DISPLAY_POWER_CURRENT_t DISPLAY_DATA_PowerCurrent;
DISPLAY_BUTTON_t DISPLAY_DATA_Buttons;

EventGroupHandle_t DISPLAY_NewDataEventHandle;

static inline HAL_StatusTypeDef DISPLAY_SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout);
static HAL_StatusTypeDef DISPLAY_CmdShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout);
static void DISPLAY_Task(void *arg);
static void DISPLAY_Update(TimerHandle_t xTimer);
static inline uint8_t DISPLAY_GetNumberOfMenuEntries(uint8_t menu_id);
static inline uint8_t DISPLAY_GetSubMenuID(uint8_t current_menu, uint8_t cursor_position, uint8_t *is_menu);

HAL_StatusTypeDef DISPLAY_Init(DISPLAY_Config_t *config)
{
  DISPLAY_NewDataEventHandle = xEventGroupCreate();

  if(xTaskCreate(DISPLAY_Task, "DISPLAY", DISPLAY_TASK_STACK_SIZE, config, tskIDLE_PRIORITY + 2, &config->task_handle) != pdPASS)
    return HAL_ERROR;
  config->update_timer = xTimerCreate("DISPLAY UPDATE", 25 / portTICK_PERIOD_MS, pdTRUE, config, DISPLAY_Update);

  return HAL_OK;
}

static void DISPLAY_Task(void *arg)
{
  DISPLAY_Config_t *config = (DISPLAY_Config_t *)arg;

  // show main menu initially
  DISPLAY_CmdShowMacro(config, DISPLAY_MACRO_Main, DISPLAY_TIMEOUT);
  config->current_menu = DISPLAY_MACRO_Main;
  config->in_menu = pdTRUE;

  EventBits_t bits_set = 0;

  while(1)
  {
    // display stuff depending on position

    if(config->in_menu)
    {
      DISPLAY_CmdShowMacro(config, config->current_menu + config->menu_position + 1, DISPLAY_TIMEOUT);

      // we are in a menu, lets wait for some buttons
      xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdTRUE, portMAX_DELAY);

      if(DISPLAY_DATA_Buttons.plus)
        config->menu_position++;

      // check for overflow
      if(config->menu_position >= DISPLAY_GetNumberOfMenuEntries(config->current_menu))
        config->menu_position = 0;

      if(DISPLAY_DATA_Buttons.minus)
        config->menu_position--;

      // check for underflow (unsigned int...)
      if(config->menu_position >= DISPLAY_GetNumberOfMenuEntries(config->current_menu))
        config->menu_position = DISPLAY_GetNumberOfMenuEntries(config->current_menu) - 1;

      if(DISPLAY_DATA_Buttons.enter)
      {
        config->current_menu = DISPLAY_GetSubMenuID(config->current_menu, config->menu_position, &config->in_menu);
        config->menu_position = 0;
        DISPLAY_CmdShowMacro(config, config->current_menu, DISPLAY_TIMEOUT);
      }
    }
    else
    {
      bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_ALL, pdTRUE, pdFALSE, portMAX_DELAY);

      if(bits_set & DISPLAY_EVENT_UPDATE)
      {
        HAL_GPIO_TogglePin(LED_3_GPIO_Port, LED_3_Pin);
      }

      if(bits_set & DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL)
      {

      }
    }
    DISPLAY_DATA_Buttons.enter = pdFALSE;
    DISPLAY_DATA_Buttons.plus = pdFALSE;
    DISPLAY_DATA_Buttons.minus = pdFALSE;
  }
}

static HAL_StatusTypeDef DISPLAY_SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout)
{
  uint8_t response = 0;

  // calculate BCC

  uint32_t bcc = 0;

  for(uint32_t i = 0; i < length - 1; ++i)
    bcc += message[i];

  message[length - 1] = bcc & 0xFF;

  // send message
  if(UART_Transmit(config->uart_config, message, length, timeout) != HAL_OK)
    return HAL_ERROR;

  // read response
  if(HAL_UART_Receive(config->uart_config->handle, &response, sizeof(response), timeout) != HAL_OK)
    return HAL_ERROR;

  // check if command was acknowledged
  if(response != DISPLAY_ACK)
    return HAL_ERROR;

  return HAL_OK;
}

static HAL_StatusTypeDef DISPLAY_CmdShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout)
{
  static uint8_t buffer[] = {DISPLAY_DC_1, 4, DISPLAY_ESC, 'M', 'N', 0x00, 0x00};

  buffer[5] = macro_number;

  return DISPLAY_SendCommand(config, buffer, 7 , timeout);
}

static void DISPLAY_Update(TimerHandle_t xTimer)
{
  xEventGroupSetBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE);
}

static inline uint8_t DISPLAY_GetNumberOfMenuEntries(uint8_t menu_id)
{
  switch(menu_id)
  {
    case DISPLAY_MACRO_Main:
      return 6;
    case DISPLAY_MACRO_ClutchSetup:
      return 4;
    case DISPLAY_MACRO_ClutchNormal:
      return 8;
    case DISPLAY_MACRO_Gear:
      return 3;
    case DISPLAY_MACRO_Power:
      return 3;
    case DISPLAY_MACRO_Diagnose:
      return 6;
    default:
      configASSERT(pdFALSE);
  }
  return 0;
}

static inline uint8_t DISPLAY_GetSubMenuID(uint8_t current_menu, uint8_t cursor_position, uint8_t *is_menu)
{
  switch(current_menu)
  {
    case DISPLAY_MACRO_Main:
      switch(cursor_position)
      {
        case 0:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_RacePage;
        case 1:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_ECU;
        case 2:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_ClutchSetup;
        case 3:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Gear;
        case 4:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Power;
        case 5:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Diagnose;
        default:
          configASSERT(pdFALSE);
      }
    case DISPLAY_MACRO_ClutchSetup:
      switch(cursor_position)
      {
        case 0:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Main;
        case 1:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_ClutchCalibration;
        case 2:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_ClutchNormal;
        case 3:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_ClutchACC;
        default:
          configASSERT(pdFALSE);
      }
    case DISPLAY_MACRO_Gear:
      switch(cursor_position)
      {
        case 0:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Main;
        case 1:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_GearControl;
        case 2:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_GearACC;
        default:
          configASSERT(pdFALSE);
      }
    case DISPLAY_MACRO_Power:
      switch(cursor_position)
      {
        case 0:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Main;
        case 1:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_PowerFanSetup;
        case 2:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_PowerLVPD;
        default:
          configASSERT(pdFALSE);
      }
    case DISPLAY_MACRO_Diagnose:
      switch(cursor_position)
      {
        case 0:
          *is_menu = pdTRUE;
          return DISPLAY_MACRO_Main;
        case 1:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_DiagnoseTireTemp;
        case 2:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_DiagnoseCarStates;
        case 3:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_DiagnoseTireStates;
        case 4:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_DiagnoseBCM;
        case 5:
          *is_menu = pdFALSE;
          return DISPLAY_MACRO_Settings;
        default:
          configASSERT(pdFALSE);
      }
    default:
      configASSERT(pdFALSE);
  }
  return 0;
}

