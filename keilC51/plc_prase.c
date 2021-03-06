/************************************************************
 * 这个文件处理脚本，实现一些标准的PLC协议，当然也会实现自己的一些特殊协议
 * 目前这是处于尝试阶段
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
#include "serial_comm_packeter.h"
#include "modbus_rtu.h"


//100ms计时器的控制数据结构
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



//1s计时器的控制数据结构
typedef struct _TIM1S_ARRAYS_T
{
    WORD  counter[TIMING1S_EVENT_COUNT];
	BYTE  upordown_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  enable_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  event_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  event_bits_last[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
	BYTE  holding_bits[BITS_TO_BS(TIMING1S_EVENT_COUNT)];
} TIM1S_ARRAYS_T;

TIM1S_ARRAYS_T    tim1s_arrys;

//计数器的控制数据结构
typedef struct _COUNTER_ARRAYS_T
{
    WORD  counter[COUNTER_EVENT_COUNT];
	BYTE  upordown_bits[BITS_TO_BS(COUNTER_EVENT_COUNT)];		
	BYTE  event_bits[BITS_TO_BS(COUNTER_EVENT_COUNT)];
	BYTE  event_bits_last[BITS_TO_BS(COUNTER_EVENT_COUNT)];
	BYTE  last_trig_bits[BITS_TO_BS(COUNTER_EVENT_COUNT)];
} COUNTER_ARRAYS_T;

COUNTER_ARRAYS_T counter_arrys;

//输入口
unsigned int  xdata input_num;
unsigned char xdata inputs_new[BITS_TO_BS(IO_INPUT_COUNT)];
unsigned char xdata inputs_last[BITS_TO_BS(IO_INPUT_COUNT)];
//输出继电器
unsigned char xdata output_last[BITS_TO_BS(IO_OUTPUT_COUNT)];
unsigned char xdata output_new[BITS_TO_BS(IO_OUTPUT_COUNT)];
//辅助继电器
unsigned char xdata auxi_relays[BITS_TO_BS(AUXI_RELAY_COUNT)];
unsigned char xdata auxi_relays_last[BITS_TO_BS(AUXI_RELAY_COUNT)];
//定时器定义，自动对内部的时钟脉冲进行计数
volatile unsigned int  time100ms_come_flag;
volatile unsigned int  time1s_come_flag;
//运算器的寄存器
#define  BIT_STACK_LEVEL     32
unsigned char idata bit_acc;
unsigned char idata bit_stack[BITS_TO_BS(BIT_STACK_LEVEL)];   //比特堆栈，PLC的位运算结果压栈在这里，总共有32层栈
unsigned char idata bit_stack_sp;   //比特堆栈的指针

//指令编码
unsigned int  idata plc_command_index;     //当前指令索引，
unsigned char idata plc_command_array[20]; //当前指令字节编码
#define       PLC_CODE     (plc_command_array[0])

//处理器状态
unsigned char idata plc_cpu_stop;
//通信元件个数
unsigned int  idata net_communication_count = 0;
unsigned int  idata net_global_send_index = 0; //发送令牌，指示下一个个发送的令牌

/*************************************************
 * 以下是私有的实现
 */
//内部用系统计数器
static unsigned long  idata last_tick;
static unsigned long  idata last_tick1s;

static void sys_time_tick_init(void)
{
	last_tick = get_sys_clock();
	last_tick1s = get_sys_clock();
}

