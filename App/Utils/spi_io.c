/*
 * spi_io.c
 *
 *  Created on: May 9, 2025
 *      Author: mickey
 */

#include "spi_io.h"

static bool is_initialized = false;
static SPIDevice_t devices[3] = {0};

static void spi_io_process_rx(SPIDevice_t *spid)
{
	if (spid->rx_buff.header.tx_len > 0
		&& spid->rx_buff.header.tx_reg < SPI_REG_COUNT)
	{
		memcpy((uint8_t *)spid->regs[spid->rx_buff.header.tx_reg],
				(uint8_t *)spid->rx_buff.data, spid->rx_buff.header.tx_len);
	}

	spid->state |= SPISTATE_RX_CPLT;
	spid->op &= ~SPIOP_RX;

	if (spid->rx_buff.header.rx_len > 0
		&& spid->rx_buff.header.rx_reg < SPI_REG_COUNT)
	{
		spi_io_transmit(spid,
				(uint8_t *)spid->regs[spid->rx_buff.header.rx_reg],
				spid->rx_buff.header.rx_len,
				spid->rx_buff.header.rx_reg, NULL);
	}
}

bool spi_io_is_initialized(void)
{
	return is_initialized;
}

void spi_io_initialize(void)
{
	if (is_initialized) return;

	bzero(devices+0, sizeof(SPIDevice_t));
	strcpy(devices[0].name, "SPI1");
	devices[0].handle = &hspi1;

	bzero(devices+1, sizeof(SPIDevice_t));
	strcpy(devices[1].name, "SPI3");
	devices[1].handle = &hspi3;
	devices[1].cs_pin_in = SPI3_CS_IN_Pin;
	devices[1].cs_pin_out = SPI3_CS_OUT_Pin;
	devices[1].cs_port_in = SPI3_CS_IN_GPIO_Port;
	devices[1].cs_port_out = SPI3_CS_OUT_GPIO_Port;

	bzero(devices+2, sizeof(SPIDevice_t));
	strcpy(devices[2].name, "SPI5");
	devices[2].handle = &hspi5;
	devices[2].cs_pin_in = SPI5_CS_IN_Pin;
	devices[2].cs_pin_out = SPI5_CS_OUT_Pin;
	devices[2].cs_port_in = SPI5_CS_IN_GPIO_Port;
	devices[2].cs_port_out = SPI5_CS_OUT_GPIO_Port;

	is_initialized = true;
}

SPIDevice_t* hspi_to_struct(SPI_HandleTypeDef *hspi)
{
	if (hspi == &hspi1) return devices+0;
	if (hspi == &hspi3) return devices+1;
	if (hspi == &hspi5) return devices+2;

	return NULL;
}

