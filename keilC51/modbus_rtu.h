#ifndef __MODBUS_RTU_H__
#define __MODBUS_RTU_H__



//组态王使用最简单的几个命令
#define FUNC_READ_COIL_STATUS           0x01
#define FUNC_READ_HOLDING_REGISTER      0x03
#define FUNC_FORCE_SINGLE_COIL          0x05
#define FUNC_FORCE_MULTIPLE_COILS       0x0F
#define FUNC_PRESET_MULTIPLE_REGISTERS  0x10

struct modbus_crc_type
{
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_force_single_coil_req_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char reg_munber_hi;
  unsigned char reg_number_lo;
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_force_single_coil_ack_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char reg_munber_hi;
  unsigned char reg_number_lo;
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_read_coil_status_req_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char reg_munber_hi;
  unsigned char reg_number_lo;
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_read_coil_status_ack_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char byte_count;
  unsigned char data_arry[BITS_TO_BS(REAL_IO_OUT_NUM)];
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_force_multiple_coils_req_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char quantiry_coils_hi;
  unsigned char quantiry_coils_lo;
  unsigned char byte_count;
  unsigned char force_data_base;
  //后面还有很多
};

struct modbus_force_multiple_coils_ack_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char quantiry_coils_hi;
  unsigned char quantiry_coils_lo;
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_read_holding_register_req_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char reg_number_hi;
  unsigned char reg_number_lo;
  unsigned char crc_hi;
  unsigned char crc_lo;
};

struct modbus_read_holding_register_ack_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char byte_count;
  unsigned char data_base;
  //后面还有不定长的数据
};



struct modbus_preset_multiple_register_req_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char reg_munber_hi;
  unsigned char reg_number_lo;
  unsigned char byte_count;
  unsigned char data_base;
  //后面还有不定长的数据
  //unsigned char crc_hi;
  //unsigned char crc_lo;
};

struct modbus_preset_multiple_register_ack_type
{
  unsigned char slave_addr;
  unsigned char function;
  unsigned char start_addr_hi;
  unsigned char start_addr_lo;
  unsigned char reg_munber_hi;
  unsigned char reg_number_lo;
  unsigned char crc_hi;
  unsigned char crc_lo;
};

extern void UartReceivetoModbusRtu(unsigned char ch);
extern void UartReceivetoModbusRtuTimeTick(void);
extern unsigned int CRC16(unsigned char *Array,unsigned int Len);

#endif

