#include "STC12C5A60S2.h"
#include <intrins.h>
#include "eeprom.h"
 

//--������IAP�йص����⹦�ܼĴ���
#define ISP_DATA   IAP_DATA 		    //ISP���ݼĴ�����ַ
#define ISP_ADDRH  IAP_ADDRH 		    //EEPROM,Flash�洢����λ��ַ
#define ISP_ADDRL  IAP_ADDRL		    //EEPROM,Flash�洢����λ��ַ
#define ISP_CMD    IAP_CMD 			    //ISPָ��Ĵ�����ַ
#define ISP_TRIG   IAP_TRIG		    //ISP��������Ĵ�����ַ
#define ISP_CONTR  IAP_CONTR		    //ISP/IAP���ƼĴ���
 
//------------------------
//--�������
//------------------------
//--Flash �����ȴ�ʱ��
//#define ENABLE_ISP 0x83		//<5MHz
//#define ENABLE_ISP 0x82		//<10MHz
//#define ENABLE_ISP 0x81			//<20MHz		 ��ISP�������ܼ��ȴ�ʱ��
#define ENABLE_ISP 0x80		//>20MHz
#define DEBUG_DATA 0x5a			//��EEPROMд�������
#define uchar unsigned char
#define uint unsigned int
//------------------------
//--IAP����ģʽ
//------------------------
#define Read 0x01				//��Flash������
#define Write 0x02				//дFlash������
#define Erase 0x03				//����Flash������
#define wait_time  0 
//---------------------------
//--ѡ���ͺ�STC54\58��ʼ��ַ��Ϊ0x8000,51��ʼ��ַΪ0x2000 stc12c5aΪ0x0000
unsigned int xdata  DATA_FLASH_START_ADDRESS = 0x0000;
unsigned char xdata DATA_memory;



//---------------------------
//--����TRIG�Ĵ���
void Trigger_ISP ()
{
    EA = 0;
	ISP_TRIG = 0x5a;
	ISP_TRIG = 0xa5;
	_nop_();
	EA = 1;
}

//---------------------------
//--��ֹIAP����
void IAP_Disable ()
{
    ISP_CONTR = 0;
	ISP_CMD = 0;
	ISP_TRIG = 0;
    ISP_ADDRH =0xff;
	ISP_ADDRL =0xff;
}

//---------------------------
//--��Flash����
unsigned char Byte_Read (unsigned int address)//����ֻ��ʹ��256����ַ�ռ�
{	
	ISP_ADDRH = ( address >> 8 );
	ISP_ADDRL = ( address);
    ISP_CONTR = ENABLE_ISP;	//��IAP����,������Flash�����ȴ�ʱ��
 
	ISP_CMD = Read;			    //ѡ��� APģʽ

	 
	ISP_TRIG = 0x5a;
	ISP_TRIG = 0xa5;
	_nop_();
	DATA_memory = ISP_DATA;
 
	IAP_Disable();
	return DATA_memory	;
}

//---------------------------
//--��������
void Sector_Erase (unsigned int address)
{
    ISP_CONTR = ENABLE_ISP;    //��IAP����,������Flash�����ȴ�ʱ�� 
	ISP_CMD = Erase;
	ISP_ADDRH = ( address >> 8 );
	ISP_ADDRL = address;
	Trigger_ISP ();
	IAP_Disable ();
}

//---------------------------
//--�ֽڱ��
void Byte_Program (unsigned int address, unsigned char write_data )
{
  // Sector_Erase (address);   //д֮ǰ�Ȳ�������
   ISP_CONTR = ENABLE_ISP;     //��IAP����,������Flash�����ȴ�ʱ��
	ISP_CMD = Write;
	ISP_ADDRH = ( address >> 8 );
	ISP_ADDRL =  address & 0xFF;
	ISP_DATA = write_data;
	Trigger_ISP ();
	IAP_Disable ();
}



//------------------------------------------------
//--- �ṩ���׽ӿ�
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
 * �ȽϺ��������ȫ����ͬ���򷵻�0
 * ����в�ͬ�ģ����ز�ͬ��λ��,1��ʾ��һ���ֽ�
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