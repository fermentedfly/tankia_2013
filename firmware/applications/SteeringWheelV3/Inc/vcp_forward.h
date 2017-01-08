/*
 * vcp_forward.h
 *
 *  Created on: Dec 18, 2016
 *      Author: manuel
 */

#ifndef INC_VCP_FORWARD_H_
#define INC_VCP_FORWARD_H_

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_usb.h"
#include "drv_usart.h"

#define VCP_FORWARD_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define VCP_FORWARD_TASK_STACK_SIZE 256

typedef struct VCP_FORWARD_Config
{
  UART_Config_t *uart_config;
  USB_Config_t *usb_config;

  uint8_t enabled;

  TaskHandle_t usb_to_uart_task_handle;
  TaskHandle_t uart_to_usb_task_handle;

} VCP_FORWARD_Config_t;

HAL_StatusTypeDef VCP_FORWARD_Init(VCP_FORWARD_Config_t *config);
void VCP_FORWARD_Enable(VCP_FORWARD_Config_t *config);
void VCP_FORWARD_Disable(VCP_FORWARD_Config_t *config);



#endif /* INC_VCP_FORWARD_H_ */
