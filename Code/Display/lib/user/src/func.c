#include "func.h"
#include "io.h"

void timeCount(void);
void workFunc(void);
void displayInDifferentStates(void);
void keyFunc(unsigned char timeBase);
void setDisplayNumber(unsigned char num1,unsigned char num2,unsigned char num3);
void displayLed(void);

void parseDataFromBottom(void);
void OneWire_init(void);
void oneWire_write(unsigned char* buff,unsigned char len);
unsigned char oneWire_read(unsigned char* buff);

unsigned char figureSum(unsigned char* buff,unsigned char len);
// void uart0_sendData(unsigned char* buff,unsigned char len);

void init(void)
{
	initAdc();
	OneWire_init();

	//led和数码管全亮
	ledEnable(4);
	dispalyNumber(98);
	//500ms
	Timer3_Delay100ms(15);

	global.device.nextStartLightCountDown = 1000;
	global.device.endLightCountDown = 1000;
	global.device.nextStartWaterCountDown = 1000;
	global.device.endWaterCountDown = 1000;
}

//
//运行在 main.c 的 Timer3_ISR 中，中断时间设置的是 1ms
//
void run(void)
{
	static unsigned char flow = 0;
	static unsigned int count = 0;
	//显示刷新时间片
	switch(flow)
	{
		case 0: { LED_ALL_OFF; LED_EN0; dispalyNumber(global.display.number1); } break;
		case 1: { LED_ALL_OFF; LED_EN1; dispalyNumber(global.display.number2); } break;
		case 2: { LED_ALL_OFF; LED_EN2; dispalyNumber(global.display.number3); } break;
		case 3: LED_ALL_OFF; displayLed(); break;
	}
	flow == 3? flow = 0:++flow;

	if(++count == 10)	//10ms进入一次
	{
		count = 0;
		keyFunc(10);	//按键功能
	}

	timeCount();	//处理一些计时功能
}

//
//运行在 main.c 的 main 函数的 while 中
//主要用于完成一些耗时操作
//
void runMain(void)
{
	displayInDifferentStates();	//不同状态下的显示
	workFunc();	//一些耗时操作
}

//
//处理一些计时功能
//该函数放在 run 函数中，根据定时器的配置，每毫秒进入一次
//该函数内的变量都是类似的模式，当变量大于等于1的时候开始计数，当计数到一定值的时候触发标志
//
void timeCount(void)
{
	//////////////////////////////////////////定时向底板发送设备状态获取请求的计时///////////////////////////////
	if(++global.device.getDataTimeCOunt >= 2000)
	{
		global.device.getDataTimeCOunt = 0;
		if(global.device.getDataFlag == false)
		{
			global.device.getDataFlag = true;
		}
	}
	//////////////////////////////////////////串口模块的计时///////////////////////////////
	//串口接收超时计时
	if(global.uart.recTimeOutCount)
	{
		//大于100ms即为超时
		if(++global.uart.recTimeOutCount >= 100)
		{
			global.uart.recTimeOutCount = 0;
			global.uart.nowIndex = 0;	//放弃当前接收的数据包，准备接收下一组数据包
		}
	}
	//串口发送后等待响应超时计时
	if(global.uart.sendTimeOutCount)
	{
		//大于250ms即为超时
		if(++global.uart.sendTimeOutCount >= 250)
		{
			global.uart.sendTimeOutCount = 0;
			global.uart.sendTimeOutFlag = true;	//置位超时标志
		}
	}
	
	//////////////////////////////////////////传感器模块的计时///////////////////////////////
	if(global.work.getTemperatureSate == free)	//如果没在获取传感器数据
	{
		if(++global.work.getTemperatureTimeCount >= 1000) //1s
		{
			global.work.getTemperatureTimeCount = 0;
			global.work.getTemperatureSate = start;	//开始获取传感器数据
		}
	}
	//////////////////////////////////////////蜂鸣器模块的计时///////////////////////////////
	//蜂鸣器只用于拉警报
	if(global.device.nowater == light && global.work.warnFlag == false)	//设备状态为缺水,且没有拉过警报
	{
		if(++global.work.warnCountDown >= 60000)	//警报60s
		{
			global.work.warnCountDown = 0;
			global.work.warnFlag = true;	//标记已经警报过了
		}
		Beep();	//800ms为一周期，每周期响100ms
	}
	else if(global.device.nowater == dark)
	{
		global.work.warnCountDown = 0;	//清警报计时
		global.work.warnFlag = false;	//清警报标志
		BEEP_OFF;
	}

	//因为都是蜂鸣器所以放在这儿了，这是按键触发的蜂鸣
	keyBeep(&global.work.keyPressBeepFlag);
	//////////////////////////////////////////界面切换的计时///////////////////////////////
	//注意：runState 的切换除了在这儿，还有 workFunc 的设置参数处
	if(global.work.runState <= rs_normal2)	//正常运行时显示的三个界面
	{
		if(global.work.switchTimeCount == 1)	//开始显示温度
		{
			global.display.light = global.device.light;	//按照设备状态显示指示灯
			global.display.water = global.device.water;
			global.work.runState = rs_normal;
		}
		else if(global.work.switchTimeCount == 20000)	//开始显示照明倒计时
		{
			global.display.light = blink;	//照明灯闪烁
			global.work.runState = rs_normal1;
		}
		else if(global.work.switchTimeCount == 25000)	//开始显示浇水倒计时
		{
			global.display.water = blink;	//浇水灯闪烁
			global.work.runState = rs_normal2;
		}
		//30s为一周期
		global.work.switchTimeCount == 30000 ? (global.work.switchTimeCount = 0):(++global.work.switchTimeCount);
	}
	else if(global.work.runState >= rs_sLH)	//设置状态下的显示切换
	{
		if(++global.set.switchTimeCount >= 5000)	//5s
		{
			global.set.switchTimeCount = 0;	//该计时变量会在按键按下时，在 keyFunc 中被清零
			//倒计时结束时，状态在设置的最后一项，则说明设置完成
			if(global.work.runState == rs_wCD || global.work.runState == rs_rtcM)	//预约或rtc设置完成
			{
				global.work.runState = rs_normal;	//切回正常状态
				global.work.switchTimeCount = 0;	//并把正常状态下的计时清零
				global.set.setDataSate = end;	//设置参数状态切换为结束，将在 workFunc 中发送设置参数设置请求
			}
			else	//未到达结束状态则让状态切换至后一个
			{
				++global.work.runState;
			}
		}
	}

	if(global.set.setDataSate == waitAck)
	{
		if(global.set.setWaitTimeOut < 5000) ++global.set.setWaitTimeOut;
	}
	else
	{
		global.set.setWaitTimeOut = 0;
	}
	
}

