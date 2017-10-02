#include "main.h"
#include "stm32f4xx_hal.h"
#include "drv_adc.h"
#include "drv_can.h"
#include "drv_i2c.h"
#include "drv_usart.h"
#include "drv_usb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SEGGER_SYSVIEW.h"
#include "can_messages.h"
#include "display.h"
#include "dev_max7313.h"
#include "rpm_leds.h"
#include "vcp_forward.h"

void SystemClock_Config(void);
void MainTask(void *arg);
static void setupGPIO(void);

TaskHandle_t SteeringWheelMainTaskHandle;

// http://www.bittiming.can-wiki.info/
CAN_HandleTypeDef hcan1 =
    {
        .Instance = CAN1,
        .Init = {
            .Prescaler = 3,
            .Mode = CAN_MODE_NORMAL,
            .SJW = CAN_SJW_1TQ,
            .BS1 = CAN_BS1_11TQ,
            .BS2 = CAN_BS2_2TQ,
            .TTCM = DISABLE,
            .ABOM = ENABLE,
            .AWUM = ENABLE,
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

};

DISPLAY_Config_t display_config = {
    .uart_config = &uart4_config,
    .current_menu = 0,
    .menu_position = 0,
};

VCP_FORWARD_Config_t vcp_forward_config = {
    .usb_config = &USB_Config,
    .uart_config = &uart4_config,
};

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  // enable clocks
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  setupGPIO();

  SEGGER_SYSVIEW_Conf();

  configASSERT(HAL_CAN_Init(&hcan1) == HAL_OK);
  configASSERT(I2C_Init(&hi2c1) == HAL_OK);
  configASSERT(UART4_Init(&uart4_config) == HAL_OK);
  configASSERT(USB_Init(&USB_Config) == HAL_OK);

  configASSERT(ADC_Init(&hadc1) == HAL_OK);
  configASSERT(HAL_ADC_ConfigChannel(&hadc1, &channel_4_config) == HAL_OK);

  xTaskCreate(MainTask, "Main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &SteeringWheelMainTaskHandle);

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
  CAN_MESSAGES_TransmitSWClutch(&hcan1, HAL_ADC_GetValue(handle) / 16, pdTRUE); //convert 12Bit ADC to 8Bit Clutch
}

void MainTask(void *arg)
{
  // enable 5v
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

  // enable display
  HAL_GPIO_WritePin(eDIP_Reset_GPIO_Port, eDIP_Reset_Pin, GPIO_PIN_SET);

  vTaskDelay(100);

  CAN_MESSAGES_Init(&hcan1);
  configASSERT(RPM_LEDS_Init(&max7313_config) == HAL_OK);
  configASSERT(DISPLAY_Init(&display_config) == HAL_OK);
  configASSERT(ADC_StartContinousConversion(&hadc1, 500) == HAL_OK);

  uint8_t buffer = 0x00;
  uint32_t bytes_read;

  while(1)
  {
    USB_VCP_readBlocking(&USB_Config, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

    if(buffer == 'x')
    {
      configASSERT(VCP_FORWARD_Init(&vcp_forward_config) == HAL_OK);
      vTaskDelay(1000);
      VCP_FORWARD_Enable(&vcp_forward_config);

      while (1)
      {

      }
    }

    if(buffer == 'p')
      DISPLAY_DATA_Buttons.plus = pdTRUE;

    else if(buffer == 'm')
      DISPLAY_DATA_Buttons.minus = pdTRUE;

    else if(buffer == 'e')
      DISPLAY_DATA_Buttons.enter = pdTRUE;

    xEventGroupSetBits(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_BUTTON_PRESSED);
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

static void setupGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;


    // Configure GPIO pin Output Level
    HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_0_Pin|LED_1_Pin|LED_2_Pin|LED_3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(eDIP_Reset_GPIO_Port, eDIP_Reset_Pin, GPIO_PIN_RESET);

    // Outputs

    // enable 5V
    GPIO_InitStruct.Pin = EN_5V_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EN_5V_GPIO_Port, &GPIO_InitStruct);

    // LEDS
    GPIO_InitStruct.Pin = LED_0_Pin|LED_1_Pin|LED_2_Pin|LED_3_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    // EDIP reset
    GPIO_InitStruct.Pin = eDIP_Reset_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(eDIP_Reset_GPIO_Port, &GPIO_InitStruct);

    // Inputs

    // MAX Shift Down, Max Minus
    GPIO_InitStruct.Pin = MAX_SHIFT_DOWN_Pin|MAX_MINUS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // MAX Shift Up, Max Plus, Max Enter
    GPIO_InitStruct.Pin = MAX_Shift_Up_Pin|MAX_Plus_Pin|MAX_Enter_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Max nCh
    GPIO_InitStruct.Pin = nMAX_CH_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(nMAX_CH_GPIO_Port, &GPIO_InitStruct);


    // EXTI interrupt initialization
    HAL_NVIC_SetPriority(EXTI0_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    HAL_NVIC_SetPriority(EXTI2_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    HAL_NVIC_SetPriority(EXTI4_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void configureTimerForRunTimeStats(void)
{

}

unsigned long getRunTimeCounterValue(void)
{
  return 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch(GPIO_Pin)
  {
    case MAX_Shift_Up_Pin:
      CAN_MESSAGES_TransmitSWShift(&hcan1, CAN_MO_SW_Shift_Direction_Up, pdTRUE);
      break;

    case MAX_Plus_Pin:
      DISPLAY_DATA_Buttons.plus = pdTRUE;
      break;

    case MAX_Enter_Pin:
      DISPLAY_DATA_Buttons.enter = pdTRUE;
      break;

    case MAX_SHIFT_DOWN_Pin:
      CAN_MESSAGES_TransmitSWShift(&hcan1, CAN_MO_SW_Shift_Direction_Down, pdTRUE);
      break;

    case MAX_MINUS_Pin:
      DISPLAY_DATA_Buttons.minus = pdTRUE;
      break;


  }
  xEventGroupSetBitsFromISR(DISPLAY_NewDataEventHandle, DISPLAY_EVENT_BUTTON_PRESSED, NULL);
}
