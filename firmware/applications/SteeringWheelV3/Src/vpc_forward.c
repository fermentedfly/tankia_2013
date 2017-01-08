/*
 * vpc_forward.c
 *
 *  Created on: Dec 18, 2016
 *      Author: manuel
 */

#include "vcp_forward.h"

static void VCP_FORWARD_USB_To_UART_Task(void *arg);
static void VCP_FORWARD_UART_to_USB_Task(void *arg);

HAL_StatusTypeDef VCP_FORWARD_Init(VCP_FORWARD_Config_t *config)
{
  if(xTaskCreate(VCP_FORWARD_USB_To_UART_Task, "USB2UART", VCP_FORWARD_TASK_STACK_SIZE, config, VCP_FORWARD_TASK_PRIORITY, &config->usb_to_uart_task_handle) != pdPASS)
    return HAL_ERROR;

  if(xTaskCreate(VCP_FORWARD_UART_to_USB_Task, "UART2USB", VCP_FORWARD_TASK_STACK_SIZE, config, VCP_FORWARD_TASK_PRIORITY + 1, &config->uart_to_usb_task_handle) != pdPASS)
    return HAL_ERROR;

  return HAL_OK;
}

void VCP_FORWARD_Enable(VCP_FORWARD_Config_t *config)
{
  UART_StartReceive_DMACircular(config->uart_config);
}

void VCP_FORWARD_Disable(VCP_FORWARD_Config_t *config)
{
  UART_StopReceiveDMACircular(config->uart_config);
}

static void VCP_FORWARD_USB_To_UART_Task(void *arg)
{
  VCP_FORWARD_Config_t *config = (VCP_FORWARD_Config_t *)arg;

  uint8_t buffer;
  uint32_t bytes_read;

  while(1)
  {
    USB_VCP_readBlocking(config->usb_config, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

    HAL_UART_Transmit(config->uart_config->handle, &buffer, sizeof(buffer), 1000);
  }
}

static void VCP_FORWARD_UART_to_USB_Task(void *arg)
{
  VCP_FORWARD_Config_t *config = (VCP_FORWARD_Config_t *)arg;

  char buffer = 0;

  while(1)
  {
    buffer = UART_GetDMACircularData(config->uart_config);

    USB_VCP_writeBlocking(config->usb_config, &buffer, sizeof(buffer), portMAX_DELAY);
  }
}
