#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "drv_usart.h"

#define DISPLAY_TASK_STACK_SIZE 512

#define DISPLAY_DC_1 0x11
#define DISPLAY_ESC 0x1B
#define DISPLAY_ACK 0x06

typedef enum DISPLAY_NEW_DATA_EVENT
{
  DISPLAY_EVENT_NEW_DATA_RACEPAGE  = 0x01,
  DISPLAY_EVENT_NEW_DATA_GEAR_CONTROL = 0x02,
  DISPLAY_EVENT_NEW_DATA_GEAR_ACC = 0x04,
  DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL = 0x08,
  DISPLAY_EVENT_NEW_DATA_CLUTCH_ACC = 0x10,
  DISPLAY_EVENT_NEW_DATA_POWER_FAN = 0x20,
  DISPLAY_EVENT_NEW_DATA_POWER_CURRENT = 0x40,

} DISPLAY_NEW_DATA_EVENT_t;

#define DISPLAY_EVENT_NEW_DATA_ALL (DISPLAY_EVENT_NEW_DATA_RACEPAGE |\
                                    DISPLAY_EVENT_NEW_DATA_GEAR_CONTROL |\
                                    DISPLAY_EVENT_NEW_DATA_GEAR_ACC |\
                                    DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL |\
                                    DISPLAY_EVENT_NEW_DATA_CLUTCH_ACC\
                                    )

typedef struct DISPLAY_Clutch_Normal
{
  uint8_t clutch_points;
  uint8_t clutch_tolerance;
  uint16_t c_sens_min;
  uint16_t c_sens_max;

} DISPLAY_Clutch_Normal_t;

typedef struct DISPLAY_ClutchACC
{
  uint8_t acc_clutch_k1;
  uint8_t acc_clutch_k2;
  uint8_t acc_clutch_k3;
  uint8_t acc_clutch_p1;
  uint8_t acc_clutch_p2;

} DISPLAY_ClutchACC_t;

typedef struct DISPLAY_GearControl
{
  uint8_t g_min_shift_delay; // *10[ms]
  uint8_t g_up_holdtime; // *10[ms]
  uint8_t g_dn_holdtime; // *10[ms]
  uint8_t g_n_holdtime; // *1[ms]

} DISPLAY_GearControl_t;

typedef struct DISPLAY_GearACC
{
  uint8_t g_acc_max_wspin;
  uint8_t g_acc_min_speed;
  uint8_t g_acc_shift_rpm_1; // RPM/100
  uint8_t g_acc_shift_rpm_2; // RPM/100
  uint8_t g_acc_shift_rpm_3; // RPM/100

} DISPLAY_GearACC_t;

typedef struct DISPLAY_Racepage
{
  uint8_t traction; // traction control level: 0 - 11
  uint8_t map; // mapping: 1 - 2
  uint16_t rev;
  uint8_t ath;
  uint16_t speed;
  uint8_t gear;

} DISPLAY_Racepage_t;

typedef struct DISPLAY_POWER_FAN
{
  uint8_t fan_off_temp;
  uint8_t fan_on_temp;
  uint8_t fan_off_rpm;
  uint8_t fan_on_rpm;

} DISPLAY_POWER_FAN_t;

typedef struct DISPLAY_POWER_CURRENT
{
  uint16_t enable_bitfield;
  uint8_t threshold_value[16];

} DISPLAY_POWER_CURRENT_t;

typedef struct DISPLAY_Config
{
  UART_Config_t *uart_config;

} DISPLAY_Config_t;

extern DISPLAY_Racepage_t DISPLAY_DATA_Racepage;
extern DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;
extern DISPLAY_GearControl_t DISPLAY_DATA_GearControl;
extern DISPLAY_GearACC_t DISPLAY_DATA_GearACC;
extern DISPLAY_ClutchACC_t DISPLAY_DATA_ClutchACC;
extern DISPLAY_POWER_FAN_t DISPLAY_DATA_PowerFan;
extern DISPLAY_POWER_CURRENT_t DISPLAY_DATA_PowerCurrent;

extern EventGroupHandle_t DISPLAY_NewDataEventHandle;

HAL_StatusTypeDef DISPLAY_Init(DISPLAY_Config_t *config);


#endif
