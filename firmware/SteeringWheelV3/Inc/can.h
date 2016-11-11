/**
  ******************************************************************************
  * File Name          : CAN.h
  * Description        : This file provides code for the configuration
  *                      of the CAN instances.
  ******************************************************************************
  *
  * Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __can_H
#define __can_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan1;

/* USER CODE BEGIN Private defines */

#define CAN1_BANK_NUMBER 28
#define CAN_RX_TASK_STACK_SIZE 512

// CAN Message IDS

// Received
#define CAN_MO_BCM_FILTER_NR 0

#define CAN_MO_BCM_SETUP_CONFIRM_1_ID 0x200
#define CAN_MO_BCM_SETUP_CONFIRM_2_ID 0x201
#define CAN_MO_BCM_SETUP_CONFIRM_3_ID 0x202
#define CAN_MO_BCM_SETUP_CONFIRM_4_ID 0x203

#define CAN_MO_BCM_STATUS_ID 0x210

#define CAN_MO_LVPD_FILTER_NR 1

#define CAN_MO_LVPD_SETUP_CONFIRM_1_ID 0x300
#define CAN_MO_LVPD_SETUP_CONFIRM_2_ID 0x301
#define CAN_MO_LVPD_SETUP_CONFIRM_3_ID 0x302
#define CAN_MO_LVPD_SETUP_CONFIRM_4_ID 0x303

#define CAN_MO_LVPD_STATUS_ID 0x310

#define CAN_MO_MS4_FILTER_NR 2

#define CAN_MO_MS4_IRA_ID 0x773

// CAN Message Containers
typedef struct __attribute__((__packed__)) CAN_MO_BCM_SETUP_1
{
  uint8_t data_select;
  uint8_t clutch_points;
  uint8_t clutch_tolerance;
  uint16_t c_sens_min;
  uint16_t c_sens_max;

} CAN_MO_BCM_SETUP_1_t;

/**
 * @struct CAN_MO_BCM_SETUP_2
 * @brief CAN Message bcm setup 2 message container
 */
typedef struct __attribute__((__packed__))  CAN_MO_BCM_SETUP_2
{
  uint8_t data_select;
  uint8_t g_acc_max_wspin;
  uint8_t g_acc_min_speed;
  uint8_t g_acc_shift_rpm_1; // RPM/100
  uint8_t g_acc_shift_rpm_2; // RPM/100
  uint8_t g_acc_shift_rpm_3; // RPM/100

} CAN_MO_BCM_SETUP_2_t;

/**
 * @struct CAN_MO_BCM_SETUP_3
 * @brief CAN Message bcm setup 3 message container
 */
typedef struct __attribute__((__packed__)) CAN_MO_BCM_SETUP_3
{
  uint8_t data_select;
  uint8_t g_min_shift_delay; // *10[ms]
  uint8_t g_up_holdtime; // *10[ms]
  uint8_t g_dn_holdtime; // *10[ms]
  uint8_t g_n_holdtime; // *1[ms]
  uint8_t traction; // traction control level: 0 - 11
  uint8_t map; // mapping: 1 - 2

} CAN_MO_BCM_SETUP_3_t;

/**
 * @struct CAN_MO_BCM_SETUP_4
 * @brief CAN Message bcm setup 4 message container
 */
typedef struct __attribute__((__packed__)) CAN_MO_BCM_SETUP_4
{
  uint8_t data_select;
  uint8_t acc_clutch_k1;
  uint8_t acc_clutch_k2;
  uint8_t acc_clutch_k3;
  uint8_t acc_clutch_p1;
  uint8_t acc_clutch_p2;

} CAN_MO_BCM_SETUP_4_t;

/* USER CODE END Private defines */

extern void Error_Handler(void);

void MX_CAN1_Init(void);

/* USER CODE BEGIN Prototypes */

HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef* hcan);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ can_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
