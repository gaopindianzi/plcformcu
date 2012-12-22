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
#include "compiler.h"
#include "serial_comm_packeter.h"
#include "bin_command_def.h"
#include "bin_command_handle.h"

#define  THIS_INFO  1
#define  THIS_ERROR 1


unsigned int CRC16(unsigned char *Array,unsigned int Len)
{
	WORD  IX,IY,CRC;
	CRC=0xFFFF;//set all 1
	if (Len<=0) {
		CRC = 0;
	} else {
		Len--;
		for (IX=0;IX<=Len;IX++)
		{
			CRC=CRC^(WORD)(Array[IX]);
			for(IY=0;IY<=7;IY++) {
				if ((CRC&1)!=0) {
					CRC=(CRC>>1)^0xA001;
				} else {
					CRC=CRC>>1;
				}
			}
		}
	}
	return CRC;
}





#if 0

extern void read_coil_status(unsigned int address,unsigned int number,unsigned char rtu_mode);
extern void force_single_coil(unsigned int address,unsigned char onoff);
extern unsigned char broadcast;

unsigned long last_rx_time;

void UartReceivetoModbusRtuTimeTick(void)
{
  if(rx_index > 0 && ((get_sys_clock() - last_rx_time) >= TICK_SECOND/10)) {
      last_rx_time = get_sys_clock();
	  rx_index = 0;
  }
}

void UartReceivetoModbusRtu(unsigned char ch)
{
    last_rx_time = get_sys_clock();
	rx_buffer[rx_index++] = ch;
prase_again:
	if(rx_index == 0) {
		return ;
	}
	if(rx_index >= 1) {
		broadcast = 0;
		if(rx_buffer[0] == 0) {
			broadcast = 1;
		} else if(rx_buffer[0] != sys_info.modbus_addr) {
			rx_index -= 1;
			goto prase_again;
		}
	}
	if(rx_index >= 2) {
		unsigned char fun = rx_buffer[1];
		switch(fun)
		{
		case FUNC_READ_COIL_STATUS:
			{
				if(rx_index >= 8) {
					unsigned int tmp,number;
					if(broadcast) {
						rx_index = 0;
						goto prase_again;
					}
					tmp = rx_buffer[7];
					tmp <<= 8;
					tmp |= rx_buffer[6];
					if(tmp != CRC16(rx_buffer,6)) {
						rx_index -= 8;
						goto prase_again;
					}
					tmp = rx_buffer[2];
					tmp <<= 8;
					tmp |= rx_buffer[3];
					number = rx_buffer[4];
					number <<= 8;
					number |= rx_buffer[5];
					read_coil_status(tmp,number,1);
					rx_index = 0;
					goto prase_again;
				}
			}
			break;
		case FUNC_FORCE_SINGLE_COIL:
			{
				if(rx_index >= sizeof(struct modbus_force_single_coil_req_type)) {
                    struct modbus_force_single_coil_req_type * preq = (struct modbus_force_single_coil_req_type *)rx_buffer;
                    unsigned int address = preq->start_addr_hi;
                    unsigned char onoff = preq->reg_munber_hi;
                    unsigned int crc = preq->crc_hi;
                    dump_data(preq,sizeof(struct modbus_force_single_coil_req_type));
                    address <<= 8;
                    address |= preq->start_addr_lo;
                    crc <<= 8;
                    crc |= preq->crc_lo;
                    if(crc != CRC16((void *)preq,sizeof(struct modbus_force_single_coil_req_type)-2)) {
					    //rx_index = 0;
					    //goto prase_again;
                    }
					//dump_data(preq,rx_index);
					force_single_coil(address,onoff);
					rx_index = 0;
					goto prase_again;
				}
			}
			break;
		case FUNC_FORCE_MULTIPLE_COILS:
			{
				if(rx_index >= 10) {
					unsigned int tmp;
					unsigned char bytecount = rx_buffer[6];
					//判断数据的有效性
					tmp = rx_buffer[rx_index-1];
					tmp <<= 8;
					tmp |= rx_buffer[rx_index-2];
					if((rx_index-9) == bytecount) {
					    if(tmp != CRC16(rx_buffer,rx_index-2)) {
						    rx_index = 0;  //错误过多
						    goto prase_again;
						} else {
							force_multiple_coils(rx_buffer,rx_index,1);
							rx_index = 0;
							goto prase_again;
						}
					} else if((rx_index-9) > bytecount) {
						//接收了过多的而数据了，无效
						rx_index = 0;  //错误过多，从来
						goto prase_again;
					} else if(rx_index >= 17) {
						rx_index = 0;  //错误过多，从来
						goto prase_again;
					}
				}
			}
			break;
		case FUNC_READ_HOLDING_REGISTER:
		    {
			    struct modbus_read_holding_register_req_type * preq =
				    (struct modbus_read_holding_register_req_type *)rx_buffer;
				if(rx_index >= sizeof(struct modbus_read_holding_register_req_type)) {
				    unsigned int crc = preq->crc_hi;
					crc <<= 8;
					crc |= preq->crc_lo;
					if(crc != CRC16((void *)preq,sizeof(struct modbus_read_holding_register_req_type)-2)) {
					    //rx_index = 0;
						//goto prase_again;
					}
					//解析
					read_holding_register(preq);
					rx_index = 0;
					goto prase_again;
				}
			}
			break;
        case FUNC_PRESET_MULTIPLE_REGISTERS:
            {
                struct modbus_preset_multiple_register_req_type * preq = 
				    (struct modbus_preset_multiple_register_req_type *)rx_buffer;
                if(rx_index >= sizeof(struct modbus_preset_multiple_register_req_type)) {
                    unsigned int curr_byte_count = rx_index - 
					    GET_OFFSET_MEM_OF_STRUCT(struct modbus_preset_multiple_register_req_type,data_base) - 2; //有两个字节是CRC
                    if(curr_byte_count == preq->byte_count) {  
					    struct modbus_crc_type * pcrc = (struct modbus_crc_type *)&rx_buffer[rx_index-2];
                        unsigned int crc  = pcrc->crc_hi;
						crc <<= 8;
						crc |= pcrc->crc_lo;
						if(crc != CRC16((void *)preq,rx_index - 2)) {
						    //rx_index = 0; //错误的
						    //goto prase_again;
						}
						//执行保持保持寄存器写操作
						//dump_data(preq,rx_index);
						modbus_preset_multiple_register(preq);
						rx_index = 0;
						goto prase_again;
                    }
                } else if(rx_index >= 128) {
					rx_index = 0; //错误过多
					goto prase_again;
				}
            }
            break;
		default:
			rx_index -= 2;
			goto prase_again;
			break;
		}
	}
}



