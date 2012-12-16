#ifndef __SYS_INFO_H__
#define __SYS_INFO_H__

#include "plc_prase.h"

struct sys_info_type
{
    unsigned char modbus_addr;
};

//����EEPROM�ĵ�ַ�ֲ�
//����ֻ��һ���ֲ������ṹ�壬���㴦��
typedef struct _eeprom_data_map
{
    plc_sys_info          plc_info;
	struct sys_info_type  sys_info;
} eeprom_data_map;

extern struct sys_info_type  sys_info;

#endif


