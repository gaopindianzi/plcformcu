#ifndef __MODBUS_ASCII_H__
#define __MODBUS_ASCII_H__


#define  MODBUS_MAX_RX_LEN     12
extern unsigned char rx_buffer[MODBUS_MAX_RX_LEN];
extern unsigned char rx_index;

extern unsigned int CRC16(unsigned char *Array,unsigned int Len);
extern void force_single_coil(unsigned int address,unsigned char onoff);
extern void read_coil_status(unsigned int address,unsigned int number,unsigned char rtu_mode);
extern void read_holding_register(void * preq);
extern void force_multiple_coils(char * hexbuf,unsigned int len,unsigned char rtu_mode);
extern void modbus_preset_multiple_register(void * preqbuf);

extern void UartReceivetoModbusAscii(unsigned char ch);


#endif

