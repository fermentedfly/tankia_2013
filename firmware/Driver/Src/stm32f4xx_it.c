#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "FreeRTOS.h"
#include "task.h"

extern DMA_HandleTypeDef hdma_uart4_tx;
extern DMA_HandleTypeDef hdma_uart4_rx;

extern TIM_HandleTypeDef htim1;

void xPortSysTickHandler( void );

void SysTick_Handler(void)
{
  // /!\ freeRTOS sets SysTick's priority to 15 (lowest!!!)
  // use different timer for HAL ticks
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    xPortSysTickHandler();
  }
}

void HardFault_Handler(void)
{
  __asm volatile
      (
          " tst lr, #4                                                \n"
          " ite eq                                                    \n"
          " mrseq r0, msp                                             \n"
          " mrsne r0, psp                                             \n"
          " ldr r1, [r0, #24]                                         \n"
          " ldr r2, handler2_address_const                            \n"
          " bx r2                                                     \n"
          " handler2_address_const: .word prvGetRegistersFromStack    \n"
      );
  while(1)
  {
  }
}

volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register - subroutine call return address */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimizing them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    __asm__ __volatile__ ("BKPT");
}


/**
* @brief This function handles Debug Monitor.
*/
void DebugMon_Handler(void)
{
  while (1)
  {
    __asm__ __volatile__ ("BKPT");
  }
}

/**
* @brief This function handles Pre-fetch fault, memory access fault.
*/
void BusFault_Handler(void)
{
  while (1)
  {
    __asm__ __volatile__ ("BKPT");
  }
}

/**
* @brief This function handles Undefined instruction or illegal state.
*/
void UsageFault_Handler(void)
{
  while (1)
  {
    __asm__ __volatile__ ("BKPT");
  }
}

/**
* @brief This function handles Memory management fault.
*/
void MemManage_Handler(void)
{
  while (1)
  {
    __asm__ __volatile__ ("BKPT");
  }
}

/**
* @brief This function handles Non Maskable Interrupt.
*/
void HAL_RCC_CCSCallback(void)
{
  while (1)
  {
    __asm__ __volatile__ ("BKPT");
  }
}

void NMI_Handler(void)
{
  HAL_RCC_NMI_IRQHandler();
}

void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */

  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
* @brief This function handles EXTI line1 interrupt.
*/
void EXTI1_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

/**
* @brief This function handles EXTI line2 interrupt.
*/
void EXTI2_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

/**
* @brief This function handles EXTI line4 interrupt.
*/
void EXTI4_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}



/**
* @brief This function handles EXTI line[9:5] interrupts.
*/
void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
}

/**
* @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
*/
void TIM1_UP_TIM10_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

/**
* @brief This function handles EXTI line[15:10] interrupts.
*/
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
}

