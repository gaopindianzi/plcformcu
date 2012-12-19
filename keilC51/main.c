#include "hal_io.h"
#include "STC12C5A60S2.h"
#include "hal_stc_io.h"
#include "eeprom.h"
#include "plc_prase.h"
#include "sys_info.h"
#include "modbus_rtu.h"
#include "compiler.h"
#include "plc_command_def.h"
#include "serial_comm_packeter.h"
#include <stdio.h>


void delayms(unsigned int ms)
{
    unsigned int i;
	while(ms--)for(i=0;i<1000;i++);
}


sbit P12 = P1^2;
sbit P13 = P1^3;


/*
//--------------------------��ַ���-------����-----------
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

    unsigned char op;
    unsigned char net_index;  //����ָ����������Ϊ������Ҫ�����ѯ������һ���ͣ�����������ڴ����
    unsigned char remote_device_addr;
    unsigned char remote_start_addr_hi;  //Զ�����ݵ���ʼ��ַ
    unsigned char remote_start_addr_lo;  //Զ�����ݵ���ʼ��ַ
    unsigned char local_start_addr_hi;  //Զ�����ݵ���ʼ��ַ
    unsigned char local_start_addr_lo;  //Զ�����ݵ���ʼ��ַ
    unsigned char data_number; //ͨ�����ݵĸ���
    //�������
    unsigned char enable_addr_hi;
    unsigned char enable_addr_lo;
    //����һ��ͨ��
    unsigned char request_addr_hi;
    unsigned char request_addr_lo;
    //ͨ�Ž����б��
    unsigned char txing_hi;
    unsigned char txing_lo;
    //��ɵ�ַ
    unsigned char done_addr_hi;
    unsigned char done_addr_lo;
    //��ʱ��ʱ������
    unsigned char timeout_addr_hi;
    unsigned char timeout_addr_lo;
    //��ʱ��ʱ,S
    unsigned char timeout_val;
*/

code unsigned char plc_test_buffer[128] = 
{
    2, //һ��ͨ�Žӿ�
    PLC_LD, 0x00,0x00,
    PLC_OUT,0x01,0x00,
    PLC_OUT,0x02,0x01,
    PLC_LD, 0x00,0x01,
    PLC_OUT,0x02,0x02,
    PLC_OUT,0x01,0x01,
    //               Զ�̵�ַ     ���ص�ַ  ����  ����       ����      ������     ���       ��ʱ��ʱ��  ��ʱʱ��
    PLC_NETRB,0,1,   0x02,0x00,   0x02,0x00,  1,  0x02,0x01, 0x02,0x02,0x02,0x03, 0x02,0x04, 0x0C,0x00,   5,

    PLC_LD, 0x00,0x02, //����������
    PLC_OUT,0x02,0x05,
    PLC_OUT,0x02,0x06,

    PLC_NETRB,1,2,   0x02,0x01,   0x02,0x01,  1,  0x02,0x05, 0x02,0x06,0x02,0x07, 0x02,0x08, 0x0C,0x01,   6,


	PLC_END,
};

code unsigned char test_strasm_in[] = 
{
  0x0F,0x0F,0xF0,0x0F,0x88,0x05,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0xF0
};


void main(void)
{
  unsigned char reg = 0xFF;
  unsigned int index = 0;
  unsigned long start;
  io_init();
  sysclk_init();
  uart1_port_initial();
  PlcInit();
  start = get_sys_clock();
  sys_info.modbus_addr = 0x88;
  sys_unlock();
#ifndef DEBUG_ON
  delayms(1000);
  uart1_send_string("\r\n");
  {
	  //дһЩ���Գ���
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

  io_out_set_bits(0,&reg,8);
  serial_rx_tx_initialize();
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
#ifdef DEBUG_ON
	//prase_in_stream(test_strasm_in[index++]);
	//if(index >= sizeof(test_strasm_in)) {
    //    index = 0;
	//}
#endif
    //Uart2SendByte('A');
	//SerialRxCheckTimeoutTick();

#ifndef DEBUG_ON

#endif

  }
}

