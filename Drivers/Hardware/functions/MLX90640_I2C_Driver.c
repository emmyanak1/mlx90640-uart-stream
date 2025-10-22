/*
 * MLX90640_I2C_Driver.c
 *
 *  Created on: Jul 1, 2025
 *      Author: emma.yanakiev
 */

/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <stdint.h>
#include "main.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "MLX90640_I2C_Driver.h"

#define MLX90640_I2C_ADDR(slaveAddr)   ((slaveAddr) << 1) //7bit address --> 8bit address


void MLX90640_I2CInit()
{
	MX_I2C1_Init();
}


extern int MLX90640_I2CGeneralReset(void);

//manual I2C transmit & receive
extern int MLX90640_I2CRead_Basic(uint8_t slaveAddr,uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
	for (uint16_t i = 0; i < nMemAddressRead; i++)
	{
		uint8_t reg[2] = {
				(uint8_t)((startAddress + i) >> 8), //Address MSB
				(uint8_t)((startAddress + i ) & 0xFF) //Address LSB
		};



		if (HAL_I2C_Master_Transmit(&hi2c1, MLX90640_I2C_ADDR(slaveAddr), reg, 2, HAL_MAX_DELAY) != HAL_OK)
		{
			return -1; //sends 2 byte address reg over I2C to the sensor
		}

		uint8_t rx[2]; //declare 2 byte array for data

		if (HAL_I2C_Master_Receive(&hi2c1, MLX90640_I2C_ADDR(slaveAddr), rx, 2, HAL_MAX_DELAY) != HAL_OK)
		{
			return -1;
		} // read 2 bytes of data, MSB is in rx[0] lsb is in RX[1]


		data[i] = ((uint16_t)rx[0] << 8) | rx[1]; // shift MSB to the left 8 bits, bitwise OR with LSB


	}

	return 0;

}

//manual I2C transmit & receive
extern int MLX90640_I2CRead_HALMem(uint8_t slaveAddr,uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
	uint8_t *p = (uint8_t *)data;

	int ack = HAL_I2C_Mem_Read(
		&hi2c1,
		MLX90640_I2C_ADDR(slaveAddr),
		startAddress,
		I2C_MEMADD_SIZE_16BIT,
		p,
		nMemAddressRead * 2,
		500
	);

	if (ack != HAL_OK)
			return -1;


// Swap bytes
	for (uint16_t i = 0; i < nMemAddressRead * 2; i += 2) {
		uint8_t temp = p[i];
		p[i] = p[i + 1];
		p[i + 1] = temp;
	}

	return 0;

}


extern int MLX90640_I2CWrite(uint8_t slaveAddr,uint16_t writeAddress, uint16_t data)
{

	uint8_t sa;
	int ack = 0;
	uint8_t cmd[2];
	static uint16_t dataCheck;

	sa = (slaveAddr << 1);

	cmd[0] = data >> 8;
	cmd[1] = data & 0x00FF;


	ack = HAL_I2C_Mem_Write(&hi2c1, sa, writeAddress, I2C_MEMADD_SIZE_16BIT, cmd, sizeof(cmd), 500);

	if (ack != HAL_OK)
	{
			return -1;
	}

	MLX90640_I2CRead_HALMem(slaveAddr,writeAddress,1, &dataCheck);

	if ( dataCheck != data)
	{
			return -2;
	}

	return 0;
}
extern void MLX90640_I2CFreqSet(int freq);