#endif



void modbus_force_multiple_coils(unsigned char * buffer,unsigned int len)
{
    unsigned int i;
    unsigned int crc;
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
	mb_force_mulcoils_req_t * pm = (mb_force_mulcoils_req_t *)&buffer[sizeof(APP_PACK_HEAD_T)];
    unsigned int md_len = len - sizeof(APP_PACK_HEAD_T);
	crc = HSB_BYTES_TO_WORD(&(((char *)pm)[md_len - 2]));
    if(THIS_INFO)printf("+modbus_force_multiple_coils\r\n");
	if(crc != CRC16((char*)pm,md_len-2)) {
		if(THIS_INFO)printf("++CRC ERROR\r\n");
		//return ;
	}
    i = HSB_BYTES_TO_WORD(&pm->quantiry_coils_hi);
	if(pm->byte_count == BITS_TO_BS(i)) {
        //字节数跟比特数符合长度规则
        if(THIS_INFO)printf("+byte_count ok\r\n");
		i = GET_OFFSET_MEM_OF_STRUCT(mb_force_mulcoils_req_t,force_data_base);
        if(pm->byte_count == (md_len - i - 2)) {
			//总长度减去头，减去CRC，也刚好等于规定字节的长度
			//那么这个指令时正确的
			unsigned int i;
			unsigned int start = HSB_BYTES_TO_WORD(&pm->start_addr_hi);
			for(i=0;i<HSB_BYTES_TO_WORD(&pm->quantiry_coils_hi);i++) {
				set_bitval(start+i,BIT_IS_SET(&pm->force_data_base,i));
			}
			//然后返回应答
			{
				struct modbus_force_multiple_coils_ack_type * pack = (struct modbus_force_multiple_coils_ack_type *)pm;
				crc = CRC16((char*)pm,sizeof(struct modbus_force_multiple_coils_ack_type) - 2);
				pack->crc_hi = crc >> 8;
				pack->crc_lo = crc & 0xFF;
                tx_pack_and_send((unsigned char *)buffer,sizeof(struct modbus_force_multiple_coils_ack_type)+sizeof(APP_PACK_HEAD_T));
			}
		} else {
			if(THIS_ERROR)printf("ERROR@: pm->byte_count == (len - i - 2)!\r\n");
		}
	} else {
		if(THIS_ERROR)printf("ERROR@: pm->byte_count == (len - i - 2)!\r\n");
	}
    if(THIS_INFO)printf("-modbus_force_multiple_coils\r\n");
}


