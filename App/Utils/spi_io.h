/*
 * spi_io.h
 *
 *  Created on: May 9, 2025
 *      Author: mickey
 */

#ifndef UTILS_SPI_IO_H_
#define UTILS_SPI_IO_H_

#define SPI_BUFF_SIZE 256

#include "main.h"

typedef enum SPIDeviceState
{
	SPISTATE_PENDING = 0x00,
	SPISTATE_CPLT = 0x01,

	SPISTATE_TX_PENDING = 0x02,
	SPISTATE_TX_CPLT = 0x03,

	SPISTATE_RX_PENDING = 0x04,
	SPISTATE_RX_CPLT = 0x05,

	SPISTATE_TX_RX_PENDING = 0x06,
	SPISTATE_TX_RX_CPLT = 0x07,

	SPISTATE_ABORT = 0x08,
	SPISTATE_ERROR = 0x10,
	SPISTATE_SELECTED = 0x20,
} SPIDeviceState_t;

typedef struct SPIDevice
{
	SPI_HandleTypeDef *handle;
	GPIO_TypeDef *cs_port_in;
	GPIO_TypeDef *cs_port_out;
	uint16_t cs_pin_in;
	uint16_t cs_pin_out;
	SPIDeviceState_t state;
	uint8_t tx_buff[SPI_BUFF_SIZE];
	uint8_t rx_buff[SPI_BUFF_SIZE];
} SPIDevice_t;

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi5;

SPIDevice_t* hspi_to_struct(SPI_HandleTypeDef *hspi);

#endif /* UTILS_SPI_IO_H_ */
