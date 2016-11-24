/*
 * rpm_leds.h
 *
 *  Created on: Nov 24, 2016
 *      Author: manuel
 */

#ifndef INC_RPM_LEDS_H_
#define INC_RPM_LEDS_H_

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "dev_max7313.h"

#define RPM_LEDS_TASK_STACK_SIZE 128

#define WATER_TEMP_CRITICAL 105
#define OIL_TEMP_CRITICAL 115
#define TEMPERATURE_HYSTERESIS 5

enum RPM_LEDS_NEW_DATA_EVENT
{
  RPM_LEDS_EVENT_NEW_DATA_REV = 0x1,
  RPM_LEDS_EVENT_NEW_DATA_TEMPERATURE = 0x02,
  RPM_LEDS_EVENT_UPDATE_LEDS = 0x04,

};


#define RPM_LEDS_EVENT_NEW_DATA_ALL (RPM_LEDS_EVENT_NEW_DATA_REV | RPM_LEDS_EVENT_NEW_DATA_TEMPERATURE | RPM_LEDS_EVENT_UPDATE_LEDS)

enum RPM_LEDS_LED
{
  RPM_LEDS_LED_OIL  = 0x001,

  RPM_LEDS_LED_01     = 0x002,
  RPM_LEDS_LED_02     = 0x004,
  RPM_LEDS_LED_03     = 0x008,
  RPM_LEDS_LED_04     = 0x010,
  RPM_LEDS_LED_05     = 0x020,
  RPM_LEDS_LED_06     = 0x040,
  RPM_LEDS_LED_07     = 0x080,
  RPM_LEDS_LED_08     = 0x100,
  RPM_LEDS_LED_09     = 0x200,
  RPM_LEDS_LED_10     = 0x400,

  RPM_LEDS_LED_WATER  = 0x800,
};

extern EventGroupHandle_t RPM_LEDS_NewDataEventHandle;
extern uint16_t RPM_LEDS_rev;
extern uint8_t RPM_LEDS_TOil;
extern uint8_t RPM_LEDS_TWater;

HAL_StatusTypeDef RPM_LEDS_Init(MAX7313_Config_t *max_config);

#endif /* INC_RPM_LEDS_H_ */
