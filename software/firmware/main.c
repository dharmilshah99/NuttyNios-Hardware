#include "system.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "sys/alt_timestamp.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_pio_regs.h"

#include "alt_types.h"
#include "sys/alt_stdio.h"
#include "sys/alt_irq.h"
#include "altera_avalon_jtag_uart.h"
#include "io.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "utils.h"

#define END_RECIEVE '>'

// GLOBALS

int buttons, switches;

alt_32 coords[3]; // contains x, y, z data
alt_32 filtered_coords[3]; // contains filtered data

int coord_data_ready;
alt_up_accelerometer_spi_dev *acc_dev;

// // performance testing
//alt_u32 time1, time2;
//alt_u32 times[1000];
//int timeindex;

float filter_coeff_float[25] = {
		-0.007515373920286,
		-0.010763509959807,
		0.010049798773992,
		0.001844805593684,
		-0.019807844578279,
		0.022797855207093,
		0.004384990844024,
		-0.046054135205711,
		0.056124756815064,
		0.006802030909455,
		-0.136368003901848,
		0.26923938202295,
		0.674466495032901,
		0.26923938202295,
		-0.136368003901848,
		0.006802030909455,
		0.056124756815064,
		-0.046054135205711,
		0.004384990844024,
		0.022797855207093,
		-0.019807844578279,
		0.001844805593684,
		0.010049798773992,
		-0.010763509959807,
		-0.007515373920286
};

alt_u32 filter_coeff_fixed[25];

alt_32 fir_mem_fixed_x[25] = {0};
alt_32 fir_mem_fixed_y[25] = {0};
alt_32 fir_mem_fixed_z[25] = {0};

alt_32 float_to_fixed23(float f) {
	alt_u32 f_bits = *(alt_u32*)(&f);
	alt_u32 sign = f_bits & (1 << 31); // gets sign bit
	alt_32 exp = ((f_bits >> 23) & (0xFF)) - 127; // exponent
	alt_u32 fraction = f_bits & (0x007FFFFF); // fraction bit mask
	fraction = fraction | (1 << 23); // put back implicit 1 in float
	alt_u32 fixed = (exp < 0) ? fraction >> -exp : fraction << exp;
	alt_32 signed_fixed = sign ? -fixed : fixed;
	return signed_fixed;
}

float fixed23_to_float(alt_32 fixed){
	return (float)fixed / (float)(1<<23);
}

void fir_filter_fixed(alt_32 memory[25], alt_32 data, alt_32 *average){
	memory[0] = data;

	alt_32 acc = 0;
	for(int i = 0; i < 25; i++){
		acc += filter_coeff_fixed[i] * memory[i];
	}
	*average = acc;

	for(int i = 1; i < 25; i++){
		memory[i] = memory[i-1];
	}
}

void generate_fixed(){
	for(int i = 0; i < 25; i++){
		filter_coeff_fixed[i] = float_to_fixed23(filter_coeff_float[i]);
	}
}

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

void get_accler_isr() {
	// reset timer
	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER0_BASE, 0);

	read_accerometer(acc_dev, coords);
	coord_data_ready = 1;
}

void timer_init(void *isr) {
	// timer0: 20000 ticks per sec, T = 50us.

    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER0_BASE, 0x0003);
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER0_BASE, 0);
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER0_BASE, 0xC4F0); // ~1000Hz Sampling
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER0_BASE, 0x0000);
    alt_irq_register(TIMER0_IRQ, 0, isr);
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER0_BASE, 0x0007);

}

char state = 0;
void getState(){
	state = getchar();
}



int main() {

	// make fixed point filter coeff.
	generate_fixed();

	acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
    if (acc_dev == NULL) { // if return 1, check if the spi ip name is "accelerometer_spi"
    	alt_printf("[ERROR] ACCELEROMETER MISSING\n");
    	return 1;
    }
    timer_init(get_accler_isr);

    if(alt_timestamp_start() < 0) {
    	alt_printf("[ERROR] NO TIMESTAMP TIMER\n");
    }

//    unsigned char word[40];
//	uart_recieve(word);
//	alt_printf("Putting \"%s\" onto HEX panel.\n", word);

    // k, m, v, w, x, z - are not in HEX table
//	int delay = 0;
//    unsigned char display_buff[40];
//    unsigned char window[6];
//
//    setBuffer(word, display_buff);


	// infinite polling loop
    while (1) {

    	// only output if there is new sample
    	if(coord_data_ready){

    		// accelerometer input
    		fir_filter_fixed(fir_mem_fixed_x, coords[0], filtered_coords);
    		fir_filter_fixed(fir_mem_fixed_y, coords[1], filtered_coords+1);
    		fir_filter_fixed(fir_mem_fixed_z, coords[2], filtered_coords+2);

    		// button & switch input
			buttons = read_buttons();
			switches = read_switches();

    		alt_printf("%x,%x,%x,%x,%x\n", \
    				filtered_coords[0], filtered_coords[1], filtered_coords[2], \
					buttons, switches);
    		fflush(stdout);

    		coord_data_ready = 0;
    	}


		// update Window
//		char debug_display[41];
//		char debug_window[7];
//		memcpy(debug_display, display_buff, 40);
//		memcpy(debug_window, window, 6);
//		debug_display[40] = '\0';
//		debug_window[6] = '\0';

//		alt_printf("%s\n", debug_display);
//		alt_printf("%s\n", debug_window);

//		if(delay == 0){
//			getWindow(window, display_buff);
//			for(int j = 0; j < 6; j++){
//				window[j] = decode_7seg(window[j]);
//			}
//			write_hex(window);
//			shiftBuffer(display_buff);
//			delay = 10;
//		}
//		delay--;
//
//		// state
//		getState();
//		alt_printf("State: %c\n", state);

//		if(bool){
//			write_leds(0b1111111111);
//		} else {
//			write_leds(0b0000000000);
//		}

    	// write_leds(leds);
    }

    // averaging time
//    int totaltime = 0;
//    for(int i = 0; i < 1000; i++){
//    	totaltime += times[i];
//    }
//
//    int totaltime_S = totaltime / 50000000;
//    int totaltime_uS = totaltime / 50;
//    int averagetime_uS = totaltime / (50*1000);
//    int averagetime_nS = totaltime / (50);
//
//    alt_printf("\n\nTotal Running Time (HEX): 0x%x uS\n", totaltime_uS);
//    alt_printf("Average Time (HEX): 0x%x uS\n", averagetime_uS);
//    alt_printf("Sampling Rate (HEX): 0x%x Hz\n", 1000000/averagetime_uS);

    return 0;
}
