#include "func.h"

void updateDeviceState(void);
void configWifi(void);
void sendDeviceStateToWifi(void);
void topRequestForSet(void);

void sendDeviceStateToTop(void);
void sendSetDataToTop(void);
void sendAckToTop(void);

unsigned char isNoWater(void);

void parseDataFromTop(void);
void OneWire_init(void);
void oneWire_write(unsigned char* buff,unsigned char len);
unsigned char oneWire_read(unsigned char* buff);
unsigned char figureSum(unsigned char* buff,unsigned char len);

void parseDataFromWifi(void);
unsigned char figureSum0(unsigned char* buff,unsigned char len);
void uart0_sendData(unsigned char* buff,unsigned char len);

void initRtc(void);
void firstInitRtc(void);
void rtcGetTime(void);
void rtcSetTime(void);

unsigned char isFristStart(void);
void saveDataToFlash(void);
void readDataFromFlash(void);

void figureCountdown(void);
void figureEndTime(void);

#define initWifiResetPin() P05_PushPull_Mode;set_P05
#define WifiReset() P05_PushPull_Mode;clr_P05

void init(void)
{
	LIGHT_CLOSE;
	global.device.light = false;
	global.set.light = false;
	WATER_CLOSE;
	global.device.water = false;
	global.set.water = false;
	OneWire_init();
	initRtc();
	initWifiResetPin();
	// Timer3_Delay100ms(5);

	//如果为第一次开机
	if(isFristStart() == true)
	{
		firstInitRtc();	//初始化rtc设置
		//这里可以初始化预约数据，默认初始化为0
		global.set.startLightHour = 0;
		global.set.startLightMinute = 0;
		global.set.keepLightHour = 0;
		global.set.keepLightMinute = 0;
		global.set.endLightHour = 0;
		global.set.endLightMinute = 0;
		global.set.startWaterHour = 0;
		global.set.startWaterHour = 0;
		global.set.keepWaterHour = 0;
		global.set.keepWaterMinute = 0;
		global.set.endWaterHour = 0;
		global.set.endWaterMinute = 0;
		global.set.waterCycleDay = 1;
		global.device.nextWaterDayCountDown = 1;
		saveDataToFlash();
		#ifdef DEBUG
		uart0_sendData("is first",8);
		#endif
	}
	else
	{
		readDataFromFlash();	//读取预约数据
		#ifdef DEBUG
		uart0_sendData("not first",9);
		#endif
	}
#ifdef OFFLINE
	rtcGetTime();
	figureEndTime();	//上电时先计算一次开始结束的时间
	figureCountdown();	//计算倒计时
#endif
	// Timer3_Delay100ms(5);
}

///////////////////////////////////////////////Run//////////////////////////////////////////////////////

//运行在定时器3中，中断时间为1ms
void run(void)
{
	/////////////////////////处理一些计时功能//////////////////////////
#ifdef ONLINE
	if(++global.work.workTimeCount >= 2000)	//2s
	{
		global.work.workTimeCount = 0;

		global.time.getTimeFlag = true;	//获取一次rtc时间
	}
	//串口接收超时计时
	if(global.uart.recTimeOutCount > 0 && global.uart.recTimeOutCount < 1000)
	{
		++global.iouart.recTimeOutCount;
	}

	if(global.uart.recvDataTimeOutFlag == false && global.uart.recvDataTimeOutClear == false)	//未超时且未在清理计时
	{
		if(++global.uart.recvDataTimeOutMs >= 1000)	//1秒
		{
			global.uart.recvDataTimeOutMs = 0;
			if(++global.uart.recvDataTimeOutSec >= 60)	//1分钟
			{
				global.uart.recvDataTimeOutSec = 0;
				if(++global.uart.recvDataTimeOutMinute >= 10)	//10分钟
				{
					global.uart.recvDataTimeOutMs = 0;
					global.uart.recvDataTimeOutSec = 0;
					global.uart.recvDataTimeOutMinute = 0;
					global.uart.recvDataTimeOutFlag = true;	//超时
				}
			}
		}
	}
	else if(global.uart.recvDataTimeOutFlag == true)	//十分钟没有收到来自WiFi的数据，则有可能错误进入一键配置,复位WiFi
	{
		if(++global.uart.recvDataTimeOutMs >= 500)	//500ms
		{
			global.uart.recvDataTimeOutMs = 0;
			global.uart.recvDataTimeOutSec = 0;
			global.uart.recvDataTimeOutMinute = 0;
			global.uart.recvDataTimeOutFlag = false;
			initWifiResetPin();
		}
		else
		{
			WifiReset();	//复位WiFi
		}
	}
#endif
#ifdef OFFLINE
	if(++global.work.workTimeCount >= 1000)	//1s
	{
		global.work.workTimeCount = 0;

		global.time.getTimeFlag = true;	//获取一次rtc时间
	}

//判断是否缺水
	if(isNoWater())
	{
		///////新的缺水报警逻辑
		switch(global.work.checkNoWaterFolw)
		{
			case 0:
			{
				if(global.device.water == false) { global.work.checkNoWaterFolw = 1; }	//在未浇水的时候
				else { global.work.checkNoWaterFolw = 2; }	//在未浇水的时候,并且关闭水泵(仅关闭设备的运行,不关闭设备的状态)
			} break;
			case 1:
			{
				if(++global.work.checkNoWaterCount > 100)	//连续缺水100ms
				{
					global.work.checkNoWaterCount = 0;
					global.work.checkNoWaterFolw = 0;
					global.device.noWater = true;
					global.device.water = false;
					WATER_CLOSE;
				}
			} break;
			case 2:
			{
				if(++global.work.checkNoWaterCount > 5*60000)	//连续缺水5min
				{
					global.work.checkNoWaterCount = 0;
					global.work.checkNoWaterFolw = 0;
					global.device.noWater = true;
					global.device.water = false;
				}
				WATER_CLOSE;
			} break;
		}
	}
	else
	{
		if(global.work.checkNoWaterFolw == 2 && global.device.water == true)	//在等待的时候缺水取消,且设备状态依旧,则恢复设备运行
		{
			WATER_OPEN;
		}
		global.work.checkNoWaterFolw = 0;
		global.work.checkNoWaterCount = 0;

		global.device.noWater = false;
	}
	
#endif
}

