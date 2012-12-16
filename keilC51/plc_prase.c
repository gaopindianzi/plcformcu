/************************************************************
 * ����ļ�����ű���ʵ��һЩ��׼��PLCЭ�飬��ȻҲ��ʵ���Լ���һЩ����Э��
 * Ŀǰ���Ǵ��ڳ��Խ׶�
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define  THIS_INFO    1
#define  THIS_ERROR   1


#include "hal_io.h"
#include "hal_stc_io.h"
#include "eeprom.h"
#include "plc_command_def.h"
#include "plc_prase.h"


//--------------------------��ַ���-------����-----------
#define  IO_INPUT_BASE          0
#define  IO_INPUT_COUNT                     16
#define  IO_OUTPUT_BASE         256  //0x00,0x01
#define  IO_OUTPUT_COUNT                    16
#define  AUXI_RELAY_BASE        512  //0x00,0x02
#define  AUXI_RELAY_COUNT                   200
//
#define  TIMING100MS_EVENT_BASE  2048  //0x08,0x00
#define  TIMING100MS_EVENT_COUNT            40

#define  TIMING1S_EVENT_BASE     3072  //0x0C,0x00
#define  TIMING1S_EVENT_COUNT               40

#define  COUNTER_EVENT_BASE      4096  //0x10,0x00
#define  COUNTER_EVENT_COUNT                40

//100ms��ʱ���Ŀ������ݽṹ
typedef struct _TIM100MS_ARRAYS_T
{
    WORD  counter[TIMING100MS_EVENT_COUNT];
	BYTE  upordown_bits[BITS_TO_BS(TIMING100MS_EVENT_COUNT)];
	BYTE  enable_bits[BITS_TO_BS(TIMING100MS_EVENT_COUNT)];
	BYTE  event_bits[BITS_TO_BS(TIMING100MS_EVENT_COUNT)];
	BYTE  event_bits_last[BITS_TO_BS(TIMING100MS_EVENT_COUNT)];
	BYTE  holding_bits[BITS_TO_BS(TIMING100MS_EVENT_COUNT)];
} TIM100MS_ARRAYS_T;

TIM100MS_ARRAYS_T  tim100ms_arrys;



//1s��ʱ���Ŀ������ݽṹ
typedef struct _TIM1S_ARRAYS_T
{
    WORD  counter[TIMING1S_EVENT_COUNT];
	BYTE  upordown_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  enable_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  event_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  event_bits_last[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  holding_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
} TIM1S_ARRAYS_T;

TIM1S_ARRAYS_T     tim1s_arrys;

//�������Ŀ������ݽṹ
typedef struct _COUNTER_ARRAYS_T
{
    WORD  counter[COUNTER_EVENT_COUNT];
	BYTE  upordown_bits[BITS_TO_BS(COUNTER_EVENT_COUNT)];		
	BYTE  event_bits[BITS_TO_BS(COUNTER_EVENT_COUNT)];
	BYTE  event_bits_last[BITS_TO_BS(COUNTER_EVENT_COUNT)];
	BYTE  last_trig_bits[BITS_TO_BS(COUNTER_EVENT_COUNT)];
} COUNTER_ARRAYS_T;

COUNTER_ARRAYS_T counter_arrys;




#define  COUNT_BASE         COUNTER_EVENT_BASE
#define  COUNTER_MAX                  COUNTER_EVENT_COUNT


//�����
unsigned int  input_num;
unsigned char inputs_new[BITS_TO_BS(IO_INPUT_COUNT)];
unsigned char inputs_last[BITS_TO_BS(IO_INPUT_COUNT)];
//����̵���
unsigned int  output_num;
unsigned char output_last[BITS_TO_BS(IO_OUTPUT_COUNT)];
unsigned char output_new[BITS_TO_BS(IO_OUTPUT_COUNT)];
//�����̵���
unsigned char xdata auxi_relays[BITS_TO_BS(AUXI_RELAY_COUNT)];
unsigned char xdata auxi_relays_last[BITS_TO_BS(AUXI_RELAY_COUNT)];
//��ʱ�����壬�Զ����ڲ���ʱ��������м���
volatile unsigned int  time100ms_come_flag;
volatile unsigned int  time1s_come_flag;
//�������ļĴ���
#define  BIT_STACK_LEVEL     32
unsigned char  bit_acc;
unsigned char  bit_stack[BITS_TO_BS(BIT_STACK_LEVEL)];   //���ض�ջ��PLC��λ������ѹջ������ܹ���32��ջ
unsigned char  bit_stack_sp;   //���ض�ջ��ָ��

//ָ�����
unsigned int  plc_command_index;     //��ǰָ��������
unsigned char plc_command_array[16]; //��ǰָ���ֽڱ���
#define       PLC_CODE     (plc_command_array[0])

//������״̬
unsigned char plc_cpu_stop;

/*************************************************
 * ������˽�е�ʵ��
 */
