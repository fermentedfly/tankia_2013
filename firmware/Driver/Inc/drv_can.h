#ifndef __can_H
#define __can_H

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef* hcan);
void CAN_StartReceive(CAN_HandleTypeDef* hcan, CanRxMsgTypeDef *rx_msg);

#endif /*__ can_H */
