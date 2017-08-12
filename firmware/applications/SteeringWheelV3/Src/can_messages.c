/*
 * can_messages.c
 *
 *  Created on: Nov 22, 2016
 *      Author: manuel
 */

#include "can_messages.h"
#include "display.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "rpm_leds.h"

QueueHandle_t CAN_MESSAGES_TxQueue;
SemaphoreHandle_t CAN_MESSAGES_TxDoneSignal;

static TaskHandle_t CAN_MESSAGES_TxTaskHandle;
static CanRxMsgTypeDef CAN1_RxMessage;

static CAN_FilterConfTypeDef CAN_filter_BCM = {
    .FilterNumber = CAN_MO_FILTER_NR_BCM,
    .BankNumber = CAN1_BANK_NUMBER,
    .FilterActivation = ENABLE,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterFIFOAssignment = CAN_FilterFIFO0,

    .FilterIdHigh = CAN_MO_ID_BCM_STATUS << 5,
    .FilterMaskIdHigh = 0x7FE << 5, // match everything except LSB

    .FilterIdLow = CAN_MO_ID_BCM_SETUP_CONFIRM_1 << 5,
    .FilterMaskIdLow = 0x7FC << 5, // match everything except 2 LSB
};

static CAN_FilterConfTypeDef CAN_filter_LVPD = {
    .FilterNumber = CAN_MO_FILTER_NR_LVPD,
    .BankNumber = CAN1_BANK_NUMBER,
    .FilterActivation = ENABLE,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_16BIT,
    .FilterFIFOAssignment = CAN_FilterFIFO0,

    .FilterIdHigh = CAN_MO_ID_LVPD_SETUP_CONFIRM_1 << 5,
    .FilterMaskIdHigh = 0x7FE << 5, // match everything except LSB

    .FilterIdLow = CAN_MO_ID_LVPD_STATUS << 5,
    .FilterMaskIdLow = 0x7FE << 5, // match everything except LSB
};

static CAN_FilterConfTypeDef CAN_filter_MS4 = {
    .FilterNumber = CAN_MO_FILTER_NR_MS4,
    .BankNumber = CAN1_BANK_NUMBER,
    .FilterActivation = ENABLE,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_16BIT,
    .FilterFIFOAssignment = CAN_FilterFIFO1,

    .FilterIdHigh = 0x0000,     // disable
    .FilterMaskIdHigh = 0xFFFF,

    .FilterIdLow = CAN_MO_ID_MS4_IRA << 5,
    .FilterMaskIdLow = 0x7F0 << 5, // match everything except 4 LSB
};