//�ڲ���ϵͳ������
static unsigned long  last_tick;
static unsigned long  last_tick1s;

static void sys_time_tick_init(void)
{
	last_tick = get_sys_clock();
	last_tick1s = get_sys_clock();
}

//һ����Ҫϵͳ����
void plc_timing_tick_process(void)
{
	unsigned long curr = get_sys_clock();
	if((curr - last_tick) >= TICK_SECOND / 10) {
	    sys_lock();
		time100ms_come_flag++;
		sys_unlock();
		last_tick = curr;
	}
	if((curr - last_tick1s) >= TICK_SECOND) {
	    sys_lock();
		time1s_come_flag++;
		sys_unlock();
		last_tick1s = curr;
	}
}

void timing_cell_prcess(void);


/*********************************************
 * ϵͳ��ʼ��
 */

void PlcInit(void)
{
    plc_cpu_stop = 0;
	bit_acc = 0;
	memset(bit_stack,0,sizeof(bit_stack));
	bit_stack_sp = 0;
	input_num = io_in_get_bits(0,inputs_new,IO_INPUT_COUNT);
	output_num = io_out_get_bits(0,output_last,IO_OUTPUT_COUNT);
	memcpy(inputs_last,inputs_new,sizeof(inputs_new));
	plc_command_index = 0;
    //memset(timing100ms,0,sizeof(timing100ms));
	//memset(counter,0,sizeof(counter));
	memset(output_last,0,sizeof(output_last));
	sys_time_tick_init();
	time100ms_come_flag = 0;
	time1s_come_flag = 0;
	//memset(timing100ms_event,0,sizeof(timing100ms_event));
	//memset(timeing1s_event,0,sizeof(timeing1s_event));
}



/**********************************************
 *  ��ȡ��һ��ָ���ָ����
 *  Ҳ���Ǵ�EEPROM�ж�ȡ�ĳ���ű�
 *  ����һ���Զ�ȡ��һ��ָ�����Ϊ�ָ���
 */

extern code unsigned char plc_test_buffer[128];

void read_next_plc_code(void)
{
#if 0
    unsigned char info_buffer[sizeof(plc_sys_info)-GET_OFFSET_MEM_OF_STRUCT(plc_sys_info,enable)];
	unsigned int  index = GET_OFFSET_MEM_OF_STRUCT(plc_sys_info,enable);
	plc_sys_info * pinfo = NULL;
	eeprom_read(index,info_buffer,sizeof(info_buffer));
	pinfo = (plc_sys_info *)(info_buffer - sizeof(pinfo->plc_data_array));  //ָ����Ҫ��ǰ�ƶ�N���ֽ�
	if(1) { //pinfo->enable == 0xAA) {
	    eeprom_read(plc_command_index,plc_command_array,sizeof(plc_command_array));
	} else {
	    PLC_CODE = PLC_NONE;
	}
#endif
    memcpy(plc_command_array,&plc_test_buffer[plc_command_index],sizeof(plc_command_array));
	plc_timing_tick_process();
}

void handle_plc_command_error(void)
{
	//��ʾ�ڼ���ָ�����
	//Ȼ��λ����ֹͣ����
    if(THIS_ERROR)printf("ERROR:handle_plc_command_error() reset PC\r\n");
	plc_command_index  = 0;
	plc_cpu_stop = 1;
}

