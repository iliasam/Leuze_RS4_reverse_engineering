#include "measure_dist.h"
#include "tdc_functions.h"
#include "apd_functions.h"
#include <reg167.h>               /* special function register 80C167  */
#include "corr_table_file.h"

unsigned short APD_amplitude;

unsigned short cur_dist_value;

extern unsigned short FIFO0;

#define CORRECTION_TABLE_LENGTH 820
#define CORRECTION_TABLE_START  30

void do_measure(void)
{
	P2|= (1<<6);//1  - enable auto-load
	//P2&= ~(1<<6);//0  - RESET rx trigger (off auto-load)
	
	tdc_start_measure();//enable TDC capture
	__asm { nop    ; another nop}
	laser_pulse();
	P2&= ~(1<<6);//0  - RESET rx trigger (off auto-load)	
	ADCON = 0;  // select AN0 A/D input,single conv.
	ADST = 1;   //start ADC conversion

	tdc_stop_measure();//disable TDC capture
	tdc_read_measurements();

	while (ADBSY) { ; }  // wait for A/D result
	APD_amplitude = ADDAT & 0x03FF;    // result of A/D process


  if (FIFO0 == 0) cur_dist_value = 2;//error
  else
  //if(1)
	{
		cur_dist_value = FIFO0;
		cur_dist_value-= get_correction(APD_amplitude);
	}	
}

void laser_pulse(void)
{
		P2&= ~(1<<13);
	  P2|= (1<<13);//laser
}


unsigned short get_correction(unsigned short adc_val)
{
	unsigned short result = 0;
	if (adc_val <= CORRECTION_TABLE_START) result = correction_table[0];
	else if (adc_val >= (CORRECTION_TABLE_LENGTH + CORRECTION_TABLE_START-5)) result = correction_table[CORRECTION_TABLE_LENGTH-1];
	else result = (unsigned short)correction_table[adc_val - CORRECTION_TABLE_START];//offset
	
	return result;
}




