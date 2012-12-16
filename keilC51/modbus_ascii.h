#ifndef __MODBUS_ASCII_H__
#define __MODBUS_ASCII_H__


#define  MODBUS_MAX_RX_LEN     128
extern unsigned char rx_buffer[MODBUS_MAX_RX_LEN];
extern unsigned char rx_index;


extern void force_single_coil(unsigned int address,unsigned char onoff);
extern void read_coil_status(unsigned int address,unsigned int number,unsigned char rtu_mode);
extern void read_holding_register(void * preq);
extern void force_multiple_coils(char * hexbuf,unsigned int len,unsigned char rtu_mode);
extern void modbus_preset_multiple_register(void * preqbuf);

extern void UartReceivetoModbusAscii(unsigned char ch);


#endif

