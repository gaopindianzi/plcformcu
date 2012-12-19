#include "STC12C5A60S2.h"
#include "hal_io.h"
#include "hal_stc_io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


sbit  IO_W1  =  P4^2;
sbit  IO_W2  =  P1^0;
sbit  IO_W3  =  P1^1;
sbit  RESET  =  P1^7;




#define SET_IO_W1(on)    do{ IO_W1 = on; }while(0)
#define SET_IO_W2(on)    do{ IO_W2 = on; }while(0)
#define SET_IO_W3(on)    do{ IO_W3 = on; }while(0)
#define SET_IO_VAL(val)  do{ P0   = val; }while(0)
#define SET_RESET(hig)   do{ RESET = hig; }while(0)
#define SET_IO_DIROUT(out)  \
        do{ if(out) {  P0M0 = 0xFF;  P0M1 = 0x00; } else {   \
		               P0M0 = 0x00;  P0M1 = 0xFF; }} while(0)
#define GET_IO_VAL()     P0


#define SET_P35_DIROUT(out)  \
        do{ if(out) {  P3M0 |= (1<<5);  P3M1 &= ~(1<<5); } else {   \
		               P3M0 &= ~(1<<5); P3M1 |= (1<<5); }} while(0)
#define SET_P35_ON(on)  do{ if(on) { P3 |= (1<<5); } else { P3 &= ~(1<<5); }}while(0)

#define SET_P47_DIROUT(out)  \
        do{ if(out) {  P4M0 |= (1<<7);  P4M1 &= ~(1<<7); } else {   \
		               P4M0 &= ~(1<<7); P4M1 |= (1<<7); }} while(0)
#define SET_P47_ON(on)  do{ if(on) { P4 |= (1<<7); } else { P4 &= ~(1<<7); }}while(0)

#if REAL_IO_INPUT_NUM > 0
unsigned char io_in[BITS_TO_BS(REAL_IO_INPUT_NUM)];
#endif

#if REAL_IO_OUT_NUM > 0
unsigned char io_out[BITS_TO_BS(REAL_IO_OUT_NUM)];
#endif

