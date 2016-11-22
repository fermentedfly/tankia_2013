#include "main.h"
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "display.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SEGGER_SYSVIEW.h"

TaskHandle_t defaultTaskHandle;

void SystemClock_Config(void);
void Error_Handler(void);
void StartDefaultTask(void *arg);

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_UART4_Init();
  MX_USB_OTG_FS_USB_Init();

  SEGGER_SYSVIEW_Conf();

  xTaskCreate(StartDefaultTask, "default", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &defaultTaskHandle);

  vTaskStartScheduler();

  while (1)
  {
  }
}

void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  configASSERT(HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  configASSERT(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) == HAL_OK);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  while(1) 
  {
  }
}

void configureTimerForRunTimeStats(void)
{

}

unsigned long getRunTimeCounterValue(void)
{
  return 0;
}


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

static inline void CAN_RxBCM(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static inline void CAN_RxLVPD(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);
static inline void CAN_RxMS4(CanRxMsgTypeDef *pRxMsg, BaseType_t *portyield);

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

void StartDefaultTask(void *arg)
{
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

  configASSERT(CAN_Init(&hcan1) == HAL_OK);
  configASSERT(HAL_CAN_ConfigFilter(&hcan1, &CAN_filter_MS4) == HAL_OK);
  configASSERT(HAL_CAN_ConfigFilter(&hcan1, &CAN_filter_LVPD) == HAL_OK);
  configASSERT(HAL_CAN_ConfigFilter(&hcan1, &CAN_filter_BCM) == HAL_OK);

  CAN_StartReceive(&hcan1);

  configASSERT(DISPLAY_Init() == HAL_OK);

  hcan1.pTxMsg->IDE = CAN_ID_STD;
  hcan1.pTxMsg->StdId = 0x200;
  hcan1.pTxMsg->DLC = 7;
  hcan1.pTxMsg->Data[0] = 0x01;
  hcan1.pTxMsg->Data[1] = 0x02;
  hcan1.pTxMsg->Data[2] = 0x03;
  hcan1.pTxMsg->Data[3] = 0x04;
  hcan1.pTxMsg->Data[4] = 0x05;
  hcan1.pTxMsg->Data[5] = 0x06;
  hcan1.pTxMsg->Data[6] = 0x07;

  for(;;)
  {
    HAL_CAN_Transmit_IT(&hcan1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
