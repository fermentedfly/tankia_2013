#ifndef __can_H
#define __can_H

#include "stm32f4xx_hal.h"

#ifdef HAL_CAN_MODULE_ENABLED

#include "stm32f4xx_hal.h"

typedef void (* CAN_rxCallback)(CanRxMsgTypeDef *msg);

HAL_StatusTypeDef CAN_startContinousReceive(CAN_HandleTypeDef* handle, CAN_rxCallback callback);
void CAN_Transmit(CAN_HandleTypeDef* hcan, CanTxMsgTypeDef *tx_msg, uint8_t fromISR);

#endif /* HAL_CAN_MODULE_ENABLED */
#endif /*__ can_H */
