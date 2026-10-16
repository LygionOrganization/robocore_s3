/*
 * lyserial.h
 * 串行通信硬件接口层程序
 * 日期: 2026.518
 * 作者: txl
 */

#ifndef _LYSERIAL_H
#define _LYSERIAL_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "LYS.h"

class LYSerial : public LYS
{
public:
	LYSerial();
	LYSerial(u8 End);
	LYSerial(u8 End, u8 Level);

protected:
	int writeSCS(unsigned char *nDat, int nLen);//输出nLen字节
	int readSCS(unsigned char *nDat, int nLen);//输入nLen字节
	int readSCS(unsigned char *nDat, int nLen, unsigned long TimeOut);
	int writeSCS(unsigned char bDat);//输出1字节
	void rFlushSCS();//
	void wFlushSCS();//
public:
	unsigned long IOTimeOut;//输入输出超时
	HardwareSerial *pSerial;//串口指针
};

#endif