//一下需要系统调用
void plc_timing_tick_process(void)
{
	unsigned long idata curr = get_sys_clock();
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

bit      plc_write_busy = 0;

unsigned char plc_write_delay(void)
{
   unsigned char ret  = 0;
   sys_lock();
   ret = plc_write_busy;
   sys_unlock();
   return ret;
}

void plc_set_busy(unsigned char busy)
{
   sys_lock();
   plc_write_busy = busy;
   sys_unlock();
}



void timing_cell_prcess(void);


/*********************************************
 * 系统初始化
 */


void PlcInit(void)
{
    plc_cpu_stop = 0;
	bit_acc = 0;
	memset(bit_stack,0,sizeof(bit_stack));
	bit_stack_sp = 0;
	io_in_get_bits(0,inputs_new,IO_INPUT_COUNT);
	memcpy(inputs_last,inputs_new,sizeof(inputs_new));
    memset(auxi_relays,0,sizeof(auxi_relays));
    memset(auxi_relays_last,0,sizeof(auxi_relays_last));
	plc_command_index = 0;
	memset(output_new,0,sizeof(output_new));
	memset(output_last,0,sizeof(output_last));
    io_out_get_bits(0,output_last,IO_OUTPUT_COUNT);
	sys_time_tick_init();
	time100ms_come_flag = 0;
	time1s_come_flag = 0;
	memset(&tim100ms_arrys,0,sizeof(tim100ms_arrys));
	memset(&tim1s_arrys,0,sizeof(tim1s_arrys));
    memset(&counter_arrys,0,sizeof(counter_arrys));
}



/**********************************************
 *  获取下一条指令的指令码
 *  也许是从EEPROM中读取的程序脚本
 *  这里一次性读取下一个指令，长度为最长指令长度
 */

extern code unsigned char plc_test_buffer[128];

void read_next_plc_code(void)
{
#if 0
    unsigned char info_buffer[sizeof(plc_sys_info)-GET_OFFSET_MEM_OF_STRUCT(plc_sys_info,enable)];
	unsigned int  index = GET_OFFSET_MEM_OF_STRUCT(plc_sys_info,enable);
	plc_sys_info * pinfo = NULL;
	eeprom_read(index,info_buffer,sizeof(info_buffer));
	pinfo = (plc_sys_info *)(info_buffer - sizeof(pinfo->plc_data_array));  //指针需要先前移动N个字节
	if(1) { //pinfo->enable == 0xAA) {
	    eeprom_read(plc_command_index,plc_command_array,sizeof(plc_command_array));
	} else {
	    PLC_CODE = PLC_NONE;
	}
#endif
    memcpy(plc_command_array,&plc_test_buffer[plc_command_index+1],sizeof(plc_command_array));
	plc_timing_tick_process();
}


unsigned long counttt = 0;

void handle_plc_command_error(void)
{
	//提示第几条指令出错
	//然后复位，或停止运行
    if(THIS_ERROR)printf("ERROR:handle_plc_command_error() reset PC\r\n");
	plc_command_index  = 0;
	plc_cpu_stop = 1;
}

unsigned char get_bitval(unsigned int index)
{
	unsigned char idata bitval = 0;
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
        bitval = BIT_IS_SET(tim100ms_arrys.event_bits,index);
	} else if(index >= TIMING1S_EVENT_BASE && index < (TIMING1S_EVENT_BASE + TIMING1S_EVENT_COUNT)) {
		index -= TIMING1S_EVENT_BASE;
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
	unsigned char idata bitval = TIMING100MS_EVENT_BASE;
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

void set_bitval(unsigned int index,unsigned char bitval)
{
	if(index >= IO_INPUT_BASE && index < (IO_INPUT_BASE+IO_INPUT_COUNT)) {
		//输入值不能修改
	} else if(index >= IO_OUTPUT_BASE && index < (IO_OUTPUT_BASE+IO_OUTPUT_COUNT)) {
		index -= IO_OUTPUT_BASE;
        SET_BIT(output_new,index,bitval);
	} else if(index >= AUXI_RELAY_BASE && index < (AUXI_RELAY_BASE + AUXI_RELAY_COUNT)) {
		index -= AUXI_RELAY_BASE;
        SET_BIT(auxi_relays,index,bitval);
	} else if(index >= COUNTER_EVENT_BASE && index < (COUNTER_EVENT_BASE+COUNTER_EVENT_COUNT)) {
	    //计数器的值不可以置位,只可以复位
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
 * 根据条件对计时器进行增加
 * 如果时间到了，则触发事件
 * 如果时间尚未到，则继续计时
 */
void timing_cell_prcess(void)
{
	unsigned int idata i;
	unsigned int idata counter;
	sys_lock();
	counter = time100ms_come_flag;
	time100ms_come_flag = 0;
	sys_unlock();
    {
	    TIM100MS_ARRAYS_T * ptiming = &tim100ms_arrys;
	    for(i=0;i<GET_ARRRYS_NUM(tim100ms_arrys.counter);i++) {
		    if(BIT_IS_SET(ptiming->enable_bits,i)) { //如果允许计时
			    if(counter > 0 && !BIT_IS_SET(ptiming->event_bits,i)) {  //如果时间事件未发生
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
				    //保持定时器
				} else {
				    //不保持
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
	{
	    TIM1S_ARRAYS_T * ptiming = &tim1s_arrys;
	    for(i=0;i<GET_ARRRYS_NUM(tim1s_arrys.counter);i++) {
		    if(BIT_IS_SET(ptiming->enable_bits,i)) { //如果允许计时
			    if(counter > 0 && !BIT_IS_SET(ptiming->event_bits,i)) {  //如果时间事件未发生
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
				    //保持定时器
				} else {
				    //不保持
					ptiming->counter[i] = 0;
					SET_BIT(ptiming->event_bits,i,0);

				}
			}
	    }
	}
}
/**********************************************
 * 打开定时器，并设定触发时间的最大值
 * 如果已经开始计时，则继续计时，如果没有开始，则开始
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
 * 关闭定时器，并取消触发事件
 */
static void timing_cell_stop(unsigned int index)
{
	if(index >= TIMING100MS_EVENT_BASE && index < (TIMING100MS_EVENT_BASE+TIMING100MS_EVENT_COUNT)) {
	    TIM100MS_ARRAYS_T * ptiming = &tim100ms_arrys;
		index -= TIMING100MS_EVENT_BASE;
		if(BIT_IS_SET(ptiming->enable_bits,index)) {
		    if(THIS_INFO)printf("stop timeimg 100ms :%d\r\n",index);
		    SET_BIT(ptiming->enable_bits,  index,0);
		}
	} else if(index >= TIMING1S_EVENT_BASE && index < (TIMING1S_EVENT_BASE+TIMING1S_EVENT_COUNT)) {
	    TIM1S_ARRAYS_T * ptiming = &tim1s_arrys;
		index -= TIMING1S_EVENT_BASE;
		if(BIT_IS_SET(ptiming->enable_bits,index)) {
		    if(THIS_INFO)printf("stop timeimg 1s :%d\r\n",index);
		    SET_BIT(ptiming->enable_bits,  index,0);
		}
	} else {
	    if(THIS_ERROR)printf("timing stop error:%d\r\n",index);
	    handle_plc_command_error();
	}
}

/**********************************************
 * 加载输入端口的输入值
 */
void handle_plc_ld(void)
{
	bit_acc = get_bitval(HSB_BYTES_TO_WORD(&plc_command_array[1]));
	if(PLC_CODE == PLC_LDI) {
		bit_acc = !bit_acc;
	}
	plc_command_index += 3;
}
/**********************************************
 * 把位运算的结果输出到输出端口中
 */
void handle_plc_out(void)
{
	set_bitval(HSB_BYTES_TO_WORD(&plc_command_array[1]),bit_acc);
	plc_command_index += 3;
}


/**********************************************
 * 与或与非运算
 */
void handle_plc_and_ani(void)
{
	unsigned char idata bittmp = get_bitval(HSB_BYTES_TO_WORD(&plc_command_array[1]));
	if(PLC_CODE == PLC_AND) {
	    bit_acc = bit_acc && bittmp;
	} else {
		bit_acc = bit_acc && (!bittmp);
	}
	plc_command_index += 3;
}

/**********************************************
 * 或或或非运算
 */
void handle_plc_or_ori(void)
{
	unsigned char idata bittmp = get_bitval(HSB_BYTES_TO_WORD(&plc_command_array[1]));
	if(PLC_CODE == PLC_OR) {
	    bit_acc = bit_acc || bittmp;
	} else if(PLC_CODE == PLC_ORI) {
		bit_acc = bit_acc || (!bittmp);
	}
	plc_command_index += 3;
}

/**********************************************
 * 加载输入端上升沿或下降沿
 */
void handle_plc_ldp_ldf(void)
{
	unsigned char idata reg;
	unsigned int idata bit_index = HSB_BYTES_TO_WORD(&plc_command_array[1]);
	reg  = get_last_bitval(bit_index)?0x01:0x00;
	reg |= get_bitval(bit_index)?     0x02:0x00;
	if(PLC_CODE == PLC_LDP) {
		if(reg == 0x02) { //只管上升沿，其他情况都是假的
			bit_acc = 1;
		} else {
		    bit_acc = 0;
		}
	} else if(PLC_CODE == PLC_LDF) {
		if(reg == 0x01) {//只管下降沿，其他情况都是假的
		    bit_acc = 1;
		} else {
		    bit_acc = 0;
		}
	}
	plc_command_index += 3;
}


/**********************************************
 * 与上升沿或下降沿
 */
void handle_plc_andp_andf(void)
{
    unsigned char idata reg;
	unsigned int idata bit_index = HSB_BYTES_TO_WORD(&plc_command_array[1]);
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
 * 或上升沿或下降沿
 */
void handle_plc_orp_orf(void)
{
	unsigned char idata reg;
	unsigned int  idata bit_index = HSB_BYTES_TO_WORD(&plc_command_array[1]);
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
 * 压栈、出栈、读栈
 */
void handle_plc_mps_mrd_mpp(void)
{
	unsigned char idata B,b;
	if(PLC_CODE == PLC_MPS) {
		if(bit_stack_sp >= BIT_STACK_LEVEL) {
		    if(THIS_ERROR)printf("堆栈溢出\r\n");
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
 * 锁或解锁指令
 */
void handle_plc_set_rst(void)
{
	unsigned int idata bit_index = HSB_BYTES_TO_WORD(&plc_command_array[1]);
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
 * 有输入去翻，没输入，不改变
 */
void handle_plc_seti(void)
{
	unsigned int idata bit_index = HSB_BYTES_TO_WORD(&plc_command_array[1]);
	if(bit_acc) {
        set_bitval(bit_index,!get_bitval(bit_index));
	}
	plc_command_index += 3;
}



/**********************************************
 * 取反结果
 */
void handle_plc_inv(void)
{
	bit_acc = !bit_acc;
	plc_command_index += 1;
}

/**********************************************
 * 输出到定时器
 * 根据编号，可能输出到100ms定时器，也可能输出到1s定时器
 */
void handle_plc_out_t(void)
{
	unsigned int idata kval;
	unsigned int idata time_index = HSB_BYTES_TO_WORD(&plc_command_array[1]);
	if(bit_acc) {
	    kval = HSB_BYTES_TO_WORD(&plc_command_array[3]);
	    timing_cell_start(time_index,kval,1,0);
	} else {
		timing_cell_stop(time_index);
	}
	plc_command_index += 5;
}
/**********************************************
 * 输出到计数器
 */
void handle_plc_out_c(void)
{
	unsigned int idata kval = HSB_BYTES_TO_WORD(&plc_command_array[3]);
	unsigned int idata index = HSB_BYTES_TO_WORD(&plc_command_array[1]);

	//判断索引是否有效
    if(index >= COUNTER_EVENT_BASE && index < (COUNTER_EVENT_BASE+COUNTER_EVENT_COUNT)) {
	    index -= COUNTER_EVENT_BASE;
	} else {
	    if(THIS_ERROR)printf("输出计数器索引值有错误!\r\n");
		handle_plc_command_error();
	    return ;
	}
	//计数器内部维持上一次的触发电平
	//触发计数器的时候，把这次的触发电平触发进计数器
	if(bit_acc) {
	    if(index < GET_ARRRYS_NUM(counter_arrys.counter)) {
	        if(!BIT_IS_SET(counter_arrys.last_trig_bits,index)) {
		        //上一次是低电平，这次是高电平，可以触发计数
			    if(counter_arrys.counter[index] < kval) {
			        if(++counter_arrys.counter[index] == kval) {
				        SET_BIT(counter_arrys.event_bits,index,1);
				    }
			    }
		    }
		}
	}
	//把电平记录到计数器去
	if(index < GET_ARRRYS_NUM(counter_arrys.counter)) {
	    SET_BIT(counter_arrys.last_trig_bits,index,bit_acc);
	}
	plc_command_index += 5;
}


/**********************************************
 * 通信位指令处理程序
 * 如果找到接收到这个指令，则处理它，然后返回一个数据
 * 这个指令可以一定允许，和激活，程序将等待上位机的位写指令
 * 并且程序也定期询问指定的远程主机ID，非此ID，不接受
 * 此ID主机可能是通用任意主机，广播主机，或指定IP的主机
 */
//网络读指令


void handle_plc_net_rb(void)
{
  //定义通信指令
  typedef struct _NetRdOptT
  {
    unsigned char op;
    unsigned char net_index;  //发送指令索引，因为发送需要间隔轮询，不能一起发送，这样会造成内存紧张
    unsigned char remote_device_addr;
    unsigned char remote_start_addr_hi;  //远端数据的起始地址
    unsigned char remote_start_addr_lo;  //远端数据的起始地址
    unsigned char local_start_addr_hi;  //远端数据的起始地址
    unsigned char local_start_addr_lo;  //远端数据的起始地址
    unsigned char data_number; //通信数据的个数
    //输入变量
    unsigned char enable_addr_hi;
    unsigned char enable_addr_lo;
    //激活一次通信
    unsigned char request_addr_hi;
    unsigned char request_addr_lo;
    //通信进行中标记
    unsigned char txing_hi;
    unsigned char txing_lo;
    //完成地址
    unsigned char done_addr_hi;
    unsigned char done_addr_lo;
    //超时定时器索引
    unsigned char timeout_addr_hi;
    unsigned char timeout_addr_lo;
    //定时超时,S
    unsigned char timeout_val;
  } NetRdOptT;
  //
  NetRdOptT * p = (NetRdOptT *)plc_command_array;
  DATA_RX_PACKET_T * prx;
  //轮到这个通信指令执行时间了
  if(net_global_send_index == p->net_index) {  //拿到令牌的
	  rx_look_up_packet(); //拿到令牌的人，必须看一遍接收的数据，要不要要说一声。
      //是的，可以发送
	  if(get_bitval(HSB_BYTES_TO_WORD(&p->enable_addr_hi))) { //这个通信单元被使能的
        if(get_bitval(HSB_BYTES_TO_WORD(&p->txing_hi))) {
            //正在发送中，判断是否超时
            if(get_bitval(HSB_BYTES_TO_WORD(&p->timeout_addr_hi))) {
                //超时了，那么，重启一次发送程序
                set_bitval(HSB_BYTES_TO_WORD(&p->done_addr_hi),0);
                goto try_again;
            } else {
                //没有超时，那么等待应答数据是否到了
                unsigned int i;
                for(i=0;i<RX_PACKS_MAX_NUM;i++) {
                    prx = &rx_ctl.rx_packs[i];
                    if(prx->finished) {
                        //MODBUS位指令翻译，根据翻译结果设置指定的位置
                        unsigned int localbits = HSB_BYTES_TO_WORD(&p->local_start_addr_hi);
                        if(THIS_ERROR)printf("rb get one rx packet.");
                        if(modbus_prase_read_multi_coils_ack(p->remote_device_addr,prx->buffer,prx->index,localbits,p->data_number)) {
                             //应答数据OK，可以完成此次数据请求，等待下一循环的请求
                            if(THIS_ERROR)printf("rb rx ack data is ok.");
                            set_bitval(HSB_BYTES_TO_WORD(&p->txing_hi),0);
                            set_bitval(HSB_BYTES_TO_WORD(&p->timeout_addr_hi),0);
                            set_bitval(HSB_BYTES_TO_WORD(&p->done_addr_hi),1);
                            break; 
		    		    }
                    }
    			}
                if(prx == NULL) {
                    //没有收到应答耶，那我们再等一等吧。。。
                    //if(THIS_ERROR)printf("rb timeout,resend packet.");
    			}
	    	}
		} else {
            //尚未发送
try_again:
    	 	if(get_bitval(HSB_BYTES_TO_WORD(&p->request_addr_hi))) {
                if(THIS_ERROR)printf("rb read coils send request.");
		    	modbus_read_multi_coils_request(HSB_BYTES_TO_WORD(&p->local_start_addr_hi),p->data_number,p->remote_device_addr);
                //然后启动定时器
                timing_cell_stop(HSB_BYTES_TO_WORD(&p->timeout_addr_hi));
                timing_cell_start(HSB_BYTES_TO_WORD(&p->timeout_addr_hi),p->timeout_val,1,0);
                //置启动标记
                set_bitval(HSB_BYTES_TO_WORD(&p->txing_hi),1);
	    	} else {
			    //允许发送，还没需要发送呢
			}
		}
	  } else {
	    //发送使能被关闭的
        //停止定时器
        timing_cell_stop(HSB_BYTES_TO_WORD(&p->timeout_addr_hi));
        set_bitval(HSB_BYTES_TO_WORD(&p->txing_hi),0);
        set_bitval(HSB_BYTES_TO_WORD(&p->done_addr_hi),0);
	  }
  } else {
	  //没有拿到令牌，那就等下一次吧
  }
  plc_command_index += sizeof(NetRdOptT);
}


void handle_plc_net_wb(void)
{
}



void PlcProcess(void)
{
    counttt++;
    if(counttt == 74) {
        counttt++;
    }
    if(plc_cpu_stop) {
		return ;
	}
	//输入处理,读取IO口的输入
	io_in_get_bits(0,inputs_new,IO_INPUT_COUNT);
	//处理通信程序
	//初始化通信令牌
    net_communication_count = plc_test_buffer[0]; //打个比喻，代码里面存在5处发送
	plc_command_index = 0;
 next_plc_command:
	read_next_plc_code();
	//逻辑运算,调用一次，运行一次用户的程序
	switch(PLC_CODE)
	{
	case PLC_END: //指令结束，处理后事
		plc_command_index = 0;
		goto plc_command_finished;
	case PLC_LD: 
	case PLC_LDI:
		handle_plc_ld();
		break;
    case PLC_LDKH:
        bit_acc = 1;
        plc_command_index++;
        break;
    case PLC_LDKL:
        bit_acc = 0;
        plc_command_index++;
        break;
    case PLC_SEI:
        handle_plc_seti();
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
    case PLC_NETRB:
        handle_plc_net_rb(); 
        break;
    case PLC_NETWB:
    case PLC_NETRW:
    case PLC_NETWW:
	default:
	    handle_plc_command_error();
		break;
	case PLC_NONE: //空指令，直接跳过
        plc_command_index++;
		break;
	}
	goto next_plc_command;
 plc_command_finished:
	//输出处理，把运算结果输出到继电器中
	io_out_set_bits(0,output_new,IO_OUTPUT_COUNT);
	memcpy(output_last,output_new,sizeof(output_new));
	//后续处理
	memcpy(inputs_last,inputs_new,sizeof(inputs_new));
	//辅助继电器
	memcpy(auxi_relays_last,auxi_relays,sizeof(auxi_relays));
	//系统时间处理，可以拿到定时器中断处理
	memcpy(tim100ms_arrys.event_bits_last,tim100ms_arrys.event_bits,sizeof(tim100ms_arrys.event_bits));
	//
	memcpy(tim1s_arrys.event_bits_last,tim1s_arrys.event_bits,sizeof(tim1s_arrys.event_bits));
	//
	memcpy(counter_arrys.event_bits_last,counter_arrys.event_bits,sizeof(counter_arrys.event_bits));
	//定时器处理
	timing_cell_prcess();
	//计数器处理
	plc_set_busy(0);
    //把接收到的无用的数据清理掉
    rx_free_useless_packet(net_communication_count);
    if(++net_global_send_index >= net_communication_count) {
        //令牌溢出，重新来一遍，每个人都有机会做一次通信动作
        net_global_send_index = 0;
    }
}






