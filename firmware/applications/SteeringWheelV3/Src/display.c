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

static const uint8_t const DISPLAY_MenuMainSubAddress[] = {
    DISPLAY_MACRO_RacePage,
    DISPLAY_MACRO_ECU,
    DISPLAY_MACRO_ClutchSetup,
    DISPLAY_MACRO_Gear,
    DISPLAY_MACRO_Power,
    DISPLAY_MACRO_Diagnose
};

static const uint8_t const DISPLAY_MenuClutchSubAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_ClutchCalibration,
    DISPLAY_MACRO_ClutchNormal,
    DISPLAY_MACRO_ClutchACC
};

static const uint8_t const DISPLAY_MenuGearSubAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_GearControl,
    DISPLAY_MACRO_GearACC,
};

static const uint8_t const DISPLAY_MenuPowerSubAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_PowerFanSetup,
    DISPLAY_MACRO_PowerLVPD,
};

static const uint8_t const DISPLAY_MenuDiagnoseAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_DiagnoseTireTemp,
    DISPLAY_MACRO_DiagnoseCarStates,
    DISPLAY_MACRO_DiagnoseTireStates,
    DISPLAY_MACRO_DiagnoseBCM,
    DISPLAY_MACRO_DiagnoseSettings
};

static const uint8_t const DISPLAY_MenuECUAddress[] = {
    DISPLAY_MACRO_Main,
};

static const uint8_t const DISPLAY_MenuClutchCalibrationAddress[] = {
    DISPLAY_MACRO_ClutchSetup,
};

static const uint8_t const DISPLAY_MenuClutchNormalAddress[] = {
    DISPLAY_MACRO_ClutchSetup,
};

static const uint8_t const * const DISPLAY_MenuMapper[] = {
    [DISPLAY_MACRO_Main] = DISPLAY_MenuMainSubAddress,
    [DISPLAY_MACRO_ClutchSetup] = DISPLAY_MenuClutchSubAddress,
    [DISPLAY_MACRO_Gear] = DISPLAY_MenuGearSubAddress,
    [DISPLAY_MACRO_Power] = DISPLAY_MenuPowerSubAddress,
    [DISPLAY_MACRO_Diagnose] = DISPLAY_MenuDiagnoseAddress,

    [DISPLAY_MACRO_ECU] = DISPLAY_MenuECUAddress,
    [DISPLAY_MACRO_ClutchCalibration] = DISPLAY_MenuClutchCalibrationAddress,
    [DISPLAY_MACRO_ClutchNormal] = DISPLAY_MenuClutchNormalAddress,
};

static const uint8_t const DISPLAY_MenuSize[] = {
    [DISPLAY_MACRO_Main] = 6,
    [DISPLAY_MACRO_ClutchSetup] = 4,
    [DISPLAY_MACRO_Gear] = 3,
    [DISPLAY_MACRO_Power] = 3,
    [DISPLAY_MACRO_Diagnose] = 6,

    [DISPLAY_MACRO_ClutchNormal] = 5,
};

