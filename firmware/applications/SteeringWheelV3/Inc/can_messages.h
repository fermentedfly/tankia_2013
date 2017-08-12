/*
 * can_messages.h
 *
 *  Created on: Nov 22, 2016
 *      Author: manuel
 */

#ifndef INC_CAN_MESSAGES_H_
#define INC_CAN_MESSAGES_H_

#include "stm32f4xx_hal.h"

#define CAN_MESSAGES_TX_TASK_STACK_SIZE 128
#define CAN1_BANK_NUMBER 28
#define CAN_MESSAGES_EVENTS_ALL 0xFFFFFFFF

// CAN Message IDS
typedef enum CAN_MO_ID
{
	CAN_MO_ID_SW_SHIFT 				= 0x10,
	CAN_MO_ID_SW_CLUTCH 			= 0x11,
	CAN_MO_ID_BCM_SETUP_CONFIRM_1   = 0x200,
	CAN_MO_ID_BCM_SETUP_CONFIRM_2   = 0x201,
	CAN_MO_ID_BCM_SETUP_CONFIRM_3   = 0x202,
	CAN_MO_ID_BCM_SETUP_CONFIRM_4   = 0x203,

	CAN_MO_ID_BCM_STATUS            = 0x210,

	CAN_MO_ID_LVPD_SETUP_CONFIRM_1  = 0x300,
	CAN_MO_ID_LVPD_SETUP_CONFIRM_2  = 0x301,

	CAN_MO_ID_LVPD_STATUS           = 0x310,

	CAN_MO_ID_MS4_IRA               = 0x773,
	CAN_MO_ID_MS4_SPEED             = 0x775,
	CAN_MO_ID_MS4_GDA               = 0x777,
	CAN_MO_ID_MS4_TC                = 0x778,
	CAN_MO_ID_MS4_SBDB              = 0x77A,

} CAN_MO_ID_t;

typedef enum CAN_MO_Filter
{
	CAN_MO_FILTER_NR_BCM = 0,
	CAN_MO_FILTER_NR_LVPD,
	CAN_MO_FILTER_NR_MS4,

} CAN_MO_Filter_t;

typedef struct __attribute__((__packed__)) CAN_MO_SW_Shift
{
	uint8_t direction;

} CAN_MO_SW_Shift_t;

typedef enum CAN_MO_SW_Shift_Direction
{
	CAN_MO_SW_Shift_Direction_Down = 0x0F,
	CAN_MO_SW_Shift_Direction_Up   = 0xF0,

} CAN_MO_SW_Shift_Direction_t;

typedef struct __attribute__((__packed__)) CAN_MO_SW_Clutch
{
	uint8_t value;

} CAN_MO_SW_Clutch_t;


// CAN Message Containers
typedef struct __attribute__((__packed__)) CAN_MO_BCM_Setup_1
{
  uint8_t data_select;
  uint8_t clutch_points;
  uint8_t clutch_tolerance;
  uint16_t c_sens_min;
  uint16_t c_sens_max;

} CAN_MO_BCM_Setup_1_t;

typedef struct __attribute__((__packed__))  CAN_MO_BCM_Setup_2
{
  uint8_t data_select;
  uint8_t g_acc_min_speed;
  uint8_t g_acc_max_wspin;
  uint8_t g_acc_shift_rpm_1; // RPM/100
  uint8_t g_acc_shift_rpm_2; // RPM/100
  uint8_t g_acc_shift_rpm_3; // RPM/100

} CAN_MO_BCM_Setup_2_t;

typedef struct __attribute__((__packed__)) CAN_MO_BCM_Setup_3
{
  uint8_t data_select;
  uint8_t g_min_shift_delay; // *10[ms]
  uint8_t g_up_holdtime; // *10[ms]
  uint8_t g_dn_holdtime; // *10[ms]
  uint8_t g_n_holdtime; // *1[ms]
  uint8_t traction; // traction control level: 0 - 11
  uint8_t map; // mapping: 1 - 2

} CAN_MO_BCM_Setup_3_t;

typedef struct __attribute__((__packed__)) CAN_MO_BCM_Setup_4
{
  uint8_t data_select;
  uint8_t acc_clutch_k1;
  uint8_t acc_clutch_k2;
  uint8_t acc_clutch_k3;
  uint8_t acc_clutch_p1;
  uint8_t acc_clutch_p2;

} CAN_MO_BCM_Setup_4_t;

typedef struct __attribute__((__packed__)) CAN_MO_LVPD_Setup_1
{
  uint8_t data_select;
  uint8_t fan_off_temp;
  uint8_t fan_on_temp;
  uint8_t fan_off_rpm;
  uint8_t fan_on_rpm;

} CAN_MO_LVPD_Setup_1_t;

typedef struct __attribute__((__packed__)) CAN_MO_LVPD_Setup_2
{
  uint8_t data_select;
  uint16_t enable_bitfield;
  uint8_t threshold_multiplex;
  uint8_t threshold_value;

} CAN_MO_LVPD_Setup_2_t;

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

typedef struct __attribute__((__packed__)) CAN_MO_MS4_Speed
{
  uint8_t speed_msb;
  uint8_t speed_lsb;
  uint8_t speedFL;
  uint8_t speedFR;
  uint8_t speedRL;
  uint8_t speedRR;

} CAN_MO_MS4_Speed_t;

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
  union __attribute__((__packed__)) muxed
  {
	  struct  __attribute__((__packed__)) raw
	  {
		  uint8_t db_muxed_1;
		  uint8_t db_muxed_2;
		  uint8_t db_muxed_3;
		  uint8_t db_muxed_4;
	  } raw;

	  struct  __attribute__((__packed__)) row_1
	  {
		  uint8_t pcrank;
		  uint8_t poil;
		  uint8_t pwat;
		  uint8_t pfuel;
	  } row_1;

	  struct  __attribute__((__packed__)) row_2
	  {
		  uint16_t pamb;
		  uint8_t mappos;
		  uint8_t tair;
	  } row_2;

	  struct  __attribute__((__packed__)) row_3
	  {
		  uint16_t fuellap;
		  uint16_t fueltank;
	  } row_3;
	  struct  __attribute__((__packed__)) row_4
	  {
		  uint8_t tfuel;
		  uint8_t toil;
		  uint8_t tlam;
		  uint8_t tlam_2;
	  } row_4;

	  struct  __attribute__((__packed__)) row_5
	  {
		  uint8_t tmot;
		  uint8_t tex;
		  uint8_t tex_2;
		  uint8_t dduleds;
	  } row_5;

  } muxed;

} CAN_MO_MS4_SBDB_t;

void CAN_MESSAGES_Init(CAN_HandleTypeDef* hcan);
void CAN_MESSAGES_Transmit(CAN_HandleTypeDef* hcan, CanTxMsgTypeDef *tx_msg, uint8_t fromISR);
void CAN_MESSAGES_TransmitSWShift(CAN_HandleTypeDef* hcan, CAN_MO_SW_Shift_Direction_t direction, uint8_t fromISR);
void CAN_MESSAGES_TransmitSWClutch(CAN_HandleTypeDef* hcan, uint8_t value, uint8_t fromISR);

#endif /* INC_CAN_MESSAGES_H_ */
