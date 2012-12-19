#ifndef __PLC_COMMAND_H__
#define __PLC_COMMAND_H__

#include "compiler.h"

//指令系统分为基本指令，和数据处理指令
//指令长度不定，但是指令的第一个头字节表示指令编码
//随后的数据代表指令数据
//例如:LD指令
//LD  X001
//LD  X3388
//LD  X7899
//这些指令，它的数据至少16位才能表示输入的索引值，所以数据位有2个字节

#define    PLC_NONE    0     //无效指令，或空指令
#define    PLC_LD      1     //加载指令
#define    PLC_LDI     2
#define    PLC_OUT     3     //输出指令
#define    PLC_AND     4
#define    PLC_ANI     5
#define    PLC_OR      6
#define    PLC_ORI     7
#define    PLC_LDP     8
#define    PLC_LDF     9
#define    PLC_ANDP    10
#define    PLC_ANDF    11
#define    PLC_ORP     12
#define    PLC_ORF     13
#define    PLC_MPS     14
#define    PLC_MRD     15
#define    PLC_MPP     16
#define    PLC_SET     17
#define    PLC_RST     18
#define    PLC_INV     19
//自定义指令
#define    PLC_OUTT    20   //输出到定时器
#define    PLC_OUTC    21   //输出都计数器
//功能指令
#define    PLC_ZRST    22
#define    PLC_CMP     23
#define    PLC_ZCP     24
#define    PLC_MOV     25
#define    PLC_CML     26
#define    PLC_BMOV    27
#define    PLC_FMOV    28

#define    PLC_NETRB   29
#define    PLC_NETWB   30
#define    PLC_NETRW   31
#define    PLC_NETWW   32




#define    PLC_END     0xFF  //结束指令


struct COUNT_TYPE
{
    WORD   maxcount;
	WORD   countdown;
};
//定时器指令操作码
typedef struct _TIME_OP
{
	WORD index;
	WORD kval;
} TIME_OP;


typedef struct _NetCmdOptT
{
  unsigned char op;
  unsigned char device_id;
  unsigned char remote_start_addr_hi;  //远端数据的起始地址
  unsigned char remote_start_addr_lo;  //远端数据的起始地址
  unsigned char local_start_addr_hi;  //远端数据的起始地址
  unsigned char local_start_addr_lo;  //远端数据的起始地址
  unsigned char data_number; //常数
  unsigned char timeout;    //常数
  unsigned char data_type     : 1;  //常数
  unsigned char rw_flag       : 1;  //常数
  unsigned char enable        : 1;  //RX,继电器位变量寻址
  unsigned char done          : 1;  //TX,辅助继电器寻址
  unsigned char error         : 1;  //TX,辅助继电器寻址
  unsigned char timeout_flag  : 1;  //TX,辅助继电器寻址
} NetCmdOptT;





#endif