static unsigned char get_bitval(unsigned int index)
{
	unsigned char bitval = 0;
	if(index >= IO_INPUT_BASE && index < (IO_INPUT_BASE+IO_INPUT_COUNT)) {
		index -= IO_INPUT_BASE;
		bitval = BIT_IS_SET(inputs_new,index);
	} else if(index >= IO_OUTPUT_BASE && index < (IO_OUTPUT_BASE+IO_OUTPUT_COUNT)) {
		index -= IO_OUTPUT_BASE;
		bitval = BIT_IS_SET(output_new,index);
	} else if(index >= AUXI_RELAY_BASE && index < (AUXI_RELAY_BASE + AUXI_RELAY_COUNT)) {
		index -= AUXI_RELAY_BASE;
		bitval = BIT_IS_SET(auxi_relays,index);
	} else if(index >= TIMING100MS_EVENT_BASE && index < (TIMING100MS_EVENT_BASE+TIMING100MS_EVENT_COUNT)) {
		index -= TIMING100MS_EVENT_BASE;
		timing_cell_prcess();
		bitval = BIT_IS_SET(tim100ms_arrys.event_bits,index);
	} else if(index >= TIMING1S_EVENT_BASE && index < (TIMING1S_EVENT_BASE + TIMING1S_EVENT_COUNT)) {
		index -= TIMING1S_EVENT_BASE;
		timing_cell_prcess();
		bitval = BIT_IS_SET(tim1s_arrys.event_bits,index);
	} else if(index >= COUNTER_EVENT_BASE && index < (COUNTER_EVENT_BASE+COUNTER_EVENT_COUNT)) {
	    index -= COUNTER_EVENT_BASE;
	    bitval = BIT_IS_SET(counter_arrys.event_bits,index);
	} else {
	    if(THIS_ERROR)printf("get bit index error:%d\r\n",index);
		handle_plc_command_error();
	}
	return bitval;
}
static unsigned char get_last_bitval(unsigned int index)
{
	unsigned char bitval = TIMING100MS_EVENT_BASE;
	if(index >= IO_INPUT_BASE && index < (IO_INPUT_BASE+IO_INPUT_COUNT)) {
		index -= IO_INPUT_BASE;
		bitval = BIT_IS_SET(inputs_last,index);
	} else if(index >= IO_OUTPUT_BASE && index < (IO_OUTPUT_BASE+IO_OUTPUT_COUNT)) {
		index -= IO_OUTPUT_BASE;
		bitval = BIT_IS_SET(output_last,index);
	} else if(index >= AUXI_RELAY_BASE && index < (AUXI_RELAY_BASE + AUXI_RELAY_COUNT)) {
		index -= AUXI_RELAY_BASE;
		bitval = BIT_IS_SET(auxi_relays_last,index);
	} else if(index >= TIMING100MS_EVENT_BASE && index < (TIMING100MS_EVENT_BASE+TIMING100MS_EVENT_COUNT)) {
		index -= TIMING100MS_EVENT_BASE;
		bitval = BIT_IS_SET(tim100ms_arrys.event_bits_last,index);
	} else if(index >= TIMING1S_EVENT_BASE && index < (TIMING1S_EVENT_BASE + TIMING1S_EVENT_COUNT)) {
		index -= TIMING1S_EVENT_BASE;
		bitval = BIT_IS_SET(tim1s_arrys.event_bits_last,index);
	} else if(index >= COUNTER_EVENT_BASE && index < (COUNTER_EVENT_BASE+COUNTER_EVENT_COUNT)) {
	    index -= COUNTER_EVENT_BASE;
	    bitval = BIT_IS_SET(counter_arrys.event_bits_last,index);
	} else {
	    if(THIS_ERROR)printf("get bit index error:%d\r\n",index);
		handle_plc_command_error();
	}
	return bitval;
}
static void set_bitval(unsigned int index,unsigned char bitval)
{
	if(index >= IO_INPUT_BASE && index < (IO_INPUT_BASE+IO_INPUT_COUNT)) {
		//����ֵ�����޸�
	} else if(index >= IO_OUTPUT_BASE && index < (IO_OUTPUT_BASE+IO_OUTPUT_COUNT)) {
		index -= IO_OUTPUT_BASE;
		SET_BIT(output_new,index,bitval);
	} else if(index >= AUXI_RELAY_BASE && index < (AUXI_RELAY_BASE + AUXI_RELAY_COUNT)) {
		index -= AUXI_RELAY_BASE;
		SET_BIT(auxi_relays,index,bitval);
	} else if(index >= COUNTER_EVENT_BASE && index < (COUNTER_EVENT_BASE+COUNTER_EVENT_COUNT)) {
	    //��������ֵ��������λ,ֻ���Ը�λ
		if(!bitval) {
		    index -= COUNTER_EVENT_BASE;
		    counter_arrys.counter[index] = 0;
			SET_BIT(counter_arrys.event_bits,index,0);
		}
	} else {
	    if(THIS_ERROR)printf("set bit index error:%d\r\n",index);
		handle_plc_command_error();
	}
}
/**********************************************
 * ���������Լ�ʱ����������
 * ���ʱ�䵽�ˣ��򴥷��¼�
 * ���ʱ����δ�����������ʱ
 */
