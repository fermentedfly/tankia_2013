#include "can.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "display.h"
#include "event_groups.h"

static CanTxMsgTypeDef        TxMessage;
static CanRxMsgTypeDef        RxMessage;

CAN_HandleTypeDef hcan1;

void CAN1_TX_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(&hcan1);
  traceISR_EXIT();
}

void CAN1_RX0_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(&hcan1);
  traceISR_EXIT();
}

void CAN1_RX1_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(&hcan1);
  traceISR_EXIT();
}

void CAN1_SCE_IRQHandler(void)
{
  traceISR_ENTER();
  HAL_CAN_IRQHandler(&hcan1);
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
  hcan->Instance = CAN1;
  hcan->Init.Prescaler = 21;
  hcan->Init.Mode = CAN_MODE_LOOPBACK;
  hcan->Init.SJW = CAN_SJW_1TQ;
  hcan->Init.BS1 = CAN_BS1_13TQ;
  hcan->Init.BS2 = CAN_BS2_2TQ;
  hcan->Init.TTCM = DISABLE;
  hcan->Init.ABOM = DISABLE;
  hcan->Init.AWUM = DISABLE;
  hcan->Init.NART = DISABLE;
  hcan->Init.RFLM = DISABLE;
  hcan->Init.TXFP = DISABLE;

  configASSERT(HAL_CAN_Init(&hcan1) == HAL_OK);

  hcan->pTxMsg = &TxMessage;
  hcan->pRxMsg = &RxMessage;

  return HAL_OK;
}

void CAN_StartReceive(CAN_HandleTypeDef* hcan)
{
  HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
  HAL_CAN_Receive_IT(hcan, CAN_FIFO1);
}
