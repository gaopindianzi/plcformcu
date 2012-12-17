#ifndef __SERIAL_COMM_PACKER_H__
#define __SERIAL_COMM_PACKER_H__

#define  PACK_MAX_RX_SIZE   64
extern unsigned char stream_in_buffer[PACK_MAX_RX_SIZE];


extern void prase_in_stream(unsigned char ch);
extern unsigned char serial_stream_rx_finished(void);
extern void serial_clear_stream(void);
extern void SerialRxCheckTimeoutTick(void); //ϵͳ��ѭ������
extern void stream_packet_send(unsigned char * buffer,unsigned int len);
extern unsigned int get_stream_len(void);
extern unsigned char * get_stream_ptr(void);

#endif