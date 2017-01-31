#include <stdio.h>                /* standard I/O .h-file              */
#include <reg167.h>               /* special function register 80C167  */
#include "apd_functions.h"
#include "tdc_functions.h"
#include "measure_dist.h"
#include "timer.h"

//after initialization
//CSSEL == "11" => CS4-CS0 enabled
//SALSEL == "00" => A19-A16 enabled
//CLKCFG == "110" => PLLx3
//cs1 enabled - see start167.a66

/****************/
/* main program */
/****************/

#define LENGTH 100

unsigned long i;
unsigned short k;

unsigned char byte1;
volatile unsigned long  tmp_val;
volatile unsigned long  tmp_val2;


extern unsigned short APD_amplitude;
extern unsigned short cur_dist_value;
extern unsigned short set_motor_period;//needed motor speed (needed period of encoder)
extern unsigned char start_send_header;//time to send header


void rx_command_handler(unsigned char command, unsigned char data);
void wait (void) { } /* wait function (empty) */

unsigned char  rx_mode = 0;
unsigned char  rx_idle_time = 0;



void blink(void)
{
    P8 |= (1<<2)|(1<<3); /* switch on LED (P2.0 = 1) */
		for (i = 0; i < 100000; i++) wait(); /* delay for 10000 counts */
		P8 &= ~((1<<2)|(1<<3)); /* switch off LED (P2.0 = 0) */
		for (i = 0; i < 100000; i++) wait(); /* delay for 10000 counts */	
}


void do_measure_APD_temp(void)
{
	unsigned char i;
	unsigned short temper_apd[3];
	
	for (i=0;i<3;i++)
	{
			ADCON = 9;  // select AN0 A/D input,single conv.
			ADST = 1;   //start ADC conversion
			while (ADBSY) { ; }  // wait for A/D result
			temper_apd[i] = ADDAT & 0x03FF;    // result of A/D process
	}
	printf ("%d;%d;%d\r\n",temper_apd[0],temper_apd[1],temper_apd[2]);
}

void main (void)  {               /* execution starts here               */
                                  /* initialize the serial interface     */
#ifndef MCB167          /* do not initialize if you use Monitor-166      */  
  P3  |= 0x0400;        /* SET PORT 3.10 OUTPUT LATCH (TXD)              */
  DP3 |= 0x0400;        /* SET PORT 3.10 DIRECTION CONTROL (TXD OUTPUT)  */
  DP3 &= 0xF7FF;        /* RESET PORT 3.11 DIRECTION CONTROL (RXD INPUT) */
  //S0TIC = 0x80;         /* SET TRANSMIT INTERRUPT FLAG                   */
  //S0RIC = 0x00;         /* DELETE RECEIVE INTERRUPT FLAG                 */
  //S0BG  = 0x6A;         /* SET BAUDRATE TO 9600 BAUD                     */
	S0BG = 1;//baud register
	S0BRS = 0;//Baudrate Selection Bit
	
	S0TIC = (0xA<<2) | (2) | 0x80;//ILVL = 0xA + GLVL = 2 - uart tx
	S0RIC = (0x5<<2) | (0) | (1<<6);//ILVL = 0x5 + GLVL  + enable rx int= 0 - uart rx
	
  S0CON = 0x8011;       /* SET SERIAL MODE                               */
#endif
	
	//p2.4 - RAM2_CE
	DP2|= (1<<4); //output
	P2&= ~(1<<4);//0 - ram2 not selected
	
	DP8|= (1<<2)|(1<<3); /* bits 0 – 7 : outputs */
	ODP8 = 0x0000; /* output driver : push/pull */
	
	DP2|= (1<<13); //laser
	ODP2 = 0x0000; /* output driver : push/pull */
	
	init_apd_module();
	init_timer2_3();
	init_timer4();
	
	for (i=0;i<10000;i++) wait();
	init_tdc();

  printf ("TEST v8\n");
	
	//set_dac_value(2900);
	//set_dac_value(2900);
	
	set_dac_value(2700);
	set_dac_value(2700);
	
	do_reset_receiver();
	//test TDC
	tmp_val = tdc_read_data(14);
	printf ("val14:%ld\n", tmp_val);
	//test TDC
	tmp_val = tdc_read_data(1);
	printf ("val1:%lX\n", tmp_val);
	if (tmp_val == 0x0620620) printf ("1-OK\n");
	
	IEN = 1; // set global interrupt enable flag
	
	
	while(1)
	{
		if (start_send_header == 1)
		{
			printf("AbCdEf");//header
			start_send_header = 0;
			rx_idle_time++;
			if (rx_idle_time > 6)//reset rx_mode if there was no data for a long time
			{
				rx_idle_time = 0;
				rx_mode = 0;
			}
		}
	}//end of while(1)                         
}     


//uart rx interrupt
static void rx_interrupt (void) interrupt 0x2B
{
	static unsigned char  command = 0;
	unsigned char rx_byte = (unsigned char)S0RBUF;//received byte
	
	rx_idle_time = 0;
	
	switch (rx_mode)
	{
		case 0: 
		{
			if (rx_byte == 65) {rx_mode++;} else {rx_mode = 0;}//A
			command = 0;
			P8 |= (1<<2)|(1<<3); // switch on LED (P2.0 = 1)
			break;
		}
		case 1: 
		{
			if (rx_byte == 66) {rx_mode++;} else {rx_mode = 0;}//B
			command = 0;
			break;
		}
		case 2: 
		{
			if (rx_byte == 67) {rx_mode++;} else {rx_mode = 0;}//C
			command = 0;
			break;
		}
		case 3: 
		{
			if (rx_byte == 68) {rx_mode++;} else {rx_mode = 0;}//D
			command = 0;
			break;
		}
		
		case 4://command
		{
			if (rx_byte == 65)
			{
				command = 0;
				rx_mode = 1;
				break;
			}
			else
			{
				command = rx_byte;
				rx_mode = 5;
				break;
			}
		}
		case 5: //data received
		{
			rx_command_handler(command, rx_byte);
			rx_mode = 0;
			command = 0;
			P8 &= ~((1<<2)|(1<<3)); // switch off LED (P2.0 = 0)
			break;
		}
		default: {rx_mode = 0; command = 0; break;}
	}
}

void rx_command_handler(unsigned char command, unsigned char data)
{
	unsigned short voltage = 0;
	switch (command)
	{
		case 66: //B - set speed
		{
			set_motor_period = (unsigned short)data;
			if (set_motor_period < 187) set_motor_period = 187;
			break;
		}
		case 67: //C - set voltage
		{
			if (data > 150) data = 140;//in volts
			voltage = (unsigned short)data * 20;
			set_dac_value(voltage);
			set_dac_value(voltage);
			break;
		}
		default: break;
	}
}



//		printf("AbCdEf");//header
//		for (k=0;k<720;k++)
//		{
//			S0TBUF = (unsigned char)(k>>8);
//			while(S0TIR == 0){}
//			S0TIR=0;
//			S0TBUF = (unsigned char)(k&0x00ff);
//			while(S0TIR == 0){}
//			S0TIR=0;
//		}
//		for (i=0;i<10000;i++) wait();
