#include "bin_command_def.h"
#include "serial_comm_packeter.h"
#include "plc_prase.h"
#include "hal_io.h"
#include <stdio.h>
#include <string.h>


#define  THISINFO   1
#define  THISERROR  1


void   CmdReadRegister(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdRegisterHead)) {
        goto len_error;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
}







void   CmdWriteRegister(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdRegisterHead)) {
        goto len_error;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
}








void   CmdGetIoOutValue(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    CmdIoValue * rio = (CmdIoValue *)GET_CMD_DATA(pcmd);
    if(THISINFO)printf("+CmdSetIoOutValue\r\n");
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead)) {
        if(THISERROR)printf("cmd set io out len error!\r\n");
        goto len_error;
    }
    //输出
    {
        unsigned int i;
        CmdIoValue * tio = (CmdIoValue *)GET_CMD_DATA(pcmd);
        memset(&tio->io_value,0,sizeof(tio->io_value));
        tio->io_count = (tio->io_count > IO_OUTPUT_COUNT)?IO_OUTPUT_COUNT:tio->io_count;
    	for(i=0;i<tio->io_count;i++) { //协议支持最大输出
            unsigned char bitval = get_bitval(IO_OUTPUT_BASE+i);
            SET_BIT(&(tio->io_value),i,bitval);
		}
        pcmd->cmd_option = 0x01;
        tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead)+sizeof(CmdIoValue));
        if(THISINFO)printf("+CmdSetIoOutValue ok\r\n");
        return ;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
    if(THISINFO)printf("+CmdSetIoOutValue ko\r\n");
}






void   CmdSetIoOutValue(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    CmdIoValue * rio = (CmdIoValue *)GET_CMD_DATA(pcmd);
    if(THISINFO)printf("+CmdSetIoOutValue\r\n");
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdIoValue)) {
        if(THISERROR)printf("cmd set io out len error!\r\n");
        goto len_error;
    }
    if(rio->io_count <= IO_OUTPUT_COUNT) {
        unsigned int i;
    	for(i=0;i<rio->io_count;i++) {
			set_bitval(IO_OUTPUT_BASE+i,BIT_IS_SET(&rio->io_value,i));
		}
    } else {
        if(THISERROR)printf(" io_count too big\r\n");
        goto len_error;
    }
    //输出
    {
        unsigned int i;
        CmdIoValue * tio = (CmdIoValue *)GET_CMD_DATA(pcmd);
        memset(&tio->io_value,0,sizeof(tio->io_value));
        tio->io_count = (tio->io_count > IO_OUTPUT_COUNT)?IO_OUTPUT_COUNT:tio->io_count;
    	for(i=0;i<tio->io_count;i++) { //协议支持最大输出
            unsigned char bitval = get_bitval(IO_OUTPUT_BASE+i);
            SET_BIT(&(tio->io_value),i,bitval);
		}
        pcmd->cmd_option = 0x01;
        tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead)+sizeof(CmdIoValue));
        if(THISINFO)printf("+CmdSetIoOutValue ok\r\n");
        return ;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
    if(THISINFO)printf("+CmdSetIoOutValue ko\r\n");
}








void   CmdRevertIoOutIndex(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    CmdIobitmap * rio = (CmdIobitmap *)GET_CMD_DATA(pcmd);
    if(THISINFO)printf("+CmdRevertIoOutIndex\r\n");
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdIobitmap)) {
        if(THISERROR)printf("cmd set io out len error!\r\n");
        goto len_error;
    }
    {
      unsigned int i;
      for(i=0;i<IO_OUTPUT_COUNT;i++) {
          if(BIT_IS_SET(&rio->io_msk,i)) {
              unsigned char bitval = get_bitval(IO_OUTPUT_BASE+i);
		      set_bitval(IO_OUTPUT_BASE+i,!bitval);
          }
	  }
    }
    //输出
    {
        unsigned int i;
        CmdIoValue * tio = (CmdIoValue *)GET_CMD_DATA(pcmd);
        memset(&tio->io_value,0,sizeof(tio->io_value));
        tio->io_count = IO_OUTPUT_COUNT;
    	for(i=0;i<tio->io_count;i++) { //协议支持最大输出
            unsigned char bitval = get_bitval(IO_OUTPUT_BASE+i);
            SET_BIT(&(tio->io_value),i,bitval);
		}
        pcmd->cmd_option = 0x01;
        tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead)+sizeof(CmdIoValue));
        if(THISINFO)printf("+CmdRevertIoOutIndex ok\r\n");
        return ;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
    if(THISINFO)printf("+CmdRevertIoOutIndex ko\r\n");
}







void   CmdSetClrVerIoOutOneBit(unsigned char * buffer,unsigned int len,unsigned char mode)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    mode = mode;
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdRegisterHead)) {
        goto len_error;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
}






void   CmdGetIoInValue(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    if(THISINFO)printf("+CmdSetIoOutValue\r\n");
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead)) {
        if(THISERROR)printf("cmd set io out len error!\r\n");
        goto len_error;
    }
    //输出
    {
        unsigned int i;
        CmdIoValue * tio = (CmdIoValue *)GET_CMD_DATA(pcmd);
        memset(&tio->io_value,0,sizeof(tio->io_value));
        tio->io_count = IO_INPUT_COUNT;
    	for(i=0;i<tio->io_count;i++) { //协议支持最大输出
            unsigned char bitval = get_bitval(IO_INPUT_BASE+i);
            SET_BIT(&(tio->io_value),i,bitval);
		}
        pcmd->cmd_option = 0x01;
        tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead)+sizeof(CmdIoValue));
        if(THISINFO)printf("+CmdSetIoOutValue ok\r\n");
        return ;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
    if(THISINFO)printf("+CmdSetIoOutValue ko\r\n");
}





void   CmdSetIoOutPownDownHold(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdRegisterHead)) {
        goto len_error;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
}






void   CmdGetIoOutPownDownHold(unsigned char * buffer,unsigned int len)
{
    APP_PACK_HEAD_T * papph = (APP_PACK_HEAD_T *)buffer;
    CmdHead * pcmd = (CmdHead *)&buffer[sizeof(APP_PACK_HEAD_T)];
    if(len < sizeof(APP_PACK_HEAD_T) + sizeof(CmdHead) + sizeof(CmdRegisterHead)) {
        goto len_error;
    }
len_error:
    pcmd->cmd_option = 0;
    pcmd->cmd_len    = 0;
    tx_pack_and_send(buffer,sizeof(APP_PACK_HEAD_T)+sizeof(CmdHead));
}




