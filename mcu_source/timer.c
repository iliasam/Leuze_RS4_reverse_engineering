#include "timer.h"
#include <stdio.h>                /* standard I/O .h-file              */
#include <reg167.h>               /* special function register 80C167  */
#include "measure_dist.h"

//total captured points = 1000-2*N_ANGLE
#define N_ANGLE 145
#define ZERO_POS  (unsigned short)(750)
#define START_POS (unsigned short)(ZERO_POS+N_ANGLE)
#define STOP_POS  (unsigned short)(ZERO_POS-N_ANGLE)


unsigned short capture_old_time = 0;
unsigned short capture_now_time = 0;
volatile unsigned short enc_period = 0;
volatile unsigned short encoder_pos = 0;
volatile unsigned short encoder_max_pos = 0;

unsigned short set_motor_period = 257;//needed motor speed (needed period of encoder)

unsigned char capture_enabled = 0;
unsigned char start_send_header = 0;//time to send header

extern unsigned short cur_dist_value;
unsigned short tmp_dist_value;

//used to calculate distance to reference plate
signed short curr_zero_value = 0;
signed short prev_zero_value = 0;

//syncro input
unsigned char start_sync = 0;
unsigned char stop_sync = 0;

//timer to capture encoder
void init_timer2_3(void)
{
	//Encoder input
	DP3&= ~(1<<7); //input
	
	//ENCODER enable output
	DP2|= (1<<14); //output
	P2&= ~(1<<14);//0 - enable encoder
	
	//aux timer
	T2CON = (1<<6) | (5<<3) | (3<<0);//run + capture mode + any edge
	
	//core timer
	T3CON = (0<<3) | (1<<6);//timer mode + run
	
	T2IC = (0xF<<2) | (3);//ILVL = 0xF + GLVL = 3
	T3IE = 0;
	T2IE = 1;
}

//timer to controll motor speed
static void timer2 (void) interrupt 0x22 
{
	static unsigned short enc_period_old = 0;
	capture_old_time = capture_now_time;
	capture_now_time = T2;
	
	//encoder overflow control
	enc_period_old = enc_period;
	if (capture_now_time >= capture_old_time) {enc_period = capture_now_time - capture_old_time;} else {enc_period = 0xFFFF - capture_old_time + capture_now_time;}
	if (enc_period > 1000) enc_period = 1000;

  encoder_pos++;
  
	if ((enc_period_old*2) < enc_period)//zero found
	{
		encoder_max_pos = encoder_pos;//998 is normal
		encoder_pos = 0;
	}
	
	if ((capture_enabled == 0) && (encoder_pos == START_POS)) 
	{
		//starting capture
		capture_enabled=1;
		if ((P5 & (1<<3))!= 0) {start_sync = 1;} else {start_sync = 0;}//check sync line (3d scanner)
	}
	else if ((capture_enabled == 1) && (encoder_pos == STOP_POS)) 
	{
		//stop capture
		capture_enabled=0;
		if ((P5 & (1<<3))!= 0) {stop_sync = 1;} else {stop_sync = 0;}//check sync line (3d scanner)
	}
	else if ((capture_enabled == 0) && (encoder_pos == (ZERO_POS-1)))//null reference distance measurement
	{
		do_measure();
		if (cur_dist_value < 2000)
		{
			curr_zero_value = prev_zero_value + (((signed short)cur_dist_value - prev_zero_value)/8);//exponential filter
			prev_zero_value = curr_zero_value;
			if ((curr_zero_value > 20000) && (curr_zero_value < 0)) {curr_zero_value = 200; prev_zero_value = 200;}
		}
		
		start_send_header = 1;//time to send header
	}
	else if ((capture_enabled == 0) && (encoder_pos == STOP_POS+1)) //sending syncro result - last value
	{
		//sending syncro info instead of distance (3d scanner)
		if (start_sync != stop_sync) tmp_dist_value = 3;
		else tmp_dist_value = (unsigned short)(start_sync+1);//equal
		//start tx
		S0TIE = 1;//enable interrupt
		S0TBUF = (unsigned char)(tmp_dist_value>>8);
	}
	
	
	if (capture_enabled == 1)//main distance meusurement function
	{
		do_measure();
		if (cur_dist_value > curr_zero_value)//normal
		{
			tmp_dist_value = cur_dist_value-curr_zero_value;//store distance value with correction
		}
		else
		{
			tmp_dist_value = 0;
		}
		
		S0TIE = 1;//enable UART TX interrupt
		S0TBUF = (unsigned char)(tmp_dist_value>>8);
	}
	
	
	
}

//TX interrupt
static void uart_tx (void) interrupt 0x2A
{
	S0TIE = 0;//disable interrupt
	S0TBUF = (unsigned char)(tmp_dist_value & 0x00FF);
}

//motor controlling speed timer
void init_timer4(void)
{
	//aux timer
	T4CON = (1<<6) | (0<<3) | (1<<7);//run + timer mode + down mode
	T4IC = (1<<2) | (1);//ILVL = 1 + GLVL = 1
	T4IE = 1;
}

static void timer4 (void) interrupt 0x24 
{
	T4 = 4125;
	motor_controlling_function();
}

void motor_controlling_function(void)
{
	signed short error = (signed short)set_motor_period - (signed short)enc_period;
	if (error > 5)
	{
			DP3|= (1<<15); //output
			P3|= (1<<15);//1 - to low speed
	}
	else if (error < (-5))
	{
			DP3|= (1<<15); //output
			P3&= ~(1<<15);//0 - to high speed
	}
	else 
	{
			DP3&= ~(1<<15); //input - no speed change
	}
}
