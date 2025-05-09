/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "interface.h"

inline static void clear_spi_states(volatile SPIDeviceState_t *cnt_state_ptr, volatile SPIDeviceState_t *tgt_state_ptr)
{
	*cnt_state_ptr = SPISTATE_PENDING;
	*tgt_state_ptr = SPISTATE_PENDING;
}

inline static void diff_spi_state_change(SPIDeviceState_t *old_state, SPIDeviceState_t *new_state, char *device_name)
{
	SPIDeviceState_t delta = *new_state ^ *old_state;

	// if delta is zero then no changes have been detected and no action is necessary
	// TODO: maybe also check if change is negative or positive, since both yield the same XOR result
	if (delta != 0x00)
	{
		// operation error
		if (delta & SPISTATE_ERROR)
		{
			serial_print(device_name, 0);
			serial_print(" reports error during operation: ", 0);

			if (*new_state & SPISTATE_TX_PENDING)
			{
				serial_print_line("Transmit.", 0);
			}
			else if (*new_state & SPISTATE_RX_PENDING)
			{
				serial_print_line("Receive.", 0);
			}
			else if (*new_state & SPISTATE_TX_RX_PENDING)
			{
				serial_print_line("Transmit and Receive.", 0);
			}
			else
			{
				serial_print_line("Unknown.", 0);
			}
		}

		// operation aborted
		if (delta & SPISTATE_ABORT)
		{
			serial_print(device_name, 0);
			serial_print(" aborted operation: ", 0);

			if (*new_state & SPISTATE_TX_PENDING)
			{
				serial_print_line("Transmit.", 0);
			}
			else if (*new_state & SPISTATE_RX_PENDING)
			{
				serial_print_line("Receive.", 0);
			}
			else if (*new_state & SPISTATE_TX_RX_PENDING)
			{
				serial_print_line("Transmit and Receive.", 0);
			}
			else
			{
				serial_print_line("Unknown.", 0);
			}
		}

		// operation completed
		if (delta & SPISTATE_CPLT)
		{
			serial_print(device_name, 0);
			serial_print(" completed operation: ", 0);

			if (*new_state & SPISTATE_TX_PENDING)
			{
				serial_print_line("Transmit.", 0);
			}
			else if (*new_state & SPISTATE_RX_PENDING)
			{
				serial_print_line("Receive.", 0);
			}
			else if (*new_state & SPISTATE_TX_RX_PENDING)
			{
				serial_print_line("Transmit and Receive.", 0);
			}
			else
			{
				serial_print_line("Unknown.", 0);
			}
		}

		// update old state to the new
		*old_state = *new_state;
	}
}

inline static void monitor_spi_operation(SPIDevice_t *cnt_device_ptr, SPIDevice_t *tgt_device_ptr)
{
	SPIDeviceState_t cnt_prev_state = 0x00;
	SPIDeviceState_t tgt_prev_state = 0x00;
	SPIDeviceState_t cnt_curr_state = 0x00;
	SPIDeviceState_t tgt_curr_state = 0x00;

	do
	{
		HAL_Delay(10);
		cnt_curr_state = cnt_device_ptr->state;
		tgt_curr_state = tgt_device_ptr->state;
		diff_spi_state_change(&cnt_prev_state, &cnt_curr_state, cnt_device_ptr->name);
		diff_spi_state_change(&tgt_prev_state, &tgt_curr_state, tgt_device_ptr->name);
	} while ((cnt_device_ptr->op + tgt_device_ptr->op) > SPIOP_NONE);
}

inline static void loopback_test_routine(SPI_HandleTypeDef *hspi_cnt, SPI_HandleTypeDef *hspi_tgt)
{
	char test_buff[64] = {0};

	// acquire data struct for the chosen controller & target spi devices
	SPIDevice_t *cnt_dev = hspi_to_struct(hspi_cnt);
	SPIDevice_t *tgt_dev = hspi_to_struct(hspi_tgt);

	// clear the devices' state monitoring flags
	clear_spi_states(&cnt_dev->state, &tgt_dev->state);

	/**
	 * Prompt user and accept input test message.
	 * The user input is stored in the controller device's TX buffer.
	 * The allowed length is the buffer size minus one, to ensure a terminator.
	 */
	serial_print("Input test message: ", 0);
	serial_scan(test_buff, sizeof(test_buff)-1);
	serial_print_line(NULL, 0); // just a newline

	/**
	 * Enabling the target devices's CS line.
	 * In this implementation this triggers an EXTI callback,
	 * which in turn calls spi_io_receive() on the target SPI device.
	 */
	serial_print_line("Selecting Target SPI Device.", 0);
	HAL_GPIO_WritePin(tgt_dev->cs_port_out, tgt_dev->cs_pin_out, GPIO_PIN_RESET);
	HAL_Delay(10);

	/**
	 * Transmitting the input entered by the user.
	 * The TX length is strlen+1 to always include a terminator,
	 * allowing the target to dynamically detect end of transmission.
	 */
	serial_print("Sending message: ", 0);
	serial_print_line(test_buff, 0);
	spi_io_transmit(hspi_cnt, (uint8_t *)test_buff, 1+strlen(test_buff));

	monitor_spi_operation(cnt_dev, tgt_dev);

	serial_print("Received message: ", 0);
	serial_print_line((char *)tgt_dev->rx_buff, 0);

	serial_print_line("Deselecting Target SPI Device.", 0);
	HAL_GPIO_WritePin(tgt_dev->cs_port_out, tgt_dev->cs_pin_out, GPIO_PIN_SET);
	HAL_Delay(10);

	serial_print_line("Loopback test concluded.", 0);
}

void interface_loop(void)
{
	char buff[8] = {0};

	HAL_Delay(1);

	if (!spi_io_is_initialized())
	{
		serial_print_line("Fresh boot! Welcome.", 0);
		serial_print_line("Initializing SPI I/O utils.", 0);
		spi_io_initialize();

		if (spi_io_is_initialized())
		{
			serial_print_line("SPI I/O utils initialized.", 0);
		}

		return;
	}

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
