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

void timer1_interrupt() interrupt 3
{
   TH1 = TMR1_RELOAD_H;
   TL1 = TMR1_RELOAD_L;
   system_current_time_tick++;
   //UartReceivetoModbusRtuTimeTick();
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
	AUXR |= 0x15;  //1T模式,允许BRT发生,UART1使用BRT
	ES = 1;
	//
#if 0 //if uart2 enable
	AUXR |= (1<<4);//enable BRTR
	AUXR |= (1<<3); //uart2 baud rate x12 enable
	AUXR |= (1<<2); //BRT 1T mode
    S2CON = 0x50;
	IE2 |= (1<<0);  //uart2 interrupt enable
	//IP2 &= ~(0x3<<0); //uart2 中断优先级设为最低
#endif
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


unsigned char rx_int_buffer[32];
unsigned char rx_push_index = 1;
unsigned char rx_pop_index = 0;

void uart1_initerrupt_receive(void) interrupt 4 using 2
{
  unsigned char len;
  unsigned char k = 0;
  if(RI == 1) 
  {
      RI = 0;
	  k = SBUF;
      sys_lock();
      len = ((rx_push_index+1)>=sizeof(rx_int_buffer))?0:rx_push_index;
      if(len != rx_pop_index) { //预先知道没有重合
          if(rx_push_index > rx_pop_index) {
              len = rx_push_index - rx_pop_index;
          } else {
              len = rx_push_index + sizeof(rx_int_buffer) - rx_pop_index;
          }
          len = sizeof(rx_int_buffer) - len;
          if(len > 1) {
              rx_int_buffer[rx_push_index++] = k;
              if(rx_push_index >= sizeof(rx_int_buffer)) {
                  rx_push_index = 0;
              }
          }
      }
      sys_unlock();
/*
      if(rx_int_count < sizeof(rx_int_buffer)) {
          rx_int_buffer[rx_int_count++] = k;
      }
*/
  }
  else 
  {
      TI = 0;
  }
}


//这个函数只能在主程序中调用，在串口中调用会出问题
void uart1_rx_buffer_process(void)
{
    unsigned char len;
    sys_lock();
    len = rx_push_index;
    sys_unlock();
    if(len != rx_pop_index) {
        if(len >= rx_pop_index) {
            len = len - rx_pop_index;
        } else {
            len = len + sizeof(rx_int_buffer) - rx_pop_index;
        }
        len -= 1;
        if(len > 0) {
            unsigned char i;
            for(i=0;i<len;i++) {
                sys_lock();
                if(++rx_pop_index >= sizeof(rx_int_buffer)) {
                    rx_pop_index = 0;
                }
                sys_unlock();
                pack_prase_in(rx_int_buffer[rx_pop_index]);
            }
        }
    }


  // unsigned char i;
  // sys_lock();
   //for(i=0;i<rx_int_count;i++) {
   //    pack_prase_in(rx_int_buffer[i]);
   //}
  // rx_int_count = 0;
  // sys_unlock();
}


#if 0
#define  S2RI   0x01
#define  S2TI   0x02

bit uart2_tx_busy = 0;

sbit     P25 =  P2^5;

void Uart2Isr(void) interrupt 8
{
  unsigned char reg;
  if(S2CON&S2RI) {
      S2CON &= ~S2RI;
	  reg = S2BUF;
	  P25 = !P25;
  }
  if(S2CON&S2TI) {
    S2CON &= ~(S2TI);
	uart2_tx_busy = 0;
  }
}

void Uart2SendByte(unsigned char ch)
{
  while(uart2_tx_busy);
  uart2_tx_busy = 1;
  S2BUF = ch;
}
#endif


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

void uart1_send_data(unsigned char * pbuf,unsigned int len)
{
    unsigned int i;
    for(i=0;i<len;i++) {
    	send_uart1(pbuf[i]);
	}
}


void uart1_send_str_hex(char * pstr,unsigned int hex)
{
    unsigned char reg = hex >> 8;
    uart1_send_string(pstr);
    send_uart1(':');
    send_uart1(((reg>>4)>=10)?((reg>>4)-10+'A'):((reg>>4)+'0'));
    send_uart1(((reg&0xF)>=10)?((reg&0xF)-10+'A'):((reg&0xF)+'0'));
    reg = hex & 0xFF;
    send_uart1(((reg>>4)>=10)?((reg>>4)-10+'A'):((reg>>4)+'0'));
    send_uart1(((reg&0xF)>=10)?((reg&0xF)-10+'A'):((reg&0xF)+'0'));
    uart1_send_string("\r\n");
}

volatile unsigned long lock = 0;

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


