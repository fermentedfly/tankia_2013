#include "main.h"
#include "stm32f4xx_hal.h"
#include "drv_adc.h"
#include "drv_can.h"
#include "drv_dma.h"
#include "drv_i2c.h"
#include "drv_usart.h"
#include "drv_usb.h"
#include "drv_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SEGGER_SYSVIEW.h"
#include "can_messages.h"
#include "display.h"
#include "dev_max7313.h"
#include "rpm_leds.h"

TaskHandle_t SteeringWheelMainTaskHandle;

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

static USBD_HandleTypeDef USBD_Device;

static PCD_HandleTypeDef USB_hpcd = {
  .Instance = USB_OTG_FS,
  .Init = {
    .dev_endpoints = 4,
    .use_dedicated_ep1 = 0,
    .ep0_mps = 0x40,
    .dma_enable = 0,
    .low_power_enable = 0,
    .phy_itface = PCD_PHY_EMBEDDED,
    .Sof_enable = 0,
    .speed = PCD_SPEED_FULL,
    .vbus_sensing_enable = DISABLE,
    .lpm_enable = DISABLE
  },
  .pData = &USBD_Device,
};

static USBD_CDC_HandleTypeDef USBD_CDC_Device;

static USBD_HandleTypeDef USBD_Device = {
    .pData = (void*)&USB_hpcd,
    .pClassData = (void*)&USBD_CDC_Device,
};

USB_Config_t USB_Config = {
  .device = &USBD_Device,
  .descriptors = &VCP_Desc,
  .class = &USBD_CDC,
  .interface = &USBD_CDC_IF
};

I2C_HandleTypeDef hi2c1 ={
    .Instance = I2C1,
    .Init.ClockSpeed = 80000,
    .Init.DutyCycle = I2C_DUTYCYCLE_2,
    .Init.OwnAddress1 = 0,
    .Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT,
    .Init.DualAddressMode = I2C_DUALADDRESS_DISABLE,
    .Init.OwnAddress2 = 0,
    .Init.GeneralCallMode = I2C_GENERALCALL_DISABLE,
    .Init.NoStretchMode = I2C_NOSTRETCH_DISABLE,
};

ADC_HandleTypeDef hadc1 =
{
    .Instance = ADC1,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .ScanConvMode = DISABLE,
        .ContinuousConvMode = DISABLE,
        .DiscontinuousConvMode = DISABLE,
        .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
        .DataAlign = ADC_DATAALIGN_RIGHT,
        .NbrOfConversion = 1,
        .DMAContinuousRequests = DISABLE,
        .EOCSelection = ADC_EOC_SINGLE_CONV,
    },
};

ADC_ChannelConfTypeDef channel_4_config =
{
    .Channel = ADC_CHANNEL_4,
    .Rank = 1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES,
};

UART_HandleTypeDef huart4 = {
    .Instance = UART4,
    .Init = {
        .BaudRate = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .Mode = UART_MODE_TX_RX,
        .HwFlowCtl = UART_HWCONTROL_NONE,
        .OverSampling = UART_OVERSAMPLING_16,
		  },
};

DMA_HandleTypeDef hdma_uart4_tx = {
    .Instance = DMA1_Stream4,
    .Init = {
        .Channel = DMA_CHANNEL_4,
        .Direction = DMA_MEMORY_TO_PERIPH,
        .PeriphInc = DMA_PINC_DISABLE,
        .MemInc = DMA_MINC_ENABLE,
        .PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
        .MemDataAlignment = DMA_MDATAALIGN_BYTE,
        .Mode = DMA_NORMAL,
        .Priority = DMA_PRIORITY_LOW,
        .FIFOMode = DMA_FIFOMODE_DISABLE,
    },
};