//
//用于处理一些耗时操作
//
void workFunc(void)
{
	parseDataFromBottom();	//接收处理来自底板的数据
	//////////////////////////////////////////传感器模块的操作///////////////////////////////
	if(global.work.getTemperatureSate == start)	//获取传感器数据
	{
		global.work.temperature =  getAdc();	//实际温度值*10，例如255 = 25.5摄氏度,最大值为1000 = 100.0
		global.work.getTemperatureSate = free;
	}
	//////////////////////////////////////////需要发串口的操作///////////////////////////////
	//////////////向底板发送设备状态获取的请求///////////
	if(global.set.setDataSate != free || global.set.setSateSate != free)	//只有空闲的时候才可以请求
		global.device.getDataFlag = false;
	if(global.device.getDataFlag == true)
	{
		global.uart.sendBuff[0] = 0x96;	//头
		global.uart.sendBuff[1] = 7;	//长度
		global.uart.sendBuff[2] = 0x00;	//消息类型-高8位
		global.uart.sendBuff[3] = 0x01;	//消息类型-低8位
		global.uart.sendBuff[4] = (unsigned char)(global.work.temperature>>8);	//温度
		global.uart.sendBuff[5] = (unsigned char)(global.work.temperature&0xff);
		global.uart.sendBuff[6] = figureSum(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//校验 0x9e

		oneWire_write(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送请求
		global.device.getDataFlag = false;
	}
	///////////////设备状态设置请求///////////////////////
	if(global.set.setSateSate == start)
	{
		global.device.getDataTimeCOunt = 0;
		global.device.getDataFlag = false;
		
		global.uart.sendBuff[0] = 0x96;	//头
		global.uart.sendBuff[1] = 6;	//长度
		global.uart.sendBuff[2] = 0x00;	//消息类型-高8位
		global.uart.sendBuff[3] = 0x05;	//消息类型-低8位
		global.uart.sendBuff[4] = global.set.setSateData;	//要设置的设备状态数据
		global.uart.sendBuff[5] = figureSum(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//校验 0x9e

		oneWire_write(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送请求
		global.set.setSateSate = free;	//进入空闲状态
	}
	else if(global.set.setSateSate == waitAck)	//等待响应
	{
		// if(global.set.setWaitTimeOut >= 3000)
		// {
		// 	global.set.setSateSate = free;	//状态切回空闲
		// 	global.work.runState = rs_normal;	//状态切换为正常
		// }
	// 	if(global.uart.sendTimeOutFlag)	//等待响应超时了
	// 	{	//开始设置时将无法进入设置，结束设置时将保存设置失败

	// 		global.uart.sendTimeOutFlag = false;
	// 		if(global.uart.resendCount >= 5)	//重发四次都失败
	// 		{
	// 			global.set.setSateSate = free;	//状态切回空闲
	// 			global.uart.resendCount = 0;
	// 		}
	// 		else
	// 		{
	// 			++global.uart.resendCount;	//重发次数加一
	// 			global.set.setSateSate = start;
	// 		}
			
	// 	}
	// }
	// else if(global.set.setSateSate == end)	//得到响应，结束
	// {
	// 	global.set.setSateSate = free;
	// 	global.uart.resendCount = 0;
	}
	
	////////////////////////设置参数//////////////////////////
	if(global.set.setDataSate == start)	//开始设置时，发送设置参数获取请求
	{
		global.uart.sendBuff[0] = 0x96;	//头
		global.uart.sendBuff[1] = 5;	//长度
		global.uart.sendBuff[2] = 0x00;	//消息类型-高8位
		global.uart.sendBuff[3] = 0x03;	//消息类型-低8位
		global.uart.sendBuff[4] = figureSum(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//校验 0x9e

		global.work.runState = rs_wait;	//状态切换为等待
		global.set.setDataSate = waitAck;	//进入等待响应状态
		oneWire_write(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送请求
	}
	else if(global.set.setDataSate == end)	//结束设置时，发送设置参数设置请求
	{
		global.uart.sendBuff[0] = 0x96;	//头
		global.uart.sendBuff[1] = 16;	//长度
		global.uart.sendBuff[2] = 0x00;	//消息类型-高8位
		global.uart.sendBuff[3] = 0x06;	//消息类型-低8位
		if(global.set.setAppointmentFlag == 2)	//如果是预约设置
		{
			global.uart.sendBuff[4] = 0xff;	//rtc-小时--不修改
			global.uart.sendBuff[5] = 0xff;	//rtc-分钟--不修改
		}
		if(global.set.setRtcFlag == 2)
		{
			global.uart.sendBuff[4] = global.set.hour;	//rtc-小时
			global.uart.sendBuff[5] = global.set.minute;	//rtc-分钟
		}
		global.uart.sendBuff[6] = global.set.startLightHour;	//开始照明的时间-小时
		global.uart.sendBuff[7] = global.set.startLightMinute;	//开始照明的时间-分钟
		global.uart.sendBuff[8] = global.set.keepLightHour;	//持续照明的时间-小时
		global.uart.sendBuff[9] = global.set.keepLightMinute;	//持续照明的时间-分钟
		global.uart.sendBuff[10] = global.set.startWaterHour;	//开始浇水的时间-小时
		global.uart.sendBuff[11] = global.set.startWaterMinute;	//开始浇水的时间-分钟
		global.uart.sendBuff[12] = global.set.keepWaterHour;	//持续浇水的时间-小时
		global.uart.sendBuff[13] = global.set.keepWaterMinute;	//持续浇水的时间-分钟
		global.uart.sendBuff[14] = global.set.waterCycleDay;	//浇水的周期-天
		global.uart.sendBuff[15] = figureSum(global.uart.sendBuff,global.uart.sendBuff[1]-1);	//校验

		global.work.runState = rs_wait;	//状态切换为等待
		global.set.setDataSate = waitAck;	//进入等待响应状态
		oneWire_write(global.uart.sendBuff,global.uart.sendBuff[1]);	//发送请求
	}
	else if(global.set.setDataSate == ing)	//得到响应进入设置
	{
		if(global.set.setAppointmentFlag == true)	//如果是预约设置
		{
			global.set.setAppointmentFlag = 2;
			global.work.runState = rs_sLH;	//切换状态至预约设置的第一项
		}
		if(global.set.setRtcFlag == true)	//如果时rtc设置
		{
			global.set.setRtcFlag = 2;
			global.work.runState = rs_rtcH;	//切换状态至rtc设置的第一项
		}
		global.uart.resendCount = 0;
		global.set.setDataSate = ing;	//当设置完成后会被设为 end
	}
	else if(global.set.setDataSate == waitAck)	//等待响应
	{
		if(global.set.setWaitTimeOut >= 3000)
		{
			global.set.setDataSate = free;	//状态切回空闲
			global.work.runState = rs_normal;	//状态切换为正常
		}
		// if(global.uart.sendTimeOutFlag)	//等待响应超时了
		// {	//开始设置时将无法进入设置，结束设置时将保存设置失败

		// 	global.uart.sendTimeOutFlag = false;
		// 	if(global.uart.resendCount >= 20)	//重发四次都失败
		// 	{
		// 		global.set.setDataSate = free;	//状态切回空闲
		// 		global.work.runState = rs_normal;	//状态切换为正常
		// 		global.work.switchTimeCount = 0;
		// 		global.uart.resendCount = 0;
		// 		global.set.setAppointmentFlag = false;
		// 		global.set.setRtcFlag = false;
		// 	}
		// 	else
		// 	{
		// 		++global.uart.resendCount;	//重发次数加一
		// 		if(global.set.setAppointmentFlag == true || global.set.setRtcFlag == true)	//有一个为true说明未进入end状态，说明上一个发送请求的状态为start
		// 		{
		// 			global.set.setDataSate = start;
		// 		}
		// 		else
		// 		{
		// 			global.set.setDataSate = end;
		// 		}
		// 	}
			
		// }
	}
	else if(global.set.setDataSate == ack)	//响应
	{
		global.set.setDataSate = free;
		global.work.runState = rs_normal;	//状态切换为正常
		global.work.switchTimeCount = 0;
		global.uart.resendCount = 0;
		global.set.setAppointmentFlag = false;
		global.set.setRtcFlag = false;
	}
}

//
//不同状态下的显示
//
void displayInDifferentStates(void)
{
	global.display.wifi = global.device.wifi;
	global.display.nowater = global.device.nowater;
	switch(global.work.runState)
	{
		case rs_normal:	//显示温度
		{
			global.display.appointment = global.device.appointment;	//除了设置时，其他时候不点亮
			global.display.light = global.device.light;	//正常情况就按照设备状态显示
			global.display.water = global.device.water;	//正常情况就按照设备状态显示
			//第二个参数加10是因为10-19表示显示 0.-9. ，使个位后面有小数点
			setDisplayNumber(global.work.temperature/100,global.work.temperature/10%10 + 10,global.work.temperature%10);
		}break;
		case rs_normal1:	//显示照明倒计时
		{
			global.display.light = blink;
			global.display.water = dark;
			if(global.device.nextStartLightCountDown != 0)	//这里是为了不显示0的倒计时,当为0时直接显示下一个倒计时
			{
				//开始的小于于结束的，说明先到达开始，那就显示距离开始的倒计时
				if(global.device.nextStartLightCountDown < global.device.endLightCountDown || global.device.endLightCountDown == 0)
				{
					//最大也就是24*60分钟
					if(global.device.nextStartLightCountDown/60 >= 999)	//正常情况下不可能出现这个值,这里为一个限定
						setDisplayNumber(9,9,9);
					else if(global.device.nextStartLightCountDown/60 >= 100)	//大于等于100小时
						setDisplayNumber(global.device.nextStartLightCountDown/60/100,global.device.nextStartLightCountDown/60/10%10,global.device.nextStartLightCountDown/60%10);
					else if(global.device.nextStartLightCountDown/60 >= 10)	//大于等于10小时
						setDisplayNumber(0,global.device.nextStartLightCountDown/60/10,global.device.nextStartLightCountDown/60%10);
					else if(global.device.nextStartLightCountDown/60 >= 1)	//大于等于1小时
						setDisplayNumber(0,0,global.device.nextStartLightCountDown/60);
					else if(global.device.nextStartLightCountDown >= 10)	//大于等于10分钟
						setDisplayNumber(10,global.device.nextStartLightCountDown/10,global.device.nextStartLightCountDown%10);	//以0.开头
					else if(global.device.nextStartLightCountDown >= 1)	//大于等于1分钟
						setDisplayNumber(10,0,global.device.nextStartLightCountDown%10);	//以0.开头
					else if(global.device.nextStartLightCountDown == 0)	//等于0分钟
						setDisplayNumber(10,0,0);	//以0.开头
				}
			}
			if(global.device.endLightCountDown != 0)
			{
				//显示距离下一次开始的倒计时
				if(global.device.endLightCountDown < global.device.nextStartLightCountDown || global.device.nextStartLightCountDown == 0)
				{
					//最大也就是24*60分钟
					if(global.device.endLightCountDown/60 >= 999)	//正常情况下不可能出现这个值,这里为一个限定
						setDisplayNumber(9,9,9);
					else if(global.device.endLightCountDown/60 >= 100)	//大于等于100小时
						setDisplayNumber(global.device.endLightCountDown/60/100,global.device.endLightCountDown/60/10%10,global.device.endLightCountDown/60%10);
					else if(global.device.endLightCountDown/60 >= 10)	//大于等于10小时
						setDisplayNumber(0,global.device.endLightCountDown/60/10,global.device.endLightCountDown/60%10);
					else if(global.device.endLightCountDown/60 >= 1)	//大于等于1小时
						setDisplayNumber(0,0,global.device.endLightCountDown/60);
					else if(global.device.endLightCountDown >= 10)	//大于等于10分钟
						setDisplayNumber(10,global.device.endLightCountDown/10,global.device.endLightCountDown%10);	//以0.开头
					else if(global.device.endLightCountDown >= 1)	//大于等于1分钟
						setDisplayNumber(10,0,global.device.endLightCountDown%10);	//以0.开头
					else if(global.device.endLightCountDown == 0)	//等于0分钟
						setDisplayNumber(10,0,0);	//以0.开头
				}
			}
			//当相等的时候，不管时0还是多少，反正显示一个值就行了,正常情况是不会出现的,这里不加的话，可能会出现当相等或都为0的时候,倒计时显示异常
			if(global.device.nextStartLightCountDown == global.device.endLightCountDown)
			{
				//最大也就是24*60分钟
				if(global.device.endLightCountDown/60 >= 999)	//正常情况下不可能出现这个值,这里为一个限定
					setDisplayNumber(9,9,9);
				else if(global.device.endLightCountDown/60 >= 100)	//大于等于100小时
					setDisplayNumber(global.device.endLightCountDown/60/100,global.device.endLightCountDown/60/10%10,global.device.endLightCountDown/60%10);
				else if(global.device.endLightCountDown/60 >= 10)	//大于等于10小时
					setDisplayNumber(0,global.device.endLightCountDown/60/10,global.device.endLightCountDown/60%10);
				else if(global.device.endLightCountDown/60 >= 1)	//大于等于1小时
					setDisplayNumber(0,0,global.device.endLightCountDown/60);
				else if(global.device.endLightCountDown >= 10)	//大于等于10分钟
					setDisplayNumber(10,global.device.endLightCountDown/10,global.device.endLightCountDown%10);	//以0.开头
				else if(global.device.endLightCountDown >= 1)	//大于等于1分钟
					setDisplayNumber(10,0,global.device.endLightCountDown%10);	//以0.开头
				else if(global.device.endLightCountDown == 0)	//等于0分钟
					setDisplayNumber(10,0,0);	//以0.开头
			}
		}break;
		case rs_normal2:	//显示浇水倒计时
		{
			global.display.light = dark;
			global.display.water = blink;
			if(global.device.nextStartWaterCountDown != 0)
			{
				//开始的小于于结束的，说明先到达开始，那就显示距离开始的倒计时
				if(global.device.nextStartWaterCountDown < global.device.endWaterCountDown || global.device.endWaterCountDown == 0)
				{
					//最大也就是30*24*60分钟，720小时
					if(global.device.nextStartWaterCountDown/60 >= 999)	//正常情况下不可能出现这个值,这里为一个限定
						setDisplayNumber(9,9,9);
					else if(global.device.nextStartWaterCountDown/60 >= 100)	//大于等于100小时
						setDisplayNumber(global.device.nextStartWaterCountDown/60/100,global.device.nextStartWaterCountDown/60/10%10,global.device.nextStartWaterCountDown/60%10);
					else if(global.device.nextStartWaterCountDown/60 >= 10)	//大于等于10小时
						setDisplayNumber(0,global.device.nextStartWaterCountDown/60/10,global.device.nextStartWaterCountDown/60%10);
					else if(global.device.nextStartWaterCountDown/60 >= 1)	//大于等于1小时
						setDisplayNumber(0,0,global.device.nextStartWaterCountDown/60);
					else if(global.device.nextStartWaterCountDown >= 10)	//大于等于10分钟
						setDisplayNumber(10,global.device.nextStartWaterCountDown/10,global.device.nextStartWaterCountDown%10);	//以0.开头
					else if(global.device.nextStartWaterCountDown >= 1)	//大于等于1分钟
						setDisplayNumber(10,0,global.device.nextStartWaterCountDown%10);	//以0.开头
					else if(global.device.nextStartWaterCountDown == 0)	//等于0分钟
						setDisplayNumber(10,0,0);	//以0.开头
				}
			}
			if(global.device.endWaterCountDown != 0)
			{
				if(global.device.endWaterCountDown < global.device.nextStartWaterCountDown || global.device.nextStartWaterCountDown == 0)	//说明未在照明，那就显示距离下一次开始的倒计时
				{
					//最大也就是15*24*60分钟，360小时
					if(global.device.endWaterCountDown/60 >= 999)	//正常情况下不可能出现这个值,这里为一个限定
						setDisplayNumber(9,9,9);
					else if(global.device.endWaterCountDown/60 >= 100)	//大于等于100小时
						setDisplayNumber(global.device.endWaterCountDown/60/100,global.device.endWaterCountDown/60/10%10,global.device.endWaterCountDown/60%10);
					else if(global.device.endWaterCountDown/60 >= 10)	//大于等于10小时
						setDisplayNumber(0,global.device.endWaterCountDown/60/10,global.device.endWaterCountDown/60%10);
					else if(global.device.endWaterCountDown/60 >= 1)	//大于等于1小时
						setDisplayNumber(0,0,global.device.endWaterCountDown/60);
					else if(global.device.endWaterCountDown >= 10)	//大于等于10分钟
						setDisplayNumber(10,global.device.endWaterCountDown/10,global.device.endWaterCountDown%10);	//以0.开头
					else if(global.device.endWaterCountDown >= 1)	//大于等于1分钟
						setDisplayNumber(10,0,global.device.endWaterCountDown%10);	//以0.开头
					else if(global.device.endWaterCountDown == 0)	//等于0分钟
						setDisplayNumber(10,0,0);	//以0.开头
				}
			}
			//当相等的时候，不管时0还是多少，反正显示一个值就行了,正常情况是不会出现的,这里不加的话，可能会出现当相等或都为0的时候,倒计时显示异常
			if(global.device.endWaterCountDown == global.device.nextStartWaterCountDown)
			{
				//最大也就是15*24*60分钟，360小时
				if(global.device.endWaterCountDown/60 >= 999)	//正常情况下不可能出现这个值,这里为一个限定
					setDisplayNumber(9,9,9);
				else if(global.device.endWaterCountDown/60 >= 100)	//大于等于100小时
					setDisplayNumber(global.device.endWaterCountDown/60/100,global.device.endWaterCountDown/60/10%10,global.device.endWaterCountDown/60%10);
				else if(global.device.endWaterCountDown/60 >= 10)	//大于等于10小时
					setDisplayNumber(0,global.device.endWaterCountDown/60/10,global.device.endWaterCountDown/60%10);
				else if(global.device.endWaterCountDown/60 >= 1)	//大于等于1小时
					setDisplayNumber(0,0,global.device.endWaterCountDown/60);
				else if(global.device.endWaterCountDown >= 10)	//大于等于10分钟
					setDisplayNumber(10,global.device.endWaterCountDown/10,global.device.endWaterCountDown%10);	//以0.开头
				else if(global.device.endWaterCountDown >= 1)	//大于等于1分钟
					setDisplayNumber(10,0,global.device.endWaterCountDown%10);	//以0.开头
				else if(global.device.endWaterCountDown == 0)	//等于0分钟
					setDisplayNumber(10,0,0);	//以0.开头
			}
		}break;
		case rs_wait:
		{
			setDisplayNumber(30,30,30);	//---
		}break;
	}
	
	//设置模式下
	if(global.set.setDataSate == ing)	//设置中
	{
		switch(global.work.runState)
		{
			case rs_sLH:	//预约设置--开始照明-小时
			{
				global.display.appointment = light;
				global.display.light = blink;
				global.display.water = dark;
				setDisplayNumber(11,global.set.startLightHour/10,global.set.startLightHour%10);
			}break;
			case rs_sLM:	//预约设置--开始照明-分钟
			{
				global.display.appointment = light;
				global.display.light = blink;
				global.display.water = dark;
				setDisplayNumber(12,global.set.startLightMinute/10,global.set.startLightMinute%10);
			}break;
			case rs_kLH:	//预约设置--持续照明-小时
			{
				global.display.appointment = light;
				global.display.light = blink;
				global.display.water = dark;
				setDisplayNumber(13,global.set.keepLightHour/10,global.set.keepLightHour%10);
			}break;
			case rs_kLM:	//预约设置--持续照明-分钟
			{
				global.display.appointment = light;
				global.display.light = blink;
				global.display.water = dark;
				setDisplayNumber(14,global.set.keepLightMinute/10,global.set.keepLightMinute%10);
			}break;
			case rs_sWH:	//预约设置--开始浇水-小时
			{
				global.display.appointment = light;
				global.display.light = dark;
				global.display.water = blink;
				setDisplayNumber(11,global.set.startWaterHour/10,global.set.startWaterHour%10);
			}break;
			case rs_sWM:	//预约设置--开始浇水-分钟
			{
				global.display.appointment = light;
				global.display.light = dark;
				global.display.water = blink;
				setDisplayNumber(12,global.set.startWaterMinute/10,global.set.startWaterMinute%10);
			}break;
			case rs_kWH:	//预约设置--持续浇水-小时
			{
				global.display.appointment = light;
				global.display.light = dark;
				global.display.water = blink;
				setDisplayNumber(13,global.set.keepWaterHour/10,global.set.keepWaterHour%10);
			}break;
			case rs_kWM:	//预约设置--持续浇水-分钟
			{
				global.display.appointment = light;
				global.display.light = dark;
				global.display.water = blink;
				setDisplayNumber(14,global.set.keepWaterMinute/10,global.set.keepWaterMinute%10);
			}break;
			case rs_wCD:	//预约设置--浇水周期-天
			{
				global.display.appointment = light;
				global.display.light = dark;
				global.display.water = blink;
				setDisplayNumber(15,global.set.waterCycleDay/10,global.set.waterCycleDay%10);
			}break;
			case rs_rtcH:	//rtc设置--小时
			{
				global.display.appointment = blink;
				global.display.light = dark;
				global.display.water = dark;
				setDisplayNumber(11,global.set.hour/10,global.set.hour%10);
			}break;
			case rs_rtcM:	//rtc设置--分钟
			{
				global.display.appointment = blink;
				global.display.light = dark;
				global.display.water = dark;
				setDisplayNumber(12,global.set.minute/10,global.set.minute%10);
			}break;
		}
	}
	else if(global.set.setDataSate == waitAck)
	{
		setDisplayNumber(30,30,30);	//三个短横
	}
}

//
//按键功能，在 run 中每 10ms 调用一次
//
void keyFunc(unsigned char timeBase)
{
	static unsigned char shortLongPressFlag = false;
	unsigned char shortlongtime100 = 1;
	keyScanf(timeBase);	//每10ms扫描一次

	if(ioData.key.vlaue == noneKey)	//没有键值
	{
		shortLongPressFlag = false;
		return;
	}

	if(ioData.key.vlaue == shortlongKey1 || ioData.key.vlaue == shortlongKey2)	//当键值是连续按时
	{
		if(global.work.runState >= rs_sLH && shortLongPressFlag == false)	//当处于设置模式时，并且是连续按的第一次触发
		{
			shortLongPressFlag = true;
			global.work.keyPressBeepFlag = true;	//有键值则置位标志，响按键音
		}
	}
	else	//不是连续按的时候
	{
		global.work.keyPressBeepFlag = true;	//有键值则置位标志，响按键音
	}

	switch(ioData.key.vlaue)
	{
		case noneKey:  break;
		/////////////////////////////////////////////短按照明键///////////////////////////////////////////////////
		case shortKey1:
		{
			global.set.switchTimeCount = 0;	//清除设置状态下的自动切换计时

			if(global.work.runState <= rs_normal2)	//在正常运行的三个状态中
			{
				if(global.device.light == light)	//如果在照明
				{
					global.set.setSateData = 0x10;	//照明关
				}
				else
				{
					global.set.setSateData = 0x11;	//照明开
				}
				if(global.set.setSateSate == free)
				{
					global.set.setSateSate = start;	//将在 workFunc 中发送设备状态设置请求
				}
			}
			else
			{
				switch (global.work.runState)
				{
					case rs_sLH:	//减 开始照明时间-小时
					{
						global.set.startLightHour == 0 ? (global.set.startLightHour = 23):(--global.set.startLightHour);
					}break;
					case rs_sLM:	//减 开始照明时间-分钟
					{
						global.set.startLightMinute == 0 ? (global.set.startLightMinute = 59):(--global.set.startLightMinute);
					}break;
					case rs_kLH:	//减 持续照明时间-小时
					{
						global.set.keepLightHour == 0 ? (global.set.keepLightHour = 23):(--global.set.keepLightHour);
					}break;
					case rs_kLM:	//减 持续照明时间-分钟
					{
						global.set.keepLightMinute == 0 ? (global.set.keepLightMinute = 59):(--global.set.keepLightMinute);
					}break;
					case rs_sWH:	//减 开始浇水时间-小时
					{
						global.set.startWaterHour == 0 ? (global.set.startWaterHour = 23):(--global.set.startWaterHour);
					}break;
					case rs_sWM:	//减 开始浇水时间-分钟
					{
						global.set.startWaterMinute == 0 ? (global.set.startWaterMinute = 59):(--global.set.startWaterMinute);
					}break;
					case rs_kWH:	//减 持续浇水时间-小时
					{
						global.set.keepWaterHour == 0 ? (global.set.keepWaterHour = 23):(--global.set.keepWaterHour);
					}break;
					case rs_kWM:	//减 持续浇水时间-分钟
					{
						global.set.keepWaterMinute == 0 ? (global.set.keepWaterMinute = 59):(--global.set.keepWaterMinute);
					}break;
					case rs_wCD:	//减 浇水周期-天
					{
						global.set.waterCycleDay == 1 ? (global.set.waterCycleDay = 30):(--global.set.waterCycleDay);
					}break;
					case rs_rtcH:	//减 rtc时间-小时
					{
						global.set.hour == 0 ? (global.set.hour = 23):(--global.set.hour);
					}break;
					case rs_rtcM:	//减 rtc时间-分钟
					{
						global.set.minute == 0 ? (global.set.minute = 59):(--global.set.minute);
					}break;
				}
			}
		}break;
		//////////////////////////////////////////////////////////短按浇水键//////////////////////////////////////////////////////////
		case shortKey2:
		{
			global.set.switchTimeCount = 0;	//清除设置状态下的自动切换计时

			if(global.work.runState <= rs_normal2)	//在正常运行的三个状态中
			{
				if(global.device.water == light)	//如果在浇水
				{
					global.set.setSateData = 0x20;	//浇水关
				}
				else
				{
					global.set.setSateData = 0x21;	//浇水开
				}
				if(global.set.setSateSate == free)
				{
					global.set.setSateSate = start;	//将在 workFunc 中发送设备状态设置请求
				}
			}
			else
			{
				switch (global.work.runState)
				{
					case rs_sLH:	//加 开始照明时间-小时
					{
						global.set.startLightHour == 23 ? (global.set.startLightHour = 0):(++global.set.startLightHour);
					}break;
					case rs_sLM:	//加 开始照明时间-分钟
					{
						global.set.startLightMinute == 59 ? (global.set.startLightMinute = 0):(++global.set.startLightMinute);
					}break;
					case rs_kLH:	//加 持续照明时间-小时
					{
						global.set.keepLightHour == 23 ? (global.set.keepLightHour = 0):(++global.set.keepLightHour);
					}break;
					case rs_kLM:	//加 持续照明时间-分钟
					{
						global.set.keepLightMinute == 59 ? (global.set.keepLightMinute = 0):(++global.set.keepLightMinute);
					}break;
					case rs_sWH:	//加 开始浇水时间-小时
					{
						global.set.startWaterHour == 23 ? (global.set.startWaterHour = 0):(++global.set.startWaterHour);
					}break;
					case rs_sWM:	//加 开始浇水时间-分钟
					{
						global.set.startWaterMinute == 59 ? (global.set.startWaterMinute = 0):(++global.set.startWaterMinute);
					}break;
					case rs_kWH:	//加 持续浇水时间-小时
					{
						global.set.keepWaterHour == 23 ? (global.set.keepWaterHour = 0):(++global.set.keepWaterHour);
					}break;
					case rs_kWM:	//加 持续浇水时间-分钟
					{
						global.set.keepWaterMinute == 59 ? (global.set.keepWaterMinute = 0):(++global.set.keepWaterMinute);
					}break;
					case rs_wCD:	//加 浇水周期-天
					{
						global.set.waterCycleDay == 30 ? (global.set.waterCycleDay = 1):(++global.set.waterCycleDay);
					}break;
					case rs_rtcH:	//加 rtc时间-小时
					{
						global.set.hour == 23 ? (global.set.hour = 0):(++global.set.hour);
					}break;
					case rs_rtcM:	//加 rtc时间-分钟
					{
						global.set.minute == 59 ? (global.set.minute = 0):(++global.set.minute);
					}break;
				}
			}
		}break;
		//////////////////////////////////////////////////////////持续按照明键--即设置模式下的连续+-//////////////////////////////////////////////////////////
		case shortlongKey1:
		{
			global.set.switchTimeCount = 0;	//清除设置状态下的自动切换计时
			
			if(global.work.runState < rs_sLH) break;
			
			switch (global.work.runState)
			{
				case rs_sLH:	//减 开始照明时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startLightHour == 0 ? (global.set.startLightHour = 23):(--global.set.startLightHour);
					}
				}break;
				case rs_sLM:	//减 开始照明时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startLightMinute == 0 ? (global.set.startLightMinute = 59):(--global.set.startLightMinute);
					}
				}break;
				case rs_kLH:	//减 持续照明时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepLightHour == 0 ? (global.set.keepLightHour = 23):(--global.set.keepLightHour);
					}
				}break;
				case rs_kLM:	//减 持续照明时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepLightMinute == 0 ? (global.set.keepLightMinute = 59):(--global.set.keepLightMinute);
					}
				}break;
				case rs_sWH:	//减 开始浇水时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startWaterHour == 0 ? (global.set.startWaterHour = 23):(--global.set.startWaterHour);
					}
				}break;
				case rs_sWM:	//减 开始浇水时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startWaterMinute == 0 ? (global.set.startWaterMinute = 59):(--global.set.startWaterMinute);
					}
				}break;
				case rs_kWH:	//减 持续浇水时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepWaterHour == 0 ? (global.set.keepWaterHour = 23):(--global.set.keepWaterHour);
					}
				}break;
				case rs_kWM:	//减 持续浇水时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepWaterMinute == 0 ? (global.set.keepWaterMinute = 59):(--global.set.keepWaterMinute);
					}
				}break;
				case rs_wCD:	//减 浇水周期-天
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.waterCycleDay == 1 ? (global.set.waterCycleDay = 30):(--global.set.waterCycleDay);
					}
				}break;
				case rs_rtcH:	//减 rtc时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.hour == 0 ? (global.set.hour = 23):(--global.set.hour);
					}
				}break;
				case rs_rtcM:	//减 rtc时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.minute == 0 ? (global.set.minute = 59):(--global.set.minute);
					}
				}break;
			}
		}break;
		//////////////////////////////////////////////////////////持续按浇水键--即设置模式下的连续+-//////////////////////////////////////////////////////////
		case shortlongKey2:
		{
			global.set.switchTimeCount = 0;	//清除设置状态下的自动切换计时
			
			if(global.work.runState < rs_sLH) break;

			switch (global.work.runState)
			{
				case rs_sLH:	//加 开始照明时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startLightHour == 23 ? (global.set.startLightHour = 0):(++global.set.startLightHour);
					}
				}break;
				case rs_sLM:	//加 开始照明时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startLightMinute == 59 ? (global.set.startLightMinute = 0):(++global.set.startLightMinute);
					}
				}break;
				case rs_kLH:	//加 持续照明时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepLightHour == 23 ? (global.set.keepLightHour = 0):(++global.set.keepLightHour);
					}
				}break;
				case rs_kLM:	//加 持续照明时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepLightMinute == 59 ? (global.set.keepLightMinute = 0):(++global.set.keepLightMinute);
					}
				}break;
				case rs_sWH:	//加 开始浇水时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startWaterHour == 23 ? (global.set.startWaterHour = 0):(++global.set.startWaterHour);
					}
				}break;
				case rs_sWM:	//加 开始浇水时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.startWaterMinute == 59 ? (global.set.startWaterMinute = 0):(++global.set.startWaterMinute);
					}
				}break;
				case rs_kWH:	//加 持续浇水时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepWaterHour == 23 ? (global.set.keepWaterHour = 0):(++global.set.keepWaterHour);
					}
				}break;
				case rs_kWM:	//加 持续浇水时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.keepWaterMinute == 59 ? (global.set.keepWaterMinute = 0):(++global.set.keepWaterMinute);
					}
				}break;
				case rs_wCD:	//加 浇水周期-天
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.waterCycleDay == 30 ? (global.set.waterCycleDay = 1):(++global.set.waterCycleDay);
					}
				}break;
				case rs_rtcH:	//加 rtc时间-小时
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.hour == 23 ? (global.set.hour = 0):(++global.set.hour);
					}
				}break;
				case rs_rtcM:	//加 rtc时间-分钟
				{
					ioData.key.modeLock = shortlongKey1;	//锁定在该模式,避免过久的长按触发长按键值
					if(ioData.key.pressTime_100ms >= shortlongtime100)	//ms触发一次
					{
						ioData.key.timeCount = 0;	//清空计数，重新计时
						global.set.minute == 59 ? (global.set.minute = 0):(++global.set.minute);
					}
				}break;
			}
		}break;
		//////////////////////////////////////////////////////////长按照明键//////////////////////////////////////////////////////////
		case longKey1:
		{
			if(global.work.runState <= rs_normal2)
			{
				global.set.setSateData = 0x01;	//连接wifi
				if(global.set.setSateSate == free)
				{
					global.set.setSateSate = start;	//将在 workFunc 中发送设备状态设置请求
				}
			}
		}break;
		//////////////////////////////////////////////////////////长按浇水键//////////////////////////////////////////////////////////
		case longKey2:
		{
			if(global.work.runState <= rs_normal2)
			{
				global.set.setAppointmentFlag = true;	//置位预约设置标志，将在得到应答后在 workFunc 中被使用
				global.set.setDataSate = start;	//将在 workFunc 中发送设置参数获取请求
			}
		}break;
		//////////////////////////////////////////////////////////长按组合键//////////////////////////////////////////////////////////
		case longDoubleKey:
		{
			if(global.work.runState <= rs_normal2)
			{
				global.set.setRtcFlag = true;	//置位预约设置标志
				global.set.setDataSate = start;	//将在 workFunc 中发送设置参数获取请求
			}
		}break;
		default:  break;
	}
}

