#include "hal_io.h"
#include "STC12C5A60S2.h"
#include "hal_stc_io.h"
#include "eeprom.h"
#include "plc_prase.h"
#include "sys_info.h"
#include "modbus_rtu.h"
#include "compiler.h"
#include "plc_command_def.h"
#include <stdio.h>


void delayms(unsigned int ms)
{
    unsigned int i;
	while(ms--)for(i=0;i<1000;i++);
}


sbit P12 = P1^2;
sbit P13 = P1^3;


/*
//--------------------------地址编号-------数量-----------
#define  IO_INPUT_BASE          0
#define  IO_INPUT_COUNT                     16
#define  IO_OUTPUT_BASE         256  //0x00,0x01
#define  IO_OUTPUT_COUNT                    16
#define  AUXI_RELAY_BASE        512  //0x00,0x02
#define  AUXI_RELAY_COUNT                   200
//
#define  TIMING100MS_EVENT_BASE  2048  //0x08,0x00
#define  TIMING100MS_EVENT_COUNT            40

#define  TIMING1S_EVENT_BASE     3072  //0x0C,0x00
#define  TIMING1S_EVENT_COUNT               40

#define  COUNTER_EVENT_BASE      4096  //0x10,0x00
#define  COUNTER_EVENT_COUNT                40
*/

/*
什么问题？
code unsigned char plc_test_buffer[128] = 
{
    PLC_LDI,   0x00,0x08,
	PLC_OUTT,  0x00,0x08,50,0x00,

	PLC_LDF,   0x00,0x08,
	PLC_SET,   0x00,0x02,

	PLC_LD,    0x00,0x02,
	PLC_OUT,   0x00,0x01,

	PLC_END,
};

*/


code unsigned char plc_test_buffer[128] = 
{
    PLC_LD,    0x00,0x00,
	PLC_OUTT,  0x00,0x08,50,0x00,

	PLC_LDP,    0x00,0x08,
	PLC_SET,   0x00,0x01,

	PLC_LD,    0x01,0x00,
	PLC_RST,   0x00,0x01,


	PLC_END,
};





void main(void)
{
  unsigned long start;
  io_init();
  sysclk_init();
  uart1_port_initial();
  PlcInit();
  start = get_sys_clock();
  sys_info.modbus_addr = 0x88;
  sys_unlock();
#if 1
  delayms(1000);
  uart1_send_string("\r\n");
  {
	  //写一些测试程序
	  if(eeprom_compare(0,plc_test_buffer,sizeof(plc_test_buffer)) != 0) {
	      uart1_send_string("write plc code,and runing...\r\n");
	      //eeprom_secotr_erase(0);
	      //eeprom_write(0,plc_test_buffer,sizeof(plc_test_buffer));
	  } else {
	      uart1_send_string("run plc normal.\r\n");
	  }
  }
  uart1_send_string("sys start ......\r\n");
#endif

  while(1)
  {
    //unsigned char reg;
	//io_in_get_bits(0,&reg,8);
	//io_out_set_bits(0,&reg,8);
    //if((get_sys_clock() - start) >= TICK_SECOND) {
     // start = get_sys_clock();
      //uart1_send_string("hahaha..");
    //}
	//
	PlcProcess();
  }
}

