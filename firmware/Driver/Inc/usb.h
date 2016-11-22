#ifndef USB_DRIVER_H
#define USB_DRIVER_H

#include <usbd_core.h>
#include <usbd_cdc.h>
#include <usbd_desc.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

typedef void(*USB_Callback)(void);

typedef struct USB_Config
{
  USBD_HandleTypeDef      *device;
  USBD_DescriptorsTypeDef *descriptors;
  USBD_ClassTypeDef       *class;
  USBD_CDC_ItfTypeDef     *interface;

  QueueHandle_t           *rxQueue;
  uint8_t                 rxBuffer[CDC_DATA_FS_OUT_PACKET_SIZE];
  uint32_t                rxBufferIndex;

  SemaphoreHandle_t       txLock;
  SemaphoreHandle_t       rxLock;


} USB_Config_t;

extern USBD_CDC_ItfTypeDef  USBD_CDC_IF;

/**
 * @brief Init's the USB VCP Device
 *
 * @param config the configuration struct
 *
 * @return STATUS_OK if successful, STATUS_ERROR else
 */
HAL_StatusTypeDef USB_Init(USB_Config_t *config);

/**
 * @brief DeInit the USB VCP Device
 *
 * @param config the configuration struct
 *
 * @return STATUS_OK if successful, STATUS_ERROR else
 */
HAL_StatusTypeDef USB_DeInit(USB_Config_t *config);

/**
 * @brief attempts to read from the USB VCP
 *
 * @param config  the configuration struct
 * @param pBuffer buffer for the new data
 * @param size the number of bytes to be read (At max a full USB FS package == 64 Bytes)
 * @param timeoutMS maximum time to wait for completion
 * @param pointer to the number of bytes actually read
 *
 * @return STATUS_OK if successful, STATUS_ERROR else (most probably TIMEOUT)
 */
HAL_StatusTypeDef USB_VCP_readBlocking(USB_Config_t *config, uint8_t *pBuffer, uint32_t size, uint32_t *bytes_read, uint32_t timeoutMS);

/**
 * @brief writes data through the USB VCP in blocking mode
 *
 * @param config  the configuration struct
 * @param pBuffer the data to be written
 * @param size the size of the data
 * @param timeoutMS maximum time to wait for completion
 *
 * @return the number of bytes written
 */
uint32_t USB_VCP_writeBlocking(USB_Config_t *config, void *pBuffer, uint32_t size, uint32_t timeoutMS);

#endif
