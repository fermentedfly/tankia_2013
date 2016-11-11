#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#define DISPLAY_TASK_STACK_SIZE 512

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

extern DISPLAY_Clutch_Normal_t DISPLAY_DATA_ClutchNormal;

HAL_StatusTypeDef DISPLAY_Init(void);


#endif
