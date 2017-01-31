#include "apd_functions.h"
#include <reg167.h>               /* special function register 80C167  */

void init_apd_module(void)
{
	//BASE5 - p2.6 //0 - do receiver reset
	DP2|= (1<<6); //output
	P2&= ~(1<<6);//0  - start reset
	
	//CLK - p7.5
	DP7|= (1<<5); //output
	P7&= ~(1<<5);//low
	
	//DI - p7.6 //slave data input
	DP7|= (1<<6); //output
	P7&= ~(1<<6);//low
	
	//CS2 - p3.4 - DAC (low active)
	DP3|= (1<<4);//output
	P3|= (1<<4);//high
	
	//CS - p7.4 - EEPROM (high active)
	DP7|= (1<<4);//output
	P7&= ~(1<<4);//low
	
	//ADC
	P5DIDIS|= (1<<0);//disable digital p5.0
}

void do_reset_receiver(void)
{
		P2&= ~(1<<6);//0  - start reset rx
		//__asm { nop    ; another nop}
		//__asm { nop    ; another nop}
		P2|= (1<<6);//1  - start rx
}


//LTC1451
//msb first
//APD voltage (V) = value/200
void set_dac_value(unsigned int value)
{
	unsigned char i;
	
	for (i=0;i<12;i++)
	{
		if ((value & (1<<11)) != 0) P7|= (1<<6); else P7&= ~(1<<6);//set data out
		__asm { nop    ; another nop}
		__asm { nop    ; another nop}
		P7|= (1<<5);//clk high
		__asm { nop    ; another nop}
		__asm { nop    ; another nop}
		P7&= ~(1<<5);//clk low
		value =  value<<1;
	}
	P3|= (1<<4);//CS2 high
	__asm { nop    ; another nop}
	__asm { nop    ; another nop}
	P3&= ~(1<<4);//CS2 low
}