/*
 * lyttlsd.cpp
 * lyttlsd应用层程序
 * 日期: 2026.5.18
 * 作者: txl
 */

#include "lyttlsd.h"

TTLSDClass::TTLSDClass()
{
	End = 0;
}

TTLSDClass::TTLSDClass(u8 End):LYSerial(End)
{
}

TTLSDClass::TTLSDClass(u8 End, u8 Level):LYSerial(End, Level)
{
}

int TTLSDClass::WritePosEx(u8 ID, u16 Position, u16 Speed, u8 ACC, u16 Torque)
{
	u8 bBuf[7];
	bBuf[0] = ACC;
	Host2SCS(bBuf+1, bBuf+2, Position);
	Host2SCS(bBuf+3, bBuf+4, Torque);
	Host2SCS(bBuf+5, bBuf+6, Speed);
	
	return genWrite(ID, LY_TTLSD_ACC, bBuf, 7);
}

int TTLSDClass::RegWritePosEx(u8 ID, u16 Position, u16 Speed, u8 ACC, u16 Torque)
{
	u8 bBuf[7];
	bBuf[0] = ACC;
	Host2SCS(bBuf+1, bBuf+2, Position);
	Host2SCS(bBuf+3, bBuf+4, Torque);
	Host2SCS(bBuf+5, bBuf+6, Speed);
	
	return regWrite(ID, LY_TTLSD_ACC, bBuf, 7);
}

void TTLSDClass::SyncWritePosEx(u8 ID[], u8 IDN, u16 Position[], u16 Speed[], u8 ACC[], u16 Torque[])
{
    u8 offbuf[7*IDN];
    for(u8 i = 0; i<IDN; i++){
		if(ACC){
			offbuf[i*7] = ACC[i];
		}else{
			offbuf[i*7] = 0;
		}
        Host2SCS(offbuf+i*7+1, offbuf+i*7+2, Position[i]);
        Host2SCS(offbuf+i*7+3, offbuf+i*7+4, Torque[i]);
        Host2SCS(offbuf+i*7+5, offbuf+i*7+6, Speed[i]);
    }
    syncWrite(ID, IDN, LY_TTLSD_ACC, offbuf, 7);
}

void TTLSDClass::SyncWriteSpe(u8 ID[], u8 IDN, s16 Speed[], u8 ACC[], u16 Torque[])
{
    u8 offbuf[7*IDN];
    for(u8 i = 0; i<IDN; i++){
		if(Speed[i]<0){
			Speed[i] = -Speed[i];
			Speed[i] |= (1<<15);
		}
		if(ACC){
			offbuf[i*7] = ACC[i];
		}else{
			offbuf[i*7] = 0;
		}
        Host2SCS(offbuf+i*7+1, offbuf+i*7+2, 0);
        Host2SCS(offbuf+i*7+3, offbuf+i*7+4, Torque[i]);
        Host2SCS(offbuf+i*7+5, offbuf+i*7+6, Speed[i]);
    }
    syncWrite(ID, IDN, LY_TTLSD_ACC, offbuf, 7);
}

int TTLSDClass::WriteSpe(u8 ID, s16 Speed, u8 ACC, u16 Torque)
{
	if(Speed<0){
		Speed = -Speed;
		Speed |= (1<<15);
	}
	u8 bBuf[7];
	bBuf[0] = ACC;
	Host2SCS(bBuf+1, bBuf+2, 0);
	Host2SCS(bBuf+3, bBuf+4, Torque);
	Host2SCS(bBuf+5, bBuf+6, Speed);
	
	return genWrite(ID, LY_TTLSD_ACC, bBuf, 7);
}

int TTLSDClass::WriteTorque(u8 ID, s16 Torque)
{
	if(Torque<0){
		Torque = -Torque;
		Torque |= (1<<15);
	}
	return writeWord(ID, LY_TTLSD_GOAL_TORQUE_L, Torque);
}

int TTLSDClass::EnableTorque(u8 ID, u8 Enable)
{
	return writeByte(ID, LY_TTLSD_TORQUE_ENABLE, Enable);
}

int TTLSDClass::unLockEprom(u8 ID)
{
	EnableTorque(ID, 0);
	return writeByte(ID, LY_TTLSD_LOCK, 0);
}

