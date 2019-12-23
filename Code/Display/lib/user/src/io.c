#include "io.h"

Struct_IOData ioData = {0};

///////////////////////////////////////////////LED//////////////////////////////////////////////////////

void ledEnable(unsigned char ch)
{
	LED_DISEN0; LED_DISEN1; LED_DISEN2; LED_DISEN3;
	
	switch(ch)
	{
		case 0: LED_ALL_OFF; LED_EN0; break;
		case 1: LED_ALL_OFF; LED_EN1; break;
		case 2: LED_ALL_OFF; LED_EN2; break;
		case 3: LED_ALL_OFF; LED_EN3; break;
		case 4: LED_ALL_EN; break;
		case 5: LED_ALL_DISEN; break;
	}
}

void dispalyNumber(unsigned char num)
{
	switch(num)
	{
		case 0: LED_SHOW_NUM0; break;
		case 1: LED_SHOW_NUM1; break;
		case 2: LED_SHOW_NUM2; break;
		case 3: LED_SHOW_NUM3; break;
		case 4: LED_SHOW_NUM4; break;
		case 5: LED_SHOW_NUM5; break;
		case 6: LED_SHOW_NUM6; break;
		case 7: LED_SHOW_NUM7; break;
		case 8: LED_SHOW_NUM8; break;
		case 9: LED_SHOW_NUM9; break;
		case 10: LED_SHOW_NUM10; break;
		case 11: LED_SHOW_NUM11; break;
		case 12: LED_SHOW_NUM12; break;
		case 13: LED_SHOW_NUM13; break;
		case 14: LED_SHOW_NUM14; break;
		case 15: LED_SHOW_NUM15; break;
		case 16: LED_SHOW_NUM16; break;
		case 17: LED_SHOW_NUM17; break;
		case 18: LED_SHOW_NUM18; break;
		case 19: LED_SHOW_NUM19; break;
		case 30: LED_SHOW_NUM30; break;
		case 98: LED_ALL_ON; break;
		case 99: LED_ALL_OFF; break;
		default: LED_ALL_ON; break;
	}
}

void setLed(unsigned char ch)
{
	switch(ch)
	{
		case 0: LED_A_ON; break;
		case 1: LED_B_ON; break;
		case 2: LED_C_ON; break;
		case 3: LED_D_ON; break;
		case 4: LED_E_ON; break;
	}
}

void resetLed(unsigned char ch)
{
	switch(ch)
	{
		case 0: LED_A_OFF; break;
		case 1: LED_B_OFF; break;
		case 2: LED_C_OFF; break;
		case 3: LED_D_OFF; break;
		case 4: LED_E_OFF; break;
	}
}

///////////////////////////////////////////////按键//////////////////////////////////////////////////////

//
//	unsigned char timeBase : 进入一次该函数的时间,单位ms
//
enum enum_keyVlaue keyScanf(unsigned char timeBase)
{
	static unsigned char pressFlag = 0,keyvalue = 0;
	static unsigned int keyTimeCount = 0;
	KEY_EN;
	
	ioData.key.vlaue = noneKey;	//清键值
	switch(pressFlag)
	{
		case 0:
		{
			if(KEY_PRESS)
			{
				++keyTimeCount;
				if((keyTimeCount*timeBase) >= 10)
				{
					keyTimeCount = 0;
					pressFlag = 1;
					//记录键值
					if(KEY_PRESS1) keyvalue = 1;
					else if(KEY_PRESS2) keyvalue = 2;
					else if(KEY_PRESS1 && KEY_PRESS2) keyvalue = 3;
				}
			}
			else keyTimeCount = 0;
		}break;
		case 1:
		{
			//判断是否属于长按
			++keyTimeCount;
			ioData.key.pressTime_100ms = keyTimeCount*timeBase/100;
			if(ioData.key.pressTime_100ms >= 10)	//1s
			{
				pressFlag = 2;
				keyTimeCount = 0;
				ioData.key.modeLock = noneKey;
				ioData.key.pressTime_100ms = 0;
				//根据当前按下的键返回键值
				if(KEY_PRESS1 && KEY_PRESS2) ioData.key.vlaue = noneKey;
				else if(KEY_PRESS2) ioData.key.vlaue = shortlongKey2;
				else if(KEY_PRESS1) ioData.key.vlaue = shortlongKey1;
			}

			//等待短按抬起
			if(KEY_NOPRESS)
			{
				pressFlag = 0;
				keyTimeCount = 0;
				ioData.key.pressTime_100ms = 0;
				ioData.key.modeLock = noneKey;
				//根据之前记录的键值返回
				if(keyvalue == 1) ioData.key.vlaue = shortKey1;
				else if(keyvalue == 2) ioData.key.vlaue = shortKey2;
				else if(keyvalue == 3) ioData.key.vlaue = noneKey;
			}
		}break;
		case 2:
		{	
			if(ioData.key.modeLock == noneKey)	//未被锁定
			{
				if(ioData.key.pressTime_100ms < 20)	//3s,因为还有上一步骤中的1s
				{
					++keyTimeCount;
					ioData.key.pressTime_100ms = keyTimeCount*timeBase/100;
				}
				else
				{
					pressFlag = 3;
					//根据当前按下的键返回键值
					if(KEY_PRESS1 && KEY_PRESS2) ioData.key.vlaue = longDoubleKey;
					else if(KEY_PRESS2) ioData.key.vlaue = longKey2;
					else if(KEY_PRESS1) ioData.key.vlaue = longKey1;
				}
			}
			else if(ioData.key.modeLock == shortlongKey1 || ioData.key.modeLock == shortlongKey1)
			{
				++ioData.key.timeCount;
				ioData.key.pressTime_100ms = ioData.key.timeCount*timeBase/100;
				if(ioData.key.pressTime_100ms >= 10) ioData.key.timeCount = 0;	//该句防止上溢，实际上几乎不可能出现，但为了防止程序中未及时清0

				if(KEY_PRESS2) ioData.key.vlaue = shortlongKey2;
				else if(KEY_PRESS1) ioData.key.vlaue = shortlongKey1;
			}
			
			//等待长按抬起
			if(KEY_NOPRESS)
			{
				pressFlag = 0;
				keyTimeCount = 0;
				ioData.key.timeCount = 0;
				ioData.key.pressTime_100ms = 0;
				ioData.key.modeLock = noneKey;
			}
		}break;
		case 3:
		{
			//等待长按抬起
			if(KEY_NOPRESS)
			{
				pressFlag = 0;
				keyTimeCount = 0;
				ioData.key.timeCount = 0;
				ioData.key.pressTime_100ms = 0;
				ioData.key.modeLock = noneKey;
			}
		}break;
	}
	KEY_DISEN;
	return noneKey;
}

