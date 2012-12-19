#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define  THIS_INFO    0
#define  THIS_ERROR   0


#include "hal_io.h"
#include "hal_stc_io.h"
#include "eeprom.h"
#include "plc_command_def.h"
#include "plc_prase.h"


extern unsigned char xdata modbus_bits[BITS_TO_BS(IO_OUTPUT_COUNT)];
extern unsigned char xdata inputs_new[BITS_TO_BS(IO_INPUT_COUNT)];


unsigned int modbus_set_out_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
	unsigned int i,index;
	unsigned char Bb,Bi;
	//参数必须符合条件
	if(startbits >= IO_OUTPUT_COUNT || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((IO_OUTPUT_COUNT - startbits) < bitcount) {
		bitcount = IO_OUTPUT_COUNT - startbits;
	}
	//开始设置
	index = 0;
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(iobits[Bb]&code_msk[Bi]) {
			modbus_bits[i/8] |=  code_msk[i%8];
		} else {
			modbus_bits[i/8] &= ~code_msk[i%8];
		}
		index++;
	}
	return bitcount;
}

unsigned int modbus_get_out_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
	unsigned int i,index;
	unsigned char Bb,Bi;

	memset(iobits,0,(bitcount+7)/8);

	if(startbits >= IO_OUTPUT_COUNT || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((IO_OUTPUT_COUNT - startbits) < bitcount) {
		bitcount = IO_OUTPUT_COUNT - startbits;
	}
	//开始设置
	index = 0;
	
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(modbus_bits[i/8]&code_msk[i%8]) {
			iobits[Bb] |=  code_msk[Bi];
		} else {
			iobits[Bb] &= ~code_msk[Bi];
		}
		index++;
	}
	return bitcount;
}


unsigned int modbus_get_in_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
	unsigned int i,index;
	unsigned char Bb,Bi;

	memset(iobits,0,(bitcount+7)/8);

	if(startbits >= IO_INPUT_COUNT || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((IO_INPUT_COUNT - startbits) < bitcount) {
		bitcount = IO_INPUT_COUNT - startbits;
	}
	//开始设置
	index = 0;
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(inputs_new[i/8]&code_msk[i%8]) {
			iobits[Bb] |=  code_msk[Bi];
		} else {
			iobits[Bb] &= ~code_msk[Bi];
		}
		index++;
	}
	return bitcount;
}

