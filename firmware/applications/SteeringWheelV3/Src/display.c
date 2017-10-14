#include "display.h"
#include "task.h"
#include "semphr.h"
#include "string.h"

typedef enum BargraphsNr
{
	Bargraph_Water = 1,
	Bargraph_Oil = 2,

} BargraphNr_t;

typedef enum TextAlignment
{
  TextAlignment_TopLeft = 1,
  TextAlignment_TopCenter = 2,
  TextAlignment_TopRight = 3,
  TextAlignment_MiddleLeft = 4,
  TextAlignment_MiddleCenter = 5,
  TextAlignment_MiddleRight = 6,
  TextAlignment_BottomLeft = 7,
  TextAlignment_BottomCenter = 8,
  TextAlignment_BottomRight = 9,
} TextAlignment_t;

typedef enum Color
{
  Color_Black = 1,
  Color_White = 8,
} Color_t;

typedef struct Textbox
{
  uint32_t x1;
  uint32_t y1;
  uint32_t x2;
  uint32_t y2;

} Textbox_t;

DISPLAY_Racepage_t DISPLAY_DATA_Racepage;
DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;
DISPLAY_GearACC_t DISPLAY_DATA_GearACC;
DISPLAY_GearControl_t DISPLAY_DATA_GearControl;
DISPLAY_ClutchACC_t DISPLAY_DATA_ClutchACC;
DISPLAY_POWER_FAN_t DISPLAY_DATA_PowerFan;
DISPLAY_POWER_CURRENT_t DISPLAY_DATA_PowerCurrent;
DISPLAY_BUTTON_t DISPLAY_DATA_Buttons;

EventGroupHandle_t DISPLAY_NewDataEventHandle;

static const uint8_t const MenuMainSubAddress[] = {
    DISPLAY_MACRO_RacePage,
    DISPLAY_MACRO_ECU,
    DISPLAY_MACRO_ClutchSetup,
    DISPLAY_MACRO_Gear,
    DISPLAY_MACRO_Power,
    DISPLAY_MACRO_Diagnose
};

static const uint8_t const MenuClutchSubAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_ClutchCalibration,
    DISPLAY_MACRO_ClutchNormal,
    DISPLAY_MACRO_ClutchACC
};

static const uint8_t const MenuGearSubAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_GearControl,
    DISPLAY_MACRO_GearACC,
};

static const uint8_t const MenuPowerSubAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_PowerFanSetup,
    DISPLAY_MACRO_PowerLVPD,
};

static const uint8_t const MenuDiagnoseAddress[] = {
    DISPLAY_MACRO_Main,
    DISPLAY_MACRO_DiagnoseTireTemp,
    DISPLAY_MACRO_DiagnoseCarStates,
    DISPLAY_MACRO_DiagnoseTireStates,
    DISPLAY_MACRO_DiagnoseBCM,
    DISPLAY_MACRO_DiagnoseSettings
};

static const uint8_t const MenuECUAddress[] = {
    DISPLAY_MACRO_Main,
};

static const uint8_t const MenuClutchCalibrationAddress[] = {
    DISPLAY_MACRO_ClutchSetup,
};

static const uint8_t const MenuClutchNormalAddress[] = {
    DISPLAY_MACRO_ClutchSetup,
};

static const uint8_t const * const MenuMapper[] = {
    [DISPLAY_MACRO_Main] = MenuMainSubAddress,
    [DISPLAY_MACRO_ClutchSetup] = MenuClutchSubAddress,
    [DISPLAY_MACRO_Gear] = MenuGearSubAddress,
    [DISPLAY_MACRO_Power] = MenuPowerSubAddress,
    [DISPLAY_MACRO_Diagnose] = MenuDiagnoseAddress,

    [DISPLAY_MACRO_ECU] = MenuECUAddress,
    [DISPLAY_MACRO_ClutchCalibration] = MenuClutchCalibrationAddress,
    [DISPLAY_MACRO_ClutchNormal] = MenuClutchNormalAddress,
};

