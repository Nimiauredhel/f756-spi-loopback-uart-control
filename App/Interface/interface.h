/*
 * door_interface.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"

#include "uart_io.h"
#include "spi_io.h"

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi5;

void interface_loop(void);

#endif /* INTERFACE_H_ */
