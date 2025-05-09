/*
 * spi_io.c
 *
 *  Created on: May 9, 2025
 *      Author: mickey
 */

#include "spi_io.h"

static SPIDevice_t devices[3] =
{
		{ 0, 0, 0, 0, &hspi1, 0x0 },
		{ SPI3_CS_IN_Pin, SPI3_CS_OUT_Pin, SPI3_CS_IN_GPIO_Port, SPI3_CS_OUT_GPIO_Port, &hspi3, 0x0 },
		{ SPI5_CS_IN_Pin, SPI5_CS_OUT_Pin, SPI5_CS_IN_GPIO_Port, SPI5_CS_OUT_GPIO_Port, &hspi5, 0x0 },
};

SPIDevice_t* hspi_to_struct(SPI_HandleTypeDef *hspi)
{
	switch(hspi)
	{
	case &hspi1:
		break;
	case &hspi3:
		break;
	case &hspi5:
		break;
	default:
		return null;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == SPI3_CS_IN_Pin
	 || GPIO_Pin == SPI5_CS_IN_Pin)
	{
		SPIDevice_t *spid = hspi_to_struct(
				GPIO_Pin == SPI3_CS_IN_Pin ? &hspi3 : &hspi5);

		// falling edge - selected
		if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(spid->cs_port_in, spid->cs_pin_in))
		{
			spid->state |= SPISTATE_SELECTED;
			HAL_SPI_Receive_IT(spid->handle, spid->rx_buff, SPI_BUFF_SIZE);
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
	SPIDevice_t *spid = hspi_to_struct(hspi);
	spid->state |= SPISTATE_ERROR;
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
	SPIDevice_t *spid = hspi_to_struct(hspi);
	spid->state |= SPISTATE_ABORT;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	SPIDevice_t *spid = hspi_to_struct(hspi);
	spid->state |= SPISTATE_TX_CPLT;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	SPIDevice_t *spid = hspi_to_struct(hspi);
	spid->state |= SPISTATE_RX_CPLT;
}
