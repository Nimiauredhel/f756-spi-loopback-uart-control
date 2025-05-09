/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "interface.h"

static void loopback_test_routine(SPI_HandleTypeDef *hspi_cnt, SPI_HandleTypeDef *hspi_tgt)
{
	// acquire data struct for the chosen controller & target spi devices
	SPIDevice_t *cnt_dev = hspi_to_struct(hspi_cnt);
	SPIDevice_t *tgt_dev = hspi_to_struct(hspi_tgt);

	// clear the devices' tx/rx buffers
	bzero(cnt_dev->tx_buff, SPI_BUFF_SIZE);
	bzero(cnt_dev->rx_buff, SPI_BUFF_SIZE);
	bzero(tgt_dev->tx_buff, SPI_BUFF_SIZE);
	bzero(tgt_dev->rx_buff, SPI_BUFF_SIZE);

	/**
	 * Prompt user and accept input test message.
	 * The user input is stored in the controller device's TX buffer.
	 */
	serial_print("Input test message: ", 0);
	serial_scan(cnt_dev->tx_buff, SPI_BUFF_SIZE);
	serial_print_line(NULL, 0); // just a newline

	/**
	 * Enabling the target devices's CS line.
	 * In this implementation, this triggers an EXTI callback
	 * which in turn calls HAL_SPI_Receive_IT() on the target device.
	 */
	serial_print_line("Selecting Target SPI Device.", 0);
	HAL_GPIO_WritePin(tgt_dev->cs_port_out, tgt_dev->cs_pin_out, GPIO_PIN_RESET);

	serial_print("Sending message: ", 0);
	serial_print_line(cnt_dev->tx_buff, 0);
	HAL_SPI_Transmit_IT(cnt_dev->handle, cnt_dev->tx_buff, strlen(cnt_dev->tx_buff));

	serial_print_line("Deselecting Target SPI Device.", 0);
	HAL_GPIO_WritePin(tgt_dev->cs_port_out, tgt_dev->cs_pin_out, GPIO_PIN_SET);

	serial_print("Received message: ", 0);
	serial_print_line(rx_buff, 0);
	serial_print_line("Loopback test concluded.", 0);
}

void interface_loop(void)
{
	char buff[8] = {0};

	HAL_Delay(1);

	serial_print_line("-\r\nPlease select a test routine from the list:", 0);
	serial_print_line("1: SPI Loopback Test (SPI1 <--> SPI3)", 0);
	serial_print_line("2: SPI Loopback Test (SPI1 <--> SPI5)", 0);
	serial_print_line("-", 1);

	bzero(buff, sizeof(buff));
	serial_scan(buff, 1);

	serial_print_line("\r\n--", 4);

	switch(buff[0])
	{
	case '1':
		loopback_test_routine(&hspi1, &hspi3);
		break;
	case '2':
		loopback_test_routine(&hspi1, &hspi5);
		break;
	default:
	serial_print_line("Invalid selection.", 0);
		break;
	}

	serial_print_line("--\r\n", 4);
}
