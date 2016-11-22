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
#include "can_messages.h"

TaskHandle_t defaultTaskHandle;

CAN_HandleTypeDef hcan1 =
    {
        .Instance = CAN1,
        .Init = {
            .Prescaler = 21,
            .Mode = CAN_MODE_LOOPBACK,
            .SJW = CAN_SJW_1TQ,
            .BS1 = CAN_BS1_13TQ,
            .BS2 = CAN_BS2_2TQ,
            .TTCM = DISABLE,
            .ABOM = DISABLE,
            .AWUM = DISABLE,
            .NART = DISABLE,
            .RFLM = DISABLE,
            .TXFP = DISABLE,

        },
    };

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

  configASSERT(CAN_Init(&hcan1) == HAL_OK);

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

void StartDefaultTask(void *arg)
{
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

  CAN_MESSAGES_Init(&hcan1);

  CanTxMsgTypeDef TxMessage = {
      .IDE = CAN_ID_STD,
      .StdId = 0x200,
      .DLC = 7,
      .Data[0] = 0x01,
      .Data[1] = 0x02,
      .Data[2] = 0x03,
      .Data[3] = 0x04,
      .Data[4] = 0x05,
      .Data[5] = 0x06,
      .Data[6] = 0x07,
  };

  hcan1.pTxMsg = &TxMessage;

  configASSERT(DISPLAY_Init() == HAL_OK);

  for(;;)
  {
    HAL_CAN_Transmit_IT(&hcan1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