static inline void CAN_MESSAGES_RxBCM(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static inline void CAN_MESSAGES_RxLVPD(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static inline void CAN_MESSAGES_RxMS4(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static void CAN_MESSAGES_TxTask(void *arg);

void CAN_MESSAGES_Init(CAN_HandleTypeDef* hcan)
{
  CAN_MESSAGES_TxQueue = xQueueCreate(20, sizeof(CanTxMsgTypeDef));
  CAN_MESSAGES_TxDoneSignal = xSemaphoreCreateBinary();

  xTaskCreate(CAN_MESSAGES_TxTask, "CAN TX", CAN_MESSAGES_TX_TASK_STACK_SIZE, hcan, tskIDLE_PRIORITY + 2, &CAN_MESSAGES_TxTaskHandle);

  if(hcan->Instance == CAN1)
  {
    CAN_StartReceive(hcan, &CAN1_RxMessage);
  }
  configASSERT(HAL_CAN_ConfigFilter(hcan, &CAN_filter_MS4) == HAL_OK);
  configASSERT(HAL_CAN_ConfigFilter(hcan, &CAN_filter_LVPD) == HAL_OK);
  configASSERT(HAL_CAN_ConfigFilter(hcan, &CAN_filter_BCM) == HAL_OK);
}

void CAN_MESSAGES_Transmit(CAN_HandleTypeDef* hcan, CanTxMsgTypeDef *tx_msg, uint8_t fromISR)
{
	if(fromISR)
	{
		  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		  xQueueSendFromISR(CAN_MESSAGES_TxQueue, tx_msg, &xHigherPriorityTaskWoken);
		  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		xQueueSend(CAN_MESSAGES_TxQueue, tx_msg, portMAX_DELAY);
	}
}

void CAN_MESSAGES_TransmitSWShift(CAN_HandleTypeDef* hcan, CAN_MO_SW_Shift_Direction_t direction, uint8_t fromISR)
{
	static CanTxMsgTypeDef msg = {
			.StdId = CAN_MO_ID_SW_SHIFT,
			.IDE = CAN_ID_STD,
			.RTR = CAN_RTR_DATA,
			.DLC = sizeof(CAN_MO_SW_Shift_t),
	};

	((CAN_MO_SW_Shift_t *)msg.Data)->direction = direction;
	CAN_MESSAGES_Transmit(hcan, &msg, fromISR);
}

void CAN_MESSAGES_TransmitSWClutch(CAN_HandleTypeDef* hcan, uint8_t value, uint8_t fromISR)
{
	static CanTxMsgTypeDef msg = {
			.StdId = CAN_MO_ID_SW_CLUTCH,
			.IDE = CAN_ID_STD,
			.RTR = CAN_RTR_DATA,
			.DLC = sizeof(CAN_MO_SW_Clutch_t),
	};

	((CAN_MO_SW_Clutch_t *)msg.Data)->value = value;
	CAN_MESSAGES_Transmit(hcan, &msg, fromISR);
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
  configASSERT(hcan->pRxMsg->IDE == CAN_ID_STD);

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);

  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  if(hcan->pRxMsg->FMI == CAN_MO_FILTER_NR_BCM)
  {
    CAN_MESSAGES_RxBCM(hcan->pRxMsg, &xHigherPriorityTaskWoken);
  }

  if(hcan->pRxMsg->FMI == CAN_MO_FILTER_NR_LVPD)
  {
    CAN_MESSAGES_RxLVPD(hcan->pRxMsg, &xHigherPriorityTaskWoken);
  }

  if(hcan->pRxMsg->FMI == CAN_MO_FILTER_NR_MS4)
  {
    CAN_MESSAGES_RxMS4(hcan->pRxMsg, &xHigherPriorityTaskWoken);
  }

  // restart receive
  HAL_CAN_Receive_IT(hcan, hcan->pRxMsg->FIFONumber);

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(CAN_MESSAGES_TxDoneSignal, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static inline void CAN_MESSAGES_RxBCM(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield)
{
  switch(pRxMsg->StdId)
  {
    case CAN_MO_ID_BCM_SETUP_CONFIRM_1:
    {
      CAN_MO_BCM_Setup_1_t *msg = (CAN_MO_BCM_Setup_1_t *)pRxMsg->Data;
      DISPLAY_DATA_ClutchNormal.clutch_points = msg->clutch_points;
      DISPLAY_DATA_ClutchNormal.clutch_tolerance = msg->clutch_tolerance;
      DISPLAY_DATA_ClutchNormal.c_sens_min = msg->c_sens_min;
      DISPLAY_DATA_ClutchNormal.c_sens_max = msg->c_sens_max;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL, portyield);
      break;
    }

    case CAN_MO_ID_BCM_SETUP_CONFIRM_2:
    {
      CAN_MO_BCM_Setup_2_t *msg = (CAN_MO_BCM_Setup_2_t *)pRxMsg->Data;
      DISPLAY_DATA_GearACC.g_acc_min_speed = msg->g_acc_min_speed;
      DISPLAY_DATA_GearACC.g_acc_max_wspin = msg->g_acc_max_wspin;
      DISPLAY_DATA_GearACC.g_acc_shift_rpm_1 = msg->g_acc_shift_rpm_1;
      DISPLAY_DATA_GearACC.g_acc_shift_rpm_2 = msg->g_acc_shift_rpm_2;
      DISPLAY_DATA_GearACC.g_acc_shift_rpm_3 = msg->g_acc_shift_rpm_3;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_GEAR_ACC, portyield);
      break;
    }

    case CAN_MO_ID_BCM_SETUP_CONFIRM_3:
    {
      CAN_MO_BCM_Setup_3_t *msg = (CAN_MO_BCM_Setup_3_t *)pRxMsg->Data;
      DISPLAY_DATA_GearControl.g_min_shift_delay = msg->g_min_shift_delay;
      DISPLAY_DATA_GearControl.g_up_holdtime = msg->g_up_holdtime;
      DISPLAY_DATA_GearControl.g_dn_holdtime = msg->g_dn_holdtime;
      DISPLAY_DATA_GearControl.g_n_holdtime = msg->g_n_holdtime;
      DISPLAY_DATA_Racepage.map = msg->map;
      DISPLAY_DATA_Racepage.traction = msg->traction;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL | DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }

    case CAN_MO_ID_BCM_SETUP_CONFIRM_4:
    {
      CAN_MO_BCM_Setup_4_t *msg = (CAN_MO_BCM_Setup_4_t *)pRxMsg->Data;
      DISPLAY_DATA_ClutchACC.acc_clutch_p1 = msg->acc_clutch_p1;
      DISPLAY_DATA_ClutchACC.acc_clutch_p2 = msg->acc_clutch_p2;
      DISPLAY_DATA_ClutchACC.acc_clutch_k1 = msg->acc_clutch_k1;
      DISPLAY_DATA_ClutchACC.acc_clutch_k2 = msg->acc_clutch_k2;
      DISPLAY_DATA_ClutchACC.acc_clutch_k3 = msg->acc_clutch_k3;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_CLUTCH_ACC, portyield);
      break;
    }
  }
}

static inline void CAN_MESSAGES_RxLVPD(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield)
{
  switch(pRxMsg->StdId)
  {
    case CAN_MO_ID_LVPD_SETUP_CONFIRM_1:
    {
      CAN_MO_LVPD_Setup_1_t *msg = (CAN_MO_LVPD_Setup_1_t *)pRxMsg->Data;
      DISPLAY_DATA_PowerFan.fan_off_temp = msg->fan_off_temp;
      DISPLAY_DATA_PowerFan.fan_on_temp = msg->fan_on_temp;
      DISPLAY_DATA_PowerFan.fan_off_rpm = msg->fan_off_rpm;
      DISPLAY_DATA_PowerFan.fan_on_rpm = msg->fan_on_rpm;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_POWER_FAN, portyield);
      break;
    }
    case CAN_MO_ID_LVPD_SETUP_CONFIRM_2:
     {
       CAN_MO_LVPD_Setup_2_t *msg = (CAN_MO_LVPD_Setup_2_t *)pRxMsg->Data;
       DISPLAY_DATA_PowerCurrent.enable_bitfield = msg->enable_bitfield;
       DISPLAY_DATA_PowerCurrent.threshold_value[msg->threshold_multiplex] = msg->threshold_value;

       xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_POWER_CURRENT, portyield);
       break;
     }
  }
}

