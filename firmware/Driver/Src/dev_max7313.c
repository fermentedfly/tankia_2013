#include "dev_max7313.h"
#include "FreeRTOS.h"
#include "task.h"

HAL_StatusTypeDef MAX7313_Init(MAX7313_Config_t *config)
{
  if(HAL_I2C_IsDeviceReady(config->i2c_handle, config->i2c_address, 5, 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  uint8_t port_config = 0x00;

  // configure IO ports
  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_PortConfig_Low, 1, &port_config, sizeof(port_config), 100) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_PortConfig_High, 1, &port_config, sizeof(port_config), 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

//  // write the configuration register
//  uint8_t config_reg = 0x00;
//
//  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_Configuration, 1, &config_reg, sizeof(config_reg), 100) != HAL_OK)
//  {
//    return HAL_ERROR;
//  }

//  // write master intensity
//  uint8_t master_intensity = 0x80;
//  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_MasterIntensity, 1, &master_intensity, sizeof(master_intensity), 100) != HAL_OK)
//  {
//    return HAL_ERROR;
//  }

  // outputs high
//  uint8_t output_value = 0xFF;
//  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_BlinkPhase0_Low, 1, &output_value, sizeof(output_value), 100) != HAL_OK)
//  {
//    return HAL_ERROR;
//  }
//  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_BlinkPhase0_High, 1, &output_value, sizeof(output_value), 100) != HAL_OK)
//  {
//    return HAL_ERROR;
//  }

  return HAL_OK;
}

HAL_StatusTypeDef MAX7313_WritePort(MAX7313_Config_t *config, uint16_t value)
{
  uint8_t test = value;
  HAL_I2C_Mem_Write_IT(config->i2c_handle, config->i2c_address, MAX7313_Register_BlinkPhase0_Low, 1, (uint8_t *)&value, sizeof(value));
  return HAL_OK;
}