////////////////////////////////////////////////////LED显示部分//////////////////////////////////////////////////////////////////////

void setDisplayNumber(unsigned char num1,unsigned char num2,unsigned char num3)
{
	global.display.number1 = num1;
	global.display.number2 = num2;
	global.display.number3 = num3;
}

void displayLed(void)
{
	static unsigned int LedBlinkTimerCount = 0,LedBlinkTimerCount1 = 0;
	unsigned char timeBase0 = 250,timeBase1 = 125;
	static unsigned char timeBase2 = 0,timeBase3 = 0;
	LedBlinkTimerCount >= timeBase0? LedBlinkTimerCount = 0: ++LedBlinkTimerCount;
	LedBlinkTimerCount1 >= timeBase2? LedBlinkTimerCount1 = 0: ++LedBlinkTimerCount1;

	LED_EN3;
	switch(global.display.wifi)
	{	//////在定时器中断中 4ms 进入一次
		case offline: resetLed(0); break;	//离线,常灭
		case connecting:	//一键配置,周期40ms,亮20ms,灭20ms
		{
			timeBase2 = 100; timeBase3 = 50;
			LedBlinkTimerCount1 < timeBase3? setLed(0):resetLed(0);
		}break;
		case connectToAp:	//连接路由,周期100ms,亮20ms,灭80ms
		{
			timeBase2 = 250; timeBase3 = 50;
			LedBlinkTimerCount1 < timeBase3? setLed(0):resetLed(0);
		}break;
		case connectToService:	//连接路由,周期100ms,亮50ms,灭50ms
		{
			timeBase2 = 250; timeBase3 = 125;
			LedBlinkTimerCount1 < timeBase3? setLed(0):resetLed(0);
		}break;
		case online: setLed(0); break;	//在线,常亮
	}
	switch(global.display.appointment)
	{
		case light: setLed(1); break;
		case dark: resetLed(1); break;
		case blink:
		{
			LedBlinkTimerCount >= timeBase1? setLed(1):resetLed(1);
		}break;
	}
	switch(global.display.light)
	{
		case light: setLed(2); break;
		case dark: resetLed(2); break;
		case blink:
		{
			LedBlinkTimerCount >= timeBase1? setLed(2):resetLed(2);
		}break;
	}
	switch(global.display.water)
	{
		case light: setLed(3); break;
		case dark: resetLed(3); break;
		case blink:
		{
			LedBlinkTimerCount >= timeBase1? setLed(3):resetLed(3);
		}break;
	}
	switch(global.display.nowater)
	{
		case light: setLed(4); break;
		case dark: resetLed(4); break;
		case blink:
		{
			LedBlinkTimerCount >= timeBase1? setLed(4):resetLed(4);
		}break;
	}
}