static const uint8_t const MenuSize[] = {
    [DISPLAY_MACRO_Main] = 6,
    [DISPLAY_MACRO_ClutchSetup] = 4,
    [DISPLAY_MACRO_Gear] = 3,
    [DISPLAY_MACRO_Power] = 3,
    [DISPLAY_MACRO_Diagnose] = 6,

    [DISPLAY_MACRO_ClutchNormal] = 5,
};

static Textbox_t RacepageTextWater = {
    .x1 = 1, .y1 = 5, .x2 = 30, .y2 = 120,
};

static Textbox_t RacepageTextOil = {
    .x1 = 1, .y1 = 120, .x2 = 30, .y2 = 235,
};

static Textbox_t RacepageTextRPM  = {
    .x1 = 208, .y1 = 50, .x2 = 236, .y2 = 150,
};

static Textbox_t RacepageTextSpeed  = {
    .x1 = 248, .y1 = 50, .x2 = 276, .y2 = 150,
};

static Textbox_t RacepageTextEmpty  = {
    .x1 = 288, .y1 = 50, .x2 = 316, .y2 = 150,
};

static HAL_StatusTypeDef SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout);
static HAL_StatusTypeDef CmdShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout);
static HAL_StatusTypeDef CmdSetBargraphValue(DISPLAY_Config_t *config, uint32_t nr, uint32_t value, TickType_t timeout);
static HAL_StatusTypeDef CmdSetFont(DISPLAY_Config_t *config, uint32_t font_number, TickType_t timeout);
static HAL_StatusTypeDef CmdSetFontZoom(DISPLAY_Config_t *config, uint32_t zoom_x, uint32_t zoom_y, TickType_t timeout);
static HAL_StatusTypeDef CmdSetTextColor(DISPLAY_Config_t *config, uint32_t color_fg, uint32_t color_bg, TickType_t timeout);
static HAL_StatusTypeDef CmdSetText(DISPLAY_Config_t *config, Textbox_t *coordintes, TextAlignment_t alignment, uint32_t length, const char *text, TickType_t timeout);
static void Task(void *arg);
static void Update(TimerHandle_t xTimer);
static void ShowMenu(DISPLAY_Config_t *config, uint8_t menu, uint8_t position);
static void Navigate(DISPLAY_Config_t *config, uint8_t base_address, uint8_t plus, uint8_t minus, uint8_t nr_entries);

HAL_StatusTypeDef DISPLAY_Init(DISPLAY_Config_t *config)
{
  DISPLAY_NewDataEventHandle = xEventGroupCreate();

  if(xTaskCreate(Task, "DISPLAY", DISPLAY_TASK_STACK_SIZE, config, tskIDLE_PRIORITY + 2, &config->task_handle) != pdPASS)
    return HAL_ERROR;
  if((config->update_timer = xTimerCreate("DISPLAY UPDATE", 20 / portTICK_PERIOD_MS, pdTRUE, config, Update)) == NULL)
    return HAL_ERROR;
  if(xTimerStart(config->update_timer, portMAX_DELAY) != pdPASS)
    return HAL_ERROR;

  return HAL_OK;
}

