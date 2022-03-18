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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>

#include "utils.h"

#define END_RECIEVE '$'
#define HEX_DELAY 40
#define FIR_FILTER_COEFF_NUM 25

// GLOBALS
int filter = 0; // 0 easy, 1 hard
int state = 0;

int buttons, switches;
unsigned int leds = 0;


alt_32 coords[3]; // contains x, y, z data
alt_32 filtered_coords[3]; // contains filtered data

int coord_data_ready;
alt_up_accelerometer_spi_dev *acc_dev;

int delay = 0;

unsigned char word[25];
unsigned char display_buff[25];
unsigned char window[6];

// // performance testing
//alt_u32 time1, time2;
//alt_u32 times[1000];
//int timeindex;

float easy_filter_coeff_float[FIR_FILTER_COEFF_NUM] = {
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

float hard_filter_coeff_float[FIR_FILTER_COEFF_NUM] = {
	-0.0004677231861,
	-0.000173866359285,
	0.000937990028395,
	0.003858075231638,
	0.009487846399465,
	0.018528590955764,
	0.031186479169928,
	0.04693810067403,
	0.064448245815707,
	0.08170464084224,
	0.096360238680506,
	0.106209378953379,
	0.109681990496945,
	0.106209378953379,
	0.096360238680506,
	0.08170464084224,
	0.064448245815707,
	0.04693810067403,
	0.031186479169928,
	0.018528590955764,
	0.009487846399465,
	0.003858075231638,
	0.000937990028395,
	-0.173866359284789,
	-0.467723186100422
};

alt_u32 easy_filter_coeff_fixed[FIR_FILTER_COEFF_NUM];
alt_u32 hard_filter_coeff_fixed[FIR_FILTER_COEFF_NUM];

alt_32 fir_mem_fixed_x[FIR_FILTER_COEFF_NUM] = {0};
alt_32 fir_mem_fixed_y[FIR_FILTER_COEFF_NUM] = {0};
alt_32 fir_mem_fixed_z[FIR_FILTER_COEFF_NUM] = {0};

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


void fir_filter_fixed(alt_32 memory[FIR_FILTER_COEFF_NUM], alt_32 data, alt_32 *average){
	memory[0] = data;

	alt_32 acc = 0;
	for(int i = 0; i < FIR_FILTER_COEFF_NUM; i++){
		if(filter == 0){ // easy
			acc += easy_filter_coeff_fixed[i] * memory[i];
		} else { // hard mode
			acc += hard_filter_coeff_fixed[i] * memory[i];
		}
	}
	*average = acc;

	for(int i = 1; i < FIR_FILTER_COEFF_NUM; i++){
		memory[i] = memory[i-1];
	}
}

void generate_fixed(){
	for(int i = 0; i < FIR_FILTER_COEFF_NUM; i++){
		easy_filter_coeff_fixed[i] = float_to_fixed23(easy_filter_coeff_float[i]);
		hard_filter_coeff_fixed[i] = float_to_fixed23(hard_filter_coeff_float[i]);
	}
}

void update_hex(){
	if(delay == 0){
		getWindow(window, display_buff);

		for(int j = 0; j < 6; j++){
			window[j] = decode_7seg(window[j]);
		}
		write_hex(window);
		shiftBuffer(display_buff);
		delay = HEX_DELAY;
	}
	delay--;
}

void update_leds(){
	if(delay == 0){
		leds |= (leds & 0x1) << 10;
		leds >>= 1;
		write_leds(leds);
	}
}

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


int main() {

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

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

	char message[1024];

	char messagebuff[1024];
	int messagebuffind = 0;

	int message_is_ready = 0;
	int curr_message_size;

	char stdin_buff[256];

	int size;
	while(1) {
		// clear
		memset(stdin_buff, 0, 256);
		size = read(STDIN_FILENO, stdin_buff, 256);

		if(size != -1){	
			for(int i = 0; i < size; i++){
				// non-terminating character
				if(stdin_buff[i] != END_RECIEVE){
					messagebuff[messagebuffind] = stdin_buff[i];
					messagebuffind++;
				}
				// character is terminating
				else {
					memcpy(message, messagebuff, messagebuffind);
					message[messagebuffind] = '\0';
					message_is_ready = 1;
					curr_message_size = messagebuffind;

					// reset message buffer
					messagebuffind = 0;
					memset(messagebuff, 0, 256);
				}
			}
		}
		

		// handle input
		if(message_is_ready){
			
			// led on
			if(message[0] == '1'){
				leds = 0b1111111111;
			} 
			
			// led off
			else if(message[0] == '0') {
				leds = 0b0000000000;
			} 
			
			// score
			else if(message[0] == '~') {
				memcpy(word, message+1, curr_message_size-1);
				setBuffer(word, display_buff);
			}
			
			// easy filter
			else if(message[0] == '-'){
				char str[25] = "Easy filter";
				memcpy(word, str, 25);
				setBuffer(word, display_buff);
				filter = 0;
			}

			// hard filter
			else if(message[0] == '+'){
				char str[25] = "Hard filter";
				memcpy(word, str, 25);
				setBuffer(word, display_buff);
				filter = 1;
			}
			
			// any other string
			else {
				memcpy(word, message, 25);
				setBuffer(word, display_buff);
			}
			message_is_ready = 0;
		}

		// only output if there is new sample
		if(coord_data_ready){

			// accelerometer input
			fir_filter_fixed(fir_mem_fixed_x, coords[0], filtered_coords);
			fir_filter_fixed(fir_mem_fixed_y, coords[1], filtered_coords+1);
			fir_filter_fixed(fir_mem_fixed_z, coords[2], filtered_coords+2);

			// button & switch input
			buttons = read_buttons();
			switches = read_switches();

			alt_printf("%x,%x,%x,%x,%x\n",
					filtered_coords[0], filtered_coords[1], filtered_coords[2],
						buttons, switches);
			fflush(stdout);

			update_hex();
			update_leds();

			coord_data_ready = 0;
		}

	}


    return 0;
}
