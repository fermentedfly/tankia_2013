#ifndef DEV_MAX7313_H_
#define DEV_MAX7313_H_

#include "stm32f4xx_hal.h"

typedef struct __attribute__((__packed__)) MAX7313_RegisterContent_Configuration
{
  uint32_t blink_enable       :1;
  uint32_t blink_flip         :1;
  uint32_t global_intensity   :1;
  uint32_t interrupt_enable   :1;
  uint32_t interrupt_control  :2;
  uint32_t filler             :1;
  uint32_t interrupt_status   :1;

} MAX7313_RegisterContent_Configuration_t;

typedef struct __attribute__((__packed__)) MAX7313_RegisterContent_MasterIntensity
{
  uint32_t o16_intensity    :4;
  uint32_t master_intensity :4;

} MAX7313_RegisterContent_MasterIntensity_t;

typedef struct __attribute__((__packed__)) MAX7313_RegisterContent_PortConfiguration_Low
{
  uint32_t port_0   :1;
  uint32_t port_1   :1;
  uint32_t port_2   :1;
  uint32_t port_3   :1;
  uint32_t port_4   :1;
  uint32_t port_5   :1;
  uint32_t port_6   :1;
  uint32_t port_7   :1;
} MAX7313_RegisterContent_PortConfiguration_Low_t;

typedef struct __attribute__((__packed__)) MAX7313_RegisterContent_PortConfiguration_High
{
  uint32_t port_8   :1;
  uint32_t port_9   :1;
  uint32_t port_10  :1;
  uint32_t port_11  :1;
  uint32_t port_12  :1;
  uint32_t port_13  :1;
  uint32_t port_14  :1;
  uint32_t port_15  :1;

} MAX7313_RegisterContent_PortConfiguration_High_t;

typedef struct __attribute__((__packed__)) MAX7313_RegisterContent_PortConfiguration
{
  MAX7313_RegisterContent_PortConfiguration_Low_t port_low;
  MAX7313_RegisterContent_PortConfiguration_High_t port_high;

} MAX7313_RegisterContent_PortConfiguration_t;

typedef struct MAX7313_Config
{
  I2C_HandleTypeDef *i2c_handle;
  uint16_t i2c_address;

  uint8_t blink_enabled;
  uint8_t global_intensity_enabled;

  uint8_t master_intensity;

  MAX7313_RegisterContent_PortConfiguration_t port_config;

} MAX7313_Config_t;

typedef enum MAX7313_Register
{
  MAX7313_Register_InputPort_Low = 0x00,
  MAX7313_Register_InputPort_High = 0x01,
  MAX7313_Register_BlinkPhase0_Low = 0x02,
  MAX7313_Register_BlinkPhase0_High = 0x03,
  MAX7313_Register_PortConfig_Low = 0x06,
  MAX7313_Register_PortConfig_High = 0x07,
  MAX7313_Register_BlinkPhase1_Low = 0x0A,
  MAX7313_Register_BlinkPhase1_High = 0x0B,
  MAX7313_Register_MasterIntensity = 0x0E,
  MAX7313_Register_Configuration = 0x0F,
  MAX7313_Register_OutputIntensity_1_0 = 0x10,
  MAX7313_Register_OutputIntensity_3_2 = 0x11,
  MAX7313_Register_OutputIntensity_5_4 = 0x12,
  MAX7313_Register_OutputIntensity_7_6 = 0x13,
  MAX7313_Register_OutputIntensity_9_8 = 0x14,
  MAX7313_Register_OutputIntensity_11_10 = 0x15,
  MAX7313_Register_OutputIntensity_13_12 = 0x16,
  MAX7313_Register_OutputIntensity_15_14 = 0x17,

} MAX7313_Register_t;

HAL_StatusTypeDef MAX7313_Init(MAX7313_Config_t *config);
HAL_StatusTypeDef MAX7313_WritePort(MAX7313_Config_t *config, uint16_t value);


#endif
