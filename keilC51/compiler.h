#ifndef __COMPILER_H__
#define __COMPILER_H__

typedef unsigned char BYTE;
typedef unsigned int  WORD;
typedef unsigned long DWORD;

#define GET_OFFSET_MEM_OF_STRUCT(type,member)   (&(((type *)0)->member) - ((type *)0))
#define GET_ARRRYS_NUM(type)                    (sizeof(type)/sizeof((&type)[0]))
#define SET_BIT(Bitarrys,Index,On)      do{ if(On) { (Bitarrys)[(Index)/8] |=  code_msk[(Index)%8]; } else { \
                                                    (Bitarrys)[(Index)/8] &= ~code_msk[(Index)%8]; } } while(0)
#define BIT_IS_SET(Bitarrys,Index)       (((Bitarrys)[(Index)/8]&code_msk[(Index)%8])?1:0)

//字节数组编程WORD型
#define LSB_BYTES_TO_WORD(bytes)            ((((WORD)((bytes)[1]))<<8)|(bytes)[0])
#define HSB_BYTES_TO_WORD(bytes)            ((((WORD)((bytes)[0]))<<8)|(bytes)[1])

#endif

