/*
 * uart_io.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "uart_io.h"

#define UART_PEER huart3

static void serial_backspace_destructive(uint16_t count)
{
	static const uint8_t* backspace = (uint8_t *)"\b \b";
	static const uint8_t len = 3;

	for (uint16_t idx = 0; idx < count; idx++)
	{
		HAL_UART_Transmit(&UART_PEER, backspace, len, HAL_MAX_DELAY);
	}
}

static void serial_newline(void)
{
	static const uint8_t newline[2] = {'\r', '\n'};
	static const uint8_t len = 2;

	HAL_UART_Transmit(&UART_PEER, newline, len, HAL_MAX_DELAY);
}

void serial_print(const char *msg, uint16_t len)
{
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&UART_PEER, (uint8_t *)msg, len, HAL_MAX_DELAY);
}

void serial_print_line(const char *msg, uint16_t len)
{
	// a NULL message is valid as a request to just print a newline
	if (msg != NULL)
	{
		if (len == 0) len = strlen(msg);
		HAL_UART_Transmit(&UART_PEER, (uint8_t *)msg, len, HAL_MAX_DELAY);
	}

	serial_newline();
}

void serial_print_char(const char c)
{
	HAL_UART_Transmit(&UART_PEER, (uint8_t *)&c, 1, HAL_MAX_DELAY);
}

uint8_t serial_scan(char *buffer, const uint8_t max_len)
{
	uint8_t inchar = ' ';
	uint8_t input_idx = 0;

	bzero(buffer, max_len);

	for(;;)
	{
		HAL_Delay(1);

		if (HAL_OK == HAL_UART_Receive(&UART_PEER, &inchar, 1, 0x0))
		{
			switch (inchar)
			{
			case '\b':
				if (input_idx > 0)
				{
					buffer[input_idx] = '\0';
					serial_backspace_destructive(1);
					input_idx--;
				}
				continue;
			case '\n':
			case '\r':
				buffer[input_idx] = '\0';
				serial_newline();
				return input_idx+1;
			default:
				if (input_idx >= max_len)
				{
					continue;
				}
				buffer[input_idx] = inchar;
				serial_print_char(inchar);
				input_idx++;
			}
		}
	}
}
