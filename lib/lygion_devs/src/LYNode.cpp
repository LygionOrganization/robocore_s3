/*
 * lynode.cpp
 * lynode应用层程序
 * 日期: 2026.5.18
 * 作者: txl
 */

#include "LYNode.h"

NodeClass::NodeClass()
{
	End = 0;
}

NodeClass::NodeClass(u8 End):LYSerial(End)
{
}

NodeClass::NodeClass(u8 End, u8 Level):LYSerial(End, Level)
{
}

//根据led的ch将rgb(0~7)颜色传输至node模块缓存
int NodeClass::ledSingleCtrl(u8 node_id, u8 led_ch, u8 r, u8 g, u8 b)
{
	u8 rgb = (r<<6)|(g<<3)|(b<<1);
	u8 ch = led_ch + LY_NODE_RGB_CH1 - 1;
	return writeByte(node_id, ch, rgb);
}

//刷新node模块点亮相应led灯,默认led为2通道
int NodeClass::ledFlush(u8 node_id, u8 led_num)
{
	return writeByte(node_id, LY_NODE_RGB_CTL, led_num);
}

//全部led写入相同rgb(0~7)颜色传输至node模块缓存
int NodeClass::ledAllCtrl(u8 node_id, u8 num, u8 r, u8 g, u8 b)
{
	u8 rgb = (r<<6)|(g<<3)|(b<<1);
	u8 buf[num+1];
	buf[0] = num;
	for(u8 i= 0; i<num; i++){
		buf[i+1] = rgb;
	}
	return genWrite(node_id, LY_NODE_RGB_CTL, buf, num+1);
}

//将node模块中sbus数据通过总线缓存至内存
int NodeClass::sbusFlush(u8 node_id)
{
	return Read(node_id, LY_NODE_SBUS_STATUS, sbus_frame, sizeof(sbus_frame));
}

//解码sbus缓存返回通道数据
u16 NodeClass::sbusGetch(u8 ch)
{
	return (sbus_frame[1+(ch-1)*2]|(sbus_frame[2+(ch-1)*2]<<8));
}

//解码sbus缓存返回状态
u8 NodeClass::sbusStatus()
{
	return sbus_frame[0];
}

//subs通道数
u8 NodeClass::subsGetNum()
{
	return (sizeof(sbus_frame)-1)/2;
}

//控制node的PWM通道输出:0~1000
u8 NodeClass::pwmCtl(u8 node_id, u8 pwm_ch, u16 pwm_val)
{
	return writeWord(node_id, LY_NODE_PWM_CH1+(pwm_ch-1)*2, pwm_val);
}

//获取adc_ch的adc值(0~4095)
int NodeClass::adcGetch(u8 node_id, u8 adc_ch)
{
	return readWord(node_id, LY_NODE_ADC_CH1+(adc_ch-1)*2);
}

//adc通道值(adc_ch1)转电源电压
float NodeClass::adcPowVol(u8 node_id)
{
	int adc_val = adcGetch(node_id, 1);
	float pow_vol = 0.0f;
	if(!u8Error){
		pow_vol = ((5.0f/4096)*adc_val*(10.0f+1.0f));
	}
	return pow_vol;
}

int NodeClass::unLockEprom(u8 ID)
{
	return writeByte(ID, LY_NODE_LOCK, 0);
}

int NodeClass::LockEprom(u8 ID)
{
	return writeByte(ID, LY_NODE_LOCK, 1);
}