int TTLSDClass::LockEprom(u8 ID)
{
	return writeByte(ID, LY_TTLSD_LOCK, 1);
}

int TTLSDClass::CalibrationOfs(u8 ID, u16 ofs)
{
	return Recal(ID, ofs);
}

int TTLSDClass::FeedBack(int ID)
{
	int nLen = Read(ID, LY_TTLSD_PRESENT_POSITION_L, Mem, sizeof(Mem));
	if(nLen!=sizeof(Mem)){
		return -1;
	}
	return nLen;
}

int TTLSDClass::ReadPos(int ID)
{
	int Pos = -1;
	if(ID==-1){
		Pos = Mem[LY_TTLSD_PRESENT_POSITION_H-LY_TTLSD_PRESENT_POSITION_L];
		Pos <<= 8;
		Pos |= Mem[LY_TTLSD_PRESENT_POSITION_L-LY_TTLSD_PRESENT_POSITION_L];
	}else{
		Pos = readWord(ID, LY_TTLSD_PRESENT_POSITION_L);
	}
	return Pos;
}

int TTLSDClass::ReadSpeed(int ID)
{
	int Speed = -1;
	if(ID==-1){
		Speed = Mem[LY_TTLSD_PRESENT_SPEED_H-LY_TTLSD_PRESENT_POSITION_L];
		Speed <<= 8;
		Speed |= Mem[LY_TTLSD_PRESENT_SPEED_L-LY_TTLSD_PRESENT_POSITION_L];
	}else{
		Speed = readWord(ID, LY_TTLSD_PRESENT_SPEED_L);
	}
	if(Speed&(1<<15)){
		Speed = -(Speed&~(1<<15));
	}	
	return Speed;
}

int TTLSDClass::ReadLoad(int ID)
{
	int Load = -1;
	if(ID==-1){
		Load = Mem[LY_TTLSD_PRESENT_LOAD_H-LY_TTLSD_PRESENT_POSITION_L];
		Load <<= 8;
		Load |= Mem[LY_TTLSD_PRESENT_LOAD_L-LY_TTLSD_PRESENT_POSITION_L];
	}else{
		Load = readWord(ID, LY_TTLSD_PRESENT_LOAD_L);
	}
	if(Load&(1<<10)){
		Load = -(Load&~(1<<10));
	}
	return Load;
}

int TTLSDClass::ReadVoltage(int ID)
{	
	int Voltage = -1;
	if(ID==-1){
		Voltage = Mem[LY_TTLSD_PRESENT_VOLTAGE-LY_TTLSD_PRESENT_POSITION_L];	
	}else{
		Voltage = readByte(ID, LY_TTLSD_PRESENT_VOLTAGE);
	}
	return Voltage;
}

int TTLSDClass::ReadTemper(int ID)
{	
	int Temper = -1;
	if(ID==-1){
		Temper = Mem[LY_TTLSD_PRESENT_TEMPERATURE-LY_TTLSD_PRESENT_POSITION_L];	
	}else{
		Temper = readByte(ID, LY_TTLSD_PRESENT_TEMPERATURE);
	}
	return Temper;
}

int TTLSDClass::ReadMove(int ID)
{
	int Move = -1;
	if(ID==-1){
		Move = Mem[LY_TTLSD_MOVING-LY_TTLSD_PRESENT_POSITION_L];	
	}else{
		Move = readByte(ID, LY_TTLSD_MOVING);
	}
	return Move;
}

int TTLSDClass::ReadCurrent(int ID)
{
	int Current = -1;
	if(ID==-1){
		Current = Mem[LY_TTLSD_PRESENT_CURRENT_H-LY_TTLSD_PRESENT_POSITION_L];
		Current <<= 8;
		Current |= Mem[LY_TTLSD_PRESENT_CURRENT_L-LY_TTLSD_PRESENT_POSITION_L];
	}else{
		Current = readWord(ID, LY_TTLSD_PRESENT_CURRENT_L);
	}
	if(Current&(1<<15)){
		Current = -(Current&~(1<<15));
	}	
	return Current;
}

int TTLSDClass::SetMode(u8 ID, u8 Mode)
{
	return writeByte(ID, LY_TTLSD_MODE, Mode);		
}
