#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define  THIS_INFO    0
#define  THIS_ERROR   0


#include "STC12C5A60S2.h"

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

RX_PACKS_CTL_T   rx_ctl;
DATA_TX_CTL_T    tx_ctl;


#define  STREAM_START        0x0F
#define  STREAM_END          0xF0
#define  STREAM_ESCAPE       0x55
#define  STREAM_ES_S         0x50   //转义字符 'S'
#define  STREAM_ES_E         0x05   //转义字符 'E'
//除了以上特殊字符外，其他都是原始字符

void serial_rx_tx_initialize(void)
{
    memset(&rx_ctl,0,sizeof(rx_ctl));
    memset(&tx_ctl,0,sizeof(tx_ctl));
}


void rx_find_next_empty_buffer(void) 
{
	unsigned int i;
	(rx_ctl.pcurrent_rx) = NULL;
	for(i=0;i<RX_PACKS_MAX_NUM;i++) {
		if(!(((rx_ctl.rx_packs)[i]).finished)) {
			rx_ctl.pcurrent_rx = &((rx_ctl.rx_packs)[i]);
			break;
		}
	}
}

void dumphex(unsigned char hex)
{
  unsigned char ch;
  ch = hex >> 4;
  ch = (ch > 9)?(ch+'A' - 10):(ch+'0');
  send_uart1(ch);
  ch = hex & 0x0F;
  ch = (ch > 9)?(ch+'A' - 10):(ch+'0');
  send_uart1(ch);
}


void dumpdata(unsigned char * buf,unsigned int len)
{
  unsigned int i;
  send_uart1('\r');
  send_uart1('\n');
  for(i=0;i<len;i++) {
    dumphex(buf[i]);
    send_uart1(',');
  }
 // send_uart1('\r');
 // send_uart1('\n');
}

void pack_prase_in(unsigned char ch)
{
   DATA_RX_PACKET_T * prx;
   if((rx_ctl.pcurrent_rx) == NULL) {
	   rx_find_next_empty_buffer();
	   if((rx_ctl.pcurrent_rx) == NULL) {
		   return ;
	   }
   }
   prx = rx_ctl.pcurrent_rx;
   if(prx->finished) {
	   rx_find_next_empty_buffer();
	   prx = rx_ctl.pcurrent_rx;
	   if(prx == NULL) {
		   return ;
	   }
   }
   switch(prx->state)
   {
   case STREAM_IDLE:
     if(ch == STREAM_START) {
       prx->state = STREAM_NORMAL;
       prx->index = 0;
     }
     break;
   case STREAM_NORMAL:
     if(ch == STREAM_ESCAPE) {
       prx->state = STREAM_IN_ESC;
     } else if(ch == STREAM_START) {
       prx->state = STREAM_IDLE;
     } else if(ch == STREAM_END) {
       if(prx->index >= 3) {
         unsigned int crc = CRC16_INTTRUPT((prx->buffer),(prx->index-2));
         if(crc == LSB_BYTES_TO_WORD(&(prx->buffer[prx->index-2]))) {
         //dumpdata(prx->buffer,prx->index-2);
        // INV_P47_ON();
			 prx->index -= 2;
			 prx->look_up_times = 0;
             prx->finished = 1;
         }
       }
	   prx->state = STREAM_IDLE;
     } else {
       prx->buffer[prx->index++] = ch;
     }
     break;
   case STREAM_IN_ESC:
     if(ch == STREAM_ESCAPE) {
       prx->buffer[prx->index++] = STREAM_ESCAPE;
       prx->state = STREAM_NORMAL;
     } else if(ch == STREAM_ES_S) {
       prx->buffer[prx->index++] = STREAM_START;
       prx->state = STREAM_NORMAL;
     } else if(ch == STREAM_ES_E) {
       prx->buffer[prx->index++] = STREAM_END;
       prx->state = STREAM_NORMAL;
     } else {
       prx->state = STREAM_IDLE;
     }
     break;
   default:
     prx->state = STREAM_IDLE;
     break;
   }
   //溢出判断
   if(prx->index >= sizeof(prx->buffer)) {
     prx->state = STREAM_IDLE;
   }
}


