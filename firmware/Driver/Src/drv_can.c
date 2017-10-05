#include "drv_can.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "string.h"

#ifdef HAL_CAN_MODULE_ENABLED

#define RX_TASK_STACK_SIZE 128
#define TX_TASK_STACK_SIZE 128

typedef struct CAN_Config
{
  TaskHandle_t RxTaskHandle;
  CAN_HandleTypeDef *Handle;
  QueueHandle_t RxQueue;
  CAN_rxCallback RxCallback;

  QueueHandle_t TxQueue;
  SemaphoreHandle_t TxDoneSignal;
  TaskHandle_t TxTaskHandle;

} CAN_Config_t;

static CAN_Config_t CAN1_Config;

static void CAN_RX_IRQHandler(CAN_Config_t *config, uint32_t FIFONumber);
static void RxTask(void *arg);
static void TxTask(void *arg);

void CAN1_TX_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(CAN1_TX_IRQn);
  HAL_CAN_IRQHandler(CAN1_Config.Handle);
}

void CAN1_RX0_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(CAN1_RX0_IRQn);
  CAN_RX_IRQHandler(&CAN1_Config, CAN_FIFO0);
}

void CAN1_RX1_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(CAN1_RX1_IRQn);
  CAN_RX_IRQHandler(&CAN1_Config, CAN_FIFO1);
}

void CAN1_SCE_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(CAN1_SCE_IRQn);
  HAL_CAN_IRQHandler(CAN1_Config.Handle);
}

static void CAN_RX_IRQHandler(CAN_Config_t *config, uint32_t FIFONumber)
{
  CanRxMsgTypeDef rxMsg;

  rxMsg.IDE = (uint8_t)0x04 & config->Handle->Instance->sFIFOMailBox[FIFONumber].RIR;

  if (rxMsg.IDE == CAN_ID_STD)
    rxMsg.StdId = (uint32_t)0x000007FF & (config->Handle->Instance->sFIFOMailBox[FIFONumber].RIR >> 21);
  else
    rxMsg.ExtId = (uint32_t)0x1FFFFFFF & (config->Handle->Instance->sFIFOMailBox[FIFONumber].RIR >> 3);

  rxMsg.RTR = (uint8_t)0x02 & config->Handle->Instance->sFIFOMailBox[FIFONumber].RIR;
  // Get the DLC
  rxMsg.DLC = (uint8_t)0x0F & config->Handle->Instance->sFIFOMailBox[FIFONumber].RDTR;
  // Get the FMI
  rxMsg.FMI = (uint8_t)0xFF & (config->Handle->Instance->sFIFOMailBox[FIFONumber].RDTR >> 8);
  // Get the data field
  memcpy(rxMsg.Data + 0, (uint8_t *)&(config->Handle->Instance->sFIFOMailBox[FIFONumber].RDLR), 4);
  memcpy(rxMsg.Data + 4, (uint8_t *)&(config->Handle->Instance->sFIFOMailBox[FIFONumber].RDHR), 4);

  __HAL_CAN_FIFO_RELEASE(config->Handle, FIFONumber);

  // send data for further processing to task
  BaseType_t xHigherPriorityTaskWoken;
  xQueueSendToBackFromISR(config->RxQueue, &rxMsg, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan)
{
  SemaphoreHandle_t tx_done_signal = NULL;
  if(hcan->Instance == CAN1)
    tx_done_signal = CAN1_Config.TxDoneSignal;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(tx_done_signal, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hcan->Instance==CAN1)
  {
    __HAL_RCC_CAN1_CLK_ENABLE();

    CAN1_Config.Handle = hcan;

    configASSERT((CAN1_Config.RxQueue = xQueueCreate(20, sizeof(CanRxMsgTypeDef))) != NULL);
    configASSERT(xTaskCreate(RxTask, "CAN1 RX", RX_TASK_STACK_SIZE, &CAN1_Config, tskIDLE_PRIORITY + 2, &CAN1_Config.RxTaskHandle) == pdPASS);

    configASSERT((CAN1_Config.TxQueue = xQueueCreate(20, sizeof(CanTxMsgTypeDef))) != NULL);
    configASSERT((CAN1_Config.TxDoneSignal = xSemaphoreCreateBinary()) != NULL);
    configASSERT(xTaskCreate(TxTask, "CAN TX", TX_TASK_STACK_SIZE, &CAN1_Config, tskIDLE_PRIORITY + 2, &CAN1_Config.TxTaskHandle) == pdPASS);
  
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

HAL_StatusTypeDef CAN_startContinousReceive(CAN_HandleTypeDef* handle, CAN_rxCallback callback)
{
  uint32_t tmp = handle->State;
  if ((tmp == HAL_CAN_STATE_READY) || (tmp == HAL_CAN_STATE_BUSY_TX))
  {
    if(handle->Instance == CAN1)
      CAN1_Config.RxCallback = callback;

    // Change CAN state
    if (handle->State == HAL_CAN_STATE_BUSY_TX)
      handle->State = HAL_CAN_STATE_BUSY_TX_RX;
    else
      handle->State = HAL_CAN_STATE_BUSY_RX;

    // Set CAN error code to none
    handle->ErrorCode = HAL_CAN_ERROR_NONE;

    // Enable Error warning Interrupt
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_EWG);
    // Enable Error passive Interrupt
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_EPV);
    // Enable Bus-off Interrupt
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_BOF);
    // Enable Last error code Interrupt
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_LEC);
    // Enable Error Interrupt
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_ERR);

    // enable both FIFO's
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_FMP0);
    __HAL_CAN_ENABLE_IT(handle, CAN_IT_FMP1);

    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;
  }
}

void CAN_Transmit(CAN_HandleTypeDef* hcan, CanTxMsgTypeDef *tx_msg, uint8_t fromISR)
{
  QueueHandle_t tx_queue = NULL;
  if(hcan->Instance == CAN1)
    tx_queue = CAN1_Config.TxQueue;

  if(fromISR)
  {
      portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
      xQueueSendFromISR(tx_queue, tx_msg, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
  else
  {
    xQueueSend(tx_queue, tx_msg, portMAX_DELAY);
  }
}

static void RxTask(void *arg)
{
  CAN_Config_t *config = (CAN_Config_t *)arg;

  while(1)
  {
    CanRxMsgTypeDef rxmsg;
    xQueueReceive(config->RxQueue, &rxmsg, portMAX_DELAY);
    if(config->RxCallback != NULL)
      config->RxCallback(&rxmsg);
  }
}

static void TxTask(void *arg)
{
  CAN_Config_t *config = (CAN_Config_t *)arg;
  CanTxMsgTypeDef tx_msg;

  while(1)
  {
    xQueueReceive(config->TxQueue, &tx_msg, portMAX_DELAY);

    config->Handle->pTxMsg = &tx_msg;
    HAL_StatusTypeDef rval = HAL_CAN_Transmit_IT(config->Handle);
    configASSERT(rval == HAL_OK);

    xSemaphoreTake(config->TxDoneSignal, portMAX_DELAY);
  }
}

#endif
