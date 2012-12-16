#ifndef __HAL_STC_IO_H__
#define __HAL_STC_IO_H__

#define   TICK_SECOND           200




void sysclk_init(void);
unsigned long get_sys_clock(void);
void uart1_port_initial(void);
void send_uart1(unsigned char ch);
void uart1_send_string(char * pstr);
void sys_lock(void);
void sys_unlock(void);

#endif


