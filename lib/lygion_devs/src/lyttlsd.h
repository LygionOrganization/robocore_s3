/*
 * lyttlsd.h
 * ttlsd应用层程序
 * 日期: 2026.5.18
 * 作者: txl
 */

#ifndef _LYLY_TTLSD_H
#define _LYLY_TTLSD_H

//内存表定义
//-------EPROM(只读)--------
#define LY_TTLSD_MODEL_L 3
#define LY_TTLSD_MODEL_H 4

//-------EPROM(读写)--------
#define LY_TTLSD_ID 5
#define LY_TTLSD_BAUD_RATE 6
#define LY_TTLSD_MIN_ANGLE_LIMIT_L 9
#define LY_TTLSD_MIN_ANGLE_LIMIT_H 10
#define LY_TTLSD_MAX_ANGLE_LIMIT_L 11
#define LY_TTLSD_MAX_ANGLE_LIMIT_H 12
#define LY_LY_TTLSD_RESOLUTION 30
#define LY_TTLSD_MODE 33

//-------SRAM(读写)--------
#define LY_TTLSD_TORQUE_ENABLE 40
#define LY_TTLSD_ACC 41
#define LY_TTLSD_GOAL_POSITION_L 42
#define LY_TTLSD_GOAL_POSITION_H 43
#define LY_TTLSD_GOAL_TORQUE_L 44
#define LY_TTLSD_GOAL_TORQUE_H 45
#define LY_TTLSD_GOAL_SPEED_L 46
#define LY_TTLSD_GOAL_SPEED_H 47
#define LY_TTLSD_TORQUE_LIMIT_L 48
#define LY_TTLSD_TORQUE_LIMIT_H 49
#define LY_TTLSD_LOCK 55

//-------SRAM(只读)--------
#define LY_TTLSD_PRESENT_POSITION_L 56
#define LY_TTLSD_PRESENT_POSITION_H 57
#define LY_TTLSD_PRESENT_SPEED_L 58
#define LY_TTLSD_PRESENT_SPEED_H 59
#define LY_TTLSD_PRESENT_LOAD_L 60
#define LY_TTLSD_PRESENT_LOAD_H 61
#define LY_TTLSD_PRESENT_VOLTAGE 62
#define LY_TTLSD_PRESENT_TEMPERATURE 63
#define LY_TTLSD_MOVING 66
#define LY_TTLSD_PRESENT_CURRENT_L 69
#define LY_TTLSD_PRESENT_CURRENT_H 70

#include "LYSerial.h"

class TTLSDClass : public LYSerial
{
public:
	TTLSDClass();
	TTLSDClass(u8 End);
	TTLSDClass(u8 End, u8 Level);
	int WritePosEx(u8 ID, u16 Position, u16 Speed, u8 ACC = 0, u16 Torque = 0);//普通写单个舵机位置指令
	int RegWritePosEx(u8 ID, u16 Position, u16 Speed, u8 ACC = 0, u16 Torque = 0);//异步写单个舵机位置指令(RegWriteAction生效)
	void SyncWritePosEx(u8 ID[], u8 IDN, u16 Position[], u16 Speed[], u8 ACC[], u16 Torque[]);//同步写多个舵机位置指令
	void SyncWriteSpe(u8 ID[], u8 IDN, s16 Speed[], u8 ACC[], u16 Torque[]);//同步写多个舵机速度指令
	int SetMode(u8 ID, u8 Mode);//模式设置
	int WriteSpe(u8 ID, s16 Speed, u8 ACC = 0, u16 Torque = 0);//恒速模式控制指令
	int WriteTorque(u8 ID, s16 Torque);
	int EnableTorque(u8 ID, u8 Enable);//扭力控制指令
	int unLockEprom(u8 ID);//eprom解锁
	int LockEprom(u8 ID);//eprom加锁
	int CalibrationOfs(u8 ID, u16 ofs);//任意位置校准
	int FeedBack(int ID);//反馈舵机信息
	int ReadPos(int ID);//读位置
	int ReadSpeed(int ID);//读速度
	int ReadLoad(int ID);//读输出至电机的电压百分比(0~1000)
	int ReadVoltage(int ID);//读电压
	int ReadTemper(int ID);//读温度
	int ReadMove(int ID);//读移动状态
	int ReadCurrent(int ID);//读电流
private:
	u8 Mem[LY_TTLSD_PRESENT_CURRENT_H-LY_TTLSD_PRESENT_POSITION_L+1];
};

#endif