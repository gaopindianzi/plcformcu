#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define  THIS_INFO    1
#define  THIS_ERROR   1


#include "hal_io.h"
#include "hal_stc_io.h"
#include "eeprom.h"
#include "plc_command_def.h"
#include "plc_prase.h"
#include "serial_comm_packeter.h"
#include "modbus_rtu.h"


unsigned char stream_in_buffer[PACK_MAX_RX_SIZE];
unsigned char stream_rx_index;
unsigned char last_byte_val;
bit           one_pack_is_come_flag;
bit           rx_started;

#define  STREAM_START        'S'
#define  STREAM_END          'E'
#define  STREAM_ESCAPE       'C'
#define  STREAM_ES_S         's'   //转义字符 'S'
#define  STREAM_ES_E         'e'   //转义字符 'E'
//除了以上特殊字符外，其他都是原始字符

void  set_pack_is_finished(void);


unsigned long serial_last_rx_time;

void SerialRxCheckTimeoutTick(void)
{
  sys_lock();
  if(stream_rx_index > 0 && ((get_sys_clock() - serial_last_rx_time) >= TICK_SECOND/10)) {  //接收100ms超时
      serial_last_rx_time = get_sys_clock();
	  stream_rx_index = 0;
  }
  sys_unlock();
}


/***************************************************
 * 解析输入原始数据流
 * 根据以太网发下来的数据，然后带软件包同步的数据包
 * 解析成原始的数据，最后两位是CRC校验，如果校验成功,
 * 则次包是正确的，等待上层软降处理
 */
void prase_in_stream(unsigned char ch)
{
    if(serial_stream_rx_finished()) {
	    return ; //丢弃
	}
	if(ch == STREAM_START) {
	   stream_rx_index = 0; 
	   rx_started = 1;
	   return ;
	} else {
	   if(!rx_started) {
	       return ; //尚未开始
	   }
	}
    switch(ch)
	{
	case STREAM_START:
	    break;
	case STREAM_END:
	    if(rx_started && stream_rx_index >= 3) { //包括CRC在内的数据必须大于3
		    unsigned int crc;
			struct modbus_crc_type * pcrc = (struct modbus_crc_type *)&stream_in_buffer[stream_rx_index - 2];
			crc = pcrc->crc_hi;
			crc <<= 8;
			crc |= pcrc->crc_lo;
			if(1) { //crc == CRC16(stream_in_buffer,stream_rx_index)) {
			    stream_rx_index -= 2; //去掉CRC的长度
			    set_pack_is_finished();
			} else {
			    stream_rx_index = 0;
				rx_started = 0;
			}
		} else {
		    stream_rx_index = 0;
			rx_started = 0;
		}
		break;
	case STREAM_ESCAPE:
		break;
	case STREAM_ES_S:
	    if(last_byte_val == STREAM_ESCAPE) {
	        if(stream_rx_index < sizeof(stream_in_buffer)) {
	            stream_in_buffer[stream_rx_index++] = STREAM_START;
	        }
		} else {
		    //无效的转义字符，数据流无效
			stream_rx_index = 0;
			rx_started = 0;
		}
		break;
	case STREAM_ES_E:
	    if(last_byte_val == STREAM_ESCAPE) {
	        if(stream_rx_index < sizeof(stream_in_buffer)) {
	            stream_in_buffer[stream_rx_index++] = STREAM_END;
	        }
		} else {
		    //无效的转义字符，数据流无效
			stream_rx_index = 0;
			rx_started = 0;
		}
		break;
	default:
	  if(stream_rx_index < sizeof(stream_in_buffer)) {
	      stream_in_buffer[stream_rx_index++] = ch;
	  }
	}
	last_byte_val = ch;
}


unsigned char serial_stream_rx_finished(void)
{
    unsigned char ret;
	sys_lock();
    ret = one_pack_is_come_flag;
	sys_unlock();
	return ret;
}

void serial_clear_stream(void)
{
	sys_lock();
    one_pack_is_come_flag = 0;
	sys_unlock();
}

void  set_pack_is_finished(void)
{
	sys_lock();
    one_pack_is_come_flag = 1;
	sys_unlock();
}

