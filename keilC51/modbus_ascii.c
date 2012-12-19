/************************************************************
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hal_io.h"
#include "hal_stc_io.h"
#include "eeprom.h"
#include "plc_command_def.h"
#include "plc_prase.h"
#include "sys_info.h"
#include "modbus_rtu.h"
#include "modbus_ascii.h"


unsigned char broadcast;


unsigned char rx_buffer[MODBUS_MAX_RX_LEN];
unsigned char rx_index = 0;




extern unsigned int CRC16(unsigned char *Array,unsigned int Len);

unsigned char get_lrc_sum(unsigned char *buffer,unsigned char length)
{
	unsigned char i;
	unsigned char sum = 0;
	for(i=0;i<length;i++) {
		sum += buffer[i];
	}
	return (unsigned char)(0-sum);
}


unsigned char StringHex2Byte(const char * pstr)
{
	unsigned char tmp = 0;
	unsigned char ch = *pstr++;
	if(ch >= 'A' && ch <= 'F') {
		tmp = ch - 'A' + 10;
	} else if(ch >= 'a' && ch <= 'f') {
		tmp = ch - 'a' + 10;
	} else if(ch >= '0' && ch <= '9') {
		tmp = ch - '0';
	}
	tmp <<= 4;
	ch = *pstr;
	if(ch >= 'A' && ch <= 'F') {
		tmp |= ch - 'A' + 10;
	} else if(ch >= 'a' && ch <= 'f') {
		tmp |= ch - 'a' + 10;
	} else if(ch >= '0' && ch <= '9') {
		tmp |= ch - '0';
	}
	return tmp;
}

void Byte2StringHex(char * pstr,unsigned char ch)
{
	unsigned char c = ch & 0xF0;
	c >>= 4;
	if(c < 10) {
		pstr[0] = c + '0';
	} else {
		pstr[0] = c + 'A' - 10;
	}
	c = ch & 0x0F;
	if(c < 10) {
		pstr[1] = c + '0';
	} else {
		pstr[1] = c + 'A' - 10;
	}
	pstr[2] = 0;
}

void StringHex2Buffer(unsigned char * buffer,const char * pstr,unsigned int len)
{
	unsigned int i;
	for(i=0;i<len;i++) {
		buffer[i] = StringHex2Byte(&pstr[i*2]);
	}
}

void Buffer2StringHex(unsigned char * strbuf,const char * buffer,unsigned int len)
{
	unsigned int i;
	for(i=0;i<len;i++) {
		Byte2StringHex(&strbuf[i*2],buffer[i]);
	}
}



void force_single_coil(unsigned int address,unsigned char onoff)
{
	//根据版本，设置不用的继电器数量
    unsigned char reg = onoff?0x01:0x00;
    io_out_set_bits(address,&reg,1);
}


void read_coil_status(unsigned int address,unsigned int number,unsigned char rtu_mode)
{
    struct modbus_read_coil_status_ack_type pack;
	//根据情况，填充rx_buffer
	//然后直接返回
    unsigned int len = io_out_get_bits(address,pack.data_arry,number);
    pack.byte_count = BITS_TO_BS(len);
    pack.slave_addr = sys_info.modbus_addr;
    pack.function = FUNC_READ_COIL_STATUS;
    len = CRC16((void *)&pack,sizeof(pack)-2);
    pack.crc_hi = len >> 8;
    pack.crc_lo = len & 0xFF;
	if(rtu_mode) {
		fwrite((void *)&pack,sizeof(char),sizeof(pack),0);
	} else {
		Buffer2StringHex(rx_buffer,(void *)&pack,sizeof(pack));
	    fwrite((void *)rx_buffer,sizeof(char),sizeof(pack)*2,0);
	}
}



void force_multiple_coils(char * hexbuf,unsigned int len,unsigned char rtu_mode)
{
    struct modbus_force_multiple_coils_req_type * pmodbus = (struct modbus_force_multiple_coils_req_type *)hexbuf;
    //设置继电器
    unsigned int addr = pmodbus->start_addr_hi;
    unsigned int coils = pmodbus->quantiry_coils_hi;
    addr <<= 8;  coils <<= 8;
    addr |= pmodbus->start_addr_lo;
    coils  |= pmodbus->quantiry_coils_lo;
    io_out_set_bits(addr,&pmodbus->force_data_base,coils);
    //返回应答
    len = len;
	if(!broadcast)
	{
        struct modbus_force_multiple_coils_ack_type ack;
        ack.slave_addr = sys_info.modbus_addr;
        ack.function = FUNC_FORCE_MULTIPLE_COILS;
        ack.start_addr_hi = pmodbus->start_addr_hi;
        ack.start_addr_lo = pmodbus->start_addr_lo;
        ack.quantiry_coils_hi = pmodbus->quantiry_coils_hi;
        ack.quantiry_coils_lo = pmodbus->quantiry_coils_lo;
		if(rtu_mode) {
			unsigned int crc = CRC16((void *)&ack,sizeof(struct modbus_force_multiple_coils_ack_type) - 2);
            ack.crc_hi = crc >> 8;
            ack.crc_lo = crc & 0xFF;
			fwrite((void *)&ack,sizeof(char),sizeof(struct modbus_force_multiple_coils_ack_type) - 2,0);
		} else {
            rx_buffer[14] = get_lrc_sum((void *)&ack,sizeof(struct modbus_force_multiple_coils_ack_type)-2);
            Buffer2StringHex(&rx_buffer[1],(void *)&ack,sizeof(struct modbus_force_multiple_coils_ack_type));
	        rx_buffer[0] = ':';
	        rx_buffer[15] = 0x0D;
	        rx_buffer[16] = 0x0A;
		   // SysDelayUs(8000);
	      //  fwrite(rx_buffer,sizeof(char),17,stream_max485);
		}
	}
}

void read_holding_register(void * preq_in)
{
    struct modbus_read_holding_register_req_type * preq =
	    (struct modbus_read_holding_register_req_type *)preq_in;
	unsigned int start_addr = preq->start_addr_hi;
	unsigned int reg_number = preq->reg_number_hi;
	start_addr <<= 8;
	start_addr |= preq->start_addr_lo;
	reg_number <<= 8;
	reg_number |= preq->reg_number_lo;
	{
	    unsigned int crc,len;
	    struct modbus_read_holding_register_ack_type * pack = 
		       (struct modbus_read_holding_register_ack_type *)preq;
		eeprom_read(start_addr,&(pack->data_base),reg_number);
		pack->byte_count = reg_number;
		len = GET_OFFSET_MEM_OF_STRUCT(struct modbus_read_holding_register_ack_type,data_base);
		len += reg_number;
		crc = CRC16((void *)pack,len);
		{
		    struct modbus_crc_type * pcrc = (struct modbus_crc_type *)&rx_buffer[len];
			pcrc->crc_hi = crc >> 8;
			pcrc->crc_lo = crc & 0xFF;
		}
		len += 2; //加上CRC的长度
		fwrite((void *)pack,sizeof(char),len,0);
	}
}

/******************************************
 * 参数:
 * preqbuf  : 输入原始指令
 */
