#include "STC12C5A60S2.h"
#include <intrins.h>
#include "eeprom.h"
 

//--定义与IAP有关的特殊功能寄存器
#define ISP_DATA   IAP_DATA 		    //ISP数据寄存器地址
#define ISP_ADDRH  IAP_ADDRH 		    //EEPROM,Flash存储器高位地址
#define ISP_ADDRL  IAP_ADDRL		    //EEPROM,Flash存储器低位地址
#define ISP_CMD    IAP_CMD 			    //ISP指令寄存器地址
#define ISP_TRIG   IAP_TRIG		    //ISP命令触发器寄存器地址
#define ISP_CONTR  IAP_CONTR		    //ISP/IAP控制寄存器
 
//------------------------
//--定义变量
//------------------------
//--Flash 操作等待时间
//#define ENABLE_ISP 0x83		//<5MHz
//#define ENABLE_ISP 0x82		//<10MHz
//#define ENABLE_ISP 0x81			//<20MHz		 打开ISP操作功能及等待时间
#define ENABLE_ISP 0x80		//>20MHz
#define DEBUG_DATA 0x5a			//往EEPROM写入的数据
#define uchar unsigned char
#define uint unsigned int
//------------------------
//--IAP操作模式
//------------------------
#define Read 0x01				//读Flash命令字
#define Write 0x02				//写Flash命令字
#define Erase 0x03				//擦除Flash命令字
#define wait_time  0 
//---------------------------
//--选择型号STC54\58起始地址都为0x8000,51起始地址为0x2000 stc12c5a为0x0000
unsigned int xdata  DATA_FLASH_START_ADDRESS = 0x0000;
unsigned char xdata DATA_memory;



//---------------------------
//--触发TRIG寄存器
void Trigger_ISP ()
{
    EA = 0;
	ISP_TRIG = 0x5a;
	ISP_TRIG = 0xa5;
	_nop_();
	EA = 1;
}

//---------------------------
//--禁止IAP操作
void IAP_Disable ()
{
    ISP_CONTR = 0;
	ISP_CMD = 0;
	ISP_TRIG = 0;
    ISP_ADDRH =0xff;
	ISP_ADDRL =0xff;
}

//---------------------------
//--读Flash操作
unsigned char Byte_Read (unsigned int address)//这里只能使用256个地址空间
{	
	ISP_ADDRH = ( address >> 8 );
	ISP_ADDRL = ( address);
    ISP_CONTR = ENABLE_ISP;	//打开IAP功能,及设置Flash操作等待时间
 
	ISP_CMD = Read;			    //选择读 AP模式

	 
	ISP_TRIG = 0x5a;
	ISP_TRIG = 0xa5;
	_nop_();
	DATA_memory = ISP_DATA;
 
	IAP_Disable();
	return DATA_memory	;
}

//---------------------------
//--擦除扇区
void Sector_Erase (unsigned int address)
{
    ISP_CONTR = ENABLE_ISP;    //打开IAP功能,及设置Flash操作等待时间 
	ISP_CMD = Erase;
	ISP_ADDRH = ( address >> 8 );
	ISP_ADDRL = address;
	Trigger_ISP ();
	IAP_Disable ();
}

//---------------------------
//--字节编程
void Byte_Program (unsigned int address, unsigned char write_data )
{
  // Sector_Erase (address);   //写之前先擦除数据
   ISP_CONTR = ENABLE_ISP;     //打开IAP功能,及设置Flash操作等待时间
	ISP_CMD = Write;
	ISP_ADDRH = ( address >> 8 );
	ISP_ADDRL =  address & 0xFF;
	ISP_DATA = write_data;
	Trigger_ISP ();
	IAP_Disable ();
}



//------------------------------------------------
//--- 提供简易接口
//---- 

void eeprom_secotr_erase(unsigned int page)
{
	Sector_Erase(page);
}

void eeprom_write(unsigned int start_addr,unsigned char * pbuf,unsigned int len)
{
    unsigned int i;
	for(i=0;i<len;i++) {
		Byte_Program(i+start_addr,pbuf[i]);
	}
}

void eeprom_read(unsigned int start_addr,unsigned char * pbuf,unsigned int len)
{
    unsigned int i;
	for(i=0;i<len;i++) {
		pbuf[i] = Byte_Read(i+start_addr);
	}
}

/***********************************************
 * 比较函数，如果全部相同，则返回0
 * 如果有不同的，返回不同的位置,1表示第一个字节
 */

unsigned int  eeprom_compare(unsigned int start_addr,unsigned char * pbuf,unsigned int len)
{
    unsigned int i;
	for(i=0;i<len;i++) {
		if(pbuf[i] != Byte_Read(i+start_addr)) {
		    return i+1;
		}
	}
	return 0;
}
