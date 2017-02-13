#ifndef __spi_H
#define __spi_H

#include "stm32f4xx_hal.h"

#ifdef HAL_SPI_MODULE_ENABLED

typedef struct SPI_Config
{
  SPI_HandleTypeDef *handle;
  DMA_HandleTypeDef *dma_tx;

  GPIO_TypeDef *mosi_port;
  uint16_t mosi_pin;

  GPIO_TypeDef *sck_port;
  uint16_t sck_pin;

  GPIO_TypeDef *nss_port;
  uint16_t nss_pin;

} SPI_Config_t;

HAL_StatusTypeDef SPI_Init(SPI_Config_t *config);

#endif
#endif
