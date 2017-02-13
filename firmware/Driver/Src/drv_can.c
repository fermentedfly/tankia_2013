#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef HAL_CAN_MODULE_ENABLED

static CAN_HandleTypeDef *CAN1_Handle;

void CAN1_TX_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(CAN1_Handle);
  traceISR_EXIT();
}

void CAN1_RX0_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(CAN1_Handle);
  traceISR_EXIT();
}

void CAN1_RX1_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(CAN1_Handle);
  traceISR_EXIT();
}

void CAN1_SCE_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(CAN1_Handle);
  traceISR_EXIT();
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(canHandle->Instance==CAN1)
  {
    __HAL_RCC_CAN1_CLK_ENABLE();
  
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 9, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
    __HAL_RCC_CAN1_CLK_DISABLE();
  
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);

    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);

    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);

    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);

    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);

  }
} 

HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef* hcan)
{
  CAN1_Handle = hcan;

  configASSERT(HAL_CAN_Init(hcan) == HAL_OK);

  return HAL_OK;
}

void CAN_StartReceive(CAN_HandleTypeDef* hcan, CanRxMsgTypeDef *rx_msg)
{
  hcan->pRxMsg = rx_msg;
  HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
  HAL_CAN_Receive_IT(hcan, CAN_FIFO1);
}

#endif
