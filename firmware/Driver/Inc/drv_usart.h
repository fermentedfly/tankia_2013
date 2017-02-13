#ifndef __usart_H
#define __usart_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#ifdef HAL_USART_MODULE_ENABLED

#define UART_DMA_RX_QUEUE_SIZE 1000
#define UART_DMA_RX_BUFFER_SIZE 1024

struct UART_Config;

typedef void (*UART_Callback)(struct UART_Config *config);

typedef struct UART_Config
{
  UART_HandleTypeDef *handle;
  DMA_HandleTypeDef *dma_rx;
  DMA_HandleTypeDef *dma_tx;

  UART_Callback tx_cb;
  UART_Callback error_cb;

  QueueHandle_t       dma_rx_queue;

  uint8_t             *dma_rx_buffer_current;
  uint8_t             dma_rx_buffer_base[UART_DMA_RX_BUFFER_SIZE];
  uint32_t            dma_rx_buffer_size;
  uint32_t            dma_rx_read_pointer;

  SemaphoreHandle_t   blockingTransfer;

} UART_Config_t;

extern void Error_Handler(void);

HAL_StatusTypeDef UART4_Init(UART_Config_t *config);
uint8_t UART_GetDMACircularData(UART_Config_t *config);
HAL_StatusTypeDef UART_ChangeBaudrate(UART_Config_t *config, uint32_t baudrate);
HAL_StatusTypeDef UART_StartReceive_DMACircular(UART_Config_t *config);
void UART_StopReceiveDMACircular(UART_Config_t *config);
HAL_StatusTypeDef UART_Transmit(UART_Config_t *config, uint8_t *pData, uint16_t Size, TickType_t timeout);

#endif /* HAL_USART_MODULE_ENABLED */
#endif /*__ usart_H */