////////////////////////////////////////////////////通信数据解析//////////////////////////////////////////////////////////////////////

void parseDataFromBottom(void)
{
	unsigned char len = 0,sum = 0;
	unsigned char buff[32] = {0};
	unsigned int cmd = 0,temp = 0;
	len = oneWire_read(buff);
	if(len != 0)	//大于0则说明有数据包
	{
		sum = figureSum(buff,len-1);
		if(buff[0] == 0x96 && buff[len-1] == sum)	//确认包头和校验
		{
			cmd = buff[2]<<8 | buff[3];
			switch(cmd)
			{
				case 0x0002:	//设备状态获取的响应
				{
					global.device.wifi = buff[4];	//wifi指示灯的状态
					global.device.appointment = buff[5];	//预约指示灯的状态
					global.device.light = buff[6];	//照明指示灯的状态
					global.device.water = buff[7];	//浇水指示灯的状态
					global.device.nowater = buff[8];	//缺水指示灯的状态
					temp = buff[9]<<8 | buff[10];	//距离下一次开始照明的倒计时，单位分钟
					global.device.nextStartLightCountDown = temp;
					// if(temp > 999) global.device.nextStartLightCountDown = 999;
					// else global.device.nextStartLightCountDown = temp;
					temp = buff[11]<<8 | buff[12];	//距离结束照明的倒计时，单位分钟
					global.device.endLightCountDown = temp;
					// if(temp > 999) global.device.endLightCountDown = 999;
					// else global.device.endLightCountDown = temp;
					temp  = buff[13]<<8 | buff[14];	//距离下一次开始浇水的倒计时，单位分钟
					global.device.nextStartWaterCountDown = temp;
					// if(temp > 999) global.device.nextStartWaterCountDown = 999;
					// else global.device.nextStartWaterCountDown = temp;
					temp = buff[15]<<8 | buff[16];	//距离结束浇水的倒计时，单位分钟
					global.device.endWaterCountDown = temp;
					// if(temp > 999) global.device.endWaterCountDown = 999;
					// else global.device.endWaterCountDown = temp;

					// global.set.setSateSate = free;
				}break;
				case 0x0004:	//设置参数获取的响应
				{
					if(buff[4] > 23) global.set.hour = 23;	//判断是否超出范围
					else global.set.hour = buff[4];	//rtc时间-小时
					if(buff[5] > 59) global.set.minute = 59;
					else global.set.minute = buff[5];	//rtc时间-分钟
					if(buff[6] > 23) global.set.startLightHour = 23;
					else global.set.startLightHour = buff[6];	//预约开始照明的时间-小时
					if(buff[7] > 59) global.set.startLightMinute = 59;
					else global.set.startLightMinute = buff[7];	//预约开始照明的时间-分钟
					if(buff[8] > 23) global.set.keepLightHour = 23;
					else global.set.keepLightHour = buff[8];	//预约持续照明的时间-小时
					if(buff[9] > 59) global.set.keepLightMinute = 59;
					else global.set.keepLightMinute = buff[9];	//预约持续照明的时间-分钟
					if(buff[10] > 23) global.set.startWaterHour = 23;
					else global.set.startWaterHour = buff[10];	//预约开始浇水的时间-小时
					if(buff[11] > 59) global.set.startWaterMinute = 59;
					else global.set.startWaterMinute = buff[11];	//预约开始浇水的时间-分钟
					if(buff[12] > 23) global.set.keepWaterHour = 23;
					else global.set.keepWaterHour = buff[12];	//预约持续浇水的时间-小时
					if(buff[13] > 59) global.set.keepWaterMinute = 59;
					else global.set.keepWaterMinute = buff[13];	//预约持续浇水的时间-分钟
					if(buff[14] > 30) global.set.waterCycleDay = 30;
					else global.set.waterCycleDay = buff[14];	//预约浇水的周期-天
					//必须在获取完参数后才可以切换状态
					if(global.set.setDataSate == waitAck)	//如果设置状态处于待应答
					{
						global.set.setDataSate = ing;	//将状态切换到设置中
					}
				}break;
				case 0x0007:	//设置参数完成的响应
				{
					if(global.set.setDataSate == waitAck)	//如果设置状态处于待应答
					{
						global.set.setDataSate = ack;	//将状态切换到已应答
					}
				}break;
			}
		}
	}
}

