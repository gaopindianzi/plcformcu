#ifndef __SERIAL_COMM_PACKER_H__
#define __SERIAL_COMM_PACKER_H__

#define  PACK_MAX_RX_SIZE   64
extern void prase_in_stream(unsigned char ch);
extern unsigned char serial_stream_rx_finished(void);
extern void serial_clear_stream(void);
extern void SerialRxCheckTimeoutTick(void); //系统主循环调用

#endif
