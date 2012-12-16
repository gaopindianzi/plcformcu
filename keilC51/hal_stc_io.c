#include "STC12C5A60S2.h"
#include "hal_io.h"
#include "hal_stc_io.h"
#include "modbus_rtu.h"
#include "plc_prase.h"
#include "serial_comm_packeter.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define  bitclr(reg,msk)  do{ reg &= ~(msk); }while(0)
#define  bitset(reg,msk)  do{ reg |=  (msk); }while(0)

sbit P12 = P1^2;
sbit P13 = P1^3;


#define TMR1_RELOAD_H 0xDF //5ms
#define TMR1_RELOAD_L 0x72 //


void Timer1Init(void)		//5毫秒@20.000MHz
{
   bitclr(AUXR,(1<<6)); //==0,12分频
   bitset(TMOD,(1<<4)); //MODE1,16位计数
   bitclr(TMOD,(1<<5));
   bitclr(TMOD,(1<<6));
   TH1 = TMR1_RELOAD_H;
   TL1 = TMR1_RELOAD_L;
   ET1 = 1;
   TR1 = 1;
}

static unsigned long system_current_time_tick = 0;

void timer1_interrupt() interrupt 3 using 1
{
   TH1 = TMR1_RELOAD_H;
   TL1 = TMR1_RELOAD_L;
   system_current_time_tick++;
   //plc_timing_tick_process();
   UartReceivetoModbusRtuTimeTick();
}

unsigned long get_sys_clock(void)
{
    unsigned long ret;
    sys_lock();
    ret = system_current_time_tick;
    sys_unlock();
    return ret;
}


void sysclk_init(void)
{
    CLK_DIV = 0x00;
    //利用PCA来做系统定时器
    Timer1Init();
}

void uart1_port_initial(void)
{
    SCON = 0x50;
	BRT  = 0xBF; //20MHz晶振、9600
	//BRT  = 0xFB; //20MHz晶振、125000
	AUXR = 0x15;  //1T模式,允许BRT发生,UART1使用BRT
	ES = 1;
}


void send_uart1(unsigned char ch)
{
  ES = 0;
  TI = 0;
  SBUF = ch;
#ifndef DEBUG_ON
  while(TI == 0);
#endif
  TI = 0;
  ES = 1;
}


void uart1_initerrupt_receive(void) interrupt 4
{
  unsigned char k = 0;
  if(RI == 1) 
  {
      RI = 0;
	  k = SBUF;
      //send_uart1(k);
	  //UartReceivetoModbusRtu(k);
	  prase_in_stream(k);
  }
  else 
  {
      TI = 0;
  }
}

char putchar(char ch)
{
    send_uart1(ch);  
    return ch;
}


void uart1_send_string(char * pstr)
{
    while(*pstr) {
    	send_uart1(*pstr++);
	}
}

unsigned char lock = 0;

void sys_lock(void)
{
  EA = 0;
  lock++;
}
void sys_unlock(void)
{
  if(lock == 0) {
    EA = 1;
  } else {
    if(--lock == 0) {
	  EA = 1;
	}
  }
}