/////////////////////////////////////////////////单总线通信////////////////////////////////////////////////////////

#define ONE_WIRE_A P06
#define ONE_WIRE_A_MODE P06_Quasi_Mode
#define ONE_WIRE_A_H set_P06
#define ONE_WIRE_A_L clr_P06
#define ONE_WIRE_B P07
#define ONE_WIRE_B_MODE P07_Quasi_Mode
#define ONE_WIRE_B_H set_P07
#define ONE_WIRE_B_L clr_P07

#define ONE_WIRE_READ ONE_WIRE_A
#define ONE_WIRE_WRITE(b) if(b) ONE_WIRE_B_H; else ONE_WIRE_B_L

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

///////////////////////////////////////////////串口0接收解析//////////////////////////////////////////////////////

// //
// //和校验计算
// //unsigned char len : 需要计算的长度
// //
// unsigned char figureSum(unsigned char* buff,unsigned char len)
// {
// 	unsigned char sum = 0;

// 	sum += *buff++;
// 	while(--len)
// 	{
// 		sum += *buff++;
// 	}
// 	return sum;
// }

// void uart0_fun(unsigned char* buff)
// {
// 	switch(buff[3])	//下标2和3为消息类型，但目前2为0x00所以判断3即可
// 	{
// 		case 0x02:	//设备状态获取的响应
// 		{
// 			//wifi指示灯的状态
// 			global.device.wifi = buff[4];	//WiFi指示灯状态
// 			global.device.appointment = buff[5];	//预约指示灯的状态
// 			global.device.light = buff[6];	//照明指示灯的状态
// 			global.device.water = buff[7];	//浇水指示灯的状态
// 			global.device.nowater = buff[8];	//缺水指示灯的状态
// 			global.device.nextStartLightCountDown = buff[9]<<8 | buff[10];	//距离下一次开始照明的倒计时，单位分钟
// 			global.device.endLightCountDown = buff[11]<<8 | buff[12];	//距离结束照明的倒计时，单位分钟
// 			global.device.nextStartWaterCountDown = buff[13]<<8 | buff[14];	//距离下一次开始浇水的倒计时，单位分钟
// 			global.device.endWaterCountDown = buff[15]<<8 | buff[16];	//距离结束浇水的倒计时，单位分钟