static void Task(void *arg)
{
  DISPLAY_Config_t *config = (DISPLAY_Config_t *)arg;

  ShowMenu(config, DISPLAY_MACRO_RacePage, 0);

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

        Navigate(config, DISPLAY_MACRO_MenuInv_1, DISPLAY_DATA_Buttons.plus, DISPLAY_DATA_Buttons.minus, MenuSize[config->current_menu]);

        if(DISPLAY_DATA_Buttons.enter)
        {
          ShowMenu(config, MenuMapper[config->current_menu][config->menu_position], 0);
        }

        break;

      case DISPLAY_MACRO_RacePage:

        bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE | DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        if(bits_set & DISPLAY_EVENT_BUTTON_PRESSED)
        {
          if(DISPLAY_DATA_Buttons.enter)
          {
            ShowMenu(config, DISPLAY_MACRO_Main, 0);
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
        else if(bits_set & DISPLAY_EVENT_UPDATE)
        {
          CmdSetFont(config, 5, DISPLAY_TIMEOUT);
          CmdSetTextColor(config, Color_Black, Color_White, DISPLAY_TIMEOUT);
          CmdSetFontZoom(config, 2, 2, DISPLAY_TIMEOUT);

          // update oil and water bar-graph
		  CmdSetBargraphValue(config, Bargraph_Water, DISPLAY_DATA_Racepage.twat > 0 ? DISPLAY_DATA_Racepage.twat : 0, DISPLAY_TIMEOUT);
		  CmdSetBargraphValue(config, Bargraph_Oil, DISPLAY_DATA_Racepage.toil > 0 ? DISPLAY_DATA_Racepage.toil : 0, DISPLAY_TIMEOUT);

          // show water temperature
          char water_buffer[20];
          uint32_t water_length = snprintf(water_buffer, 20, "%d", DISPLAY_DATA_Racepage.twat);
          CmdSetText(config, &RacepageTextWater, TextAlignment_MiddleRight, water_length + 1, water_buffer, DISPLAY_TIMEOUT);

          // show oil temperature
          char oil_buffer[20];
          uint32_t oil_length = snprintf(oil_buffer, 20, "%d", DISPLAY_DATA_Racepage.toil);
          CmdSetText(config, &RacepageTextOil, TextAlignment_MiddleLeft, oil_length + 1, oil_buffer, DISPLAY_TIMEOUT);

          // show RPM
          char rpm_buffer[20];
          uint32_t rpm_length = snprintf(rpm_buffer, 20, "%.1f", DISPLAY_DATA_Racepage.rev);
          CmdSetText(config, &RacepageTextRPM, TextAlignment_MiddleRight, rpm_length + 1, rpm_buffer, DISPLAY_TIMEOUT);

          // show speed
          char speed_buffer[20];
          uint32_t speed_length = snprintf(speed_buffer, 20, "%.1f", DISPLAY_DATA_Racepage.speed);
          CmdSetText(config, &RacepageTextSpeed, TextAlignment_MiddleRight, speed_length + 1, speed_buffer, DISPLAY_TIMEOUT);

          // show testing value
          CmdSetText(config, &RacepageTextEmpty, TextAlignment_MiddleRight, 6, "empty", DISPLAY_TIMEOUT);



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
            ShowMenu(config, MenuMapper[config->current_menu][config->menu_position], 0);
          }
        }

        break;

      case DISPLAY_MACRO_ClutchCalibration:

        bits_set = xEventGroupWaitBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE | DISPLAY_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);

        if(bits_set & DISPLAY_EVENT_BUTTON_PRESSED)
        {
          if(DISPLAY_DATA_Buttons.enter)
          {
            ShowMenu(config, MenuMapper[config->current_menu][config->menu_position], 0);
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
              ShowMenu(config, MenuMapper[config->current_menu][config->menu_position], 0);
            }
            else
            {
              // toggle edit mode
              config->edit_mode = !config->edit_mode;
              CmdShowMacro(config, DISPLAY_MACRO_PageInv_1 + config->menu_position, DISPLAY_TIMEOUT);
              CmdShowMacro(config, DISPLAY_MACRO_PageValue_1 + config->menu_position, DISPLAY_TIMEOUT);
            }
          }
          if(config->edit_mode)
          {

          }
          else
          {
            Navigate(config, DISPLAY_MACRO_PageInv_1, DISPLAY_DATA_Buttons.plus, DISPLAY_DATA_Buttons.minus, MenuSize[config->current_menu]);
          }
        }
    }
  }
}

