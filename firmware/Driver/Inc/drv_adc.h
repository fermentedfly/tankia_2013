#ifndef __adc_H
#define __adc_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"

HAL_StatusTypeDef  ADC_Init(ADC_HandleTypeDef *handle);
HAL_StatusTypeDef ADC_StartSingleConversion(ADC_HandleTypeDef *handle);
HAL_StatusTypeDef ADC_StartContinousConversion(ADC_HandleTypeDef *handle, TickType_t period);

#endif /*__ adc_H */
