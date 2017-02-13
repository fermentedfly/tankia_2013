#ifndef __i2c_H
#define __i2c_H

#include "stm32f4xx_hal.h"

#ifdef HAL_I2C_MODULE_ENABLED

HAL_StatusTypeDef I2C_Init(I2C_HandleTypeDef *handle);

#endif
#endif