///////////////////////////////////////////////蜂鸣器//////////////////////////////////////////////////////

//
//可以用于周期性的蜂鸣
//
void Beep(void)
{
	static unsigned int count = 0;

	if(++count < 100)	//0-99ms响
	{
		BEEP_ON;
	}
	else if(count < 800)	//100-799ms不响
	{
		BEEP_OFF;
	}
	else	//800ms时重置计数
	{
		count = 0;
	}
}

//
//可以用于按键音，在按键触发时置为标志位，蜂鸣器就开始工作，工作时间过后自动清标志位
//unsigned char* bool : 蜂鸣标志位
//
void keyBeep(unsigned char* bool)
{
	static unsigned int count = 0;

	if(*bool)
	{
		if(++count < 50)	//0-49ms响
		{
			BEEP_ON;
		}
		else
		{
			*bool = 0;
			BEEP_OFF;
			count = 0;
		}
	}
}

///////////////////////////////////////////////ADC//////////////////////////////////////////////////////
#define RT_TABLE			m_3950K_10K_Rt
#define RT_TABLE_SIZE		101

const unsigned int m_3950K_10K_Rt[RT_TABLE_SIZE] = 
{
	32116,
	30570,
	29105,
	27716,
	26399,
	25150,
	23519,
	22427,
	21391,
	20407,
	19452,
	18584,
	17741,
	16940,
	16178,
	15455,
	14766,
	14112,
	13490,
	12898,
	12335,
	11799,
	11288,
	10803,
	10340,
	9900,	//25��
	9472,
	9064,
	8676,
	8307,
	7955,
	7619,
	7300,
	6995,
	6705,
	6428,
	6163,
	5911,
	5671,
	5441,
	5222,
	5013,
	4897,
	4704,
	4521,
	4345,
	4177,
	4016,
	3863,
	3716,
	3588,
	3440,
	3311,
	3188,
	3096,
	3956,
	2848,
	2744,
	2644,
	2548,
	2457,
	2369,
	2284,
	2204,
	2126,
	2051,
	1980,
	1911,
	1845,
	1782,
	1721,
	1663,
	1606,
	1552,
	1500,
	1450,
	1402,
	1356,
	1312,
	1269,
	1228,
	1188,
	1150,
	1113,
	1078,
	1044,	
	1011,
	979,
	948,
	919,
	891,
	863,
	837,
	811,
	787,
	763,
	740,
	718,
	697,
	676,
	657
};

/*
功能：转换获取温度值
参数：
	ain：采集ADC值，12位ADC
返回：温度值，单位0.1℃
*/
uint16_t convert_temp(uint16_t ain)
{
	uint32_t Rt;
	uint32_t Rtdt;
	uint32_t temp;
	uint8_t i;
	
	temp = (uint32_t)ain;
	if (temp < 50)
	{
		// 如果端口短路，直接返回0度
		return 0;
	}
	Rt = temp * 10000L / ( 4096L - temp );			// 计算热敏电阻电阻值

	if ( Rt > RT_TABLE[0] )							// 温度低于0°
	{
		return 0;
	}
	
	if ( Rt <= RT_TABLE[RT_TABLE_SIZE-1] )			// 温度大于等于100度
	{
		return 999;
	}
	
	for(i=0; i<(RT_TABLE_SIZE-1); i++)
	{
		// 判断在哪个两个温度值范围内
		if ( ( Rt > RT_TABLE[i+1] ) && ( Rt <= RT_TABLE[i] ) )
		{
			// 计算1度内的插值
			temp = RT_TABLE[i] - RT_TABLE[i+1];
			Rtdt = RT_TABLE[i] - Rt;
			temp = Rtdt * 10 / temp;
			temp += i * 10;
			break;
		}
	}
	
	return (unsigned int)temp;
}
void initAdc(void)
{
	P17_Input_Mode;
	set_P17DIDS;	//关闭ADC信道数字输入功能
	clr_ADCHS0;clr_ADCHS1;clr_ADCHS2;clr_ADCHS3;	//通道选择0000-即通道AIN0
	clr_ADCEX;	//软件触发转换
	set_ADCEN;	//打开ADC转换电路
}

unsigned int getAdc(void)
{
	unsigned int adcValue = 0;
	clr_ADCF;	//清转换结束标志
	set_ADCS;	//开始转换
	while(!ADCF);	//等待转换完成
	adcValue = (ADCRH<<4) | (ADCRL&0x0f);
	return convert_temp(adcValue);
}
