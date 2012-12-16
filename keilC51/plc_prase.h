#ifndef __PLC_PRASE_H__
#define __PLC_PRASE_H__

#define  PLC_DATA_LEN     512


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



//#define   DEBUG_PLC    


#endif