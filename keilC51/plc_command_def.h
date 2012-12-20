#ifndef __PLC_COMMAND_H__
#define __PLC_COMMAND_H__

#include "compiler.h"

//ָ��ϵͳ��Ϊ����ָ������ݴ���ָ��
//ָ��Ȳ���������ָ��ĵ�һ��ͷ�ֽڱ�ʾָ�����
//�������ݴ���ָ������
//����:LDָ��
//LD  X001
//LD  X3388
//LD  X7899
//��Щָ�������������16λ���ܱ�ʾ���������ֵ����������λ��2���ֽ�

#define    PLC_NONE    0     //��Чָ����ָ��
#define    PLC_LD      1     //����ָ��
#define    PLC_LDKH    2
#define    PLC_LDI     3
#define    PLC_OUT     4     //���ָ��
#define    PLC_AND     5
#define    PLC_ANI     6
#define    PLC_OR      7
#define    PLC_ORI     8
#define    PLC_LDP     9
#define    PLC_LDF     10
#define    PLC_ANDP    11
#define    PLC_ANDF    12
#define    PLC_ORP     13
#define    PLC_ORF     14
#define    PLC_MPS     15
#define    PLC_MRD     16
#define    PLC_MPP     17
#define    PLC_SET     18
#define    PLC_RST     19
#define    PLC_INV     20
//�Զ���ָ��
#define    PLC_OUTT    21   //�������ʱ��
#define    PLC_OUTC    22   //�����������
//����ָ��
#define    PLC_ZRST    23
#define    PLC_CMP     24
#define    PLC_ZCP     25
#define    PLC_MOV     26
#define    PLC_CML     27
#define    PLC_BMOV    28
#define    PLC_FMOV    29

#define    PLC_NETRB   30
#define    PLC_NETWB   31
#define    PLC_NETRW   32
#define    PLC_NETWW   33

#define    PLC_LDKL    34
#define    PLC_SEI     35 //�����룬��ȡ��




#define    PLC_END     0xFF  //����ָ��


struct COUNT_TYPE
{
    WORD   maxcount;
	WORD   countdown;
};
//��ʱ��ָ�������
typedef struct _TIME_OP
{
	WORD index;
	WORD kval;
} TIME_OP;


typedef struct _NetCmdOptT
{
  unsigned char op;
  unsigned char device_id;
  unsigned char remote_start_addr_hi;  //Զ�����ݵ���ʼ��ַ
  unsigned char remote_start_addr_lo;  //Զ�����ݵ���ʼ��ַ
  unsigned char local_start_addr_hi;  //Զ�����ݵ���ʼ��ַ
  unsigned char local_start_addr_lo;  //Զ�����ݵ���ʼ��ַ
  unsigned char data_number; //����
  unsigned char timeout;    //����
  unsigned char data_type     : 1;  //����
  unsigned char rw_flag       : 1;  //����
  unsigned char enable        : 1;  //RX,�̵���λ����Ѱַ
  unsigned char done          : 1;  //TX,�����̵���Ѱַ
  unsigned char error         : 1;  //TX,�����̵���Ѱַ
  unsigned char timeout_flag  : 1;  //TX,�����̵���Ѱַ
} NetCmdOptT;





#endif


