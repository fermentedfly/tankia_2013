#include "drv_spi.h"

#ifdef HAL_SPI_MODULE_ENABLED

static SPI_Config_t *spi2_config = NULL;

void DMA1_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(spi2_config->dma_tx);
}

HAL_StatusTypeDef SPI_Init(SPI_Config_t *config)
{
  if(config->handle->Instance == SPI2)
  {
    __HAL_RCC_SPI2_CLK_ENABLE();

    spi2_config = config;

    // init hardware pins

    GPIO_InitTypeDef GPIO_InitStruct_MOSI = {
        .Pin = config->mosi_pin,
        .Mode = GPIO_MODE_AF_PP,
        .Speed = GPIO_SPEED_HIGH,
        .Alternate = GPIO_AF5_SPI2,
    };
    HAL_GPIO_Init(config->mosi_port, &GPIO_InitStruct_MOSI);

    GPIO_InitTypeDef GPIO_InitStruct_SCK = {
        .Pin = config->sck_pin,
        .Mode = GPIO_MODE_AF_PP,
        .Speed = GPIO_SPEED_HIGH,
        .Alternate = GPIO_AF5_SPI2,
    };
    HAL_GPIO_Init(config->sck_port, &GPIO_InitStruct_SCK);

    GPIO_InitTypeDef GPIO_InitStruct_NSS = {
        .Pin = config->nss_pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Speed = GPIO_SPEED_HIGH,
        //.Alternate = GPIO_AF5_SPI2,
    };
    HAL_GPIO_Init(config->nss_port, &GPIO_InitStruct_NSS);
  }

  if(HAL_SPI_Init(config->handle) != HAL_OK)
    return HAL_ERROR;

  if(HAL_DMA_Init(config->dma_tx) != HAL_OK)
    return HAL_ERROR;

  __HAL_LINKDMA(config->handle, hdmatx, *config->dma_tx);

  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

  return HAL_OK;
}

#endif
