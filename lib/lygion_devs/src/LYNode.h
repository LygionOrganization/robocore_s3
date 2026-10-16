/*
 * lynode.h
 * lynode应用层程序
 * 日期: 2026.5.18
 * 作者: txl
 */

#ifndef _LY_NODE_H
#define _LY_NODE_H

//内存表定义
//-------EPROM(只读)--------
#define LY_NODE_MODEL_L 3
#define LY_NODE_MODEL_H 4

//-------EPROM(读写)--------
#define LY_NODE_ID 5
#define LY_NODE_BAUD_RATE 6

//-------SRAM(读写)--------
#define LY_NODE_PWM_CH1 34
#define LY_NODE_PWM_CH2 36
#define LY_NODE_RGB_CTL 42
#define LY_NODE_RGB_CH1 43
#define LY_NODE_RGB_CH2 44
#define LY_NODE_LOCK 51

//-------SRAM(只读)--------
#define LY_NODE_SBUS_STATUS 67
#define LY_NODE_ADC_CH1 100

#include "LYSerial.h"

class NodeClass : public LYSerial
{
public:
	NodeClass();
	NodeClass(u8 End);
	NodeClass(u8 End, u8 Level);
	int ledSingleCtrl(u8 node_id, u8 led_ch, u8 r, u8 g, u8 b);//根据led的ch将rgb(0~7)颜色传输至node模块缓存
	int ledAllCtrl(u8 node_id, u8 num, u8 r, u8 g, u8);//全部led写入相同rgb(0~7)颜色传输至node模块缓存
	int ledFlush(u8 node_id, u8 led_num = 2);//刷新node模块点亮相应led灯,默认led为2通道
	int sbusFlush(u8 node_id);//将node模块中sbus数据通过总线缓存至内存
	u16 sbusGetch(u8 ch);//解码sbus缓存返回通道数据
	u8 sbusStatus();//解码sbus缓存返回状态
	u8 subsGetNum();//subs通道数
	u8 pwmCtl(u8 node_id, u8 pwm_ch, u16 pwm_val);//控制node的PWM通道输出:0~1000
	int adcGetch(u8 node_id, u8 adc_ch);//获取adc_ch的adc值(0~4095)
	float adcPowVol(u8 node_id);//获取(adc_ch1)电源电压
	int unLockEprom(u8 ID);//eprom解锁
	int LockEprom(u8 ID);//eprom加锁
private:
	u8 sbus_frame[16*2+1];//sbus缓存
};

#endif