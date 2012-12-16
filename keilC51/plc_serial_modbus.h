#ifndef __MODBUS_H__
#define __MODBUS_H__

extern unsigned int modbus_set_out_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);
extern unsigned int modbus_get_out_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);
extern unsigned int modbus_get_in_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);


#endif