// 			global.set.setSateSate = end;
// 		}break;
// 		case 0x04:	//设置参数获取的响应
// 		{
// 			global.set.hour = buff[4];	//rtc时间-小时
// 			global.set.minute = buff[5];	//rtc时间-分钟
// 			global.set.startLightHour = buff[6];	//预约开始照明的时间-小时
// 			global.set.startLightMinute = buff[7];	//预约开始照明的时间-分钟
// 			global.set.keepLightHour = buff[8];	//预约持续照明的时间-小时
// 			global.set.keepLightMinute = buff[9];	//预约持续照明的时间-分钟
// 			global.set.startWaterHour = buff[10];	//预约开始浇水的时间-小时
// 			global.set.startWaterMinute = buff[11];	//预约开始浇水的时间-分钟
// 			global.set.keepWaterHour = buff[12];	//预约持续浇水的时间-小时
// 			global.set.keepWaterMinute = buff[13];	//预约持续浇水的时间-分钟
// 			global.set.waterCycleDay = buff[14];	//预约浇水的周期-天
// 			//必须在获取完参数后才可以切换状态
// 			if(global.set.setDataSate == waitAck)	//如果设置状态处于待应答
// 			{
// 				global.set.setDataSate = ing;	//将状态切换到设置中
// 			}
// 		}break;
// 		case 0x07:	//设置参数完成的响应
// 		{
// 			if(global.set.setDataSate == waitAck)	//如果设置状态处于待应答
// 			{
// 				global.set.setDataSate = ack;	//将状态切换到已应答
// 			}
// 		}break;
// 	}
// }