static inline void CAN_MESSAGES_RxMS4(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield)
{
  switch(pRxMsg->StdId)
  {
    case CAN_MO_ID_MS4_IRA:
    {
      CAN_MO_MS4_IRA_t *msg = (CAN_MO_MS4_IRA_t *)pRxMsg->Data;
      DISPLAY_DATA_Racepage.rev = (msg->rev_msb << 16) + msg->rev_lsb;
      DISPLAY_DATA_Racepage.ath = msg->ath;

      RPM_LEDS_rev = (msg->rev_msb << 16) + msg->rev_lsb;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      xEventGroupSetBitsFromISR(RPM_LEDS_NewDataEventHandle, RPM_LEDS_EVENT_NEW_DATA_REV, portyield);
      break;
    }

    case CAN_MO_ID_MS4_SPEED:
    {
      CAN_MO_MS4_Speed_t *msg = (CAN_MO_MS4_Speed_t *)pRxMsg->Data;
      DISPLAY_DATA_Racepage.speed = (msg->speed_msb << 16) + msg->speed_lsb;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }

    case CAN_MO_ID_MS4_GDA:
    {
      CAN_MO_MS4_GDA_t *msg = (CAN_MO_MS4_GDA_t *)pRxMsg->Data;
      DISPLAY_DATA_Racepage.gear = msg->gear;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }

    case CAN_MO_ID_MS4_SBDB:
    {
    	CAN_MO_MS4_SBDB_t *msg = (CAN_MO_MS4_SBDB_t *)pRxMsg->Data;

    	if(msg->row_counter == 4)
    		DISPLAY_DATA_Racepage.toil = msg->muxed.row_4.toil - 40;
    	if(msg->row_counter == 5)
    		DISPLAY_DATA_Racepage.twat = msg->muxed.row_5.tmot - 40;

    	xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
    	break;
    }
  }
}

static void CAN_MESSAGES_TxTask(void *arg)
{
  CAN_HandleTypeDef *handle = (CAN_HandleTypeDef *)arg;
  CanTxMsgTypeDef tx_msg;

  while(1)
  {
    xQueueReceive(CAN_MESSAGES_TxQueue, &tx_msg, portMAX_DELAY);

    handle->pTxMsg = &tx_msg;
    HAL_StatusTypeDef rval = HAL_CAN_Transmit_IT(handle);
    configASSERT(rval == HAL_OK);

    xSemaphoreTake(CAN_MESSAGES_TxDoneSignal, portMAX_DELAY);
  }

}