void modbus_preset_multiple_register(void * preqbuf)
{
    struct modbus_preset_multiple_register_req_type * preq = preqbuf;
	unsigned int start_addr = preq->start_addr_hi;
	start_addr <<= 8;
	start_addr |= preq->start_addr_lo;
	//把这些数据往EEPROM上写
	if(preq->start_addr_hi == 0xFF) {
		eeprom_secotr_erase(preq->start_addr_lo);
	} else {
	    eeprom_write(start_addr,&preq->data_base,preq->byte_count);
	}
	{
	    struct modbus_preset_multiple_register_ack_type * pack = 
		    (struct modbus_preset_multiple_register_ack_type *)preqbuf;
	    unsigned char crc = CRC16((void *)pack,sizeof(struct modbus_preset_multiple_register_ack_type)-2);
	   	pack->crc_hi = crc >> 8;
	    pack->crc_lo = crc & 0xFF;
		fwrite((void *)pack,sizeof(char),sizeof(struct modbus_preset_multiple_register_ack_type),0);
	}
}

void UartReceivetoModbusAscii(unsigned char ch)
{
	if(ch == ':') {
		rx_index = 0;  //帧同步
	}
	rx_buffer[rx_index++] = ch;
prase_again:
	if(rx_index == 0) {
		return ;
	}
	//判断:号
	if(rx_index >= 1) {
		if(rx_buffer[0] != ':') {
			rx_index--;
			goto prase_again;
		}
	}
	//判断地址
	broadcast = 0;
	if(rx_index >= 3) {
		unsigned char deviceaddr = StringHex2Byte(&rx_buffer[1]);
		if(deviceaddr == 0) {
		    broadcast = 1;
		} else if(sys_info.modbus_addr != deviceaddr) {
			rx_index -= 3;
			goto prase_again;
		}
	}
	//判断功能
	if(rx_index >= 5) {
		unsigned char function = StringHex2Byte(&rx_buffer[3]);
		switch(function)
		{
			case FUNC_FORCE_SINGLE_COIL:
		    {
				if(rx_index >= 13) {
					if(StringHex2Byte(&rx_buffer[11]) != 0x00) {
						rx_index -= 13;
						goto prase_again;
					}
					if(rx_index >= 17) {
						unsigned char hexbuf[7];
						//检查CRLF
						if(rx_buffer[15] != 0x0D || rx_buffer[16] != 0x0A) {
							rx_index -= 17;
							goto prase_again;
						}
						//转换为二进制
						StringHex2Buffer(hexbuf,&rx_buffer[1],7);
						//检查校验和
						if(hexbuf[6] != get_lrc_sum(&hexbuf[0],6)) {
							rx_index -= 17;
							goto prase_again;
						}
						//一切都对了，执行
						{
							unsigned int address = hexbuf[2];
							address <<= 8;
							address |= hexbuf[3];
							force_single_coil(address,hexbuf[4]);
							if(!broadcast) {
							    fwrite(rx_buffer,sizeof(char),17,0);
							}
							rx_index -= 17;
							goto prase_again;
						}
					}
				}
		    }
		    break;
		case FUNC_READ_HOLDING_REGISTER:
		    {
				if(rx_index >= 17) {
					unsigned char hexbuf[7];
					//检查CRLF
					if(rx_buffer[15] != 0x0D || rx_buffer[16] != 0x0A) {
						rx_index -= 17;
						goto prase_again;
					}
					//转换为二进制
					StringHex2Buffer(hexbuf,&rx_buffer[1],7);
					//检查校验和
					if(hexbuf[6] != get_lrc_sum(&hexbuf[0],6)) {
						rx_index -= 17;
						goto prase_again;
					}
					//一切都对了，执行
					if(!broadcast) {
						unsigned int number;
						unsigned int address = hexbuf[2];
						address <<= 8;
						address |= hexbuf[3];
						number = hexbuf[4];
						number <<= 8;
						number |= hexbuf[5];
						//read_holding_register(address,number);
						rx_index = 0;
						goto prase_again;
					} else {
					    rx_index = 0; //不支持广播
					    goto prase_again;
					}
				}
		    }
		break;
		case FUNC_READ_COIL_STATUS:
			{
				if(rx_index >= 17) {
					unsigned char hexbuf[7];
					//检查CRLF
					if(rx_buffer[15] != 0x0D || rx_buffer[16] != 0x0A) {
						rx_index -= 17;
						goto prase_again;
					}
					//转换为二进制
					StringHex2Buffer(hexbuf,&rx_buffer[1],7);
					//检查校验和
					if(hexbuf[6] != get_lrc_sum(&hexbuf[0],6)) {
						rx_index -= 17;
						goto prase_again;
					}
					//一切都对了，执行
					if(!broadcast) {
						unsigned int number;
						unsigned int address = hexbuf[2];
						address <<= 8;
						address |= hexbuf[3];
						number = hexbuf[4];
						number <<= 8;
						number |= hexbuf[5];
						read_coil_status(address,number,0);
						rx_index = 0;
						goto prase_again;
					} else {
					    rx_index = 0; //不支持广播
					    goto prase_again;
					}
				}
			}
			break;
		case FUNC_FORCE_MULTIPLE_COILS:
			{
				if(rx_index >= 21 && (rx_index&0x01)?1:0) {
					//必须是奇数
					//检查倒数两个字符是不是结束字符\r\n，如果是，则完整帧
					if(rx_buffer[rx_index-2] == 0x0D && rx_buffer[rx_index-1] == 0x0A) {
						//完整的,则解析
						unsigned int bin_len = (rx_index-3)/2;
						unsigned char hexbuffer[128];
						StringHex2Buffer(hexbuffer,&rx_buffer[1],bin_len);
						//判断校验
					    if(hexbuffer[bin_len-1] != get_lrc_sum(&hexbuffer[0],bin_len-1)) {
						    rx_index -= rx_index;
						    goto prase_again;
					    }
						//解析命令
						force_multiple_coils(hexbuffer,bin_len,0);
						rx_index -= rx_index;
						goto prase_again;
					}
				}
			}
			break;
		default:
		    rx_index -= 5;
		    goto prase_again;
		}
	}
}

