#ifndef __SERIAL_COMM_PACKER_H__
#define __SERIAL_COMM_PACKER_H__



#define  PACK_MAX_RX_SIZE   64
#define  RX_PACKS_MAX_NUM   2
#define  TX_PACKS_MAX_NUM   1


typedef struct _APP_PACK_HEAD_T
{
  unsigned char port_num_hi;
  unsigned char port_num_lo;
  unsigned char app_id;
} APP_PACK_HEAD_T;


typedef struct _APP_PACK_ERROR_T
{
  unsigned char port_num_hi;
  unsigned char port_num_lo;
  unsigned char app_id;
  unsigned char error_hi;
  unsigned char error_lo;
} APP_PACK_ERROR_T;

#define  NOT_SUPPORT_THIS_COMMAND    0x0001


typedef struct _DATA_RX_PACKET_T
{
  unsigned char buffer[PACK_MAX_RX_SIZE];
  unsigned int  look_up_times;  //别查看次数
  unsigned char index;
  unsigned char state;
  volatile unsigned char finished;
} DATA_RX_PACKET_T;



typedef struct _RX_PACKS_CTL_T
{
	DATA_RX_PACKET_T   rx_packs[RX_PACKS_MAX_NUM];
	DATA_RX_PACKET_T * pcurrent_rx;
} RX_PACKS_CTL_T;





typedef struct _DATA_TX_PACKET_T
{
  unsigned char buffer[PACK_MAX_RX_SIZE + PACK_MAX_RX_SIZE/2];
  unsigned char index;  //发送的大小
  volatile   unsigned char finished;
} DATA_TX_PACKET_T;

typedef struct _DATA_TX_CTL_T
{
	DATA_TX_PACKET_T   packet[TX_PACKS_MAX_NUM];
	DATA_TX_PACKET_T * ptx_sending;
} DATA_TX_CTL_T;


extern RX_PACKS_CTL_T rx_ctl;
extern DATA_TX_CTL_T  tx_ctl;

extern void pack_prase_in(unsigned char ch);
extern void rx_look_up_packet(void);
extern void rx_free_useless_packet(unsigned int net_communication_count);
extern unsigned int tx_pack_and_send(unsigned char * src,unsigned int len);
extern void serial_rx_tx_initialize(void);
extern unsigned int modbus_prase_read_multi_coils_ack(unsigned char slave_device,unsigned char * rx_buffer,unsigned int len,unsigned int startbit,unsigned int count);
extern unsigned char modbus_read_multi_coils_request(unsigned int start_coils,unsigned int coils_num,unsigned char slave_device);
extern void dumpdata(unsigned char * buf,unsigned int len);

#endif