//
//运行在main-while中
//用于执行一些耗时操作
//
void runMain(void)
{
	static char lastMinute = -1;

#ifdef ONLINE
	parseDataFromTop();
	parseDataFromWifi();

	//判断是否缺水
	if(isNoWater())
	{
		global.device.noWater = true;
		global.device.water = false;
		WATER_CLOSE;
		// global.set.noWater = true;
	}
	else
	{
		global.device.noWater = false;
		// global.set.noWater = false;
	}

	//获取RTC时间
	if(global.time.getTimeFlag == true)
	{
		rtcGetTime();
		global.time.getTimeFlag = false;
	}
	//每过一分钟执行的操作
	if(lastMinute != global.time.minute)
	{
		lastMinute = global.time.minute;	//记录当前的分钟
		// figureCountdown();
	}

/////////////////////////////////////////////////////////////////////////////////

	updateDeviceState();	//更新设备状态

////////////////////////////////////////////////////////和 wifi 模块的通信/////////////////////////////////////////////////////////////////
/////////////////////////接收/////////////////////////
	// ///////// wifi 设置设备数据//////////
	if(global.work.wifiSetDeviceFlag == true)
	{
		global.device.light = global.set.light;
		if(global.device.light == true) { LIGHT_OPEN; }
		else { LIGHT_CLOSE; }
		global.device.water = global.set.water;
		if(global.device.water == true && global.device.noWater == false) { WATER_OPEN; }	//
		else { WATER_CLOSE; global.device.water =  false; }
		global.device.getStateFlag = true;	//将更新的设备状态告诉从机
		global.work.wifiSetDeviceFlag = false;
	}
/////////////////////////发送/////////////////////////
	////////一键配置////////
	if(global.work.wifiConfigFlag == true)
	{
		global.work.wifiConfigFlag = false;
		configWifi();
	}
	/////////顶板请求设置设备数据/////////
	if(global.work.serventSetDevieDataFlag == true)
	{
		global.work.serventSetDevieDataFlag = false;
		topRequestForSet();
	}
	/////////往 wifi 发送设备数据////////
	if(global.work.wifiGetDeviceFlag == true)
	{
		global.work.wifiGetDeviceFlag = false;
		sendDeviceStateToWifi();
		if(global.device.wifi == online) rtcSetTime();	//判断时间差后，有条件选择设置与否
	}

////////////////////////////////////////////////////////和 顶板 的通信/////////////////////////////////////////////////////////////////
/////////////////////////接收/////////////////////////
	//////////顶板的请求///////////////
	if(global.work.serventSetDevieSateFlag == true)
	{
		global.work.serventSetDevieSateFlag = false;
		switch(global.work.keyDo)
		{
			case 0x01: { global.device.wifi = connecting; global.work.wifiConfigFlag = true;}break;
			case 0x10: { global.set.light = false; global.work.serventSetDevieDataFlag = true;}break;
			case 0x11: { global.set.light = true; global.work.serventSetDevieDataFlag = true;}break;
			case 0x20: { global.set.water = false; global.work.serventSetDevieDataFlag = true;}break;
			case 0x21: { global.set.water = true; global.work.serventSetDevieDataFlag = true;}break;
		}
	}

/////////////////////////发送/////////////////////////
	//////////往 顶板 发送设备数据///////////////////
	if(global.device.getStateFlag == true)	//收到设备状态设置请求或设备状态获取请求，他们的响应相同
	{
		sendDeviceStateToTop();
		global.device.getStateFlag = false;
	}
	//////////往 顶板 发送设置数据///////////////////
	if(global.set.getDataFlag == true)
	{
		global.set.getDataFlag = false;
		sendSetDataToTop();
	}
	//////////往 顶板 发送设置数据响应///////////////////
	if(global.set.setDatFlag == true)	//收到设置参数设置请求
	{
		global.set.setDatFlag = false;
		sendAckToTop();
		if(global.set.setRtcTimeDataFlag == true)	//等于0xff说明不修改rtc，那不等于时修改
		{
			rtcSetTime();
		}
		global.work.serventSetDevieDataFlag = true;	//向WIFI发起设置请求
	}
#endif
#ifdef OFFLINE
	parseDataFromTop();

	

	//获取RTC时间
	if(global.time.getTimeFlag == true)
	{
		rtcGetTime();
		global.time.getTimeFlag = false;
	}
	//每过一分钟执行的操作
	if(lastMinute != global.time.minute)
	{
		lastMinute = global.time.minute;	//记录当前的分钟
		figureEndTime();	//先计算一次开始结束的时间
		figureCountdown();	//计算倒计时
	}
	
/////////////////////////////////////////////////////////////////////////////////

	if(global.device.light == true) { LIGHT_OPEN; }
	else if(global.device.light == false) { LIGHT_CLOSE; }
	if(global.device.water == true && global.device.noWater == false && global.work.checkNoWaterFolw == 0) { WATER_OPEN; }
	else if(global.device.water == false || global.device.noWater == true) { WATER_CLOSE; }

////////////////////////////////////////////////////////和 顶板 的通信/////////////////////////////////////////////////////////////////
/////////////////////////接收/////////////////////////
	//////////顶板的请求///////////////
	if(global.work.serventSetDevieSateFlag == true)
	{
		global.work.serventSetDevieSateFlag = false;
		switch(global.work.keyDo)
		{
			case 0x10: { global.device.light = false; global.device.getStateFlag = true; }break;
			case 0x11: { global.device.light = true; global.device.getStateFlag = true; }break;
			case 0x20: { global.device.water = false; global.device.getStateFlag = true; }break;
			case 0x21: { global.device.water = true; global.device.getStateFlag = true; }break;
		}
	}

/////////////////////////发送/////////////////////////
	//////////往 顶板 发送设备数据///////////////////
	if(global.device.getStateFlag == true)	//收到设备状态设置请求或设备状态获取请求，他们的响应相同
	{
		sendDeviceStateToTop();
		global.device.getStateFlag = false;
	}
	//////////往 顶板 发送设置数据///////////////////
	if(global.set.getDataFlag == true)
	{
		global.set.getDataFlag = false;
		readDataFromFlash();
		sendSetDataToTop();
	}
	//////////往 顶板 发送设置数据响应///////////////////
	if(global.set.setDatFlag == true)	//收到设置参数设置请求
	{
		global.set.setDatFlag = false;
		if(global.set.setRtcTimeDataFlag == true)	//等于0xff说明不修改rtc，那不等于时修改
		{
			global.set.setRtcTimeDataFlag = false;
			rtcSetTime();
		}
		if(global.set.setTimeDataFlag == true)	//保存预约参数
		{
			global.set.setTimeDataFlag = false;
			saveDataToFlash();
		}
		sendAckToTop();	//发送响应
		figureEndTime();	//计算开始结束的时间
		figureCountdown();	//计算倒计时
	}
#endif
}

//
//更新设备状态
//
void updateDeviceState(void)
{
	//更新 wifi 连接状态
	if(global.work.wifiStateUpdateFlag == true)
	{
		global.work.wifiStateUpdateFlag = false;
		global.device.wifi = global.set.wifiConnectState;
	}
	//更新 照明 状态
	if(global.work.lightSateUpdateFlag == true)
	{
		global.work.lightSateUpdateFlag = false;
		global.device.light = global.set.light;
	}
	//更新 浇水 状态
	if(global.work.waterSateUpdateFlag == true)
	{
		global.work.waterSateUpdateFlag = false;
		global.device.water = global.set.water;
	}
	//更新 缺水 状态
	if(global.work.nowaterSateUpdateFlag == true)
	{
		global.work.nowaterSateUpdateFlag = false;
		global.device.noWater = global.set.noWater;
	}
}

