#ifndef __PLC_PRASE_H__
#define __PLC_PRASE_H__

#define  PLC_DATA_LEN     512


//--------------------------µØÖ·±àºÅ-------ÊýÁ¿-----------
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


typedef struct _plc_sys_info
{
    unsigned char plc_data_array[PLC_DATA_LEN];
    unsigned char enable;
	unsigned char power_run;
	unsigned char len_hi;
	unsigned char len_lo;
	unsigned char crc_hi;
	unsigned char crc_lo;
} plc_sys_info;


extern void PlcInit(void);
extern void PlcProcess(void);
extern void plc_timing_tick_process(void);
extern unsigned char plc_write_delay(void);
extern void plc_set_busy(unsigned char busy);


//#define   DEBUG_PLC    


#endif