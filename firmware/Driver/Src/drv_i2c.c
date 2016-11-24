#include "drv_i2c.h"
#include "drv_gpio.h"
#include "FreeRTOS.h"

I2C_HandleTypeDef *I2C1_Handle;

void I2C1_EV_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_I2C_EV_IRQHandler(I2C1_Handle);
  traceISR_EXIT();
}

void I2C1_ER_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_I2C_ER_IRQHandler(I2C1_Handle);
  traceISR_EXIT();
}

HAL_StatusTypeDef I2C_Init(I2C_HandleTypeDef *handle)
{
  if(handle->Instance == I2C1)
  {
    I2C1_Handle = handle;
  }
  if (HAL_I2C_Init(handle) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(i2cHandle->Instance==I2C1)
  {
    /**I2C1 GPIO Configuration    
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
  if(i2cHandle->Instance==I2C1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();
  
    /**I2C1 GPIO Configuration    
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);

    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);

  }
} 