void timing_cell_prcess(void)
{
	unsigned int i;
	unsigned int counter;
	sys_lock();
	counter = time100ms_come_flag;
	time100ms_come_flag = 0;
	sys_unlock();
	if(counter) {
	    TIM100MS_ARRAYS_T * ptiming = &tim100ms_arrys;
	    for(i=0;i<GET_ARRRYS_NUM(tim100ms_arrys.counter);i++) {
		    if(BIT_IS_SET(ptiming->enable_bits,i)) { //��������ʱ
			    if(!BIT_IS_SET(ptiming->event_bits,i)) {  //���ʱ���¼�δ����
				    if(ptiming->counter[i] > counter) {
					    ptiming->counter[i] -= counter;
					} else {
					    ptiming->counter[i] = 0;
					}
					if(ptiming->counter[i] == 0) {
					    SET_BIT(ptiming->event_bits,i,1);
						if(THIS_INFO)printf("timing100ms event come:%d\r\n",i);
					}
				}
			} else {
			    if(BIT_IS_SET(ptiming->holding_bits,i)) {
				    //���ֶ�ʱ��
				} else {
				    //������
					ptiming->counter[i] = 0;
					SET_BIT(ptiming->event_bits,i,0);

				}
			}
	    }
	}
 	sys_lock();
	counter = time1s_come_flag;
	time1s_come_flag = 0;
	sys_unlock();
	if(counter) {
	    TIM1S_ARRAYS_T * ptiming = &tim1s_arrys;
		if(THIS_INFO)printf("1s come\r\n");
	    for(i=0;i<GET_ARRRYS_NUM(tim1s_arrys.counter);i++) {
		    if(BIT_IS_SET(ptiming->enable_bits,i)) { //��������ʱ
			    if(!BIT_IS_SET(ptiming->event_bits,i)) {  //���ʱ���¼�δ����
				    if(ptiming->counter[i] > counter) {
					    ptiming->counter[i] -= counter;
					} else {
					    ptiming->counter[i] = 0;
					}
					if(ptiming->counter[i] == 0) {
					    SET_BIT(ptiming->event_bits,i,1);
						if(THIS_INFO)printf("timing1s event come:%d\r\n",i);
					}
				}
			} else {
			    if(BIT_IS_SET(ptiming->holding_bits,i)) {
				    //���ֶ�ʱ��
				} else {
				    //������
					ptiming->counter[i] = 0;
					SET_BIT(ptiming->event_bits,i,0);
				}
			}
	    }
	}
}
/**********************************************
 * �򿪶�ʱ�������趨����ʱ������ֵ
 * ����Ѿ���ʼ��ʱ���������ʱ�����û�п�ʼ����ʼ
 */
static void timing_cell_start(unsigned int index,unsigned int event_count,unsigned char upordown,unsigned char holding)
{
	if(index >= TIMING100MS_EVENT_BASE && index < (TIMING100MS_EVENT_BASE+TIMING100MS_EVENT_COUNT)) {
	    TIM100MS_ARRAYS_T * ptiming = &tim100ms_arrys;
		index -= TIMING100MS_EVENT_BASE;
		if(!BIT_IS_SET(ptiming->enable_bits,index)) {
		    if(THIS_INFO)printf("start 100ms timer:%d,k=%d\r\n",index,event_count);
		    ptiming->counter[index]    = event_count;
			SET_BIT(ptiming->enable_bits,  index,1);
			SET_BIT(ptiming->upordown_bits,index,upordown);
			SET_BIT(ptiming->holding_bits, index,holding);
			SET_BIT(ptiming->event_bits,   index,0);
		}
	} else if(index >= TIMING1S_EVENT_BASE && index < (TIMING1S_EVENT_BASE+TIMING1S_EVENT_COUNT)) {
	    TIM1S_ARRAYS_T * ptiming = &tim1s_arrys;
		index -= TIMING1S_EVENT_BASE;
		if(!BIT_IS_SET(ptiming->enable_bits,index)) {
		    if(THIS_INFO)printf("start 1s timer:%d,k=%d\r\n",index,event_count);
		    ptiming->counter[index]    = event_count;
			SET_BIT(ptiming->enable_bits,  index,1);
			SET_BIT(ptiming->upordown_bits,index,upordown);
			SET_BIT(ptiming->holding_bits, index,holding);
			SET_BIT(ptiming->event_bits,   index,0);
		}
	} else {
	    if(THIS_INFO)printf("start timing error:%d,k=%d,%d,%d\r\n",index,event_count,upordown,holding);
		handle_plc_command_error();
	}
}

