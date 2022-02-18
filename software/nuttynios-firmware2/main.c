#include "system.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "altera_avalon_pio_regs.h"
#include <stdlib.h>

//#define OFFSET -32
//#define PWM_PERIOD 16

//alt_8 pwm = 0;
//alt_u8 led;
//int level;
//
//void led_write(alt_u8 led_pattern) {
//    IOWR(LED_BASE, 0, led_pattern);
//}
//
//void convert_read(alt_32 acc_read, int * level, alt_u8 * led) {
//    acc_read += OFFSET;
//    alt_u8 val = (acc_read >> 6) & 0x07;
//    * led = (8 >> val) | (8 << (8 - val));
//    * level = (acc_read >> 1) & 0x1f;
//}

//void sys_timer_isr() {
//    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
//
//    if (pwm < abs(level)) {
//
//        if (level < 0) {
//            led_write(led << 1);
//        } else {
//            led_write(led >> 1);
//        }
//
//    } else {
//        led_write(led);
//    }
//
//    if (pwm > PWM_PERIOD) {
//        pwm = 0;
//    } else {
//        pwm++;
//    }
//
//}

//void timer_init(void * isr) {
//
//    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x0003);
//    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
//    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_BASE, 0x0900);
//    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_BASE, 0x0000);
//    alt_irq_register(TIMER_IRQ, 0, isr);
//    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x0007);
//
//}

int main() {

	int button, switches;
	int leds = 0b1010101010;
	int hex0 = 0b10001100; // P
	int hex1 = 0b11000001; // U

    alt_32 x_read;
    alt_up_accelerometer_spi_dev *acc_dev;
    acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
    if (acc_dev == NULL) { // if return 1, check if the spi ip name is "accelerometer_spi"
        return 1;
    }

    while (1) {
    	button = IORD_ALTERA_AVALON_PIO_DATA(BUTTON_BASE);
    	switches = IORD_ALTERA_AVALON_PIO_DATA(SWITCH_BASE);
    	IOWR_ALTERA_AVALON_PIO_DATA(LED_BASE, leds);
    	IOWR_ALTERA_AVALON_PIO_DATA(HEX0_BASE, hex0);
    	IOWR_ALTERA_AVALON_PIO_DATA(HEX1_BASE, hex1);

        alt_up_accelerometer_spi_read_x_axis(acc_dev, &x_read);
        alt_printf("raw data: %x button: %x switches: %x\n", x_read, button, switches);
    }

    return 0;
}
