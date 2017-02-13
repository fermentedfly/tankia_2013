#include "drv_adc.h"
#include "FreeRTOS.h"
#include "timers.h"

#ifdef HAL_ADC_MODULE_ENABLED

static ADC_HandleTypeDef *ADC_Handle_;
static TimerHandle_t ADC_ConversionTimerHandle_;

static void ADC_TimerCallback(TimerHandle_t xTimer);

void ADC_IRQHandler(void)
{
  HAL_ADC_IRQHandler(ADC_Handle_);
}

HAL_StatusTypeDef ADC_Init(ADC_HandleTypeDef *handle)
{
  ADC_Handle_ = handle;

  ADC_ConversionTimerHandle_ = xTimerCreate("ADC Conv", 1, pdTRUE, NULL, ADC_TimerCallback);

  if (HAL_ADC_Init(handle) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef ADC_StartSingleConversion(ADC_HandleTypeDef *handle)
{
  return HAL_ADC_Start_IT(handle);
}

HAL_StatusTypeDef ADC_StartContinousConversion(ADC_HandleTypeDef *handle, TickType_t period)
{
  vTimerSetTimerID(ADC_ConversionTimerHandle_, handle);
  xTimerChangePeriod(ADC_ConversionTimerHandle_, period, portMAX_DELAY);
  xTimerStart(ADC_ConversionTimerHandle_, portMAX_DELAY);

  return HAL_OK;
}

static void ADC_TimerCallback(TimerHandle_t xTimer)
{
  ADC_HandleTypeDef *handle = (ADC_HandleTypeDef *)pvTimerGetTimerID(xTimer);
  HAL_ADC_Start_IT(handle);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(adcHandle->Instance==ADC1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
  
    /**ADC1 GPIO Configuration    
    PA4     ------> ADC1_IN4 
    */
    GPIO_InitStruct.Pin = ADC1_Clutch_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ADC1_Clutch_GPIO_Port, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(ADC_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
  if(adcHandle->Instance==ADC1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();
  
    /**ADC1 GPIO Configuration    
    PA4     ------> ADC1_IN4 
    */
    HAL_GPIO_DeInit(ADC1_Clutch_GPIO_Port, ADC1_Clutch_Pin);

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(ADC_IRQn);

  }
} 

#endif