/**********************************************
 * �رն�ʱ������ȡ�������¼�
 */
static void timing_cell_stop(unsigned int index)
{
	if(index >= TIMING100MS_EVENT_BASE && index < (TIMING100MS_EVENT_BASE+TIMING100MS_EVENT_COUNT)) {
	    TIM100MS_ARRAYS_T * ptiming = &tim100ms_arrys;
		index -= TIMING100MS_EVENT_BASE;
		if(BIT_IS_SET(ptiming->enable_bits,index)) {
		    if(THIS_INFO)printf("stop timeimg 100ms :%d\r\n",index);
		    SET_BIT(ptiming->enable_bits,  index,0);
		    SET_BIT(ptiming->holding_bits, index,0);
		    SET_BIT(ptiming->event_bits,   index,0);
		}
	} else if(index >= TIMING1S_EVENT_BASE && index < (TIMING1S_EVENT_BASE+TIMING1S_EVENT_COUNT)) {
	    TIM1S_ARRAYS_T * ptiming = &tim1s_arrys;
		index -= TIMING1S_EVENT_BASE;
		if(BIT_IS_SET(ptiming->enable_bits,index)) {
		    if(THIS_INFO)printf("stop timeimg 1s :%d\r\n",index);
		    SET_BIT(ptiming->enable_bits,  index,0);
		    SET_BIT(ptiming->holding_bits, index,0);
		    SET_BIT(ptiming->event_bits,   index,0);
		}
	} else {
	    if(THIS_ERROR)printf("timing stop error:%d\r\n",index);
	    handle_plc_command_error();
	}
}

/**********************************************
 * ��������˿ڵ�����ֵ
 */
void handle_plc_ld(void)
{
	bit_acc = get_bitval(BYTES_TO_WORD(&plc_command_array[1]));
	if(PLC_CODE == PLC_LDI) {
		bit_acc = !bit_acc;
	}
	plc_command_index += 3;
}
/**********************************************
 * ��λ����Ľ�����������˿���
 */
void handle_plc_out(void)
{
	set_bitval(BYTES_TO_WORD(&plc_command_array[1]),bit_acc);
	plc_command_index += 3;
}

/**********************************************
 * ����������
 */
void handle_plc_and_ani(void)
{
	unsigned char bittmp = get_bitval(BYTES_TO_WORD(&plc_command_array[1]));
	if(PLC_CODE == PLC_AND) {
	    bit_acc = bit_acc && bittmp;
	} else {
		bit_acc = bit_acc && (!bittmp);
	}
	plc_command_index += 3;
}

/**********************************************
 * ���������
 */
void handle_plc_or_ori(void)
{
	unsigned char bittmp = get_bitval(BYTES_TO_WORD(&plc_command_array[1]));
	if(PLC_CODE == PLC_OR) {
	    bit_acc = bit_acc || bittmp;
	} else if(PLC_CODE == PLC_ORI) {
		bit_acc = bit_acc || (!bittmp);
	}
	plc_command_index += 3;
}

/**********************************************
 * ��������������ػ��½���
 */
void handle_plc_ldp_ldf(void)
{
	unsigned char reg;
	unsigned int bit_index = BYTES_TO_WORD(&plc_command_array[1]);
	reg  = get_last_bitval(bit_index)?0x01:0x00;
	reg |= get_bitval(bit_index)?     0x02:0x00;
	if(PLC_CODE == PLC_LDP) {
		if(reg == 0x02) { //ֻ�������أ�����������Ǽٵ�
			bit_acc = 1;
		} else {
		    bit_acc = 0;
		}
	} else if(PLC_CODE == PLC_LDF) {
		if(reg == 0x01) {//ֻ���½��أ�����������Ǽٵ�
		    bit_acc = 1;
		} else {
		    bit_acc = 0;
		}
	}
	plc_command_index += 3;
}


