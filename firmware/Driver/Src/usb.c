#include "usb.h"

#define USBD_CDC_TX_TASK_STACKSIZE (configMINIMAL_STACK_SIZE)

typedef struct USBD_CDC_TxPackage {
  uint8_t *data;
  uint32_t len;
  USB_Callback cb;

} USBD_CDC_TxPackage_t;

static USB_Config_t *USB_Config;

static int8_t USBD_CDC_IF_Init(void);
static int8_t USBD_CDC_IF_DeInit(void);
static int8_t USBD_CDC_IF_Control(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t USBD_CDC_IF_Receive(uint8_t* pbuf, uint32_t *Len);

USBD_CDC_ItfTypeDef USBD_CDC_IF = {
    USBD_CDC_IF_Init,
    USBD_CDC_IF_DeInit,
    USBD_CDC_IF_Control,
    USBD_CDC_IF_Receive
};

static USBD_CDC_LineCodingTypeDef linecoding = {
    115200, /* baud rate*/
    0x00, /* stop bits-1*/
    0x00, /* parity - none*/
    0x08 /* nb. of bits 8*/
};

HAL_StatusTypeDef USB_Init(USB_Config_t *config)
{
  if (config == NULL)
    return HAL_ERROR;

  USB_Config = config;

  if (config->rxQueue == NULL)
    config->rxQueue = xQueueCreate(4, sizeof(uint8_t));
  if (config->rxQueue == NULL)
    return HAL_ERROR;

  vQueueAddToRegistry( config->rxQueue, "USB_RXQ" );

  if (config->txLock == NULL)
    config->txLock = xSemaphoreCreateMutex();
  if (config->txLock == NULL)
    return HAL_ERROR;

  if (config->rxLock == NULL)
    config->rxLock = xSemaphoreCreateMutex();
  if (config->rxLock == NULL)
    return HAL_ERROR;

  configASSERT(USBD_OK == USBD_Init(config->device, config->descriptors, 0));
  configASSERT(USBD_OK == USBD_RegisterClass(config->device, config->class));
  configASSERT(USBD_OK == USBD_CDC_RegisterInterface(config->device, config->interface));
  USBD_Start(config->device);

  return HAL_OK;
}

HAL_StatusTypeDef USB_DeInit(USB_Config_t *config)
{
  if (config == NULL)
    return HAL_ERROR;

  USBD_Stop(config->device);
  configASSERT(USBD_OK == USBD_DeInit(config->device));

  return HAL_OK;
}

void OTG_FS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(USB_Config->device->pData);
}

HAL_StatusTypeDef USB_VCP_readBlocking(USB_Config_t *config, uint8_t *pBuffer, uint32_t size, uint32_t *bytes_read, uint32_t timeoutMS)
{
  uint32_t received = 0;
  uint32_t to_read = 0;
  *bytes_read = 0;

  if (xSemaphoreTake(config->rxLock, timeoutMS))
  {
    while(size > *bytes_read)
    {
      if (xQueueReceive(config->rxQueue, &received, timeoutMS))
      {
        to_read = MIN(size - *bytes_read, received);

        memcpy(pBuffer + *bytes_read, config->rxBuffer + config->rxBufferIndex, to_read);

        *bytes_read += to_read;
      }
      else
      {
        xSemaphoreGive(config->rxLock);
        return HAL_ERROR;  // timeout occurred
      }

      uint32_t remaining = received - to_read;

      if (remaining > 0)
      {
        // more bytes have been received than requested, re-enqueue the remaining number of bytes for the next read operation
        xQueueSendToBack(config->rxQueue, &remaining, portMAX_DELAY); // we have to block indefinitely, otherwise the RX buffer index will be corrupted in case of timeout
        config->rxBufferIndex += to_read;
      }
      else
      {
        // else, start next receive operation
        config->rxBufferIndex = 0;
        USBD_CDC_ReceivePacket(config->device);
      }
    }

    xSemaphoreGive(config->rxLock);
    return HAL_OK;
  }

  return HAL_ERROR;  // timeout occurred
}

uint32_t USB_VCP_writeBlocking(USB_Config_t *config, void *pBuffer, uint32_t size, uint32_t timeoutMS)
{
  if (xSemaphoreTake(config->txLock, timeoutMS))
  {
    USBD_CDC_SetTxBuffer(USB_Config->device, pBuffer, size);
    if (USBD_CDC_TransmitPacket(USB_Config->device) != USBD_OK)
    {
      xSemaphoreGive(config->txLock);
      return 0;
    }

    while (((USBD_CDC_HandleTypeDef *) config->device->pClassData)->TxState)
    {
    }  //Wait until transfer is done

    xSemaphoreGive(config->txLock);
    return size;
  }
  else
    return 0;
}

/**
 * @brief  CDC_IF_Init
 *         Initializes the CDC media low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t USBD_CDC_IF_Init(void)
{
  /*
   Add your initialization code here
   */
  USBD_CDC_SetRxBuffer(USB_Config->device, USB_Config->rxBuffer);
  return (0);
}

/**
 * @brief  TEMPLATE_DeInit
 *         DeInitializes the CDC media low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t USBD_CDC_IF_DeInit(void)
{
  /*
   Add your deinitialization code here
   */
  return (0);
}

/**
 * @brief  TEMPLATE_Control
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t USBD_CDC_IF_Control(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  switch (cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:
      /* Add your code here */
      break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
      /* Add your code here */
      break;

    case CDC_SET_COMM_FEATURE:
      /* Add your code here */
      break;

    case CDC_GET_COMM_FEATURE:
      /* Add your code here */
      break;

    case CDC_CLEAR_COMM_FEATURE:
      /* Add your code here */
      break;

    case CDC_SET_LINE_CODING:
      linecoding.bitrate = (uint32_t) (pbuf[0] | (pbuf[1] << 8)
          |\
 (pbuf[2] << 16) | (pbuf[3] << 24));
      linecoding.format = pbuf[4];
      linecoding.paritytype = pbuf[5];
      linecoding.datatype = pbuf[6];

      /* Add your code here */
      break;

    case CDC_GET_LINE_CODING:
      pbuf[0] = (uint8_t) (linecoding.bitrate);
      pbuf[1] = (uint8_t) (linecoding.bitrate >> 8);
      pbuf[2] = (uint8_t) (linecoding.bitrate >> 16);
      pbuf[3] = (uint8_t) (linecoding.bitrate >> 24);
      pbuf[4] = linecoding.format;
      pbuf[5] = linecoding.paritytype;
      pbuf[6] = linecoding.datatype;

      /* Add your code here */
      break;

    case CDC_SET_CONTROL_LINE_STATE:
      /* Add your code here */
      break;

    case CDC_SEND_BREAK:
      /* Add your code here */
      break;

    default:
      break;
  }

  return (0);
}

/**
 * @brief  TEMPLATE_Receive
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will issue a NAK packet on any OUT packet received on
 *         USB endpoint untill exiting this function. If you exit this function
 *         before transfer is complete on CDC interface (ie. using DMA controller)
 *         it will result in receiving more data while previous ones are still
 *         not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t USBD_CDC_IF_Receive(uint8_t* Buf, uint32_t *Len)
{
  // enqueue size of new package
  // copying the data is not necessary because the next receive operation is not started until all data has been read
  (void)Buf;
  xQueueSendToBackFromISR(USB_Config->rxQueue, Len, NULL);

  return USBD_OK;
}