DMA_HandleTypeDef hdma_uart4_rx = {
    .Instance = DMA1_Stream2,
    .Init = {
        .Channel = DMA_CHANNEL_4,
        .Direction = DMA_PERIPH_TO_MEMORY,
        .PeriphInc = DMA_PINC_DISABLE,
        .MemInc = DMA_MINC_ENABLE,
        .PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
        .MemDataAlignment = DMA_MDATAALIGN_BYTE,
        .Mode = DMA_NORMAL,
        .Priority = DMA_PRIORITY_LOW,
        .FIFOMode = DMA_FIFOMODE_DISABLE,
    },
};

UART_Config_t uart4_config = {
    .handle = &huart4,
    .dma_tx = &hdma_uart4_tx,
    .dma_rx = &hdma_uart4_rx,
};


MAX7313_Config_t max7313_config = {
    .i2c_handle = &hi2c1,
    .i2c_address = 0b0100000 << 1,

    .blink_enabled = 0,
    .global_intensity_enabled = 1,
    .master_intensity =0, //no PWM

    .port_config = {
          .port_0 = 0,
          .port_1 = 0,
          .port_2 = 0,
          .port_3 = 0,
          .port_4 = 0,
          .port_5 = 0,
          .port_6 = 0,
          .port_7 = 0,
          .port_8  = 0,
          .port_9  = 0,
          .port_10 = 0,
          .port_11 = 0,
          .port_12 = 1,
          .port_13 = 1,
          .port_14 = 1,
          .port_15 = 1,
    },
    .port_intensity = {
        .port_0 = 0x0,
        .port_1 = 0x1,
        .port_2 = 0x2,
        .port_3 = 0x3,
        .port_4 = 0x4,
        .port_5 = 0x5,
        .port_6 = 0x6,
        .port_7 = 0x7,
        .port_8  = 0x8,
        .port_9  = 0x9,
        .port_10 = 0xA,
        .port_11 = 0xB,
        .port_12 = 0xC,
        .port_13 = 0xD,
        .port_14 = 0xE,
        .port_15 = 0xF,
    },
};

DISPLAY_Config_t display_config = {
    .uart_config = &uart4_config,
};

void SystemClock_Config(void);
void SteeringWheelMainTask(void *arg);

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();

  SEGGER_SYSVIEW_Conf();

  configASSERT(CAN_Init(&hcan1) == HAL_OK);
  configASSERT(I2C_Init(&hi2c1) == HAL_OK);
  configASSERT(UART4_Init(&uart4_config) == HAL_OK);
  configASSERT(USB_Init(&USB_Config) == HAL_OK);

  configASSERT(ADC_Init(&hadc1) == HAL_OK);
  configASSERT(HAL_ADC_ConfigChannel(&hadc1, &channel_4_config) == HAL_OK);

  xTaskCreate(SteeringWheelMainTask, "Main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &SteeringWheelMainTaskHandle);

  vTaskStartScheduler();

  while (1)
  {
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* handle)
{
  static CanTxMsgTypeDef ClutchTxMessage = {
      .IDE = CAN_ID_STD,
      .StdId = 0x010,
      .DLC = 1,
  };

  ClutchTxMessage.Data[0] = HAL_ADC_GetValue(handle) / 16; //convert 12Bit ADC to 8Bit Clutch
  CAN_MESSAGES_TransmitFromISR(&hcan1, &ClutchTxMessage);
}

void SteeringWheelMainTask(void *arg)
{
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

  CAN_MESSAGES_Init(&hcan1);
  configASSERT(DISPLAY_Init(&display_config) == HAL_OK);
  configASSERT(RPM_LEDS_Init(&max7313_config) == HAL_OK);
  configASSERT(ADC_StartContinousConversion(&hadc1, 500) == HAL_OK);

  MAX7313_WritePort(&max7313_config, 0xFFFF);

  static CanTxMsgTypeDef TxMessage = {
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

  for(;;)
  {
    CAN_MESSAGES_Transmit(&hcan1, &TxMessage);

    vTaskDelay(100 / portTICK_PERIOD_MS);
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