/**********************************************
 * �������ػ��½���
 */
void handle_plc_andp_andf(void)
{
    unsigned char reg;
	unsigned int bit_index = BYTES_TO_WORD(&plc_command_array[1]);
	reg =  get_last_bitval(bit_index)?0x01:0x00;
	reg |= get_bitval(bit_index)?     0x02:0x00;
	if(PLC_CODE == PLC_ANDP) {
		if(reg == 0x02) {
			bit_acc = bit_acc && 1;
		} else {
			bit_acc = 0;
		}
	} else if(PLC_CODE == PLC_ANDF) {
		if(reg == 0x01) {
		    bit_acc = bit_acc && 1;
		} else {
			bit_acc = 0;
		}
	}
	plc_command_index += 3;
}
/**********************************************
 * �������ػ��½���
 */
void handle_plc_orp_orf(void)
{
	unsigned char reg;
	unsigned int bit_index = BYTES_TO_WORD(&plc_command_array[1]);
	reg =  get_last_bitval(bit_index)?0x01:0x00;
	reg |= get_bitval(bit_index)?     0x02:0x00;
	if(PLC_CODE == PLC_ORP) {
		if(reg == 0x02) {
			bit_acc = bit_acc || 1;
		} else {
			bit_acc = bit_acc || 0;
		}
	} else if(PLC_CODE == PLC_ORF) {
		if(reg == 0x01) {
		    bit_acc = bit_acc || 1;
		} else {
			bit_acc = bit_acc || 0;
		}
	}
	plc_command_index += 3;
}

/**********************************************
 * ѹջ����ջ����ջ
 */
void handle_plc_mps_mrd_mpp(void)
{
	unsigned char B,b;
	if(PLC_CODE == PLC_MPS) {
		if(bit_stack_sp >= BIT_STACK_LEVEL) {
		    if(THIS_ERROR)printf("��ջ���\r\n");
		    handle_plc_command_error();
			return ;
		}
	    B = bit_stack_sp / 8;
	    b = bit_stack_sp % 8;
		if(bit_acc) {
			bit_stack[B] |=  code_msk[b];
		} else {
			bit_stack[B] &= ~code_msk[b];
		}
		bit_stack_sp++;
	} else if(PLC_CODE == PLC_MRD) {
	    unsigned char last_sp = bit_stack_sp - 1;
	    B = last_sp / 8;
	    b = last_sp % 8;
		bit_acc = (bit_stack[B] & code_msk[b])?1:0;
	} else if(PLC_CODE == PLC_MPP) {
	    bit_stack_sp--;
	    B = bit_stack_sp / 8;
	    b = bit_stack_sp % 8;
		bit_acc = (bit_stack[B] & code_msk[b])?1:0;
	}
	plc_command_index += 1;
}
/**********************************************
 * �������ָ��
 */
void handle_plc_set_rst(void)
{
	unsigned int bit_index = BYTES_TO_WORD(&plc_command_array[1]);
	if(PLC_CODE == PLC_SET) {
	    if(bit_acc) {
			set_bitval(bit_index,1);
	    }
	} else if(PLC_CODE == PLC_RST) {
	    if(bit_acc) {
			set_bitval(bit_index,0);
	    }
	}
	plc_command_index += 3;
}
/**********************************************
 * ȡ�����
 */
void handle_plc_inv(void)
{
	bit_acc = !bit_acc;
	plc_command_index += 1;
}

/**********************************************
 * �������ʱ��
 * ���ݱ�ţ����������100ms��ʱ����Ҳ���������1s��ʱ��
 */
void handle_plc_out_t(void)
{
	unsigned int kval;
	unsigned int time_index = BYTES_TO_WORD(&plc_command_array[1]);
	if(bit_acc) {
	    kval = BYTES_TO_WORD(&plc_command_array[3]);
	    timing_cell_start(time_index,kval,1,0);
	} else {
		timing_cell_stop(time_index);
	}
	plc_command_index += 5;
}
/**********************************************
 * �����������
 */
