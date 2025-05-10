/*
 * spi_io.h
 *
 *  Created on: May 9, 2025
 *      Author: mickey
 */

#ifndef UTILS_SPI_IO_H_
#define UTILS_SPI_IO_H_

#define SPI_DATA_MAX_LEN (64u)
#define SPI_REG_COUNT (2u)

#include <stdbool.h>
#include <string.h>

#include "main.h"

#include "uart_io.h"

typedef enum SPIOperation
{
	SPIOP_NONE = 0x00,
	SPIOP_TX = 0x01,
	SPIOP_RX = 0x02,
	SPIOP_TX_RX = 0x03,
} SPIOperation_t;

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

typedef struct SPIHeader
{
	uint8_t pad_head[2];
	uint8_t opcode;
	uint8_t tx_reg;
	uint8_t tx_len;
	uint8_t rx_reg;
	uint8_t rx_len;
	uint8_t pad_tail[1];
} SPIHeader_t;

typedef struct SPIPacket
{
	SPIHeader_t header;
	uint8_t data[SPI_DATA_MAX_LEN];
} SPIPacket_t;

typedef struct SPIDevice
{
	SPI_HandleTypeDef *handle;
	GPIO_TypeDef *cs_port_in;
	GPIO_TypeDef *cs_port_out;
	struct SPIDevice *target_device;
	uint16_t cs_pin_in;
	uint16_t cs_pin_out;
	volatile SPIDeviceState_t state;
	volatile SPIOperation_t op;
	volatile uint8_t tx_pos;
	volatile uint8_t rx_pos;
	volatile SPIPacket_t tx_buff;
	volatile SPIPacket_t rx_buff;
	volatile char regs[SPI_REG_COUNT][SPI_DATA_MAX_LEN];
	char name[8];
} SPIDevice_t;

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi5;

bool spi_io_is_initialized(void);
void spi_io_initialize(void);
SPIDevice_t* hspi_to_struct(SPI_HandleTypeDef *hspi);
bool spi_io_transmit(SPIDevice_t *spid, uint8_t *data, uint8_t len, uint8_t dst_reg, SPIDevice_t *target_device);
bool spi_io_receive(SPIDevice_t *spid);

#endif /* UTILS_SPI_IO_H_ */
