/*
 * rpm_leds.c
 *
 *  Created on: Nov 24, 2016
 *      Author: manuel
 */

#include "rpm_leds.h"
#include "FreeRTOS.h"
#include "timers.h"

EventGroupHandle_t RPM_LEDS_NewDataEventHandle;
uint16_t RPM_LEDS_rev;
uint8_t RPM_LEDS_TOil;
uint8_t RPM_LEDS_TWater;

static TaskHandle_t RPM_LEDS_TaskHandle;
static TimerHandle_t RPM_LEDS_UpdateTimer;

void RPM_LEDS_Task(void *arg);
void RPM_LEDS_Update(TimerHandle_t xTimer);

HAL_StatusTypeDef RPM_LEDS_Init(MAX7313_Config_t *max_config)
{
  if(MAX7313_Init(max_config) != HAL_OK)
    return HAL_ERROR;

  RPM_LEDS_NewDataEventHandle = xEventGroupCreate();

  xTaskCreate(RPM_LEDS_Task, "RPM LEDS", RPM_LEDS_TASK_STACK_SIZE, max_config, tskIDLE_PRIORITY + 2, &RPM_LEDS_TaskHandle);
  RPM_LEDS_UpdateTimer = xTimerCreate("LED_UPDATE", 50 / portTICK_PERIOD_MS, pdTRUE, NULL, RPM_LEDS_Update);

  xTimerStart(RPM_LEDS_UpdateTimer, portMAX_DELAY);

  return HAL_OK;
}

void RPM_LEDS_Task(void *arg)
{
  MAX7313_Config_t *max_config = (MAX7313_Config_t *)arg;

  EventBits_t bits_set = 0;
  static uint16_t leds = 0;

  while(1)
  {
    bits_set = xEventGroupWaitBits(RPM_LEDS_NewDataEventHandle, RPM_LEDS_EVENT_NEW_DATA_ALL, pdTRUE, pdFALSE, portMAX_DELAY);

    if(bits_set & RPM_LEDS_EVENT_NEW_DATA_REV)
    {
      // clear all except OIL and WATER
      leds &= RPM_LEDS_LED_OIL | RPM_LEDS_LED_WATER;

      // fill the rev LEDS
      leds |= RPM_LEDS_rev >= 7500 ? RPM_LEDS_LED_01 : 0;
      leds |= RPM_LEDS_rev >= 7750 ? RPM_LEDS_LED_02 : 0;
      leds |= RPM_LEDS_rev >= 8000 ? RPM_LEDS_LED_03 : 0;
      leds |= RPM_LEDS_rev >= 8250 ? RPM_LEDS_LED_04 : 0;
      leds |= RPM_LEDS_rev >= 8500 ? RPM_LEDS_LED_05 : 0;
      leds |= RPM_LEDS_rev >= 8750 ? RPM_LEDS_LED_06 : 0;
      leds |= RPM_LEDS_rev >= 9000 ? RPM_LEDS_LED_07 : 0;
      leds |= RPM_LEDS_rev >= 9250 ? RPM_LEDS_LED_08 : 0;
      leds |= RPM_LEDS_rev >= 9500 ? RPM_LEDS_LED_09 : 0;
      leds |= RPM_LEDS_rev > 9500 ? RPM_LEDS_LED_10  : 0;
    }

    else if(bits_set & RPM_LEDS_EVENT_NEW_DATA_TEMPERATURE)
    {
      // add some hysteresis to prevent OIL and WATER from flickering
      if(RPM_LEDS_TOil > OIL_TEMP_CRITICAL)
        leds |= RPM_LEDS_LED_OIL;
      else if(RPM_LEDS_TOil < OIL_TEMP_CRITICAL - TEMPERATURE_HYSTERESIS)
        leds &= ~RPM_LEDS_LED_OIL;

      if(RPM_LEDS_TWater > WATER_TEMP_CRITICAL)
        leds |= RPM_LEDS_LED_WATER;
      else if(RPM_LEDS_TOil < WATER_TEMP_CRITICAL - TEMPERATURE_HYSTERESIS)
        leds &= ~RPM_LEDS_LED_WATER;
    }

    else if(bits_set & RPM_LEDS_EVENT_UPDATE_LEDS)
    {
      MAX7313_WritePort(max_config,  ~leds);
    }
  }
}

void RPM_LEDS_Update(TimerHandle_t xTimer)
{
  xEventGroupSetBits(RPM_LEDS_NewDataEventHandle, RPM_LEDS_EVENT_UPDATE_LEDS);
}
