#include "can.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "display.h"
#include "event_groups.h"

static CAN_FilterConfTypeDef CAN_filter_BCM = {
    .FilterNumber = CAN_MO_BCM_FILTER_NR,
    .BankNumber = CAN1_BANK_NUMBER,
    .FilterActivation = ENABLE,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_16BIT,
    .FilterFIFOAssignment = CAN_FilterFIFO0,

    .FilterIdHigh = CAN_MO_BCM_STATUS_ID << 5,
    .FilterMaskIdHigh = 0x7FE << 5, // match everything except LSB

    .FilterIdLow = CAN_MO_BCM_SETUP_CONFIRM_1_ID << 5,
    .FilterMaskIdLow = 0x7FC << 5, // match everything except 2 LSB
};

static CAN_FilterConfTypeDef CAN_filter_LVPD = {
    .FilterNumber = CAN_MO_LVPD_FILTER_NR,
    .BankNumber = CAN1_BANK_NUMBER,
    .FilterActivation = ENABLE,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_16BIT,
    .FilterFIFOAssignment = CAN_FilterFIFO0,

    .FilterIdHigh = CAN_MO_LVPD_SETUP_CONFIRM_1_ID << 5,
    .FilterMaskIdHigh = 0x7FE << 5, // match everything except LSB

    .FilterIdLow = CAN_MO_LVPD_STATUS_ID << 5,
    .FilterMaskIdLow = 0x7FE << 5, // match everything except LSB
};

static CAN_FilterConfTypeDef CAN_filter_MS4 = {
    .FilterNumber = CAN_MO_MS4_FILTER_NR,
    .BankNumber = CAN1_BANK_NUMBER,
    .FilterActivation = ENABLE,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_16BIT,
    .FilterFIFOAssignment = CAN_FilterFIFO1,

    .FilterIdHigh = 0x0000,     // disable
    .FilterMaskIdHigh = 0xFFFF,

    .FilterIdLow = CAN_MO_MS4_IRA_ID << 5,
    .FilterMaskIdLow = 0x7F0 << 5, // match everything except 4 LSB
};

static CanTxMsgTypeDef        TxMessage;
static CanRxMsgTypeDef        RxMessage;

CAN_HandleTypeDef hcan1;

