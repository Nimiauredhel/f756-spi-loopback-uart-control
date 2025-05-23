/*
 * uart_io.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef UART_IO_H_
#define UART_IO_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "main.h"

#define ASCII_PRINTABLE ' ', '~'
#define ASCII_NUMERIC '0', '9'

extern UART_HandleTypeDef huart3;

void serial_print(const char *msg, uint16_t len);
void serial_print_line(const char *msg, uint16_t len);
void serial_print_char(const char c);
uint8_t serial_scan(char *buffer, const uint8_t max_len, const char min, const char max);

#endif /* UART_IO_H_ */
