#include "drv_usart.h"
#include "drv_gpio.h"
#include "drv_dma.h"

static UART_Config_t *UART_uart4_config;



void DMA1_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(UART_uart4_config->dma_rx);
}

void DMA1_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(UART_uart4_config->dma_tx);
}

void UART4_IRQHandler(void)
{
  HAL_UART_IRQHandler(UART_uart4_config->handle);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == UART4 && UART_uart4_config->tx_cb)
  {
    UART_uart4_config->tx_cb(UART_uart4_config);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == UART4 && UART_uart4_config->error_cb)
  {
    UART_uart4_config->error_cb(UART_uart4_config);
  }
}


HAL_StatusTypeDef UART4_Init(UART_Config_t *config)
{
  if(config->handle->Instance == UART4)
    UART_uart4_config = config;

  GPIO_InitTypeDef GPIO_InitStruct = {
      .Pin = GPIO_PIN_10|GPIO_PIN_11,
      .Mode = GPIO_MODE_AF_PP,
      .Pull = GPIO_PULLUP,
      .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
      .Alternate = GPIO_AF8_UART4,
  };

  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(UART4_IRQn);

  if(HAL_UART_Init(config->handle) != HAL_OK)
    return HAL_ERROR;

  if (HAL_DMA_Init(config->dma_rx) != HAL_OK)
    return HAL_ERROR;

  __HAL_LINKDMA(config->handle, hdmarx, *config->dma_rx);

  if (HAL_DMA_Init(config->dma_tx) != HAL_OK)
    return HAL_ERROR;

  __HAL_LINKDMA(config->handle, hdmatx, *config->dma_tx);

  return HAL_OK;
}
