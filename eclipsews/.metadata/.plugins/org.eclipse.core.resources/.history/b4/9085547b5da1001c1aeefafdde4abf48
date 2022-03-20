/*
 * utils.h
 *
 *  Created on: Feb 19, 2022
 *      Author: Derek Lai
 */

#include "system.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "altera_avalon_pio_regs.h"

#ifndef UTILS_H_
#define UTILS_H_

void write_leds(int leds){
	IOWR_ALTERA_AVALON_PIO_DATA(LED_BASE, leds);
}

void write_hex(int hex[6]) {
	IOWR_ALTERA_AVALON_PIO_DATA(HEX0_BASE, hex[0]);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX1_BASE, hex[1]);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX2_BASE, hex[2]);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX3_BASE, hex[3]);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX4_BASE, hex[4]);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX5_BASE, hex[5]);
}

int read_switches() {
	return IORD_ALTERA_AVALON_PIO_DATA(SWITCH_BASE);
}

int read_buttons() {
	return IORD_ALTERA_AVALON_PIO_DATA(BUTTON_BASE);
}

void read_accerometer(alt_up_accelerometer_spi_dev *acc_dev, alt_32 coords[3]) {
	alt_up_accelerometer_spi_read_x_axis(acc_dev, &coords[0]);
	alt_up_accelerometer_spi_read_y_axis(acc_dev, &coords[1]);
	alt_up_accelerometer_spi_read_z_axis(acc_dev, &coords[2]);
}

#endif /* UTILS_H_ */