// void uart0_isr(unsigned char dat)
// {
// 	unsigned char i,sum = 0;
// 	//判断接收的数据是否符合通信格式
// 	if(global.uart.nowIndex == 0 && dat == 0x96)	//头
// 	{
// 		global.uart.recTimeOutCount = 1;	//开启超时计时，为1则计时，如果超时，则放弃当前数据包，收到数据包则刷新
// 		global.uart.recBuff[global.uart.nowIndex] = dat;
// 		++global.uart.nowIndex;
// 	}
// 	else if(global.uart.nowIndex == 1)	//数据长度
// 	{
// 		global.uart.recTimeOutCount = 1;	//重置超时计时，如果为0则不计时，如果超时，则放弃当前数据包，收到数据包则刷新
// 		global.uart.recBuff[global.uart.nowIndex] = dat;	//长度：从头到校验
// 		++global.uart.nowIndex;
// 	}
// 	else if(global.uart.nowIndex >= 2 && global.uart.nowIndex <= global.uart.recBuff[1]-2)	//减去了校验字节，因为下标比实际长度小1，所以-2
// 	{
// 		global.uart.recTimeOutCount = 1;	//重置超时计时，如果为0则不计时，如果超时，则放弃当前数据包，收到数据包则刷新
// 		global.uart.recBuff[global.uart.nowIndex] = dat;	//数据
// 		++global.uart.nowIndex;
// 	}
// 	else if(global.uart.nowIndex == global.uart.recBuff[1]-1)	//校验
// 	{
// 		//之后更改模式，将耗时操作都移走

// 		global.uart.recTimeOutCount = 0;	//关闭超时计时，为0则不计时
// 		global.uart.sendTimeOutCount = 0;
// 		for(i = 0;i <= global.uart.recBuff[1] - 2;i++)	//计算校验和
// 		{
// 			sum += global.uart.recBuff[i];
// 		}
// 		if(sum == dat)	//校验通过
// 		{
// 			global.uart.recBuff[global.uart.nowIndex] = dat;	//校验
// 			uart0_fun(global.uart.recBuff);	//功能判断
// 		}
// 		global.uart.nowIndex = 0;
// 	}
// }

// void delay(unsigned int a)
// {
// 	unsigned int b = 0;
// 	for(;a > 0;a--)
// 	{
// 		for(;b < 100;b++);
// 	}
// }

// void uart0_sendData(unsigned char* buff,unsigned char len)
// {
// 	global.uart.sendTimeOutCount = 1;
	
// 	while(len--)
// 	{
// 		SBUF = *buff++;
//     	while(TI==0);
// 		delay(20);
// 	}
// }
