#ifndef _global_h
#define _global_h

#include "N76E003.h"
#include "SFR_Macro.h"
#include "Function_define.h"
#include "Common.h"
#include "Delay.h"

enum Enum_Bool {false, true};

enum Enum_NetState {offline, connecting, connectToAp, connectToService, online};

enum Eunm_LedState {dark, light, blink};

enum Enum_ExcState {free, start, ing, end, waitAck, ack};

enum Enum_RunState {rs_normal, rs_normal1, rs_normal2,
					rs_sLH, rs_sLM, rs_kLH, rs_kLM,
					rs_sWH, rs_sWM, rs_kWH, rs_kWM, rs_wCD,
					rs_rtcH, rs_rtcM,
					rs_wait};

typedef struct
{
	unsigned char hour;
	unsigned char minute;
	
	unsigned char startLightHour;	//开始照明-时
	unsigned char startLightMinute;	//开始照明-分
	unsigned char keepLightHour;	//持续照明-时
	unsigned char keepLightMinute;	//持续照明-分
	unsigned char startWaterHour;	//开始浇水-时
	unsigned char startWaterMinute;	//开始浇水-分
	unsigned char keepWaterHour;	//持续浇水-时
	unsigned char keepWaterMinute;	//持续浇水-分
	unsigned char waterCycleDay;	//浇水周期-天

	int setWaitTimeOut;

	//主要用于管理设置数据时的各个状态
	//当进入设置模式时，状态为 start ，此时将发送设置参数获取请求并切换为 waitAck
	//当接收到响应时，切换为 ing，此时开始设置，设置完成切换为 end，此时发送设置参数设置请求并切换为 waitAck
	enum Enum_ExcState setDataSate;
	unsigned int switchTimeCount;
	enum Enum_Bool setAppointmentFlag;
	enum Enum_Bool setRtcFlag;

	unsigned char setSateData;	//参考通信协议的1.3.1
	enum Enum_ExcState setSateSate;
}Struct_SetData;

typedef struct
{
	enum Enum_NetState wifi;
	unsigned char appointment;
	unsigned char light;
	unsigned char water;
	unsigned char nowater;

	unsigned int nextStartLightCountDown;	//距离下一次开始照明倒计时-分
	unsigned int endLightCountDown;	//距离结束照明倒计时-分
	unsigned int nextStartWaterCountDown;	//距离下一次开始浇水倒计时-分
	unsigned int endWaterCountDown;	//距离结束浇水倒计时-分

	enum Enum_Bool getDataFlag;
	unsigned int getDataTimeCOunt;
}Struct_DeviceData;

typedef struct
{
	unsigned int temperature;	//温度
	unsigned int getTemperatureTimeCount;	//获取温度的计时
	enum Enum_ExcState getTemperatureSate;	//获取温度的状态

	unsigned int timerCount1ms;
	unsigned int timerCount1s;

	enum Enum_Bool warnFlag;
	unsigned int warnCountDown;

	enum Enum_RunState runState;
	unsigned int switchTimeCount;

	enum Enum_Bool keyPressBeepFlag;
}Struct_WorkData;

typedef struct
{
	enum Enum_NetState wifi;
	unsigned char appointment;
	unsigned char light;
	unsigned char water;
	unsigned char nowater;

	unsigned int number;
	unsigned char number1;
	unsigned char number2;
	unsigned char number3;
}Struct_DisplaykData;

typedef struct
{
	unsigned char nowIndex;	//该变量用于定位当前接收到第几个字节
	unsigned char recBuff[128];
	unsigned char sendBuff[128];

	unsigned int recTimeOutCount;	//接收的每个字节的时间间隔不能超过超时设定
	enum Enum_Bool recTimeOutFlag;
	unsigned int sendTimeOutCount;	//这里的发送超时指的是，发送数据包后，等待对方的应答超时
	enum Enum_Bool sendTimeOutFlag; 
	unsigned char resendCount;	//重发计数
}Struct_UartData;

typedef struct
{
	Struct_SetData set;
	Struct_DeviceData device;
	Struct_WorkData work;
	Struct_DisplaykData display;
	Struct_UartData uart;
}Struct_GlobalData;

extern Struct_GlobalData global;

#endif