#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_spi.h"
#include "SEGGER_SYSVIEW.h"

void SystemClock_Config(void);
void MainTask(void *arg);
static void setupGPIO(void);

TaskHandle_t MainTaskHandle;

SPI_HandleTypeDef hspi2 = {
    .Instance = SPI2,
    .Init= {
        .Mode = SPI_MODE_MASTER,
        .Direction = SPI_DIRECTION_2LINES,
        .DataSize = SPI_DATASIZE_8BIT,
        .CLKPolarity = SPI_POLARITY_LOW,
        .CLKPhase = SPI_PHASE_1EDGE,
        .NSS = SPI_NSS_SOFT,
        .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2,
        .FirstBit = SPI_FIRSTBIT_MSB,
        .TIMode = SPI_TIMODE_DISABLE,
        .CRCCalculation = SPI_CRCCALCULATION_DISABLE,
    },
};

DMA_HandleTypeDef hdma_spi2_tx = {
    .Instance = DMA1_Stream4,
    .Init = {
        .Channel = DMA_CHANNEL_0,
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

SPI_Config_t spi2_config = {
    .handle = &hspi2,
    .dma_tx = &hdma_spi2_tx,

    .mosi_port = GPIOB,
    .mosi_pin = GPIO_PIN_15,

    .sck_port = GPIOB,
    .sck_pin = GPIO_PIN_13,

    .nss_port = GPIOB,
    .nss_pin = GPIO_PIN_12,
};

int main(void)
{
  // enable clocks
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  HAL_Init();

  SystemClock_Config();

  setupGPIO();

  configASSERT(SPI_Init(&spi2_config) == HAL_OK);

  SEGGER_SYSVIEW_Conf();

  xTaskCreate(MainTask, "Main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &MainTaskHandle);

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

#define NR_LEDS 144
#define HEADER_LENGTH 4
#define FOOTER_LENGTH (NR_LEDS / 16)
#define FOOTER_ADDRESS HEADER_LENGTH + NR_LEDS * 4
#define BUFFER_LENGTH HEADER_LENGTH + NR_LEDS * 4 + FOOTER_LENGTH
#define LED_FRAME_BITS 0xE0
#define LED_ADDRESS(led_nr) (HEADER_LENGTH + (led_nr) * 4)
#define LED_ADDRESS_R(led_nr) (LED_ADDRESS(led_nr) + 3)
#define LED_ADDRESS_G(led_nr) (LED_ADDRESS(led_nr) + 2)
#define LED_ADDRESS_B(led_nr) (LED_ADDRESS(led_nr) + 1)

static uint8_t color_range[24] = {
      0,  12,  23,  34,  45,  56,
     67,  78,  89, 100, 111, 122,
    134, 145, 156, 167, 178, 189,
    200, 211, 222, 233, 244, 255};

void MainTask(void *arg)
{
  // enable the level shifter
  HAL_GPIO_WritePin(spi2_config.nss_port, spi2_config.nss_pin, GPIO_PIN_SET);

  static uint8_t data[BUFFER_LENGTH] = {
          [FOOTER_ADDRESS ... FOOTER_ADDRESS + FOOTER_LENGTH - 1] = 0xFF,
      };

  for(uint32_t i = 0; i < 24; ++i)
  {
    // red to yellow
    data[LED_ADDRESS(i)] = LED_FRAME_BITS | 0x2;
    data[LED_ADDRESS_R(i)] = color_range[23];
    data[LED_ADDRESS_G(i)] = color_range[i];
    data[LED_ADDRESS_B(i)] = 0x00;

    // yellow to green
    data[LED_ADDRESS(i + 24)] = LED_FRAME_BITS | 0x2;
    data[LED_ADDRESS_R(i + 24)] = color_range[23 - i];
    data[LED_ADDRESS_G(i + 24)] = color_range[23];
    data[LED_ADDRESS_B(i + 24)] = 0x00;

    // green to cyan
    data[LED_ADDRESS(i + 48)] = LED_FRAME_BITS | 0x2;
    data[LED_ADDRESS_R(i + 48)] = 0x00;
    data[LED_ADDRESS_G(i + 48)] = color_range[23];
    data[LED_ADDRESS_B(i + 48)] = color_range[i];

    // cyan to blue
    data[LED_ADDRESS(i + 72)] = LED_FRAME_BITS | 0x2;
    data[LED_ADDRESS_R(i + 72)] = 0x00;
    data[LED_ADDRESS_G(i + 72)] = color_range[23 - i];
    data[LED_ADDRESS_B(i + 72)] = color_range[23];

    // blue to purple
    data[LED_ADDRESS(i + 96)] = LED_FRAME_BITS | 0x2;
    data[LED_ADDRESS_R(i + 96)] = color_range[i];
    data[LED_ADDRESS_G(i + 96)] = 0x00;
    data[LED_ADDRESS_B(i + 96)] = color_range[23];

    // purple to red
    data[LED_ADDRESS(i + 120)] = LED_FRAME_BITS | 0x2;
    data[LED_ADDRESS_R(i + 120)] = color_range[23];
    data[LED_ADDRESS_G(i + 120)] = 0x00;
    data[LED_ADDRESS_B(i + 120)] = color_range[23 - i];
  }


  configASSERT(HAL_SPI_Transmit_DMA(spi2_config.handle, data, BUFFER_LENGTH) == HAL_OK);

  vTaskDelay(100);

  while(1)
  {
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    vTaskDelay(250);
  }
}

void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

  // LEDS
  GPIO_InitTypeDef GPIO_InitStruct = {
      .Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
      .Mode = GPIO_MODE_OUTPUT_PP,
      .Pull = GPIO_PULLUP,
      .Speed = GPIO_SPEED_LOW,
  };
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void configureTimerForRunTimeStats(void)
{

}

unsigned long getRunTimeCounterValue(void)
{
  return 0;
}