/***************************************
 * 根据协议，这里处理PLC里面没用到的指令，
 * 包括2000端口号发出来的指令，前面两个字节即是端口号，第三字节代表其他含义
 * 上层应用发过来的不同应用，利用前面三字节区分出来，然后读处理，然后再发回去，
 * 上层应用也根据这三自己区分不同的应用，然后处理它自己的应答
 */

void handle_modbus_force_cmd(unsigned char * buffer,unsigned int len)
{
    unsigned int port_num;
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    if(len < sizeof(APP_PACK_HEAD_T)) {
        //长度错误，免谈
        return ;
    }
	//是这个设备
    port_num = HSB_BYTES_TO_WORD(&papph->port_num_hi);
    if(port_num == 502) {
	    modbus_head_t * ph = (modbus_head_t *)&buffer[sizeof(APP_PACK_HEAD_T)];  //后面几个字节才是modbus应用
        unsigned int mb_len = len - sizeof(APP_PACK_HEAD_T);
        if(mb_len < sizeof(modbus_head_t)) {
            if(THIS_ERROR)printf("modbus protocal modbus head error!\r\n");
            return ;
        }
	    if(ph->slave_addr != sys_info.modbus_addr) {
	 	   //不是这个设备
		    if(THIS_ERROR)printf("handle modbus: not this device(0x%x)!\r\n",ph->slave_addr);
		    return ;
	    }
        dumpdata((char*)ph,len-3);
	    switch(ph->function) {
		    case FUNC_FORCE_MULTIPLE_COILS:
			    modbus_force_multiple_coils(buffer,len);
			    break;
		    default:
                //如果却是没有人需要，最好也上报一下，对上层领导报告，下层没有办法处理，不支持该指令!
                //502端口号的错误报告，须查看modbus协议
                {
                    APP_PACK_ERROR_T * perr = (APP_PACK_ERROR_T *)buffer;
                    perr->error_hi = NOT_SUPPORT_THIS_COMMAND >> 8; //
                    perr->error_lo = NOT_SUPPORT_THIS_COMMAND & 0xFF;
                    tx_pack_and_send(buffer,sizeof(APP_PACK_ERROR_T));
                }
			    break;
	    }
    } else if(port_num == 2000) {
        CmdHead * pch = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
        if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead)) {
            //长度错误
            if(THIS_ERROR)printf("bin cmd head len error!\r\n");
            return ;
        }
        switch(pch->cmd)
        {
	    case CMD_READ_REGISTER:     CmdReadRegister(buffer,len);     break;
    	case CMD_WRITE_REGISTER:    CmdWriteRegister(buffer,len);    break;
	    case CMD_GET_IO_OUT_VALUE:  CmdGetIoOutValue(buffer,len);    break;
    	case CMD_SET_IO_OUT_VALUE:  CmdSetIoOutValue(buffer,len);    break;
	    case CMD_REV_IO_SOME_BIT:   CmdRevertIoOutIndex(buffer,len); break;
    	case CMD_SET_IO_ONE_BIT:    CmdSetClrVerIoOutOneBit(buffer,len,0);    break;
	    case CMD_CLR_IO_ONE_BIT:    CmdSetClrVerIoOutOneBit(buffer,len,1);    break;
    	case CMD_REV_IO_ONE_BIT:    CmdSetClrVerIoOutOneBit(buffer,len,2);    break;
	    case CMD_GET_IO_IN_VALUE:   CmdGetIoInValue(buffer,len);     break;
    	case CMD_SET_IO_OUT_POWERDOWN_HOLD:  CmdSetIoOutPownDownHold(buffer,len);  break;
	    case CMD_GET_IO_OUT_POWERDOWN_HOLD:  CmdGetIoOutPownDownHold(buffer,len);  break;
        default:
            {
                //上报，这个指令没法处理
                pch->cmd_option = 0x00;
                pch->cmd_len    = 0;
                tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
            }
            break;
        }
    } else {
        //这个端口没有人处理，那么，上位机肯定也是发错了。
    }
}