#if 0
//中断调用函数
//这里面使用的函数全部独立编写
void pack_prase_in(unsigned char ch)
{ 
   DATA_RX_PACKET_T * prx;

  // send_uart1(ch);

   //return ;

   if(rx_ctl.pcurrent_rx == NULL) {
	   rx_find_next_empty_buffer();
	   if(rx_ctl.pcurrent_rx == NULL) {
		   return ;
	   }
   }
   prx = rx_ctl.pcurrent_rx;
   if(prx->finished) {
	   rx_find_next_empty_buffer();
	   prx = rx_ctl.pcurrent_rx;
	   if(prx == NULL) {
		   return ;
	   }
   }
   switch(prx->state)
   {
   case STREAM_IDLE:
     if(ch == STREAM_START) {
       prx->state = STREAM_NORMAL;
       prx->index = 0;
     }
     break;
   case STREAM_NORMAL:
     if(ch == STREAM_ESCAPE) {
       prx->state = STREAM_IN_ESC;
     } else if(ch == STREAM_START) {
       prx->state = STREAM_IDLE;
     } else if(ch == STREAM_END) {
       if(prx->index >= 3) {
         unsigned int crc;
         dumpdata(prx->buffer,prx->index);
         INV_P47_ON();
#if 0         
         crc = CRC16_INTTRUPT(prx->buffer,prx->index-2);
         if(crc == LSB_BYTES_TO_WORD(&prx->buffer[prx->index-2])) {
             
			 prx->index -= 2;
			 prx->look_up_times = 0;
             prx->finished = 1;
             if(THIS_INFO)printf("one rx  pack is finished\r\n");
         }
#endif
       }
	   prx->state = STREAM_IDLE;
     } else {
       prx->buffer[prx->index++] = ch;
     }
     break;
   case STREAM_IN_ESC:
     if(ch == STREAM_ESCAPE) {
       prx->buffer[prx->index++] = STREAM_ESCAPE;
       prx->state = STREAM_NORMAL;
     } else if(ch == STREAM_ES_S) {
       prx->buffer[prx->index++] = STREAM_START;
       prx->state = STREAM_NORMAL;
     } else if(ch == STREAM_ES_E) {
       prx->buffer[prx->index++] = STREAM_END;
       prx->state = STREAM_NORMAL;
     } else {
       prx->state = STREAM_IDLE;
     }
     break;
   default:
     prx->state = STREAM_IDLE;
     break;
   }
   //溢出判断
   if(prx->index >= sizeof(prx->buffer)) {
     prx->state = STREAM_IDLE;
     
   }
}

#endif





/*************************************************
 * 找到空的发送缓冲，意识是找到可以填充数据的缓冲
 */
DATA_TX_PACKET_T * find_next_empty_tx_buffer(void)
{
	DATA_TX_PACKET_T * ptx = NULL;
	unsigned int i;
	for(i=0;i<TX_PACKS_MAX_NUM;i++) {
		if(!tx_ctl.packet[i].finished) {
			ptx = &tx_ctl.packet[i];
		}
	}
	return ptx;
}

/*************************************************
 * 找到已经填充完成的指针，然后用串口等等发送出去
 */
DATA_TX_PACKET_T * find_ready_tx_buffer(void)
{
	DATA_TX_PACKET_T * ptx = NULL;
	unsigned int i;
	for(i=0;i<TX_PACKS_MAX_NUM;i++) {
		if(tx_ctl.packet[i].finished) {
			ptx = &tx_ctl.packet[i];
		}
	}
	return ptx;
}


DATA_TX_PACKET_T * prase_in_buffer(unsigned char * src,unsigned int len)
{
	 DATA_TX_PACKET_T * ptx = NULL;
	 unsigned int i = 0;
	 if(len == 0) {
		 return NULL;
	 }
	 if((ptx = find_next_empty_tx_buffer()) == NULL) {
		 return NULL;
	 }
	 if(1) {
         unsigned int crc = CRC16(src,len);
		 ptx->index = 0;
		 ptx->buffer[ptx->index++] = STREAM_START;
		 while(len--) {
			 unsigned char reg = src[i++];
			 if(ptx->index >= (sizeof(ptx->buffer) - 4)) {
				 //溢出判断,2字节CRC，一个结束位,一个转义字符
				 ptx->index = 0;
				 break;
			 }
			 if(reg == STREAM_START) {
				 ptx->buffer[ptx->index++] = STREAM_ESCAPE;
				 ptx->buffer[ptx->index++] = STREAM_ES_S;
		     } else if(reg == STREAM_ESCAPE) {
				 ptx->buffer[ptx->index++] = STREAM_ESCAPE;
				 ptx->buffer[ptx->index++] = STREAM_ESCAPE;
			 } else if(reg == STREAM_END) {
				 ptx->buffer[ptx->index++] = STREAM_ESCAPE;
				 ptx->buffer[ptx->index++] = STREAM_ES_E;
			 } else {
				 ptx->buffer[ptx->index++] = reg;
			 }
		 }
		 if(ptx->index > 0) {
		     ptx->buffer[ptx->index++] = crc & 0xFF;
		     ptx->buffer[ptx->index++] = crc >> 8;
             ptx->buffer[ptx->index++] = STREAM_END;
		     ptx->finished = 1;
		 }
	 }
	 return ptx;
}

