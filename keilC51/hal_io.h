#ifndef __HAL_H__
#define __HAL_H__




//-------------------------------------------------------------------------
//以下常量由代码编写人员定义
#define   EXT_BOARD_IS_6CHIN_7CHOUT       1
#define   EXT_BOARD_IS_8CHIN_8CHOUT       2
#define   EXT_BOARD_IS_CAN485_MINIBOARD   3
#define   EXT_BOARD_IS_16CHOUT10A         4
#define   EXT_BOARD_IS_4CHIN_4CHOUT       5
//一个273，面积比较小
#define   EXT_BOARD_IS_2CHIN_2CHOUT_BOX   6 

//最老的，两个273   
#define   EXT_BOARD_IS_2CHIN_2CHOUT_V1    7 

// //大板，第二版，一个273  
#define   EXT_BOARD_IS_2CHIN_2CHOUT_V2    8  

// //带光耦的8路输入
#define   EXT_BOARD_IS_8CHIN_8CHOUT_V2    9  
// //带光耦的8路输入
#define   EXT_BOARD_IS_32_EXTEND_TO_SERIAL    10




#include "build_def.h"







//-------------------------------------------------------------------------
//以下需要代码实现人员编写
//
#if CURRENT_BOARD == EXT_BOARD_IS_16CHOUT10A
#define REAL_IO_OUT_NUM          16
#define REAL_IO_INPUT_NUM        0
#endif

#if CURRENT_BOARD == EXT_BOARD_IS_8CHIN_8CHOUT_V2
#define REAL_IO_OUT_NUM          8
#define REAL_IO_INPUT_NUM        8
#endif


//如果没有定义好，返回一下错误

#ifndef REAL_IO_OUT_NUM
#error  "REAL_IO_OUT_NUM not define!"
#endif
#ifndef REAL_IO_INPUT_NUM
#error  "REAL_IO_INPUT_NUM not define!"
#endif


//-------------------------------------------------------------------------
//以下不需要修改
//
#include "compiler.h"

extern unsigned char io_out[BITS_TO_BS(REAL_IO_OUT_NUM)];
extern code const unsigned char code_msk[8];

extern void io_init(void);
extern unsigned int io_out_convert_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);
extern unsigned int io_out_set_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);
extern unsigned int io_out_get_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);
extern unsigned int io_in_get_bits(unsigned int startbits,unsigned char * iobits,unsigned int bitcount);
extern void fwrite(unsigned char * buffer,unsigned int s,unsigned int len,void * file);
extern void dump_data(void * buffer,unsigned int len);

#endif