static HAL_StatusTypeDef SendCommand(DISPLAY_Config_t *config, uint8_t *message, uint32_t length, TickType_t timeout)
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

static HAL_StatusTypeDef CmdShowMacro(DISPLAY_Config_t *config, uint32_t macro_number, TickType_t timeout)
{
  uint8_t buffer[] = {DISPLAY_DC_1, 4, DISPLAY_ESC, 'M', 'N', macro_number, 0x00};

  return SendCommand(config, buffer, 7 , timeout);
}

static HAL_StatusTypeDef CmdSetBargraphValue(DISPLAY_Config_t *config, uint32_t nr, uint32_t value, TickType_t timeout)
{
	uint8_t buffer[] = {DISPLAY_DC_1, 5, DISPLAY_ESC, 'B', 'A', nr, value, 0x00};

	return SendCommand(config, buffer, 8 , timeout);
}

static HAL_StatusTypeDef CmdSetFont(DISPLAY_Config_t *config, uint32_t font_number, TickType_t timeout)
{
  uint8_t buffer[] = {DISPLAY_DC_1, 4, DISPLAY_ESC, 'Z', 'F', font_number, 0x00};

  return SendCommand(config, buffer, 7 , timeout);
}

static HAL_StatusTypeDef CmdSetFontZoom(DISPLAY_Config_t *config, uint32_t zoom_x, uint32_t zoom_y, TickType_t timeout)
{
  uint8_t buffer[] = {DISPLAY_DC_1, 5, DISPLAY_ESC, 'Z', 'Z', zoom_x, zoom_y, 0x00};

  return SendCommand(config, buffer, 8 , timeout);
}

static HAL_StatusTypeDef CmdSetTextColor(DISPLAY_Config_t *config, uint32_t color_fg, uint32_t color_bg, TickType_t timeout)
{
  uint8_t buffer[] = {DISPLAY_DC_1, 5, DISPLAY_ESC, 'F', 'Z', color_fg, color_bg, 0x00};

  return SendCommand(config, buffer, 8 , timeout);
}

static HAL_StatusTypeDef CmdSetText(DISPLAY_Config_t *config, Textbox_t *coordintes, TextAlignment_t alignment, uint32_t length, const char *text, TickType_t timeout)
{
  uint8_t buffer[15 + length];

  buffer[0] = DISPLAY_DC_1;
  buffer[1] = 12 + length;
  buffer[2] = DISPLAY_ESC;
  buffer[3] = 'Z';
  buffer[4] = 'B';
  buffer[5] = coordintes->x1 & 0x00FF;
  buffer[6] = (coordintes->x1 & 0xFF00)  >> 8;
  buffer[7] = coordintes->y1 & 0x00FF;
  buffer[8] = (coordintes->y1 & 0xFF00)  >> 8;
  buffer[9] = coordintes->x2 & 0x00FF;
  buffer[10] = (coordintes->x2 & 0xFF00)  >> 8;
  buffer[11] = coordintes->y2 & 0x00FF;
  buffer[12] = (coordintes->y2 & 0xFF00)  >> 8;
  buffer[13] = alignment;
  memcpy(buffer + 14, text, length);

  return SendCommand(config, buffer, 15 + length , timeout);
}

static void Update(TimerHandle_t xTimer)
{
  xEventGroupSetBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_UPDATE);
}

static void ShowMenu(DISPLAY_Config_t *config, uint8_t menu, uint8_t position)
{
  config->current_menu = menu;
  config->menu_position = position;
  CmdShowMacro(config, config->current_menu, DISPLAY_TIMEOUT);
}

static void Navigate(DISPLAY_Config_t *config, uint8_t base_address, uint8_t plus, uint8_t minus, uint8_t nr_entries)
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

  CmdShowMacro(config, base_address + old_position, DISPLAY_TIMEOUT);
  CmdShowMacro(config, base_address + config->menu_position, DISPLAY_TIMEOUT);
}

