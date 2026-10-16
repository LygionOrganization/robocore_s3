/*
 * lyst.h
 * 串行通信协议指令定义
 * 日期: 2026.5.18
 * 作者: txl
 */

#ifndef _LYST_H
#define _LYST_H

typedef	char s8;
typedef	unsigned char u8;	
typedef	unsigned short u16;	
typedef	short s16;
typedef	unsigned long u32;	
typedef	long s32;

#define LYST_PING 0x01
#define LYST_READ 0x02
#define LYST_WRITE 0x03
#define LYST_REG_WRITE 0x04
#define LYST_REG_ACTION 0x05
#define LYST_SYNC_READ 0x82
#define LYST_SYNC_WRITE 0x83
#define LYST_RECOVERY 0x06
#define LYST_RESET 0x0A
#define LYST_CAL 0x0B

//波特率定义
#define	LY_1M 0
#define	LY_0_5M 1
#define	LY_250K 2
#define	LY_128K 3
#define	LY_115200 4
#define	LY_76800 5
#define	LY_57600 6
#define	LY_38400 7

#endif