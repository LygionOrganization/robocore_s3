/*
 * lyserial.h
 * 串行通信硬件接口层程序
 * 日期: 2026.518
 * 作者: txl
 */

#include "LYSerial.h"

LYSerial::LYSerial()
{
	IOTimeOut = 10;
	pSerial = NULL;
}

LYSerial::LYSerial(u8 End):LYS(End)
{
	IOTimeOut = 10;
	pSerial = NULL;
}

LYSerial::LYSerial(u8 End, u8 Level):LYS(End, Level)
{
	IOTimeOut = 10;
	pSerial = NULL;
}

int LYSerial::readSCS(unsigned char *nDat, int nLen, unsigned long TimeOut)
{
	int Size = 0;
	int ComData;
	unsigned long t_begin = millis();
	unsigned long t_user;
	while(1){
		ComData = pSerial->read();
		if(ComData!=-1){
			if(nDat){
				nDat[Size] = ComData;
			}
			Size++;
		}
		if(Size>=nLen){
			break;
		}
		t_user = millis() - t_begin;
		if(t_user>TimeOut){
			break;
		}
	}
	return Size;
}

int LYSerial::readSCS(unsigned char *nDat, int nLen)
{
	int Size = 0;
	int ComData;
	unsigned long t_begin = millis();
	unsigned long t_user;
	while(1){
		ComData = pSerial->read();
		if(ComData!=-1){
			if(nDat){
				nDat[Size] = ComData;
			}
			Size++;
			t_begin = millis();
		}
		if(Size>=nLen){
			break;
		}
		t_user = millis() - t_begin;
		if(t_user>IOTimeOut){
			break;
		}
	}
	return Size;
}

int LYSerial::writeSCS(unsigned char *nDat, int nLen)
{
	if(nDat==NULL){
		return 0;
	}
	return pSerial->write(nDat, nLen);
}

int LYSerial::writeSCS(unsigned char bDat)
{
	return pSerial->write(&bDat, 1);
}

void LYSerial::rFlushSCS()
{
	while(pSerial->read()!=-1);
}

void LYSerial::wFlushSCS()
{
}