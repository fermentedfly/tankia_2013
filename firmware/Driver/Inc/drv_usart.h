#ifndef __usart_H
#define __usart_H

#include "stm32f4xx_hal.h"
#include "main.h"

struct UART_Config;

typedef void (*UART_Callback)(struct UART_Config *config);

typedef struct UART_Config
{
  UART_HandleTypeDef *handle;
  DMA_HandleTypeDef *dma_rx;
  DMA_HandleTypeDef *dma_tx;

  UART_Callback tx_cb;
  UART_Callback error_cb;

} UART_Config_t;

extern void Error_Handler(void);

HAL_StatusTypeDef UART4_Init(UART_Config_t *config);

#endif /*__ usart_H */

