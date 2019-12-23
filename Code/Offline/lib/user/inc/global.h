#ifndef _global_h
#define _global_h

#include "N76E003.h"
#include "SFR_Macro.h"
#include "Function_define.h"
#include "Common.h"
#include "Delay.h"

//#define ONLINE
#define OFFLINE

enum Enum_Bool {false, true};
enum Enum_NetState {offline, connecting, connectToAp, connectToService, online};


typedef struct
{
	unsigned int year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char sec;

	enum Enum_Bool getTimeFlag;
}Struct_Time;

#define UARTBUFFDATAMAXSIZE 64
#define UARTBUFFCMDMAXSIZE 5
typedef struct
{
	unsigned char nowIndex;	//该变量用于定位当前接收到第几个字节
	unsigned char recBuff[UARTBUFFDATAMAXSIZE];
	unsigned char sendBuff[UARTBUFFDATAMAXSIZE];
	unsigned char recvDealBuff[UARTBUFFCMDMAXSIZE][UARTBUFFDATAMAXSIZE];	//接收处理缓冲区
	unsigned char recvCmdNowIndex;	//正在接收指令的下标
	unsigned char recvCmdIndex;	//接收指令的下标
	unsigned char recvCmdCount;	//对接收的指令计数

	unsigned int recvDataTimeOutMs;	//接受数据超时毫秒
	unsigned int recvDataTimeOutSec;	//接受数据超时秒
	unsigned int recvDataTimeOutMinute;	//接受数据超时分
	enum Enum_Bool recvDataTimeOutFlag;	//接受数据超时标志
	enum Enum_Bool recvDataTimeOutClear;	//接受数据超时清除
	unsigned int recTimeOutCount;	//接收的每个字节的时间间隔不能超过超时设定
	enum Enum_Bool recTimeOutFlag;
	unsigned int sendTimeOutCount;	//这里的发送超时指的是，发送数据包后，等待对方的应答超时
	enum Enum_Bool sendTimeOutFlag;

	volatile enum Enum_Bool sendIsrFlag;	//发送中断标志
}Struct_UartData;

typedef struct
{
	unsigned char recEnable;
	unsigned char recCount;
	unsigned char recData;
	unsigned char recComplete;
	unsigned char recBuff[64];
	unsigned char nowIndex;
	
	unsigned int recTimeOutCount;	//接收的每个字节的时间间隔不能超过超时设定
	// enum Enum_Bool recTimeOutFlag;

	unsigned char sendEnable;
	unsigned char sendCount;
	unsigned char sendData;
	unsigned char sendComplete;
	unsigned char sendBuff[64];
	// enum enum_sendFlag sendFlag;
}Struct_IOUart;

typedef struct
{
	unsigned int year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char sec;

	
	enum Enum_NetState wifiConnectState;	//wifi 连接状态
	enum Enum_Bool light;	//照明
	enum Enum_Bool water;	//浇水
	enum Enum_Bool noWater;
	
	unsigned char startLightHour;	//开始照明-时
	unsigned char startLightMinute;	//开始照明-分
	unsigned char keepLightHour;	//持续照明-时
	unsigned char keepLightMinute;	//持续照明-分
	unsigned char endLightHour;	//结束照明-时
	unsigned char endLightMinute;	//结束照明-分
	unsigned char startWaterHour;	//开始浇水-时
	unsigned char startWaterMinute;	//开始浇水-分
	unsigned char keepWaterHour;	//持续浇水-时
	unsigned char keepWaterMinute;	//持续浇水-分
	unsigned char endWaterHour;	//结束浇水-时
	unsigned char endWaterMinute;	//结束浇水-分
	unsigned char waterCycleDay;	//浇水周期-天

	enum Enum_Bool getDataFlag;
	enum Enum_Bool setDatFlag;
	enum Enum_Bool setTimeDataFlag;	//设置照明浇水时间啥的
	enum Enum_Bool setRtcTimeDataFlag;	//设置rtc时间
}Struct_SetData;

typedef struct
{
	enum Enum_NetState wifi;
	enum Enum_Bool light;
	enum Enum_Bool water;
	enum Enum_Bool noWater;

	unsigned int nextStartLightCountDown;	//距离下一次开始照明倒计时-分
	unsigned int endLightCountDown;	//距离结束照明倒计时-分
	unsigned int nextStartWaterCountDown;	//距离下一次开始浇水倒计时-分
	unsigned int endWaterCountDown;	//距离结束浇水倒计时-分
	unsigned char nextWaterDayCountDown;	//距离下一次浇水的周期的天数的计算，1即为一天浇水一次则当天浇水,2即为两天浇水一次则明天浇水

	unsigned int temperature;	//温度

	enum Enum_Bool setStateFlag;
	enum Enum_Bool getStateFlag;
}Struct_DeviceData;

typedef struct
{
	unsigned char keyDo;	//来自顶板的按键触发的操作

	enum Enum_Bool wifiGetDeviceFlag;	//wifi 请求获取设备数据标志
	enum Enum_Bool wifiSetDeviceFlag;	//wifi 请求设置设备数据标志

	enum Enum_Bool wifiConfigFlag;	//WiFi 一键配置标志
	enum Enum_Bool wifiStateUpdateFlag;	//wifi 状态更新标志

	enum Enum_Bool lightSateUpdateFlag;	//照明状态更新标志
	enum Enum_Bool waterSateUpdateFlag;	//浇水状态更新标志
	enum Enum_Bool nowaterSateUpdateFlag;	//缺水状态更新标志

	// enum Enum_Bool device

	enum Enum_Bool serventSetDevieSateFlag;	//从机请求设置设备状态
	enum Enum_Bool serventSetDevieDataFlag;	//从机请求设置设备数据

	unsigned int workTimeCount;
}Struct_WorkData;

typedef struct
{
	Struct_Time time;
	Struct_UartData uart;
	Struct_IOUart iouart;
	Struct_SetData set;
	Struct_DeviceData device;
	Struct_WorkData work;
}Struct_GlobalData;

extern Struct_GlobalData global;

#endif