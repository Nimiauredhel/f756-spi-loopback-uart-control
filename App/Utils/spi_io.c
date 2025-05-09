/*
 * spi_io.c
 *
 *  Created on: May 9, 2025
 *      Author: mickey
 */

#include "spi_io.h"

static bool is_initialized = false;
static SPIDevice_t devices[3] = {0};

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
	if (!is_initialized) return NULL;

	if (hspi == &hspi1) return devices+0;
	if (hspi == &hspi3) return devices+1;
	if (hspi == &hspi5) return devices+2;

	return NULL;
}

bool spi_io_transmit(SPI_HandleTypeDef *hspi, uint8_t *data, uint8_t len)
{
	if (len < 1) return false;

	SPIDevice_t *spid = hspi_to_struct(hspi);

	if (spid == NULL) return false;

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

	if (len > SPI_BUFF_SIZE) len = SPI_BUFF_SIZE;

	spid->tx_pos = 0;
	spid->tx_len = len;

	bzero(spid->tx_buff, SPI_BUFF_SIZE);
	memcpy(spid->tx_buff, data, len);
	HAL_SPI_Transmit_IT(spid->handle, spid->tx_buff, 1);

	return true;
}

bool spi_io_receive(SPI_HandleTypeDef *hspi, uint8_t max_len)
{
	if (max_len < 1) return false;

	SPIDevice_t *spid = hspi_to_struct(hspi);

	if (spid == NULL) return false;

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

	if (max_len > SPI_BUFF_SIZE) max_len = SPI_BUFF_SIZE;

	spid->rx_pos = 0;
	spid->rx_len = max_len;

	bzero(spid->rx_buff, SPI_BUFF_SIZE);
	HAL_SPI_Receive_IT(spid->handle, spid->rx_buff, 1);

	return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// TODO: also check the EXTI line (this case will work regardless, but it's a good habit)
	if (GPIO_Pin == SPI3_CS_IN_Pin
	 || GPIO_Pin == SPI5_CS_IN_Pin)
	{
		if (!is_initialized) return;

		SPIDevice_t *spid = hspi_to_struct(
				GPIO_Pin == SPI3_CS_IN_Pin ? &hspi3 : &hspi5);

		if (spid == NULL) return;

		GPIO_PinState edge = HAL_GPIO_ReadPin(spid->cs_port_in, spid->cs_pin_in);

		// falling edge - selected
		if (edge == GPIO_PIN_RESET)
		{
			spid->state |= SPISTATE_SELECTED;
			spi_io_receive(spid->handle, SPI_BUFF_SIZE);
		}
		// rising edge - deselected
		else
		{
			spid->state &= ~SPISTATE_SELECTED;
		}
	}
}

void HAL_SPI_ErrorCpltCallback(SPI_HandleTypeDef *hspi)
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
	if (!is_initialized) return;

	SPIDevice_t *spid = hspi_to_struct(hspi);


	if (spid->tx_pos+1 >= spid->tx_len)
	{
		spid->state |= SPISTATE_TX_CPLT;
		spid->op &= ~SPIOP_TX;
	}
	else
	{
		spid->tx_pos++;
		HAL_SPI_Transmit_IT(spid->handle, spid->tx_buff+spid->tx_pos, 1);
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (!is_initialized) return;

	SPIDevice_t *spid = hspi_to_struct(hspi);

	if (spid->rx_pos+1 >= spid->rx_len
		|| spid->rx_buff[spid->rx_pos] == '\0')
	{
		spid->state |= SPISTATE_RX_CPLT;
		spid->op &= ~SPIOP_RX;
	}
	else
	{
		spid->rx_pos++;
		HAL_SPI_Receive_IT(spid->handle, spid->rx_buff+spid->rx_pos, 1);
	}
}