unsigned int tx_pack_and_send(unsigned char * src,unsigned int len)
{
	//立即发送，只有发完之后才能返回
	DATA_TX_PACKET_T * ptx = prase_in_buffer(src,len);
	if(ptx == NULL) {
        if(THIS_ERROR)printf("tx_pack_and_send start ptx == NULL ERROR! len(%d)\r\n",len);
		return 0;
	}
	if(ptx->finished) {
		//立即发送
        unsigned int i;
        //if(THIS_INFO)printf("tx_pack_and_send start send:\r\n");
        for(i=0;i<ptx->index;i++) {
            send_uart1(ptx->buffer[i]);
        }
        ptx->finished = 0;
        //连续多发几个指令，这样就给网络处理芯片很多时间处理接收有用的数据
	} else {
        if(THIS_ERROR)printf("ptx->finished ERROR.\r\n");
    }
	return ptx->index;
}

unsigned int tx_pack_and_post(unsigned char * src,unsigned int len)
{
	//不发送
	DATA_TX_PACKET_T * ptx = prase_in_buffer(src,len);
    //启动发送中断
    if(ptx) {
        return (ptx->finished)?len:0;
    } else {
        return 0;
    }
}




DATA_RX_PACKET_T * GetFinishedPacket(void)
{
	unsigned int i;
	DATA_RX_PACKET_T * prx;
	for(i=0;i<RX_PACKS_MAX_NUM;i++) {
		prx = &rx_ctl.rx_packs[i]; 
		if(prx->finished) {
			break;
		}
	}
	if(i == RX_PACKS_MAX_NUM) {
		return NULL;
	} else {
		return prx;
	}
}


void rx_look_up_packet(void)
{
	unsigned int i;
	DATA_RX_PACKET_T * prx;
	for(i=0;i<RX_PACKS_MAX_NUM;i++) {
		prx = &rx_ctl.rx_packs[i]; 
		if(prx->finished) {
            prx->look_up_times++;
            if(THIS_ERROR)printf("see it.\r\n");
		}
	}
}

void rx_free_useless_packet(unsigned int net_communication_count)
{
	unsigned int i;
	DATA_RX_PACKET_T * prx;
	for(i=0;i<RX_PACKS_MAX_NUM;i++) {
		prx = &rx_ctl.rx_packs[i]; 
		if(prx->finished) {
            if(THIS_INFO)printf("start free useless packet!\r\n");
            if(net_communication_count > 0) { //有通信指令
			    if(prx->look_up_times >= net_communication_count) {
   				    //发现这条指令没人需要，看看是否系统可接受的默认指令
				    handle_modbus_force_cmd(prx->buffer,prx->index);
				    prx->finished = 0;
                    if(1)printf("clear it\r\n");
			    } else {
                    if(1)printf("save it\r\n");
                }
            } else {
                //没有通信指令
                handle_modbus_force_cmd(prx->buffer,prx->index);
                prx->finished = 0;
            }
		}
	}
}









//-----------------------------------------------------------
// MODBUS读指令
//-----------------------------------------------------------


unsigned char modbus_read_multi_coils_request(unsigned int start_coils,unsigned int coils_num,unsigned char slave_device)
{
    struct modbus_read_coil_status_req_type
    {
		unsigned char slave_addr;
		unsigned char function;
		unsigned char start_addr_hi;
		unsigned char start_addr_lo;
		unsigned char reg_munber_hi;
		unsigned char reg_number_lo;
		unsigned char crc_hi;
		unsigned char crc_lo;
	};
	struct modbus_read_coil_status_req_type tx;
	unsigned int crc;
	tx.slave_addr = slave_device;
	tx.function = 0x01;  //READ_COIL_STATUS
	tx.start_addr_hi = start_coils >> 8;
	tx.start_addr_lo = start_coils & 0xFF;
	tx.reg_munber_hi = coils_num >> 8;
	tx.reg_number_lo = coils_num & 0xFF;
	crc = CRC16((unsigned char *)&tx,sizeof(tx)-2);
	tx.crc_hi = crc >> 8;
	tx.crc_lo = crc & 0xFF;
	return tx_pack_and_send((unsigned char *)&tx,sizeof(tx));
}

unsigned int modbus_prase_read_multi_coils_ack(unsigned char slave_device,unsigned char * rx_buffer,unsigned int len,unsigned int startbit,unsigned int count)
{
	typedef struct __modbus_force_multiple_coils_req_type
	{
		unsigned char slave_addr;
		unsigned char function;
		unsigned char start_addr_hi;
		unsigned char start_addr_lo;
		unsigned char quantiry_coils_hi;
		unsigned char quantiry_coils_lo;
		unsigned char byte_count;
		unsigned char database;
	} modbus_force_multiple_coils_req_type;
	modbus_force_multiple_coils_req_type * pack = (modbus_force_multiple_coils_req_type *)rx_buffer;
    count = count;
	if(len > sizeof(modbus_force_multiple_coils_req_type) && pack->slave_addr == slave_device) {
		if(pack->function == 0x01) {  //READ_COIL_STATUS
			if(startbit == HSB_BYTES_TO_WORD(&pack->start_addr_hi)) {
				unsigned int count = HSB_BYTES_TO_WORD(&pack->start_addr_hi);
				if(count > 0) {
					//根据读到的数据，修改某些寄存器的位
					return count;
				}
			}
		}
	}
	return 0;
}

