#include "dev_max7313.h"
#include "FreeRTOS.h"
#include "task.h"

HAL_StatusTypeDef MAX7313_Init(MAX7313_Config_t *config)
{
  if(HAL_I2C_IsDeviceReady(config->i2c_handle, config->i2c_address, 5, 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // write the configuration register
  MAX7313_RegisterContent_Configuration_t reg_config =
  {
      .blink_enable = config->blink_enabled,
      .global_intensity = config->global_intensity_enabled,
  };

  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_Configuration, 1, (uint8_t *)&reg_config, sizeof(reg_config), 100) != HAL_OK)
    {
      return HAL_ERROR;
    }

  // write master intensity
  MAX7313_RegisterContent_MasterIntensity_t reg_intensity = {
      .master_intensity = config->master_intensity,
  };

  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_MasterIntensity, 1, (uint8_t *)&reg_intensity, sizeof(reg_intensity), 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // write port intensity
  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_OutputIntensity_1_0, 1, (uint8_t *)&config->port_intensity, sizeof(config->port_intensity), 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // configure IO ports
  if(HAL_I2C_Mem_Write(config->i2c_handle, config->i2c_address, MAX7313_Register_PortConfig_Low, 1, (uint8_t *)&config->port_config, sizeof(config->port_config), 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef MAX7313_WritePort(MAX7313_Config_t *config, uint16_t value)
{
  return HAL_I2C_Mem_Write_IT(config->i2c_handle, config->i2c_address, MAX7313_Register_BlinkPhase0_Low, 1, (uint8_t*)&value, sizeof(value));
}