code const unsigned char code_msk[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

void   SET_IO_DELAY(void)
{
    volatile unsigned int i;
	for(i=0;i<100;i++) {
	}
}

void io_init(void)
{
  P1M0 |=  (1<<0)|(1<<1)|(1<<7);
  P1M1 &= ~((1<<0)|(1<<1)|(1<<7));
  P4M0 |=  (1<<2);
  P4M1 &= ~(1<<2);
  SET_IO_DIROUT(1);
  SET_RESET(0);
  SET_IO_W1(0);
  SET_IO_W2(0);
  SET_IO_W3(0);
  SET_IO_DELAY();
  memset(io_out,0,sizeof(io_out));
  SET_RESET(1);
  SET_P35_DIROUT(1);
  SET_P47_DIROUT(1);
}


static void  io_out_clock_out(void)
{
#if CURRENT_BOARD == EXT_BOARD_IS_16CHOUT10A
	SET_IO_VAL(io_out[0]);
	SET_IO_W1(1);
	SET_IO_DELAY();
	SET_IO_W1(0);
	SET_IO_VAL(io_out[1]);
	SET_IO_W3(1);
	SET_IO_DELAY();
	SET_IO_W3(0);
#endif
#if CURRENT_BOARD == EXT_BOARD_IS_8CHIN_8CHOUT_V2
	SET_IO_VAL(io_out[0]);
	SET_IO_DELAY();
	SET_IO_DELAY();
	SET_IO_DELAY();
	SET_IO_W1(1);
	SET_IO_DELAY();
	SET_IO_W1(0);
    SET_P35_ON(io_out[0]&0x01);
    SET_P47_ON(io_out[0]&0x01);
#endif
}

static void io_input_clock_in(void)
{
#if CURRENT_BOARD == EXT_BOARD_IS_8CHIN_8CHOUT_V2
    SET_IO_DIROUT(0);
	SET_IO_W2(1);
	SET_IO_DELAY();
	SET_IO_DELAY();
	SET_IO_DELAY();
	io_in[0] = GET_IO_VAL();
	SET_IO_W2(0);
	SET_IO_DIROUT(1);
#endif
}


unsigned int io_out_convert_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
	unsigned int i,index;
	unsigned char Bb,Bi;
	//参数必须符合条件
	if(startbits >= REAL_IO_OUT_NUM || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((REAL_IO_OUT_NUM - startbits) < bitcount) {
		bitcount = REAL_IO_OUT_NUM - startbits;
	}
	//开始设置
	index = 0;
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(iobits[Bb]&code_msk[Bi]) {
			io_out[i/8] ^= code_msk[i%8];
		}
		index++;
	}
	//设置IO口
	io_out_clock_out();
	//返回
	//
	return bitcount;
}
unsigned int io_out_set_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
	unsigned int i,index;
	unsigned char Bb,Bi;
	//参数必须符合条件
	if(startbits >= REAL_IO_OUT_NUM || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((REAL_IO_OUT_NUM - startbits) < bitcount) {
		bitcount = REAL_IO_OUT_NUM - startbits;
	}
	//开始设置
	index = 0;
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(iobits[Bb]&code_msk[Bi]) {
			io_out[i/8] |=  code_msk[i%8];
		} else {
			io_out[i/8] &= ~code_msk[i%8];
		}
		index++;
	}
	//设置IO口
	io_out_clock_out();
    
	//返回
	return bitcount;
}
unsigned int io_out_get_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
	unsigned int i,index;
	unsigned char Bb,Bi;

	memset(iobits,0,(bitcount+7)/8);

	if(startbits >= REAL_IO_OUT_NUM || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((REAL_IO_OUT_NUM - startbits) < bitcount) {
		bitcount = REAL_IO_OUT_NUM - startbits;
	}
	//开始设置
	index = 0;
	
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(io_out[i/8]&code_msk[i%8]) {
			iobits[Bb] |=  code_msk[Bi];
		} else {
			iobits[Bb] &= ~code_msk[Bi];
		}
		index++;
	}
	return bitcount;
}
unsigned int io_in_get_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount)
{
#if REAL_IO_INPUT_NUM > 0
	unsigned int  i,index;
	unsigned char Bb,Bi;

	memset(iobits,0,(bitcount+7)/8);

	if(startbits >= REAL_IO_INPUT_NUM || bitcount == 0) {
		return 0;
	}
	//进一步判断是否符合条件
	if((REAL_IO_INPUT_NUM - startbits) < bitcount) {
		bitcount = REAL_IO_INPUT_NUM - startbits;
	}

	//读取底层的实现
	io_input_clock_in();

	//开始读取
	index = 0;
	for(i=startbits;i<startbits+bitcount;i++) {
	    Bb = index / 8;
	    Bi = index % 8;
		if(io_in[i/8]&code_msk[i%8]) {
			iobits[Bb] |=  code_msk[Bi];
		} else {
			iobits[Bb] &= ~code_msk[Bi];
		}
		index++;
	}
	return bitcount;
#else
    startbits = startbits;
    memset(iobits,0,(bitcount+7)/8);
    return 0;
#endif
}


void fwrite(unsigned char * buffer,unsigned int s,unsigned int len,void * file)
{
    unsigned int i;
    len *= s;
    file = file;
    for(i=0;i<len;i++) {
        send_uart1(buffer[i]);
    }
}

void dump_data(void * buffer,unsigned int len)
{
    unsigned char * pbuf = buffer;
    if(len == 0) {
        return ;
    }
    //send_uart1('\r');
    //send_uart1('\n');
    while(len--) {
      send_uart1(*pbuf++);
    }
    //send_uart1('\r');
    //send_uart1('\n');
}
