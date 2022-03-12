#include "system.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "altera_avalon_pio_regs.h"

#include "alt_types.h"
#include "sys/alt_irq.h"
#include "altera_avalon_jtag_uart.h"
#include "io.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"

#define END_RECIEVE '>'

void uart_recieve(unsigned char* buffer){
	FILE* fp;
	int i = 0;
	char c = 0;
	fp = fopen("/dev/jtag_uart", "r");
	if(fp){
		while(c != '>'){
			c = getc(fp);
			buffer[i] = c;
			i++;
		}
		fclose(fp);
		alt_printf("[DEBUG] CLOSED FP\n");
	}
}

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

void setBuffer(unsigned char* word, unsigned char buffer[40]){
	memset(buffer, 255, 40);
	memcpy(buffer, word, strlen(word));
}

void shiftBuffer(unsigned char buffer[40]){
	int tmp = buffer[0];
	for(int i = 1; i < 40; i++){
		buffer[i-1] = buffer[i];
	}
	buffer[39] = tmp;
}

void getWindow(unsigned char window[6], unsigned char buffer[40]){
	memcpy(window, buffer, 6);
}


char state = 0;
void getState(){
	state = getchar();
}



int main() {

	int buttons, switches;

    alt_32 coords[3]; // contains x, y, z data
	alt_up_accelerometer_spi_dev *acc_dev;
	acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");

    if (acc_dev == NULL) { // if return 1, check if the spi ip name is "accelerometer_spi"
    	alt_printf("[ERROR] ACCELEROMETER MISSING\n");
    	return 1;
    }

    unsigned char word[40];
	uart_recieve(word);
	alt_printf("Putting \"%s\" onto HEX panel.\n", word);

    // k, m, v, w, x, z - are not in HEX table
	int delay = 0;
    unsigned char display_buff[40];
    unsigned char window[6];

    setBuffer(word, display_buff);



	// infinite polling loop
    while (1) {

    	// button & switch input
    	buttons = read_buttons();
    	switches = read_switches();

    	// accelerometer
    	read_accerometer(acc_dev, coords);
		alt_printf("%x,%x,%x,%x,%x\n", coords[0], coords[1], coords[2], buttons, switches);

		// update Window
//		char debug_display[41];
//		char debug_window[7];
//		memcpy(debug_display, display_buff, 40);
//		memcpy(debug_window, window, 6);
//		debug_display[40] = '\0';
//		debug_window[6] = '\0';

//		alt_printf("%s\n", debug_display);
//		alt_printf("%s\n", debug_window);

		if(delay == 0){
			getWindow(window, display_buff);
			for(int j = 0; j < 6; j++){
				window[j] = decode_7seg(window[j]);
			}
			write_hex(window);
			shiftBuffer(display_buff);
			delay = 10;
		}
		delay--;

		// state
		getState();
		alt_printf("State: %c\n", state);

//		if(bool){
//			write_leds(0b1111111111);
//		} else {
//			write_leds(0b0000000000);
//		}

    	// write_leds(leds);
    }

    return 0;
}