void handle_plc_out_c(void)
{
	unsigned int kval = BYTES_TO_WORD(&plc_command_array[3]);
	unsigned int index = BYTES_TO_WORD(&plc_command_array[1]);

	//�ж������Ƿ���Ч
    if(index >= COUNTER_EVENT_BASE && index < (COUNTER_EVENT_BASE+COUNTER_EVENT_COUNT)) {
	    index -= COUNTER_EVENT_BASE;
	} else {
	    if(THIS_ERROR)printf("�������������ֵ�д���!\r\n");
		handle_plc_command_error();
	    return ;
	}
	//�������ڲ�ά����һ�εĴ�����ƽ
	//������������ʱ�򣬰���εĴ�����ƽ������������
	if(bit_acc) {
	    if(index < GET_ARRRYS_NUM(counter_arrys.counter)) {
	        if(!BIT_IS_SET(counter_arrys.last_trig_bits,index)) {
		        //��һ���ǵ͵�ƽ������Ǹߵ�ƽ�����Դ�������
			    if(counter_arrys.counter[index] < kval) {
			        if(++counter_arrys.counter[index] == kval) {
				        SET_BIT(counter_arrys.event_bits,index,1);
				    }
			    }
		    }
		}
	}
	//�ѵ�ƽ��¼��������ȥ
	if(index < GET_ARRRYS_NUM(counter_arrys.counter)) {
	    SET_BIT(counter_arrys.last_trig_bits,index,bit_acc);
	}
	plc_command_index += 5;
}


void PlcProcess(void)
{
    if(plc_cpu_stop) {
		return ;
	}
	//���봦��,��ȡIO�ڵ�����
	io_in_get_bits(0,inputs_new,IO_INPUT_COUNT);

	//inputs_new[0] = 0xFF;

	plc_command_index = 0;
 next_plc_command:
	read_next_plc_code();
	//�߼�����,����һ�Σ�����һ���û��ĳ���
	switch(PLC_CODE)
	{
	case PLC_END: //ָ��������������
		plc_command_index = 0;
		goto plc_command_finished;
	case PLC_LD: 
	case PLC_LDI:
		handle_plc_ld();
		break;
	case PLC_OUT:
		handle_plc_out();
		break;
	case PLC_AND:
	case PLC_ANI:
		handle_plc_and_ani();
		break;
	case PLC_OR:
	case PLC_ORI:
		handle_plc_or_ori();
		break;
	case PLC_LDP:
	case PLC_LDF:
		handle_plc_ldp_ldf();
		break;
	case PLC_ANDP:
	case PLC_ANDF:
		handle_plc_andp_andf();
		break;
	case PLC_ORP:
	case PLC_ORF:
		handle_plc_orp_orf();
		break;
	case PLC_MPS:
	case PLC_MRD:
	case PLC_MPP:
		handle_plc_mps_mrd_mpp();
		break;
	case PLC_SET:
	case PLC_RST:
		handle_plc_set_rst();
		break;
	case PLC_INV:
		handle_plc_inv();
		break;
	case PLC_OUTT:
		handle_plc_out_t();
		break;
	case PLC_OUTC:
		handle_plc_out_c();
		break;
	default:
	    handle_plc_command_error();
		break;
	case PLC_NONE: //��ָ�ֱ������
        plc_command_index++;
		break;
	}
	goto next_plc_command;
 plc_command_finished:
	//���������������������̵�����
	io_out_set_bits(0,output_new,IO_OUTPUT_COUNT);
	memcpy(output_last,output_new,sizeof(output_new));
	//��������
	memcpy(inputs_last,inputs_new,sizeof(inputs_new));
	//�����̵���
	memcpy(auxi_relays_last,auxi_relays,sizeof(auxi_relays));
	//ϵͳʱ�䴦�������õ���ʱ���жϴ���
	memcpy(tim100ms_arrys.event_bits_last,tim100ms_arrys.event_bits,sizeof(tim100ms_arrys.event_bits));
	//
	memcpy(tim1s_arrys.event_bits_last,tim1s_arrys.event_bits,sizeof(tim1s_arrys.event_bits));
	//
	memcpy(counter_arrys.event_bits_last,counter_arrys.event_bits,sizeof(counter_arrys.event_bits));
	//��ʱ������
	timing_cell_prcess();
	//����������
}