static inline void CAN_RxBCM(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static inline void CAN_RxLVPD(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static inline void CAN_RxMS4(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);

void MX_CAN1_Init(void)
{

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 21;
  hcan1.Init.Mode = CAN_MODE_LOOPBACK;
  hcan1.Init.SJW = CAN_SJW_1TQ;
  hcan1.Init.BS1 = CAN_BS1_13TQ;
  hcan1.Init.BS2 = CAN_BS2_2TQ;
  hcan1.Init.TTCM = DISABLE;
  hcan1.Init.ABOM = DISABLE;
  hcan1.Init.AWUM = DISABLE;
  hcan1.Init.NART = DISABLE;
  hcan1.Init.RFLM = DISABLE;
  hcan1.Init.TXFP = DISABLE;
  configASSERT(HAL_CAN_Init(&hcan1) == HAL_OK);

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
  hcan->pTxMsg = &TxMessage;
  hcan->pRxMsg = &RxMessage;

  if( (HAL_CAN_ConfigFilter(hcan, &CAN_filter_MS4) != HAL_OK) ||
      (HAL_CAN_ConfigFilter(hcan, &CAN_filter_LVPD) != HAL_OK) ||
      (HAL_CAN_ConfigFilter(hcan, &CAN_filter_BCM) != HAL_OK))
  {
    return HAL_ERROR;
  }

  // start receive

  HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
  HAL_CAN_Receive_IT(hcan, CAN_FIFO1);

  return HAL_OK;
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
  configASSERT(hcan->pRxMsg->IDE == CAN_ID_STD);

  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  if(hcan->pRxMsg->FMI == CAN_MO_BCM_FILTER_NR)
  {
    CAN_RxBCM(hcan->pRxMsg, &xHigherPriorityTaskWoken);
  }

  if(hcan->pRxMsg->FMI == CAN_MO_LVPD_FILTER_NR)
  {
    CAN_RxLVPD(hcan->pRxMsg, &xHigherPriorityTaskWoken);
  }

  if(hcan->pRxMsg->FMI == CAN_MO_MS4_FILTER_NR)
  {
    CAN_RxMS4(hcan->pRxMsg, &xHigherPriorityTaskWoken);
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  // restart receive
  HAL_CAN_Receive_IT(hcan, hcan->pRxMsg->FIFONumber);
}

static inline void CAN_RxBCM(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield)
{
  switch(pRxMsg->StdId)
  {
    case CAN_MO_BCM_SETUP_CONFIRM_1_ID:
    {
      CAN_MO_BCM_SETUP_1_t *msg = (CAN_MO_BCM_SETUP_1_t *)pRxMsg->Data;
      DISPLAY_DATA_ClutchNormal.clutch_points = msg->clutch_points;
      DISPLAY_DATA_ClutchNormal.clutch_tolerance = msg->clutch_tolerance;
      DISPLAY_DATA_ClutchNormal.c_sens_min = msg->c_sens_min;
      DISPLAY_DATA_ClutchNormal.c_sens_max = msg->c_sens_max;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL, portyield);
      break;
    }

    case CAN_MO_BCM_SETUP_CONFIRM_2_ID:
    {
      CAN_MO_BCM_SETUP_2_t *msg = (CAN_MO_BCM_SETUP_2_t *)pRxMsg->Data;
      DISPLAY_DATA_GearACC.g_acc_min_speed = msg->g_acc_min_speed;
      DISPLAY_DATA_GearACC.g_acc_max_wspin = msg->g_acc_max_wspin;
      DISPLAY_DATA_GearACC.g_acc_shift_rpm_1 = msg->g_acc_shift_rpm_1;
      DISPLAY_DATA_GearACC.g_acc_shift_rpm_2 = msg->g_acc_shift_rpm_2;
      DISPLAY_DATA_GearACC.g_acc_shift_rpm_3 = msg->g_acc_shift_rpm_3;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_GEAR_ACC, portyield);
      break;
    }

    case CAN_MO_BCM_SETUP_CONFIRM_3_ID:
    {
      CAN_MO_BCM_SETUP_3_t *msg = (CAN_MO_BCM_SETUP_3_t *)pRxMsg->Data;
      DISPLAY_DATA_GearControl.g_min_shift_delay = msg->g_min_shift_delay;
      DISPLAY_DATA_GearControl.g_up_holdtime = msg->g_up_holdtime;
      DISPLAY_DATA_GearControl.g_dn_holdtime = msg->g_dn_holdtime;
      DISPLAY_DATA_GearControl.g_n_holdtime = msg->g_n_holdtime;
      DISPLAY_DATA_Racepage.map = msg->map;
      DISPLAY_DATA_Racepage.traction = msg->traction;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_CLUTCH_NORMAL | DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }

    case CAN_MO_BCM_SETUP_CONFIRM_4_ID:
    {
      CAN_MO_BCM_SETUP_4_t *msg = (CAN_MO_BCM_SETUP_4_t *)pRxMsg->Data;
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

static inline void CAN_RxLVPD(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield)
{
  switch(pRxMsg->StdId)
  {
    case CAN_MO_LVPD_SETUP_CONFIRM_1_ID:
    {
      CAN_MO_LVPD_SETUP_1_t *msg = (CAN_MO_LVPD_SETUP_1_t *)pRxMsg->Data;
      DISPLAY_DATA_PowerFan.fan_off_temp = msg->fan_off_temp;
      DISPLAY_DATA_PowerFan.fan_on_temp = msg->fan_on_temp;
      DISPLAY_DATA_PowerFan.fan_off_rpm = msg->fan_off_rpm;
      DISPLAY_DATA_PowerFan.fan_on_rpm = msg->fan_on_rpm;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_POWER_FAN, portyield);
      break;
    }
    case CAN_MO_LVPD_SETUP_CONFIRM_2_ID:
     {
       CAN_MO_LVPD_SETUP_2_t *msg = (CAN_MO_LVPD_SETUP_2_t *)pRxMsg->Data;
       DISPLAY_DATA_PowerCurrent.enable_bitfield = msg->enable_bitfield;
       DISPLAY_DATA_PowerCurrent.threshold_value[msg->threshold_multiplex] = msg->threshold_value;

       xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_POWER_CURRENT, portyield);
       break;
     }
  }
}

static inline void CAN_RxMS4(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield)
{
  switch(pRxMsg->StdId)
  {
    case CAN_MO_MS4_IRA_ID:
    {
      CAN_MO_MS4_IRA_t *msg = (CAN_MO_MS4_IRA_t *)pRxMsg->Data;
      DISPLAY_DATA_Racepage.rev = (msg->rev_msb << 16) + msg->rev_lsb;
      DISPLAY_DATA_Racepage.ath = msg->ath;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }

    case CAN_MO_MS4_SPEED_ID:
    {
      CAN_MO_MS4_SPEED_t *msg = (CAN_MO_MS4_SPEED_t *)pRxMsg->Data;
      DISPLAY_DATA_Racepage.speed = (msg->speed_msb << 16) + msg->speed_lsb;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }

    case CAN_MO_MS4_GDA_ID:
    {
      CAN_MO_MS4_GDA_t *msg = (CAN_MO_MS4_GDA_t *)pRxMsg->Data;
      DISPLAY_DATA_Racepage.gear = msg->gear;

      xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_NEW_DATA_RACEPAGE, portyield);
      break;
    }
  }
}
