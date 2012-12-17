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
bit           one_pack_is_come_flag = 0;
bit           rx_started = 0;

#define  STREAM_START        0x0F
#define  STREAM_END          0xF0
#define  STREAM_ESCAPE       0x55
#define  STREAM_ES_S         0x50   //ת���ַ� 'S'
#define  STREAM_ES_E         0x05   //ת���ַ� 'E'
//�������������ַ��⣬��������ԭʼ�ַ�

void  set_pack_is_finished(void);


unsigned long serial_last_rx_time;

void SerialRxCheckTimeoutTick(void)
{
  sys_lock();
  if(stream_rx_index > 0 && ((get_sys_clock() - serial_last_rx_time) >= TICK_SECOND/10)) {  //����100ms��ʱ
      serial_last_rx_time = get_sys_clock();
	  stream_rx_index = 0;
  }
  sys_unlock();
}


/***************************************************
 * ��������ԭʼ������
 * ������̫�������������ݣ�Ȼ��������ͬ�������ݰ�
 * ������ԭʼ�����ݣ������λ��CRCУ�飬���У��ɹ�,
 * ��ΰ�����ȷ�ģ��ȴ��ϲ�������
 */
void prase_in_stream(unsigned char ch)
{
    if(serial_stream_rx_finished()) {
	    goto exit; //����
	}
	if(ch == STREAM_START) {
	   stream_rx_index = 0; 
	   rx_started = 1;
	   goto exit;
	} else {
	   if(!rx_started) {
	       goto exit; //��δ��ʼ
	   }
	}
    switch(ch)
	{
	case STREAM_START:
	    break;
	case STREAM_END:
	    if(rx_started && stream_rx_index >= 3) { //����CRC���ڵ����ݱ������3
		    unsigned int crc;
			struct modbus_crc_type * pcrc = (struct modbus_crc_type *)&stream_in_buffer[stream_rx_index - 2];
			crc = pcrc->crc_hi;
			crc <<= 8;
			crc |= pcrc->crc_lo;
			if(1) { //crc == CRC16(stream_in_buffer,stream_rx_index)) {
			    stream_rx_index -= 2; //ȥ��CRC�ĳ���
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
		    //��Ч��ת���ַ�����������Ч
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
		    //��Ч��ת���ַ�����������Ч
			stream_rx_index = 0;
			rx_started = 0;
		}
		break;
	default:
	  if(stream_rx_index < sizeof(stream_in_buffer)) {
	      stream_in_buffer[stream_rx_index++] = ch;
	  }
	}
exit:
	last_byte_val = ch;
}


unsigned int get_stream_len(void)
{
    return stream_rx_index;
}

extern unsigned char * get_stream_ptr(void)
{
   return stream_in_buffer;
}

void stream_packet_send(unsigned char * buffer,unsigned int len)
{
    unsigned int i;
    if(len == 0) {
        return ;
    }
    send_uart1(STREAM_START);
    for(i=0;i<len;i++) {
	    unsigned char ch = buffer[i];
        if(ch == STREAM_START) {
		    send_uart1(STREAM_ESCAPE);
            send_uart1(STREAM_ES_S);
        } else if(ch == STREAM_END) {
		    send_uart1(STREAM_ESCAPE);
            send_uart1(STREAM_ES_E);
        } else {
            send_uart1(ch);
        }
    }
    send_uart1(0);
    send_uart1(0);
    send_uart1(STREAM_END);
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
    stream_rx_index = 0;
	sys_unlock();
}

void  set_pack_is_finished(void)
{
	sys_lock();
    one_pack_is_come_flag = 1;
	sys_unlock();
}

