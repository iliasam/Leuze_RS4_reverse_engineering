#include "tdc_functions.h"
#include <stdio.h>                /* standard I/O .h-file              */
#include <reg167.h>               /* special function register 80C167  */

#define TDC_BASE_ADDR 0x40000
//#define TDC_BASE_ADDR 32790 //used for simulation

unsigned short FIFO0 = 0;

void init_tdc(void)
{
	//TDC_OEn2 - p2.0 //0 - contineous read from TDC
	DP2|= (1<<0); //output
	P2|= (1<<0);//1 - no contineous red
	
	//TDC_ALU_trig - p2.1 //1 - start TDC ALU
	DP2|= (1<<1); //output
	P2&= ~(1<<1);//0 - no start
	
	//DisTDC_IN - p2.2 //1 - disable TDC start and stop
	DP2|= (1<<2); //output
	P2|= (1<<2);//1 - disable is ON
	
	//TDC_RES_N - p3.6 //0 - reset TDC
	DP3|= (1<<6); //output
	P3|= (1<<6);//1 - no reset
	
	switch_tdc_to_16bit_mode();
	
	//configure TDC registers
	//tdc_write_data( 0,0x000008B); // Rising edges, Start ringoscil.
	tdc_write_data( 0,(1<<0)|(1<<1)|(1<<3)|(1<<5)|(1<<7)); // Rising edges, Start ringoscil.
	tdc_write_data(1,0x0620620); // Channel adjust = 6 & 2
	tdc_write_data(2,0x0062004); // R-Mode, channel adjust = 6 & 2
	tdc_write_data(3,0x000001E); // Use LVPECL inputs, MSet = 30
	tdc_write_data(4,0x6000300); // EFlagHiZN, Quiet Mode, M-Mode
	tdc_write_data(5,0x0000000); // -
	tdc_write_data(6,0x8000000); // Switch on ECL power
	tdc_write_data(7,0x0001FB4); // Bin = 0.8850ps (resolution ~ 10ps rms)
	tdc_write_data(11,0); // 0 -> ErrFlagPin
	tdc_write_data(12,0); // 0 -> IrFlagPin
	
	tdc_master_reset();// Master reset
}

__inline void tdc_start_measure(void)
{
	P2&= ~(1<<2);//0 - disable is OFF
}


__inline void tdc_stop_measure(void)
{
	P2|= (1<<2);//1 - disable is ON
}


void tdc_read_measurements(void)
{
	unsigned char i;
	
	if(tdc_tdc_cheek_hit_flag() == 0) //11bit - (1 - HIT fifo is empty)/(0 - data present)
	{
		P2|= (1<<1);//1 - start TDC ALU
		for (i=0;i<6;i++) __asm { nop    ; another nop}
		FIFO0 = tdc_read_fifo0();// Read FIFO0
	}
	else
	{
		FIFO0 = 0;
	}
	P2&= ~(1<<1);//0 - stop TDC ALU
	
	tdc_master_reset();// Master reset
}

//read 24bit data from TDC
unsigned long tdc_read_data(unsigned char adr)
{
	unsigned short tmp_value;
	unsigned long result;
	volatile  unsigned short far *tmp_adr = (unsigned short far *)(TDC_BASE_ADDR + (adr<<1));//adress is shifted at bus

	((unsigned short *)&result)[0] = *tmp_adr;
	tmp_value = *tmp_adr & 0x00FF;//MSW
	((unsigned short *)&result)[1] = tmp_value;
	return result;
}

void switch_tdc_to_16bit_mode(void)
{
	unsigned char far *tmp_adr = (unsigned char far *)(TDC_BASE_ADDR + 14*2);//adress is shifted at bus
	*tmp_adr = 0x10;
	//*tmp_adr = 0x00; //do not work
}


void tdc_write_data(unsigned char adr, unsigned long value)
{
	volatile unsigned short far *tmp_adr = (unsigned short far *)(TDC_BASE_ADDR + (adr<<1));//adress is shifted at bus
	
	*tmp_adr = (unsigned short)(value & 0xffff);//LSW
	*tmp_adr = (unsigned short)((value>>16) & 0xffff);//MSW
}

void tdc_master_reset(void)
{
	volatile unsigned short far *tmp_adr = (unsigned short far *)(TDC_BASE_ADDR + (4<<1));//adress is shifted at bus
		
	*tmp_adr = (unsigned short)(0x6400300 & 0xffff);//LSW
	*tmp_adr = (unsigned short)((0x6400300>>16) & 0xffff);//MSW
}

//specialised for quicker run
unsigned short tdc_read_fifo0(void)
{
	unsigned short tmp_value;
	unsigned long result;
	unsigned short result2;
	volatile  unsigned short far *tmp_adr = (unsigned short far *)(TDC_BASE_ADDR + (8<<1));//adress is shifted at bus

	((unsigned short *)&result)[0] = *tmp_adr;
	tmp_value = *tmp_adr & 0x00FF;//MSW
	((unsigned short *)&result)[1] = tmp_value;
	result = result&0x7FFFFF;
	result2 = (unsigned short)(result/16);
	return result2;
}

//cheek if internal hit fifo flag is set
unsigned short tdc_tdc_cheek_hit_flag(void)
{
	volatile  unsigned short tmp_value1;
	volatile  unsigned short tmp_value2;
	volatile  unsigned short far *tmp_adr = (unsigned short far *)(TDC_BASE_ADDR + (12<<1));//adress is shifted at bus

	tmp_value1 = *tmp_adr;
	tmp_value2 = *tmp_adr;//not used, for reading stability
	if (tmp_value1 & (1<<11)) return 1; else return 0;
}
