#ifndef __BIN_COMMAND_HANDLE_H__
#define __BIN_COMMAND_HANDLE_H__


void   CmdReadRegister(unsigned char * buffer,unsigned int len);
void   CmdWriteRegister(unsigned char * buffer,unsigned int len);
void   CmdGetIoOutValue(unsigned char * buffer,unsigned int len);
void   CmdSetIoOutValue(unsigned char * buffer,unsigned int len);
void   CmdRevertIoOutIndex(unsigned char * buffer,unsigned int len);
void   CmdSetClrVerIoOutOneBit(unsigned char * buffer,unsigned int len,unsigned char mode);
void   CmdGetIoInValue(unsigned char * buffer,unsigned int len);
void   CmdSetIoOutPownDownHold(unsigned char * buffer,unsigned int len);
void   CmdGetIoOutPownDownHold(unsigned char * buffer,unsigned int len);


#endif