bool spi_io_transmit(SPIDevice_t *spid, uint8_t *data, uint8_t len, uint8_t dst_reg, SPIDevice_t *target_device)
{
	if (len < 1) return false;

	if (dst_reg >= SPI_REG_COUNT) return false;

	if (spid->op & SPIOP_TX)
	{
		/*
		serial_print("Device ", 0);
		serial_print(spid->name, 0);
		serial_print_line(" TX line busy, aborting transmission.", 0);
		*/
		return false;
	}

	spid->state |= SPISTATE_TX_PENDING;
	spid->op |= SPIOP_TX;

	if (len > SPI_DATA_MAX_LEN) len = SPI_DATA_MAX_LEN;

	bzero((uint8_t *)&spid->tx_buff, sizeof(SPIPacket_t));
	memset((uint8_t *)spid->tx_buff.header.pad_head, 255u, 2);
	memset((uint8_t *)spid->tx_buff.header.pad_tail, 255u, 1);
	spid->tx_buff.header.opcode = SPIOP_TX;
	spid->tx_buff.header.tx_len = len;
	spid->tx_buff.header.tx_reg = dst_reg;
	spid->tx_buff.header.rx_len = 0u;
	spid->tx_buff.header.rx_reg = 0u;
	memcpy((uint8_t *)spid->tx_buff.data, data, len);

	spid->tx_pos = 0;

	// the target device is only set when a Controller is transmitting,
	// since it is only used for controlling the CS line
	if (target_device != NULL)
	{
		/**
		 * Enabling the target devices's CS line.
		 * In this implementation this triggers an EXTI callback,
		 * which in turn calls spi_io_receive() on the target SPI device.
		 */
		spid->target_device = target_device;
		serial_print_line("Selecting Target SPI Device.", 0);
		// making sure that the CS line is low first
		HAL_GPIO_WritePin(spid->target_device->cs_port_out, target_device->cs_pin_out, GPIO_PIN_SET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(spid->target_device->cs_port_out, target_device->cs_pin_out, GPIO_PIN_RESET);
	}

	HAL_SPI_Transmit_IT(spid->handle, (uint8_t *)&spid->tx_buff.header,
			sizeof(SPIHeader_t));

	return true;
}

bool spi_io_receive(SPIDevice_t *spid)
{
	if (spid->op & SPIOP_RX)
	{
		/*
		serial_print("Device ", 0);
		serial_print(spid->name, 0);
		serial_print_line(" RX line busy, aborting reception.", 0);
		*/
		return false;
	}

	spid->state |= SPISTATE_RX_PENDING;
	spid->op |= SPIOP_RX;

	spid->rx_pos = 0;

	HAL_SPI_Receive_IT(spid->handle, (uint8_t *)&spid->rx_buff, sizeof(SPIHeader_t));

	return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// TODO: also check the EXTI line (this case will work regardless, but it's a good habit)
	if (GPIO_Pin == SPI3_CS_IN_Pin
	 || GPIO_Pin == SPI5_CS_IN_Pin)
	{
		SPIDevice_t *spid = hspi_to_struct(
				GPIO_Pin == SPI3_CS_IN_Pin ? &hspi3 : &hspi5);

		if (spid == NULL) return;

		// falling edge - selected
		if (HAL_GPIO_ReadPin(spid->cs_port_in, spid->cs_pin_in) == GPIO_PIN_RESET)
		{
			spid->state |= SPISTATE_SELECTED;

			if (spid->op == SPIOP_NONE)
			{
				spi_io_receive(spid);
			}
		}
		// rising edge - deselected
		else
		{
			spid->state &= ~SPISTATE_SELECTED;
		}
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if (!is_initialized) return;

	SPIDevice_t *spid = hspi_to_struct(hspi);
	spid->state |= SPISTATE_ERROR;
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (!is_initialized) return;

	SPIDevice_t *spid = hspi_to_struct(hspi);
	spid->state |= SPISTATE_ABORT;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	SPIDevice_t *spid = hspi_to_struct(hspi);

	if (spid->tx_pos == 0)
	{
		spid->tx_pos = 1;
		HAL_SPI_Transmit_IT(spid->handle, (uint8_t *)spid->tx_buff.data,
				spid->tx_buff.header.tx_len);
	}
	else
	{
		// If this is a Controller callback,
		// deselect the Target device.
		// TODO: check here if we sent an Rx request,
		//       in which case, we immediately transition to Rx mode
		if (spid->target_device != NULL)
		{
			HAL_GPIO_WritePin(spid->target_device->cs_port_out,
				spid->target_device->cs_pin_out, GPIO_PIN_SET);
			spid->target_device = NULL;
		}

		HAL_Delay(2);

		spid->state |= SPISTATE_TX_CPLT;
		spid->op &= ~SPIOP_TX;
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	SPIDevice_t *spid = hspi_to_struct(hspi);

	if (spid->rx_pos == 0)
	{
		spid->rx_pos = 1;
		HAL_SPI_Receive_IT(spid->handle,
			(uint8_t *)spid->rx_buff.data,
			spid->rx_buff.header.tx_len);
	}
	else
	{
		spi_io_process_rx(spid);
	}

}
