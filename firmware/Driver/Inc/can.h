#ifndef __can_H
#define __can_H

#include "stm32f4xx_hal.h"
#include "main.h"

extern CAN_HandleTypeDef hcan1;

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

#define CAN_MO_LVPD_STATUS_ID 0x310

#define CAN_MO_MS4_FILTER_NR 2

#define CAN_MO_MS4_IRA_ID 0x773
#define CAN_MO_MS4_SPEED_ID 0x775
#define CAN_MO_MS4_GDA_ID 0x777
#define CAN_MO_MS4_TC_ID 0x778
#define CAN_MO_MS4_SBDB_ID 0x77A

// CAN Message Containers
typedef struct __attribute__((__packed__)) CAN_MO_BCM_SETUP_1
{
  uint8_t data_select;
  uint8_t clutch_points;
  uint8_t clutch_tolerance;
  uint16_t c_sens_min;
  uint16_t c_sens_max;

} CAN_MO_BCM_SETUP_1_t;

typedef struct __attribute__((__packed__))  CAN_MO_BCM_SETUP_2
{
  uint8_t data_select;
  uint8_t g_acc_max_wspin;
  uint8_t g_acc_min_speed;
  uint8_t g_acc_shift_rpm_1; // RPM/100
  uint8_t g_acc_shift_rpm_2; // RPM/100
  uint8_t g_acc_shift_rpm_3; // RPM/100

} CAN_MO_BCM_SETUP_2_t;

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

typedef struct __attribute__((__packed__)) CAN_MO_BCM_SETUP_4
{
  uint8_t data_select;
  uint8_t acc_clutch_k1;
  uint8_t acc_clutch_k2;
  uint8_t acc_clutch_k3;
  uint8_t acc_clutch_p1;
  uint8_t acc_clutch_p2;

} CAN_MO_BCM_SETUP_4_t;

typedef struct __attribute__((__packed__)) CAN_MO_LVPD_SETUP_1
{
  uint8_t data_select;
  uint8_t fan_off_temp;
  uint8_t fan_on_temp;
  uint8_t fan_off_rpm;
  uint8_t fan_on_rpm;

} CAN_MO_LVPD_SETUP_1_t;

typedef struct __attribute__((__packed__)) CAN_MO_LVPD_SETUP_2
{
  uint8_t data_select;
  uint16_t enable_bitfield;
  uint8_t threshold_multiplex;
  uint8_t threshold_value;

} CAN_MO_LVPD_SETUP_2_t;

typedef struct __attribute__((__packed__)) CAN_MO_MS4_IRA
{
  int8_t igbase;
  int8_t igmap;
  uint8_t tdwell;
  uint8_t rev_msb;
  uint8_t rev_lsb;
  uint8_t ath;
  int8_t dath;

} CAN_MO_MS4_IRA_t;

typedef struct __attribute__((__packed__)) CAN_MO_MS4_SPEED
{
  uint8_t speed_msb;
  uint8_t speed_lsb;
  uint8_t speedFL;
  uint8_t speedFR;
  uint8_t speedRL;
  uint8_t speedRR;

} CAN_MO_MS4_SPEED_t;

typedef struct __attribute__((__packed__)) CAN_MO_MS4_GDA
{
  uint8_t gear;
  uint8_t gcstate;
  uint8_t gearratio;
  uint8_t gearcut_u;
  uint8_t ddugear;
  int8_t accx;
  int8_t accy;
  int8_t accz;

} CAN_MO_MS4_GDA_t;

typedef struct __attribute__((__packed__)) CAN_MO_MS4_ETC
{
  uint8_t etb;
  uint8_t etb_sp;
  uint8_t aps;
  uint16_t p1;
  uint8_t batt_u;
  uint8_t camshaftpos;
  uint8_t lap_c;

} CAN_MO_MS4_ETC_t;

typedef struct __attribute__((__packed__)) CAN_MO_MS4_SBDB
{
  uint8_t row_counter;
  uint8_t state_byte_1;
  uint8_t state_byte_2;
  uint8_t state_byte_3;
  uint8_t db_muxed_1;
  uint8_t db_muxed_2;
  uint8_t db_muxed_3;
  uint8_t db_muxed_4;

} CAN_MO_MS4_SBDB_t;

void MX_CAN1_Init(void);

HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef* hcan);
void CAN_StartReceive(CAN_HandleTypeDef* hcan);

#endif /*__ can_H */