static inline HAL_StatusTypeDef DISPLAY_SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout);
static HAL_StatusTypeDef DISPLAY_CmdShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout);
static void DISPLAY_Task(void *arg);
static void DISPLAY_Update(TimerHandle_t xTimer);
static inline void DISPLAY_ShowMenu(DISPLAY_Config_t *config, uint8_t menu, uint8_t position);
static inline void DISPLAY_Navigate(DISPLAY_Config_t *config, uint8_t base_address, uint8_t plus, uint8_t minus, uint8_t nr_entries);

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
  DISPLAY_ShowMenu(config, DISPLAY_MACRO_Main, 0);

  EventBits_t bits_set = 0;

  while(1)
  {
    DISPLAY_DATA_Buttons.enter = pdFALSE;
    DISPLAY_DATA_Buttons.plus = pdFALSE;
    DISPLAY_DATA_Buttons.minus = pdFALSE;

    // display stuff depending on position

    switch(config->current_menu)
    {
      // all following menus have the same layout -> can be handled the same way
      case DISPLAY_MACRO_Main:
      case DISPLAY_MACRO_ClutchSetup:
      case DISPLAY_MACRO_Gear:
      case DISPLAY_MACRO_Power:
      case DISPLAY_MACRO_Diagnose:

        xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        DISPLAY_Navigate(config, DISPLAY_MACRO_MenuInv_1, DISPLAY_DATA_Buttons.plus, DISPLAY_DATA_Buttons.minus, DISPLAY_MenuSize[config->current_menu]);

        if(DISPLAY_DATA_Buttons.enter)
        {
          DISPLAY_ShowMenu(config, DISPLAY_MenuMapper[config->current_menu][config->menu_position], 0);
        }

        break;

      case DISPLAY_MACRO_RacePage:

        bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE | DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        if(bits_set & DISPLAY_EVENT_BUTTON_PRESSED)
        {
          if(DISPLAY_DATA_Buttons.enter)
          {
            DISPLAY_ShowMenu(config, DISPLAY_MACRO_Main, 0);
          }
          else if(DISPLAY_DATA_Buttons.plus)
          {
            // TODO Shift UP
          }

          else if(DISPLAY_DATA_Buttons.minus)
          {
            // TODO Shift DOWN
          }
        }
        // TODO display stuff if some other bits are set

        break;

      case DISPLAY_MACRO_ECU:

        // read only page, just wait for button to exit
        bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE | DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        if(bits_set & DISPLAY_EVENT_BUTTON_PRESSED)
        {
          if(DISPLAY_DATA_Buttons.enter)
          {
            DISPLAY_ShowMenu(config, DISPLAY_MenuMapper[config->current_menu][config->menu_position], 0);
          }
        }

        break;

      case DISPLAY_MACRO_ClutchCalibration:

        bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE | DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        if(bits_set & DISPLAY_EVENT_BUTTON_PRESSED)
        {
          if(DISPLAY_DATA_Buttons.enter)
          {
            DISPLAY_ShowMenu(config, DISPLAY_MenuMapper[config->current_menu][config->menu_position], 0);
          }
          else if(DISPLAY_DATA_Buttons.plus)
          {
            // TODO Change Clutch Poti Calibration
          }

          else if(DISPLAY_DATA_Buttons.minus)
          {
            // TODO Change Clutch Poti Calibration
          }
        }

      case DISPLAY_MACRO_ClutchNormal:

        bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE | DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        if(bits_set & DISPLAY_EVENT_BUTTON_PRESSED)
        {
          if(DISPLAY_DATA_Buttons.enter)
          {
            if(config->menu_position == 0)
            {
              DISPLAY_ShowMenu(config, DISPLAY_MenuMapper[config->current_menu][config->menu_position], 0);
            }
            else
            {
              // toggle edit mode
              config->edit_mode = !config->edit_mode;
              DISPLAY_CmdShowMacro(config, DISPLAY_MACRO_PageInv_1 + config->menu_position, DISPLAY_TIMEOUT);
              DISPLAY_CmdShowMacro(config, DISPLAY_MACRO_PageValue_1 + config->menu_position, DISPLAY_TIMEOUT);
            }
          }
          if(config->edit_mode)
          {

          }
          else
          {
            DISPLAY_Navigate(config, DISPLAY_MACRO_PageInv_1, DISPLAY_DATA_Buttons.plus, DISPLAY_DATA_Buttons.minus, DISPLAY_MenuSize[config->current_menu]);
          }
        }
    }
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

static inline void DISPLAY_ShowMenu(DISPLAY_Config_t *config, uint8_t menu, uint8_t position)
{
  config->current_menu = menu;
  config->menu_position = position;
  DISPLAY_CmdShowMacro(config, config->current_menu, DISPLAY_TIMEOUT);
}

static inline void DISPLAY_Navigate(DISPLAY_Config_t *config, uint8_t base_address, uint8_t plus, uint8_t minus, uint8_t nr_entries)
{
  if(!plus && !minus)
    return;

  uint8_t old_position = config->menu_position;

  if(plus)
    config->menu_position++;

  // check for overflow
  if(config->menu_position >= nr_entries)
    config->menu_position = 0;

  if(minus)
    config->menu_position--;

  // check for underflow (unsigned int...)
  if(config->menu_position >= nr_entries)
    config->menu_position = nr_entries - 1;

  DISPLAY_CmdShowMacro(config, base_address + old_position, DISPLAY_TIMEOUT);
  DISPLAY_CmdShowMacro(config, base_address + config->menu_position, DISPLAY_TIMEOUT);
}