//
// 向 wifi 发送一键配置命令
//
void configWifi(void)
{
	global.uart.sendBuff[0] = 0xAA;	//起始标志
	global.uart.sendBuff[1] = 11;	//长度

	global.uart.sendBuff[2] = 0x10;	//包描述高字节
	global.uart.sendBuff[3] = 0x00;	//包描述低字节
	global.uart.sendBuff[4] = 0x10;	//消息类型高字节
	global.uart.sendBuff[5] = 0x01;	//消息类型低字节

	global.uart.sendBuff[6] = 0x11;
	global.uart.sendBuff[7] = 0x22;
	global.uart.sendBuff[8] = 0x33;
	global.uart.sendBuff[9] = 0x44;
	global.uart.sendBuff[10] = figureSum0(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//计算校验和

	uart0_sendData(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送
}

//
//向 wifi 发送设备状态
//
void sendDeviceStateToWifi(void)
{
	global.uart.sendBuff[0] = 0xAA;	//起始标志
	global.uart.sendBuff[1] = 20;	//长度

	global.uart.sendBuff[2] = 0x10;	//包描述高字节
	global.uart.sendBuff[3] = 0x00;	//包描述低字节
	global.uart.sendBuff[4] = 0x28;	//消息类型高字节
	global.uart.sendBuff[5] = 0x01;	//消息类型低字节

	global.uart.sendBuff[6] = global.time.year;	//rtc年调整
	global.uart.sendBuff[7] = global.time.month;	//rtc月调整
	global.uart.sendBuff[8] = global.time.day;	//rtc日调整
	global.uart.sendBuff[9] = global.time.hour;	//rtc时调整
	global.uart.sendBuff[10] = global.time.minute;	//rtc分调整
	global.uart.sendBuff[11] = global.time.sec;	//rtc秒调整

	global.uart.sendBuff[12] = global.device.temperature>>8;
	global.uart.sendBuff[13] = global.device.temperature&0xff;
	global.uart.sendBuff[14] = global.device.water;	//浇水状态
	global.uart.sendBuff[15] = global.device.light;	//照明状态
	global.uart.sendBuff[16] = global.device.noWater;	//缺水状态

	global.uart.sendBuff[17] = 0x01;	//版本信息高字节
	global.uart.sendBuff[18] = 0x01;	//版本信息低字节
	global.uart.sendBuff[19] = figureSum0(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//计算校验和

	uart0_sendData(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送
}

//
//向 wifi 发送来自 Top 的设置请求
//
void topRequestForSet(void)
{
	global.uart.sendBuff[0] = 0xAA;	//起始标志
	global.uart.sendBuff[1] = 24;	//长度

	global.uart.sendBuff[2] = 0x10;	//包描述高字节
	global.uart.sendBuff[3] = 0x00;	//包描述低字节
	global.uart.sendBuff[4] = 0x10;	//消息类型高字节
	global.uart.sendBuff[5] = 0x03;	//消息类型低字节

	if(global.set.setTimeDataFlag == true)
	{
		global.uart.sendBuff[6] = global.set.startLightHour;	//照明启动小时调整--0xFF即不做调整
		global.uart.sendBuff[7] = global.set.startLightMinute;	//照明启动分钟调整
		global.uart.sendBuff[8] = global.set.keepLightHour;	//照明持续小时调整
		global.uart.sendBuff[9] = global.set.keepLightMinute;	//照明持续分钟调整
		global.uart.sendBuff[10] = global.set.startWaterHour;	//浇水启动小时调整
		global.uart.sendBuff[11] = global.set.startWaterMinute;	//浇水启动分钟调整
		global.uart.sendBuff[12] = global.set.keepWaterHour;	//浇水持续小时调整
		global.uart.sendBuff[13] = global.set.keepWaterMinute;	//浇水持续分钟调整
		global.uart.sendBuff[14] = global.set.waterCycleDay;	//浇水周期时间调整
		global.set.setTimeDataFlag = false;
	}
	else
	{
		global.uart.sendBuff[6] = 0xff;//global.set.startLightHour;	//照明启动小时调整--0xFF即不做调整
		global.uart.sendBuff[7] = 0xff;//global.set.startLightMinute;	//照明启动分钟调整
		global.uart.sendBuff[8] = 0xff;//global.set.keepLightHour;	//照明持续小时调整
		global.uart.sendBuff[9] = 0xff;//global.set.keepLightMinute;	//照明持续分钟调整
		global.uart.sendBuff[10] = 0xff;//global.set.startWaterHour;	//浇水启动小时调整
		global.uart.sendBuff[11] = 0xff;//global.set.startWaterMinute;	//浇水启动分钟调整
		global.uart.sendBuff[12] = 0xff;//global.set.keepWaterHour;	//浇水持续小时调整
		global.uart.sendBuff[13] = 0xff;//global.set.keepWaterMinute;	//浇水持续分钟调整
		global.uart.sendBuff[14] = 0xff;//global.set.waterCycleDay;	//浇水周期时间调整
	}

	global.uart.sendBuff[15] = global.set.light;	//请求照明设置
	global.uart.sendBuff[16] = global.set.water;	//请求浇水设置

	global.uart.sendBuff[17] = 0xff;//global.set.year;	//年调整
	global.uart.sendBuff[18] = 0xff;//global.set.month;	//月调整
	global.uart.sendBuff[19] = 0xff;//global.set.day;	//日调整
	if(global.set.setRtcTimeDataFlag == true)
	{
		global.uart.sendBuff[20] = global.set.hour;	//时调整
		global.uart.sendBuff[21] = global.set.minute;	//分调整
		global.uart.sendBuff[22] = 0;//global.set.sec;	//秒调整
		global.set.setRtcTimeDataFlag = false;
	}
	else
	{
		global.uart.sendBuff[20] = 0xff;//global.set.hour;	//时调整
		global.uart.sendBuff[21] = 0xff;//global.set.minute;	//分调整
		global.uart.sendBuff[22] = 0xff;//global.set.sec;	//秒调整
	}

	global.uart.sendBuff[23] = figureSum0(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//计算校验和

	uart0_sendData(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送
}

//
//向 顶板 发送设备状态
//
void sendDeviceStateToTop(void)
{
	global.iouart.sendBuff[0] = 0x96;	//头
	global.iouart.sendBuff[1] = 18;	//长度
	global.iouart.sendBuff[2] = 0x00;	//消息类型高8位
	global.iouart.sendBuff[3] = 0x02;	//消息类型d低8位
#ifdef ONLINE
	global.iouart.sendBuff[4] = global.device.wifi;
#endif
#ifdef OFFLINE
	global.iouart.sendBuff[4] = offline;
#endif
	global.iouart.sendBuff[5] = 0x00;
	global.iouart.sendBuff[6] = global.device.light;
	global.iouart.sendBuff[7] = global.device.water;
	global.iouart.sendBuff[8] = global.device.noWater;
	global.iouart.sendBuff[9] = global.device.nextStartLightCountDown>>8;
	global.iouart.sendBuff[10] = global.device.nextStartLightCountDown&0x00ff;
	global.iouart.sendBuff[11] = global.device.endLightCountDown>>8;
	global.iouart.sendBuff[12] = global.device.endLightCountDown&0x00ff;
	global.iouart.sendBuff[13] = global.device.nextStartWaterCountDown>>8;
	global.iouart.sendBuff[14] = global.device.nextStartWaterCountDown&0x00ff;
	global.iouart.sendBuff[15] = global.device.endWaterCountDown>>8;
	global.iouart.sendBuff[16] = global.device.endWaterCountDown&0x00ff;
	global.iouart.sendBuff[17] = figureSum(global.iouart.sendBuff,global.iouart.sendBuff[1]-1);

	oneWire_write(global.iouart.sendBuff,global.iouart.sendBuff[1]);
}

//
//向 顶板 发送设置数据
//
void sendSetDataToTop(void)
{
	global.iouart.sendBuff[0] = 0x96;	//头
	global.iouart.sendBuff[1] = 16;	//长度
	global.iouart.sendBuff[2] = 0x00;	//消息类型高8位
	global.iouart.sendBuff[3] = 0x04;	//消息类型d低8位
	global.iouart.sendBuff[4] = global.time.hour;
	global.iouart.sendBuff[5] = global.time.minute;
	global.iouart.sendBuff[6] = global.set.startLightHour;
	global.iouart.sendBuff[7] = global.set.startLightMinute;
	global.iouart.sendBuff[8] = global.set.keepLightHour;
	global.iouart.sendBuff[9] = global.set.keepLightMinute;
	global.iouart.sendBuff[10] = global.set.startWaterHour;
	global.iouart.sendBuff[11] = global.set.startWaterMinute;
	global.iouart.sendBuff[12] = global.set.keepWaterHour;
	global.iouart.sendBuff[13] = global.set.keepWaterMinute;
	global.iouart.sendBuff[14] = global.set.waterCycleDay;
	global.iouart.sendBuff[15] = figureSum(global.iouart.sendBuff,global.iouart.sendBuff[1]-1);

	oneWire_write(global.iouart.sendBuff,global.iouart.sendBuff[1]);
	#ifdef DEBUG
	uart0_sendData(global.iouart.sendBuff,global.iouart.sendBuff[1]);
	#endif
}

//
//向 顶板 发送响应
//
void sendAckToTop(void)
{
	global.iouart.sendBuff[0] = 0x96;	//头
	global.iouart.sendBuff[1] = 5;	//长度
	global.iouart.sendBuff[2] = 0x00;	//消息类型高8位
	global.iouart.sendBuff[3] = 0x07;	//消息类型d低8位
	global.iouart.sendBuff[4] = figureSum(global.iouart.sendBuff,global.iouart.sendBuff[1]-1);

	oneWire_write(global.iouart.sendBuff,global.iouart.sendBuff[1]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned char isNoWater(void)
{
	NOWATER_INIT;
	if(IS_NOWATER == 0)
	{
		return true;
	}

	return false;
}

void parseDataFromTop(void)
{
	unsigned char len = 0,sum = 0;
	unsigned char buff[32] = {0};
	unsigned int cmd = 0;
	len = oneWire_read(buff);
	if(len != 0)	//大于0则说明有数据包
	{
		sum = figureSum(buff,len-1);
		if(buff[0] == 0x96 && buff[len-1] == sum)	//确认包头和校验
		{
			cmd = buff[2]<<8 | buff[3];
			switch(cmd)
			{
				case 0x0001:	//设备状态获取请求	//顶板索要设备状态数据，并给出当前温度
				{
					global.device.temperature = buff[4]<<8 | buff[5];	//拿到温度
					global.device.getStateFlag = true;	//标记获取状态标志，将要发送设备状态给顶板
				}break;
				case 0x0003:	//设置参数获取请求
				{
					global.set.getDataFlag = true;
				}break;
				case 0x0005:	//设备状态设置请求，按下照明或浇水或连wifi
				{
					global.work.keyDo = buff[4];
					global.work.serventSetDevieSateFlag = true;	//判断键值，并且发送给模块决断
				}break;
				case 0x0006:	//设置参数设置请求
				{
					if(buff[4] != 0xff)
					{
						global.set.hour = buff[4];
						global.set.minute = buff[5];
						global.set.sec = 0;
						global.set.setRtcTimeDataFlag = true;	//设置了rtc时间
					}
					else global.set.setRtcTimeDataFlag = false;
					if(buff[6] != 0xff)
					{
						global.set.startLightHour = buff[6];
						global.set.startLightMinute = buff[7];
						global.set.keepLightHour = buff[8];
						global.set.keepLightMinute = buff[9];
						global.set.startWaterHour = buff[10];
						global.set.startWaterMinute = buff[11];
						global.set.keepWaterHour = buff[12];
						global.set.keepWaterMinute = buff[13];
						global.set.waterCycleDay = buff[14];
						global.device.nextWaterDayCountDown = buff[14];
						global.set.setTimeDataFlag = true;	//设置了照明浇水的时间
					}
					else global.set.setTimeDataFlag = false;

					global.set.setDatFlag = true;
				}break;
			}
		}
	}
}

/////////////////////////////////////////////////单总线通信////////////////////////////////////////////////////////

#define ONE_WIRE_A P03
#define ONE_WIRE_A_MODE P03_Quasi_Mode
#define ONE_WIRE_A_H set_P03
#define ONE_WIRE_A_L clr_P03
#define ONE_WIRE_B P04
#define ONE_WIRE_B_MODE P04_Quasi_Mode
#define ONE_WIRE_B_H set_P04
#define ONE_WIRE_B_L clr_P04

#define ONE_WIRE_READ ONE_WIRE_B
#define ONE_WIRE_WRITE(b) if(b) ONE_WIRE_A_H; else ONE_WIRE_A_L

void OneWire_init(void)
{
	ONE_WIRE_A_MODE;
	ONE_WIRE_B_MODE;

	ONE_WIRE_WRITE(1);	//拉高以释放写总线
}

void oneWire_start(void)
{
	ONE_WIRE_WRITE(0);	//拉低写总线作为起始信号
	Timer0_Delay1ms(10);
}

void oneWire_stop(void)
{
	ONE_WIRE_WRITE(1);	//拉高以释放写总线作为停止信号
}

//
//	写一个字节八位数据
//
void oneWire_writeByte(unsigned char dat)
{
	unsigned char i = 8;

	while(i--)
	{
		ONE_WIRE_WRITE(1);	//拉高写总线传输一位数据
		if(dat & 0x80)
			Timer0_Delay100us(5);	//写高电平
		else
			Timer0_Delay100us(2);	//写低电平
		dat <<= 1;
		ONE_WIRE_WRITE(0);	//拉低写总线作为位之间的间隔
		Timer0_Delay100us(2);
	}
}

//
//	写数据包，通信规则中，起始信号后的第一个字节必须为数据包的长度(该字节不计算在长度内)
//
void oneWire_write(unsigned char* buff,unsigned char len)
{
	oneWire_start();	//起始信号
	oneWire_writeByte(len);	//发送数据长度
	while(len--)	//数据包长度
	{
		oneWire_writeByte(*buff);	//发送一个字节的数据
		if(len != 0) ++buff;	//防止溢出
	}
	oneWire_stop();	//停止信号
}

//
//	读数据包，成功后返回数据包长度,失败为0
//
unsigned char oneWire_read(unsigned char* buff)
{
	unsigned int timeOut = 0;
	unsigned char count = 0,dat = 0,transFlag = 0,index = 0,lenFlag = 0,lenCount = 0,len = 0;
	if(!ONE_WIRE_READ)	//低电平，收到起始信号
	{
		while(1)
		{
			if(ONE_WIRE_READ)	//高电平，计算持续时间
			{
				++count;	//记录高电平持续时间
				transFlag = 1;	//电平跳变标志
			}
			else if(!ONE_WIRE_READ && transFlag == 1)	//读总线为低电平并且只有在电平跳变后才进入
			{	//一位数据结束，判断之前的高电平传输的是1还是0
				dat <<= 1;
				if(count > 3)	//大于300us就算高电平
					dat |= 1;
				count = 0;	//清高电平计数
				timeOut = 0;	//清超时计数
				transFlag = 0;	//清电平跳变标志
				if(++index == 8)	//每读八位，即一个字节进入一次
				{
					if(lenFlag == 0)	//起始信号后的第一个字节，用于说明数据包的长度
					{
						lenFlag = 1;	//已接收到长度
						len = dat;
					}
					else	//数据包的内容
					{
						*buff = dat;
						if(++lenCount == len)	//数据包接收完成
						{
							Timer0_Delay100us(3);	//等低电平过去，以防止连续传输时错误的将用作间隔的低电平识别为起始信号
							return len;	//结束,并返回长度
						}
						else	//没接收完成就继续接收，这里单独自增是为了防止溢出
						{
							++buff;
						}
						
					}
					index = 0;	//清计数
					dat = 0x00;	//清变量
				}
			}
			Timer0_Delay100us(1);	//超时处理
			if(++timeOut > 200) return 0;	//超时
		}
	}
	return 0;
}

//
//和校验计算
//unsigned char len : 需要计算的长度
//
unsigned char figureSum(unsigned char* buff,unsigned char len)
{
	unsigned char sum = 0;

	while(len--)
	{
		sum += *buff;
		if(len != 0) ++buff;	//防止溢出
	}
	return sum;
}

///////////////////////////////////////////////串口0功能//////////////////////////////////////////////////////

void uart0_fun(unsigned char* buff);	//声明
//
//	处理数据
//
void parseDataFromWifi(void)
{
	unsigned char sum = 0;
	if(global.uart.recvCmdCount > 0)	//有待处理的指令
	{
		if(global.uart.recvCmdCount > UARTBUFFCMDMAXSIZE - 1)	//如果待处理的指令过多，大于上限-1(因为正在接收的指令会占用一条，所以要-1)
		{
			//因为待处理过多了，所以缓冲区中每一条都被占用了，所以处理当前在接收的下标的+2，之所以不是+1是为了防止还在处理+1时，当前的下标就接收完了接着又开始接收+1导致数据错乱
			//例如当前在接收的下标是0，那么如果处理1时，还未处理完1就接收完0了，接着又开始接收1，那么就会导致错乱
			if(global.uart.recvCmdNowIndex + 2 > UARTBUFFCMDMAXSIZE - 1)	//如果下标+2会超过最大下标，则计算下标
			{
				global.uart.recvCmdIndex = 2 - (UARTBUFFCMDMAXSIZE - 1 - global.uart.recvCmdNowIndex);	//超出多少则从0加上多少，最多加2
			}
			else
			{
				global.uart.recvCmdIndex = global.uart.recvCmdNowIndex + 2;
			}
		}
		else	//待处理的指令不多,则直接处理当前接收的上一条
		{
			if(global.uart.recvCmdNowIndex == 0)
			{
				global.uart.recvCmdIndex = UARTBUFFCMDMAXSIZE - 1;
			}
			else
			{
				global.uart.recvCmdIndex = global.uart.recvCmdNowIndex - 1;
			}
		}
		
		//处理指令
		sum = figureSum0(global.uart.recvDealBuff[global.uart.recvCmdIndex],global.uart.recvDealBuff[global.uart.recvCmdIndex][1]-1);
		if(sum == global.uart.recvDealBuff[global.uart.recvCmdIndex][global.uart.recvDealBuff[global.uart.recvCmdIndex][1]-1])	//校验通过
		{
			global.uart.recvDataTimeOutClear = true;	//开启清理标志，防止清理时被中断修改
			global.uart.recvDataTimeOutMs = 0;
			global.uart.recvDataTimeOutSec = 0;
			global.uart.recvDataTimeOutMinute = 0;
			global.uart.recvDataTimeOutClear = false;	//关闭清理标志，让中断继续计时
			uart0_fun(global.uart.recvDealBuff[global.uart.recvCmdIndex]);	//功能判断
		}
		--global.uart.recvCmdCount;	//待处理指令-1
	}
}

//
//和校验计算
//unsigned char len : 需要计算的长度
//
unsigned char figureSum0(unsigned char* buff,unsigned char len)
{
	unsigned char sum = 0;

	while(len--)
	{
		sum += *buff;
		if(len != 0) ++buff;	//防止溢出
	}
	sum = ~sum;
	sum += 1;
	return sum;
}

void uart0_fun(unsigned char* buff)
{
	unsigned int messageType = 0,temp = 0;
	messageType = (buff[4]<<8) | buff[5];
	switch(messageType)
	{
		case 0x1801:	//一键配置响应
		{
			
		}break;
		case 0x1803:	//设置参数响应
		{
			
		}break;
		case 0x1002:	//网络状态
		{
			if(buff[7] == 0x00)	global.set.wifiConnectState = connectToAp;	//连接路由
			else if(buff[7] == 0x01 && buff[8] == 0x00)	global.set.wifiConnectState = connectToService;	//连接至路由
			else if(buff[8] == 0x01)	global.set.wifiConnectState = online;	//连接至服务器

			global.work.wifiStateUpdateFlag = true;	//置位 wifi 状态更新标志
		}break;
		case 0x2001:	//WIFI模块请求设备状态并给时间
		{
			if(buff[6] != 0xff)
			{
				global.set.year = buff[6];// + 2000;
			}
			if(buff[7] != 0xff)
			{
				global.set.month = buff[7];
			}
			if(buff[8] != 0xff)
			{
				global.set.day = buff[8];
			}
			if(buff[9] != 0xff)
			{
				global.set.hour = buff[9];
			}
			if(buff[10] != 0xff)
			{
				global.set.minute = buff[10];
			}
			if(buff[11] != 0xff)
			{
				global.set.sec = buff[11];
			}
			global.work.wifiGetDeviceFlag = true;
		}break;
		case 0x2002:	//WIFI模块请求设置设备
		{
			if(buff[6] == 0x00) global.set.water = false;
			else if(buff[6] == 0x01) global.set.water = true;
			if(buff[7] == 0x00) global.set.light = false;
			else if(buff[7] == 0x01) global.set.light = true;

			temp = (buff[8]<<8) | buff[9];
			global.device.nextStartWaterCountDown = temp;
			//因为底板与顶板交互显示倒计时的机制，所以不再判断
			// if(temp != 0xffff)
			// {
			// 	global.device.nextStartWaterCountDown = temp;
			// }
			temp = (buff[10]<<8) | buff[11];
			global.device.endWaterCountDown = temp;
			// if(temp != 0xffff)
			// {
			// 	global.device.endWaterCountDown = temp;
			// }
			temp = (buff[12]<<8) | buff[13];
			global.device.nextStartLightCountDown = temp;
			// if(temp != 0xffff)
			// {
			// 	global.device.nextStartLightCountDown = temp;
			// }
			temp = (buff[14]<<8) | buff[15];
			global.device.endLightCountDown = temp;
			// if(temp != 0xffff)
			// {
			// 	global.device.endLightCountDown = temp;
			// }

			if(buff[16] != 0xff)
			{
				global.set.startLightHour = buff[16];
			}
			if(buff[17] != 0xff)
			{
				global.set.startLightMinute = buff[17];
			}
			if(buff[18] != 0xff)
			{
				global.set.keepLightHour = buff[18];
			}
			if(buff[19] != 0xff)
			{
				global.set.keepLightMinute = buff[19];
			}
			if(buff[20] != 0xff)
			{
				global.set.startWaterHour = buff[20];
			}
			if(buff[21] != 0xff)
			{
				global.set.startWaterMinute = buff[21];
			}
			if(buff[22] != 0xff)
			{
				global.set.keepWaterHour = buff[22];
			}
			if(buff[23] != 0xff)
			{
				global.set.keepWaterMinute = buff[23];
			}
			if(buff[24] != 0xff)
			{
				global.set.waterCycleDay = buff[24];
			}

			global.work.wifiSetDeviceFlag = true;
		}break;
	}
}

void uart0_isr(unsigned char dat)
{
	unsigned char sum = 0,i = 0;
	
	//超时处理
	if(global.uart.recTimeOutCount == 1000)
	{
		global.uart.recTimeOutCount = 0;	//已超时，关掉超时计时
		global.uart.nowIndex = 0;	//重置下标
	}
	
	//判断接收的数据是否符合通信格式
	if(global.uart.nowIndex == 0 && dat == 0xAA)	//头
	{
		global.uart.recTimeOutCount = 1;	//开启超时计时，为1则计时，如果超时，则放弃当前数据包，收到数据包则刷新
		global.uart.recvDealBuff[global.uart.recvCmdNowIndex][global.uart.nowIndex] = dat;
		++global.uart.nowIndex;
	}
	else if(global.uart.nowIndex == 1)	//数据长度
	{
		if(dat > UARTBUFFDATAMAXSIZE - 14)	//长度大于缓冲区最大-14，则退出程序，防止溢出
		{
			global.uart.nowIndex = 0;
			return;
		}
		global.uart.recvDealBuff[global.uart.recvCmdNowIndex][global.uart.nowIndex] = dat;	//长度：从头到校验
		++global.uart.nowIndex;
	}
	else if(global.uart.nowIndex >= 2 && global.uart.nowIndex <= global.uart.recvDealBuff[global.uart.recvCmdNowIndex][1]-2)	//减去了校验字节，因为下标比实际长度小1，所以-2
	{
		global.uart.recvDealBuff[global.uart.recvCmdNowIndex][global.uart.nowIndex] = dat;	//数据
		++global.uart.nowIndex;
	}
	else if(global.uart.nowIndex == global.uart.recvDealBuff[global.uart.recvCmdNowIndex][1]-1)	//记录收到的指令，之后在主循环中处理，处理时再做校验
	{
		global.uart.recTimeOutCount = 0;	//接收到一个可能完整的数据包，关掉超时计时，为0则不计时
		global.uart.recvDealBuff[global.uart.recvCmdNowIndex][global.uart.nowIndex] = dat;	//数据

		++global.uart.recvCmdCount;	//对接收到的指令进行计数
		if(global.uart.recvCmdNowIndex == UARTBUFFCMDMAXSIZE - 1)	//接收的指令数量已经达到上限，因为用作数组下标所以这里需要减1
		{
			global.uart.recvCmdNowIndex = 0;	//重置或者也有可能会造成覆盖
		}
		else
		{
			++global.uart.recvCmdNowIndex;
		}

		global.uart.nowIndex = 0;
	}
}

void delay(unsigned int a)
{
	unsigned int b = 0;
	for(;a > 0;a--)
	{
		for(;b < 100;b++);
	}
}

void uart0_sendData(unsigned char* buff,unsigned char len)
{
	while(len--)
	{
		//因为下面的阻塞等待，所以该变量不会存在同时在中断和函数调用时同时被修改的情况
		global.uart.sendIsrFlag = false;	//发送完成会在中断中被置位
		SBUF = *buff;
		//这里存在可能导致死机的问题，当发送后被某中断打断
		//在回到这里之前，串口已经发送完成，中断中 TI 被清 0
		//导致回到这里之后，无法跳出循环
    	// while(TI==0);
		//这里使用额外的变量标记串口发送完成标志
		//当上述的情况发生之后，该标志依然正常被置位
		while(global.uart.sendIsrFlag == false);	//未被置位就阻塞等待
		delay(20);
		if(len != 0) ++buff;	//防止溢出
	}
}

///////////////////////////////////////////////RTC//////////////////////////////////////////////////////

#define RTC_RST_H	set_P01
#define RTC_RST_L	clr_P01
#define RTC_SCL_H	set_P13
#define RTC_SCL_L	clr_P13
#define RTC_SDA_H	set_P14
#define RTC_SDA_L	clr_P14
#define RTC_SDA	P14

#define RTCADDR_SEC 0x80
#define RTCADDR_MINUTE 0x82
#define RTCADDR_HOUR 0x84
#define RTCADDR_DAY 0x86
#define RTCADDR_MONTH 0x88
#define RTCADDR_YEAR 0x8c
#define RTCADDR_WP 0x8e	//写保护
#define RTCADDR_CHARGER 0x90	//涓流充电

#define RTC_WP_OPEN	0x80 //写保护开
#define RTC_WP_CLOSE	0x00 //写保护关
#define RTC_CHARGER_LEVEL4 0xa9	//
#define RTC_CHARGER_LEVEL6 0xab	//应该是最慢的充电

void rctWriteByte(unsigned char addr,unsigned char dat);
unsigned char rtcReadByte(unsigned char addr);

void initRtc(void)
{
	P01_PushPull_Mode;	//NRST
	P13_PushPull_Mode;	//SCL
	P14_PushPull_Mode;	//SDA

	RTC_RST_L;
	RTC_SCL_H;
	RTC_SDA_H;
}

void firstInitRtc(void)
{
	rctWriteByte(RTCADDR_WP,RTC_WP_CLOSE);	//关闭写保护
	rctWriteByte(RTCADDR_SEC,0x00);	//开启计时,秒清零
	rctWriteByte(RTCADDR_HOUR,0x00);	//清零小时
	rctWriteByte(RTCADDR_MINUTE,0x00);	//清零分钟
	rctWriteByte(RTCADDR_CHARGER,RTC_CHARGER_LEVEL4);
	// rctWriteByte(RTCADDR_CHARGER,RTC_CHARGER_LEVEL6);	//设置涓流充电
	rctWriteByte(RTCADDR_WP,RTC_WP_OPEN);	//开启写保护
}

void rctWriteByte(unsigned char addr,unsigned char dat)
{
	unsigned char count;

	P14_PushPull_Mode;

	RTC_SCL_L;
	RTC_RST_H;

	addr &= 0xfe;

	for(count = 0;count < 8;count++)
	{
		RTC_SCL_L;
		if(addr & 0x01)
		{
			RTC_SDA_H;
		}
		else
		{
			RTC_SDA_L;
		}
		addr >>= 1;
		RTC_SCL_H;
	}

	for(count = 0;count < 8;count++)
	{
		RTC_SCL_L;
		if(dat & 0x01)
		{
			RTC_SDA_H;
		}
		else
		{
			RTC_SDA_L;
		}
		dat >>= 1;
		RTC_SCL_H;
	}

	RTC_RST_L;
}

unsigned char rtcReadByte(unsigned char addr)
{
	unsigned char count,dat = 0;

	P14_PushPull_Mode;

	RTC_SCL_L;
	RTC_RST_H;

	addr |= 0x01;

	for(count = 0;count < 8;count++)
	{
		RTC_SCL_L;
		if(addr & 0x01)
		{
			RTC_SDA_H;
		}
		else
		{
			RTC_SDA_L;
		}
		addr >>= 1;
		RTC_SCL_H;
	}

	P14_Input_Mode;

	for(count = 0;count < 8;count++)
	{
		RTC_SCL_H;
		if(RTC_SDA)
		{
			dat |= 0x80;
		}
		RTC_SCL_L;	
		dat >>= 1;
	}
	RTC_RST_L;

	return dat;
}

void rtcGetTime(void)
{
	unsigned char temp = 0;
	temp = rtcReadByte(RTCADDR_YEAR);
	global.time.year = ((temp>>4)*10) + (temp&0x0f);
	temp = rtcReadByte(RTCADDR_MONTH);
	global.time.month = ((temp>>4)*10) + (temp&0x0f);
	temp = rtcReadByte(RTCADDR_DAY);
	global.time.day = ((temp>>4)*10) + (temp&0x0f);
	temp = rtcReadByte(RTCADDR_HOUR);
	global.time.hour = ((temp>>4)*10) + (temp&0x0f);
	temp = rtcReadByte(RTCADDR_MINUTE);
	global.time.minute = ((temp>>4)*10) + (temp&0x0f);
	temp = rtcReadByte(RTCADDR_SEC);
	global.time.sec = ((temp>>4)*10) + (temp&0x0f);
}

void rtcSetTime(void)
{
	unsigned char temp = 0;
	
	//当年月日时分都相同且秒相差小于5秒则退出不设置，否则设置
	if(global.set.year == global.time.year && global.set.month == global.time.month && global.set.day == global.time.day &&
		global.set.hour == global.time.hour && global.set.minute == global.time.minute)
	{
		if(global.set.sec == global.time.sec)
			return;
		else if(global.set.sec > global.time.sec)
			temp = global.set.sec - global.time.sec;
		else if(global.set.sec < global.time.sec)
			temp = global.time.sec - global.set.sec;
		if(temp < 5) return;
	}

	rctWriteByte(RTCADDR_WP,RTC_WP_CLOSE);	//关闭写保护
	temp = ((global.set.year/10)<<4) | (global.set.year%10);
	rctWriteByte(RTCADDR_YEAR,temp);
	temp = ((global.set.month/10)<<4) | (global.set.month%10);
	rctWriteByte(RTCADDR_MONTH,temp);
	temp = ((global.set.day/10)<<4) | (global.set.day%10);
	rctWriteByte(RTCADDR_DAY,temp);
	temp = ((global.set.hour/10)<<4) | (global.set.hour%10);
	rctWriteByte(RTCADDR_HOUR,temp);
	temp = ((global.set.minute/10)<<4) | (global.set.minute%10);
	rctWriteByte(RTCADDR_MINUTE,temp);
	temp = ((global.set.sec/10)<<4) | (global.set.sec%10);
	rctWriteByte(RTCADDR_SEC,0);
	rctWriteByte(RTCADDR_WP,RTC_WP_OPEN);	//开启写保护
	rtcGetTime();
}

///////////////////////////////////////////////IAP Flash//////////////////////////////////////////////////////

#define PAGE_ERASE_AP 0x22
#define BYTE_PROGRAM_AP 0x21

#define FLASHADDRHEARD 0x4550	//存储首地址

//存储区--首地址用于存储首次开机标志
// volatile unsigned char code Data_Flash[128] _at_ 0x4550;

unsigned char flash_readByte(unsigned int code* addr)
{
	return (unsigned char)((*addr)>>8);
}

void iapEnable(void)
{
	clr_EA;
	//使能IAP
	TA = 0xAA; //CHPCON is TA protected
	TA = 0x55;
	CHPCON |= 0x01; //IAPEN = 1, enable IAP mode
	TA = 0xAA; //IAPUEN is TA protected
	TA = 0x55;
	IAPUEN |= 0x01; //APUEN = 1, enable APROM update
	set_EA;
}

void iapErasePage(unsigned int addr)
{
	//写字节
	IAPCN = PAGE_ERASE_AP; // Program 201h with 55h
	IAPAH = HIBYTE(addr);
	IAPAL = LOBYTE(addr);
	IAPFD = 0xff;
}

void iapWriteByte(unsigned int addr,unsigned char dat)
{
	//写字节
	IAPCN = BYTE_PROGRAM_AP; // Program 201h with 55h
	IAPAH = HIBYTE(addr);
	IAPAL = LOBYTE(addr);
	IAPFD = dat;
}

void iapTrigger(void)
{
	clr_EA;
	//执行IAP
	TA = 0xAA;
	TA = 0x55;
	IAPTRG |= 0x01; //write ‘1’ to IAPGO to trigger IAP process
	set_EA;
}

void iapUnenable(void)
{
	clr_EA;
	//失能IAP
	TA = 0xAA; //IAPUEN is TA protected
	TA = 0x55;
	IAPUEN &= ~0x01; //APUEN = 0, disable APROM update
	TA = 0xAA; //CHPCON is TA protected
	TA = 0x55;
	CHPCON &= ~0x01; //IAPEN = 0, disable IAP mode
	set_EA;
}

unsigned char isFristStart(void)
{
	if(flash_readByte(FLASHADDRHEARD) == 0xff)
	{
		iapEnable();
		iapWriteByte(FLASHADDRHEARD,0x96);	//写已开机标志
		iapTrigger();
		iapUnenable();
		return true;
	}
	else if(flash_readByte(FLASHADDRHEARD) == 0x96)
	{
		return false;
	}
	return false;
}

void resetFlash(void)
{
	iapEnable();
	iapErasePage(FLASHADDRHEARD);
	iapTrigger();
	iapUnenable();
}

void saveDataToFlash(void)
{
	//先读出数据
	
	//再擦除
	resetFlash();
	iapEnable();
	//写首次开机标志
	iapWriteByte(FLASHADDRHEARD,0x96);	//都能存预约数据了，那肯定开机了，那就要重新写标志
	iapTrigger();
	//存预约数据
	iapWriteByte(FLASHADDRHEARD+1,global.set.startLightHour);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+2,global.set.startLightMinute);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+3,global.set.keepLightHour);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+4,global.set.keepLightMinute);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+5,global.set.endLightHour);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+6,global.set.endLightMinute);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+7,global.set.startWaterHour);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+8,global.set.startWaterMinute);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+9,global.set.keepWaterHour);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+10,global.set.keepWaterMinute);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+11,global.set.endWaterHour);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+12,global.set.endWaterMinute);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+13,global.set.waterCycleDay);
	iapTrigger();
	iapWriteByte(FLASHADDRHEARD+14,global.device.nextWaterDayCountDown);
	iapTrigger();
	iapUnenable();
}

