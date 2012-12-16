#ifndef __EEPROM_H__
#define __EEPROM_H__

#define  SECTOR_SIZE    512   //写新的页面之前要先擦除


unsigned char Byte_Read (unsigned int address);
void Sector_Erase (unsigned int address);
void Byte_Program (unsigned int address, unsigned char write_data );

extern void eeprom_secotr_erase(unsigned int page);
extern void eeprom_write(unsigned int start_addr,unsigned char * pvuf,unsigned int len);
extern void eeprom_read(unsigned int start_addr,unsigned char * pbuf,unsigned int len);
extern unsigned int eeprom_compare(unsigned int start_addr,unsigned char * pbuf,unsigned int len);
#endif


