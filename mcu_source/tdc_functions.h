#ifndef _TDC_FUNC
#define _TDC_FUNC

void init_tdc(void);
unsigned long tdc_read_data(unsigned char adr);
void tdc_write_data(unsigned char adr, unsigned long value);
void switch_tdc_to_16bit_mode(void);
void tdc_start_measure(void);
void tdc_stop_measure(void);
void tdc_read_measurements(void);

void tdc_master_reset(void);
unsigned short tdc_read_fifo0(void);
unsigned short tdc_tdc_cheek_hit_flag(void);


#endif