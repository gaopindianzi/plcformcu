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
#include "modbus_ascii.h"


#define STREAM_IDLE   0
#define STREAM_NORMAL 1
#define STREAM_IN_ESC 2

DATA_RX_PACKET_T rx_pack;
DATA_TX_PACKET_T tx_pack;


#define  STREAM_START        0x0F
#define  STREAM_END          0xF0
#define  STREAM_ESCAPE       0x55
#define  STREAM_ES_S         0x50   //转义字符 'S'
#define  STREAM_ES_E         0x05   //转义字符 'E'
//除了以上特殊字符外，其他都是原始字符



 void pack_prase_in(unsigned char ch)
 {
   if(rx_pack.finished) {
     return ;
   }
   switch(rx_pack.state)
   {
   case STREAM_IDLE:
     if(ch == STREAM_START) {
       rx_pack.state = STREAM_NORMAL;
       rx_pack.index = 0;
     }
     break;
   case STREAM_NORMAL:
     if(ch == STREAM_ESCAPE) {
       rx_pack.state = STREAM_IN_ESC;
     } else if(ch == STREAM_START) {
       rx_pack.state = STREAM_IDLE;
     } else if(ch == STREAM_END) {
       if(rx_pack.index >= 3) {
         unsigned int crc = CRC16(rx_pack.buffer,rx_pack.index-2);
         if(crc == BYTES_TO_WORD(&rx_pack.buffer[rx_pack.index-2])) {
			 rx_pack.index -= 2;
             rx_pack.finished = 1;
         }
       }
       rx_pack.state = STREAM_IDLE;
     } else {
       rx_pack.buffer[rx_pack.index++] = ch;
     }
     break;
   case STREAM_IN_ESC:
     if(ch == STREAM_ESCAPE) {
       rx_pack.buffer[rx_pack.index++] = STREAM_ESCAPE;
       rx_pack.state = STREAM_NORMAL;
     } else if(ch == STREAM_ES_S) {
       rx_pack.buffer[rx_pack.index++] = STREAM_START;
       rx_pack.state = STREAM_NORMAL;
     } else if(ch == STREAM_ES_E) {
       rx_pack.buffer[rx_pack.index++] = STREAM_END;
       rx_pack.state = STREAM_NORMAL;
     } else {
       rx_pack.state = STREAM_IDLE;
     }
     break;
   default:
     rx_pack.state = STREAM_IDLE;
     break;
   }
   //溢出判断
   if(rx_pack.index >= sizeof(rx_pack.buffer)) {
     rx_pack.state = STREAM_IDLE;
   }
 }


 unsigned int prase_in_buffer(unsigned char * src,unsigned int len)
 {
	 unsigned int i = 0;
	 if(len == 0) {
		 return 0;
	 } else if(tx_pack.finished) {
		 return 0;
	 } else {
		 unsigned int crc = CRC16(src,len);
		 tx_pack.index = 0;
		 tx_pack.buffer[tx_pack.index++] = STREAM_START;
		 while(len--) {
			 unsigned char reg = src[i++];
			 if(tx_pack.index >= (sizeof(tx_pack.buffer) - 4)) {
				 //溢出判断,2字节CRC，一个结束位,一个转义字符
				 tx_pack.index = 0;
				 break;
			 }
			 if(reg == STREAM_START) {
				 tx_pack.buffer[tx_pack.index++] = STREAM_ESCAPE;
				 tx_pack.buffer[tx_pack.index++] = STREAM_ES_S;
		     } else if(reg == STREAM_ESCAPE) {
				 tx_pack.buffer[tx_pack.index++] = STREAM_ESCAPE;
				 tx_pack.buffer[tx_pack.index++] = STREAM_ESCAPE;
			 } else if(reg == STREAM_END) {
				 tx_pack.buffer[tx_pack.index++] = STREAM_ESCAPE;
				 tx_pack.buffer[tx_pack.index++] = STREAM_ES_E;
			 } else {
				 tx_pack.buffer[tx_pack.index++] = reg;
			 }
		 }
		 if(tx_pack.index > 0) {
		     tx_pack.buffer[tx_pack.index++] = crc & 0xFF;
		     tx_pack.buffer[tx_pack.index++] = crc >> 8;
		     tx_pack.buffer[tx_pack.index++] = STREAM_END;
		     tx_pack.finished = 1;
		 }
	 }
	 return tx_pack.index;
 }



