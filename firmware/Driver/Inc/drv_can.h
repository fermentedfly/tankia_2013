#ifndef __can_H
#define __can_H

#include "stm32f4xx_hal.h"

#ifdef HAL_CAN_MODULE_ENABLED

HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef* hcan);
void CAN_StartReceive(CAN_HandleTypeDef* hcan, CanRxMsgTypeDef *rx_msg);

#endif /* HAL_CAN_MODULE_ENABLED */
#endif /*__ can_H */