void readDataFromFlash(void)
{
	global.set.startLightHour = flash_readByte(FLASHADDRHEARD+1);
	global.set.startLightMinute = flash_readByte(FLASHADDRHEARD+2);
	global.set.keepLightHour = flash_readByte(FLASHADDRHEARD+3);
	global.set.keepLightMinute = flash_readByte(FLASHADDRHEARD+4);
	global.set.endLightHour = flash_readByte(FLASHADDRHEARD+5);
	global.set.endLightMinute = flash_readByte(FLASHADDRHEARD+6);
	global.set.startWaterHour = flash_readByte(FLASHADDRHEARD+7);
	global.set.startWaterMinute = flash_readByte(FLASHADDRHEARD+8);
	global.set.keepWaterHour = flash_readByte(FLASHADDRHEARD+9);
	global.set.keepWaterMinute = flash_readByte(FLASHADDRHEARD+10);
	global.set.endWaterHour = flash_readByte(FLASHADDRHEARD+11);
	global.set.endWaterMinute = flash_readByte(FLASHADDRHEARD+12);
	global.set.waterCycleDay = flash_readByte(FLASHADDRHEARD+13);
	global.device.nextWaterDayCountDown = flash_readByte(FLASHADDRHEARD+14);
}



//
//计算倒计时，然后执行计时结束的一些操作，放在 runMain 中，每分钟进入一次
//
void figureCountdown(void)
{
	//////////////////////////////////////照明/////////////////////////////////////
	//距离下一次开始照明倒计时的计算
	if(global.set.startLightHour > global.time.hour)	//说明设定开始的小时还未到
	{
		if(global.set.startLightMinute >= global.time.minute)	//说明设定开始的分钟还没到或正达到
		{
			global.device.nextStartLightCountDown = (global.set.startLightHour - global.time.hour)*60 + (global.set.startLightMinute - global.time.minute);
		}
		else if (global.set.startLightMinute < global.time.minute)	//说明设定开始的分钟已经超过了
		{
			global.device.nextStartLightCountDown = (global.set.startLightHour - global.time.hour)*60 - (global.time.minute - global.set.startLightMinute);
		}
	}
	else if(global.set.startLightHour == global.time.hour)
	{
		if(global.set.startLightMinute >= global.time.minute)	//说明设定开始的分钟还没到或正达到
		{
			global.device.nextStartLightCountDown = global.set.startLightMinute - global.time.minute;
		}
		else if (global.set.startLightMinute < global.time.minute)	//说明设定开始的分钟已经超过了
		{
			global.device.nextStartLightCountDown = 24*60 - (global.time.minute - global.set.startLightMinute);
		}
	}
	else if(global.set.startLightHour < global.time.hour)	//说明设定开始的小时已超过
	{
		if(global.set.startLightMinute >= global.time.minute)	//说明设定开始的分钟还没到或正达到
		{
			global.device.nextStartLightCountDown = (24 - global.time.hour + global.set.startLightHour)*60 + (global.set.startLightMinute - global.time.minute);
		}
		else if (global.set.startLightMinute < global.time.minute)	//说明设定开始的分钟已经超过了
		{
			global.device.nextStartLightCountDown = (24 - global.time.hour + global.set.startLightHour)*60 - (global.time.minute - global.set.startLightMinute);
		}
	}
	//距离结束照明倒计时的计算
	if(global.set.endLightHour > global.time.hour)	//说明设定结束的小时还未到或正达到
	{
		if(global.set.endLightMinute >= global.time.minute)	//说明设定结束的分钟还没到或正达到
		{
			global.device.endLightCountDown = (global.set.endLightHour - global.time.hour)*60 + (global.set.endLightMinute - global.time.minute);
		}
		else if (global.set.endLightMinute < global.time.minute)	//说明设定结束的分钟已经超过了
		{
			global.device.endLightCountDown = (global.set.endLightHour - global.time.hour)*60 - (global.time.minute - global.set.endLightMinute);
		}
	}
	else if(global.set.endLightHour == global.time.hour)
	{
		if(global.set.endLightMinute >= global.time.minute)	//说明设定结束的分钟还没到或正达到
		{
			global.device.endLightCountDown = global.set.endLightMinute - global.time.minute;
		}
		else if (global.set.endLightMinute < global.time.minute)	//说明设定结束的分钟已经超过了
		{
			global.device.endLightCountDown = 24*60 - (global.time.minute - global.set.endLightMinute);
		}
	}
	else if(global.set.endLightHour < global.time.hour)	//说明设定结束的小时已超过
	{
		if(global.set.endLightMinute >= global.time.minute)	//说明设定结束的分钟还没到或正达到
		{
			global.device.endLightCountDown = (24 - global.time.hour + global.set.endLightHour)*60 + (global.set.endLightMinute - global.time.minute);
		}
		else if (global.set.endLightMinute < global.time.minute)	//说明设定结束的分钟已经超过了
		{
			global.device.endLightCountDown = (24 - global.time.hour + global.set.endLightHour)*60 - (global.time.minute - global.set.endLightMinute);
		}
	}

	//////////////////////////////////////浇水/////////////////////////////////////

	if(global.time.hour == 0 && global.time.minute == 0)	//零点，做浇水周期计算
	{
		//每天零点减一天，减到1则当天浇水，浇水结束后重置。
		if(global.device.nextWaterDayCountDown > 1) --global.device.nextWaterDayCountDown;

		if(global.device.nextWaterDayCountDown == 0)	//如果为0，则重置
		{
			global.device.nextWaterDayCountDown = global.set.waterCycleDay;
		}
		saveDataToFlash();
	}

	//距离下一次开始浇水倒计时的计算
	if(global.set.startWaterHour > global.time.hour)	//说明设定开始的小时还未到或正达到
	{
		if(global.set.startWaterMinute > global.time.minute)	//说明设定开始的分钟还没到或正达到
		{
			global.device.nextStartWaterCountDown = (global.set.startWaterHour - global.time.hour)*60 + (global.set.startWaterMinute - global.time.minute);
			global.device.nextStartWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;	//再加上周期天数的偏移
		}
		else if (global.set.startWaterMinute < global.time.minute)	//说明设定开始的分钟已经超过了
		{
			global.device.nextStartWaterCountDown = (global.set.startWaterHour - global.time.hour)*60 - (global.time.minute - global.set.startWaterMinute);
			global.device.nextStartWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
	}
	else if(global.set.startWaterHour == global.time.hour)
	{
		if(global.set.startWaterMinute >= global.time.minute)	//说明设定开始的分钟还没到或正达到
		{
			global.device.nextStartWaterCountDown = global.set.startWaterMinute - global.time.minute;
			global.device.nextStartWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;	//再加上周期天数的偏移
		}
		else if (global.set.startWaterMinute < global.time.minute)	//说明设定开始的分钟已经超过了
		{
			global.device.nextStartWaterCountDown = 24*60 - (global.time.minute - global.set.startWaterMinute);
			global.device.nextStartWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
	}
	else if(global.set.startWaterHour < global.time.hour)	//说明设定开始的小时已超过
	{
		if(global.set.startWaterMinute >= global.time.minute)	//说明设定开始的分钟还没到或正达到
		{
			global.device.nextStartWaterCountDown = (24 - global.time.hour + global.set.startWaterHour)*60 + (global.set.startWaterMinute - global.time.minute);
			global.device.nextStartWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
		else if (global.set.startWaterMinute < global.time.minute)	//说明设定开始的分钟已经超过了
		{
			global.device.nextStartWaterCountDown = (24 - global.time.hour + global.set.startWaterHour)*60 - (global.time.minute - global.set.startWaterMinute);
			global.device.nextStartWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
	}
	//距离结束浇水倒计时的计算
	if(global.set.endWaterHour > global.time.hour)	//说明设定结束的小时还未到或正达到
	{
		if(global.set.endWaterMinute >= global.time.minute)	//说明设定结束的分钟还没到或正达到
		{
			global.device.endWaterCountDown = (global.set.endWaterHour - global.time.hour)*60 + (global.set.endWaterMinute - global.time.minute);
			global.device.endWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
		else if (global.set.endWaterMinute < global.time.minute)	//说明设定结束的分钟已经超过了
		{
			global.device.endWaterCountDown = (global.set.endWaterHour - global.time.hour)*60 - (global.time.minute - global.set.endWaterMinute);
			global.device.endWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
	}
	else if(global.set.endWaterHour == global.time.hour)
	{
		if(global.set.endWaterMinute >= global.time.minute)	//说明设定结束的分钟还没到或正达到
		{
			global.device.endWaterCountDown = global.set.endWaterMinute - global.time.minute;
			global.device.endWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
		else if (global.set.endWaterMinute < global.time.minute)	//说明设定结束的分钟已经超过了
		{
			global.device.endWaterCountDown = 24*60 - (global.time.minute - global.set.endWaterMinute);
			global.device.endWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
	}
	else if(global.set.endWaterHour < global.time.hour)	//说明设定结束的小时已超过
	{
		if(global.set.endWaterMinute >= global.time.minute)	//说明设定结束的分钟还没到或正达到
		{
			global.device.endWaterCountDown = (24 - global.time.hour + global.set.endWaterHour)*60 + (global.set.endWaterMinute - global.time.minute);
			global.device.endWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
		else if (global.set.endWaterMinute < global.time.minute)	//说明设定结束的分钟已经超过了
		{
			global.device.endWaterCountDown = (24 - global.time.hour + global.set.endWaterHour)*60 - (global.time.minute - global.set.endWaterMinute);
			global.device.endWaterCountDown += (global.device.nextWaterDayCountDown-1)*24*60;
		}
	}

	/////////////////////////////////////////////////////////////根据倒计时判断是否执行对应操作//////////////////////////////////////////////////////////////
	if(global.device.nextStartWaterCountDown == 0)	//结束照明,这里的不管在线离线，只是为了重置浇水周期的计时
	{
		global.device.nextWaterDayCountDown = global.set.waterCycleDay;
	}
	/////////////前提是处于离线状态，在线状态的控制有wifi模块负责///////////////
	if(global.device.wifi == offline)
	{
		if(global.device.nextStartLightCountDown == 0)	//计时到0，说明可以开始照明了
		{
			global.device.light = true;
			LIGHT_OPEN;
		}
		if(global.device.endLightCountDown == 0)	//结束照明
		{
			global.device.light = false;
			LIGHT_CLOSE;
		}
		if(global.device.nextStartWaterCountDown == 0)	//计时到0，说明可以开始浇水了
		{
			global.device.water = true;
			WATER_OPEN;
		}
		if(global.device.endWaterCountDown == 0)	//结束照明
		{
			global.device.water = false;
			WATER_CLOSE;
		}
	}
}

//通过开始和持续时间，计算结束时间
void figureEndTime(void)
{
	//////首先计算分钟//////////
	if(global.set.startLightMinute + global.set.keepLightMinute >= 60)	//不在同一小时了，需要让小时加一
	{
		global.set.endLightMinute = global.set.startLightMinute + global.set.keepLightMinute - 60;
		if((global.set.startLightHour + global.set.keepLightHour + 1) >= 24)	//开始结束不在同一天
		{
			global.set.endLightHour = global.set.startLightHour + global.set.keepLightHour + 1 - 24;
		}
		else	//在同一天
		{
			global.set.endLightHour = global.set.startLightHour + global.set.keepLightHour + 1;
		}
		
	}
	else	//在同一小时
	{
		global.set.endLightMinute = global.set.startLightMinute + global.set.keepLightMinute;
		if(global.set.startLightHour + global.set.keepLightHour >= 24)	//开始结束不在同一天
		{
			global.set.endLightHour = global.set.startLightHour + global.set.keepLightHour - 24;
		}
		else	//在同一天
		{
			global.set.endLightHour = global.set.startLightHour + global.set.keepLightHour;
		}
	}

	
	if(global.set.startWaterMinute + global.set.keepWaterMinute >= 60)	//不在同一小时了，需要让小时加一
	{
		global.set.endWaterMinute = global.set.startWaterMinute + global.set.keepWaterMinute - 60;
		if((global.set.startWaterHour + global.set.keepWaterHour + 1) >= 24)	//开始结束不在同一天
		{
			global.set.endWaterHour = global.set.startWaterHour + global.set.keepWaterHour + 1 - 24;
		}
		else	//在同一天
		{
			global.set.endWaterHour = global.set.startWaterHour + global.set.keepWaterHour + 1;
		}
		
	}
	else	//在同一小时
	{
		global.set.endWaterMinute = global.set.startWaterMinute + global.set.keepWaterMinute;
		if(global.set.startWaterHour + global.set.keepWaterHour >= 24)	//开始结束不在同一天
		{
			global.set.endWaterHour = global.set.startWaterHour + global.set.keepWaterHour - 24;
		}
		else	//在同一天
		{
			global.set.endWaterHour = global.set.startWaterHour + global.set.keepWaterHour;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////被废弃的方法////////////////////////////////////////////////////////////////

///////////////////////////////////////////////串口9功能//////////////////////////////////////////////////////

// void uart9_fun(unsigned char* buff)
// {
// 	switch(buff[3])	//下标2和3为消息类型，但目前2为0x00所以判断3即可
// 	{
// 		case 0x01:	//设备状态获取请求	//顶板索要设备状态数据，并给出当前温度
// 		{
// 			global.device.temperature = buff[4]<<8 | buff[5];	//拿到温度
// 			global.device.getStateFlag = true;	//标记获取状态标志，将要发送设备状态给顶板
// 		}break;
// 		case 0x03:	//设置参数获取请求
// 		{
// 			global.set.getDataFlag = true;
// 		}break;
// 		case 0x05:	//设备状态设置请求，按下照明或浇水或连wifi
// 		{
// 			global.work.keyDo = buff[4];
// 			global.work.serventSetDevieSateFlag = true;	//判断键值，并且发送给模块决断
// 		}break;
// 		case 0x06:	//设置参数设置请求
// 		{
// 			global.set.hour = buff[4];
// 			global.set.minute = buff[5];
// 			global.set.startLightHour = buff[6];
// 			global.set.startLightMinute = buff[7];
// 			global.set.keepLightHour = buff[8];
// 			global.set.keepLightMinute = buff[9];
// 			global.set.startWaterHour = buff[10];
// 			global.set.startWaterMinute = buff[11];
// 			global.set.keepWaterHour = buff[12];
// 			global.set.keepWaterMinute = buff[13];
// 			global.set.waterCycleDay = buff[14];

// 			global.set.setDatFlag = true;
// 		}break;
// 	}
// }

// ////发送一个字节
// void uart9SendByte(unsigned char dat)
// {
// 	// while(runData.iouart.recEnable);	//当接收使能时，不发送数据，等待接收完毕再发送	//测试后发现并没有软用
// 	global.iouart.sendData = dat;
// 	global.iouart.sendCount = 0;
// 	global.iouart.sendComplete = false;
// 	global.iouart.sendEnable = true;
// 	while(!global.iouart.sendComplete);	//等待数据发送完成
// }

// ////发送一串字节
// void uart9Send(unsigned char* buff,unsigned char len)
// {
// 	while(len)
// 	{
// 		uart9SendByte(*buff);
// 		--len; 
// 		if(len != 0) ++buff;
// 	}
// }

// #define UART9_RD P03
// #define UART9_TD_H set_P04
// #define UART9_TD_L clr_P04

// void initUart9(void)
// {
// 	P04_Quasi_Mode;	//在电路中的标号为Rx,但是电路不改的话需要软件设置为Tx
// 	P03_Quasi_Mode;	//在电路中的标号为Tx,但是电路不改的话需要软件设置为Rx

// 	TMOD |= 0x02;	//模式2,8位自动重装载
// 	clr_T0M;	//12分频
// 	TH0 = 256 - 139;	//9600
// 	set_ET0;	//使能定时器0中断
// 	set_EA;	//使能总中断
// 	set_TR0;	//开启定时器0
// }

// //1起始位--8数据位--1停止位
// void uart9_isr(void)
// {
// 	unsigned char i,sum = 0;

// 	////////接收部分////////
// 	if(global.iouart.recEnable == false)
// 	{
// 		if(!UART9_RD)	//RD脚被拉低--即接收到起始位--起始位为0
// 		{
// 			global.iouart.recEnable = true;
// 			global.iouart.recCount = 0;
// 			global.iouart.recData = 0x00;
// 		}
// 	}
// 	else
// 	{
// 		//接收数据
// 		if(global.iouart.recCount <= 7)
// 		{
// 			global.iouart.recTimeOutCount = 1;	//开启超时计时，为1则计时，如果超时，则放弃当前数据位，收到数据位则刷新
// 			++global.iouart.recCount;
// 			global.iouart.recData >>= 1;
// 			if(UART9_RD)
// 			{
// 				global.iouart.recData |= 0x80;	//低位在先
// 			}
// 		}
// 		else if(global.iouart.recCount == 8)
// 		{
// 			if(UART9_RD)	//RD脚被拉高--即接收到停止位--停止位为1
// 			{
// 				global.iouart.recComplete = true;
// 			}
// 			global.iouart.recTimeOutCount = 0;	//关闭超时计时，为0则不计时
// 			global.iouart.recEnable = false;
// 		}
// 	}

// 	////数据处理
// 	if(global.iouart.recComplete == true)
// 	{
// 		global.iouart.recComplete = false;
		
// 		//判断接收的数据是否符合通信格式
// 		if(global.iouart.nowIndex == 0 && global.iouart.recData == 0x96)	//头
// 		{
// 			global.iouart.recTimeOutCount = 1;	//开启超时计时，为1则计时，如果超时，则放弃当前数据包，收到数据包则刷新
// 			global.iouart.recBuff[global.iouart.nowIndex] = global.iouart.recData;
// 			++global.iouart.nowIndex;
// 		}
// 		else if(global.iouart.nowIndex == 1)	//数据长度
// 		{
// 			global.iouart.recTimeOutCount = 1;	//重置超时计时，如果为0则不计时，如果超时，则放弃当前数据包，收到数据包则刷新
// 			global.iouart.recBuff[global.iouart.nowIndex] = global.iouart.recData;	//长度：从头到校验
// 			++global.iouart.nowIndex;
// 		}
// 		else if(global.iouart.nowIndex >= 2 && global.iouart.nowIndex <= global.iouart.recBuff[1]-2)	//减去了校验字节，因为下标比实际长度小1，所以-2
// 		{
// 			global.iouart.recTimeOutCount = 1;	//重置超时计时，如果为0则不计时，如果超时，则放弃当前数据包，收到数据包则刷新
// 			global.iouart.recBuff[global.iouart.nowIndex] = global.iouart.recData;	//数据
// 			++global.iouart.nowIndex;
// 		}
// 		else if(global.iouart.nowIndex == global.iouart.recBuff[1]-1)	//校验
// 		{
// 			//之后更改模式，将耗时操作都移走

// 			global.iouart.recTimeOutCount = 0;	//关闭超时计时，为0则不计时
// 			for(i = 0;i <= global.iouart.recBuff[1] - 2;i++)	//计算校验和
// 			{
// 				sum += global.iouart.recBuff[i];
// 			}
// 			if(sum == global.iouart.recData)	//校验通过
// 			{
// 				global.iouart.recBuff[global.iouart.nowIndex] = global.iouart.recData;	//校验
// 				uart9_fun(global.iouart.recBuff);	//功能判断
// 			}
// 			global.iouart.nowIndex = 0;
// 		}
// 	}

// 	////////发送部分////////
// 	if(global.iouart.sendEnable)
// 	{
// 		if (global.iouart.sendCount == 0)
// 		{
// 			UART9_TD_L;	//产生一个起始位--0
// 			++global.iouart.sendCount;
// 		}
// 		else if(global.iouart.sendCount >= 1 && global.iouart.sendCount <= 8)
// 		{
// 			if( (global.iouart.sendData >> (global.iouart.sendCount-1)) & 0x01)	//先发低位
// 			{
// 				UART9_TD_H;
// 			}
// 			else
// 			{
// 				UART9_TD_L;
// 			}
// 			++global.iouart.sendCount;
// 		}
// 		else if (global.iouart.sendCount == 9)
// 		{
// 			UART9_TD_H;	//产生一个停止位--1
// 			global.iouart.sendCount = 0;
// 			global.iouart.sendEnable = false;
// 			global.iouart.sendComplete = true;
// 		}
// 	}
// }

