#include "drv_usart.h"
#include "drv_gpio.h"
#include "drv_dma.h"

static UART_Config_t *UART_uart4_config;

static void UART_RxIdleIRQ(UART_Config_t *config);
static inline void UART_IRQ_Handler(UART_Config_t *config, uint32_t isr_id);
static inline uint32_t UART_DMAGetNumUnreadBytes(UART_Config_t *config);
static void UART_Callback_TxComplete(UART_Config_t *config);

void DMA1_Stream2_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
  HAL_DMA_IRQHandler(UART_uart4_config->dma_rx);
}

void DMA1_Stream4_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);
  HAL_DMA_IRQHandler(UART_uart4_config->dma_tx);
}

void UART4_IRQHandler(void)
{
  UART_IRQ_Handler(UART_uart4_config, UART4_IRQn);
}

static inline void UART_IRQ_Handler(UART_Config_t *config, uint32_t isr_id)
{
  HAL_NVIC_ClearPendingIRQ(isr_id);

  // check for IDLE interrupt
  uint32_t tmp1, tmp2;
  tmp1 = __HAL_UART_GET_FLAG(config->handle, UART_FLAG_IDLE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(config->handle, UART_IT_IDLE);
  if((tmp1 != RESET) && (tmp2 != RESET))
  {
    __HAL_UART_CLEAR_IDLEFLAG(config->handle);
    UART_RxIdleIRQ(config);
    return;
  }

  HAL_UART_IRQHandler(config->handle);
}

static void UART_RxIdleIRQ(UART_Config_t *config)
{
  uint32_t nr_bytes = UART_DMAGetNumUnreadBytes(config);
  BaseType_t xHigherPriorityTaskWoken;
  if (nr_bytes >= 1)
  {
    configASSERT(xQueueSendToBackFromISR(config->dma_rx_queue, &nr_bytes, &xHigherPriorityTaskWoken) == pdTRUE);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
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

  if(config->blockingTransfer == NULL)
    config->blockingTransfer = xSemaphoreCreateBinary();
  if(config->blockingTransfer == NULL)
    return HAL_ERROR;

  if(config->dma_rx_queue == NULL)
    config->dma_rx_queue = xQueueCreate(UART_DMA_RX_QUEUE_SIZE, sizeof(uint32_t));
  if(config->dma_rx_queue == NULL)
    return HAL_ERROR;

  config->dma_rx_buffer_current =  config->dma_rx_buffer_base;
  config->dma_rx_buffer_size = sizeof(config->dma_rx_buffer_base);

  __UART4_CLK_ENABLE();

  if(HAL_UART_Init(config->handle) != HAL_OK)
    return HAL_ERROR;

  if(UART_ChangeBaudrate(config, config->handle->Init.BaudRate) != HAL_OK)
    return HAL_ERROR;

  if (HAL_DMA_Init(config->dma_rx) != HAL_OK)
    return HAL_ERROR;

  __HAL_LINKDMA(config->handle, hdmarx, *config->dma_rx);

  if (HAL_DMA_Init(config->dma_tx) != HAL_OK)
    return HAL_ERROR;

  __HAL_LINKDMA(config->handle, hdmatx, *config->dma_tx);

  HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(UART4_IRQn);

  return HAL_OK;
}

HAL_StatusTypeDef UART_ChangeBaudrate(UART_Config_t *config, uint32_t baudrate)
{
  config->handle->Init.BaudRate = baudrate;
  if(HAL_UART_Init(config->handle) != HAL_OK)
    return HAL_ERROR;

  //---------------------------------------------------------------------------
  // UART BRR wrong fractional part fix

  uint32_t apbclock;
  uint32_t integerdivider;
  uint32_t fractionaldivider;
  uint32_t tmpreg;

  if ((config->handle->Instance == USART1) || (config->handle->Instance == USART6))
  {
    apbclock = HAL_RCC_GetPCLK2Freq();
  }
  else
  {
    apbclock = HAL_RCC_GetPCLK1Freq();
  }

  /* Determine the integer part */
  if ((config->handle->Instance->CR1 & USART_CR1_OVER8) != 0)
  {
    /* Integer part computing in case Oversampling mode is 8 Samples */
    integerdivider = ((25 * apbclock) / (2 * (config->handle->Init.BaudRate)));
  }
  else /* if ((USARTx->CR1 & USART_CR1_OVER8) == 0) */
  {
    /* Integer part computing in case Oversampling mode is 16 Samples */
    integerdivider = ((25 * apbclock) / (4 * (config->handle->Init.BaudRate)));
  }
  tmpreg = (integerdivider / 100) << 4;

  /* Determine the fractional part */
  fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

  /* Implement the fractional part in the register */
  if ((config->handle->Instance->CR1 & USART_CR1_OVER8) != 0)
  {
    tmpreg |= ((((fractionaldivider * 8) + 50) / 100)) & ((uint8_t)0x07);
  }
  else /* if ((USARTx->CR1 & USART_CR1_OVER8) == 0) */
  {
    tmpreg |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
  }

  /* Write to USART BRR register */
  config->handle->Instance->BRR = (uint16_t)tmpreg;

  return HAL_OK;
}

HAL_StatusTypeDef UART_StartReceive_DMACircular(UART_Config_t *config)
{
  __HAL_UART_FLUSH_DRREGISTER(config->handle);

  if(HAL_UART_Receive_DMA(config->handle, config->dma_rx_buffer_current, config->dma_rx_buffer_size) != HAL_OK)
    return HAL_ERROR;

  __HAL_UART_ENABLE_IT(config->handle, UART_IT_IDLE);

  return HAL_OK;
}

void UART_StopReceiveDMACircular(UART_Config_t *config)
{
  HAL_UART_DMAStop(config->handle);
  __HAL_UART_DISABLE_IT(config->handle, UART_IT_IDLE);
}


uint8_t UART_GetDMACircularData(UART_Config_t *config)
{
  static uint32_t nr_bytes = 0;
  uint8_t rval = 0;

  while(nr_bytes == 0)
  {
    xQueueReceive(config->dma_rx_queue, &nr_bytes, portMAX_DELAY);
  }

  rval = *(config->dma_rx_buffer_current);

  --nr_bytes;

  // increment DMA buffer pointer
  if(++config->dma_rx_buffer_current >= config->dma_rx_buffer_base + config->dma_rx_buffer_size)
    config->dma_rx_buffer_current = config->dma_rx_buffer_base;

  return rval;
}

HAL_StatusTypeDef UART_Transmit(UART_Config_t *config, uint8_t *pData, uint16_t Size, TickType_t timeout)
{
  config->tx_cb = UART_Callback_TxComplete;

  if(HAL_UART_Transmit_IT(config->handle, pData, Size) != HAL_OK)
    return HAL_ERROR;

  xSemaphoreTake(config->blockingTransfer, timeout);

  return HAL_OK;
}

static inline uint32_t UART_DMAGetNumUnreadBytes(UART_Config_t *config)
{
  uint8_t *head = config->dma_rx_buffer_base + config->dma_rx_buffer_size - __HAL_DMA_GET_COUNTER(config->dma_rx);

  if(head >= config->dma_rx_buffer_current)
    return head - config->dma_rx_buffer_current;
  else
    return head + config->dma_rx_buffer_size - config->dma_rx_buffer_current;
}

static void UART_Callback_TxComplete(UART_Config_t *config)
{
  BaseType_t xHigherPriorityTaskWoken;
  configASSERT(xSemaphoreGiveFromISR(config->blockingTransfer, &xHigherPriorityTaskWoken) == pdTRUE);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


