#include "global.h"
#include "func.h"

void feedDogInIsr(unsigned int ms);

void initTimer3(void)
{
	T3CON = 0x07;	//128分频
	
	RH3 = (65536-125)/256;	//1ms
	RL3 = (65536-125)%256;   
 
	set_ET3;	//enable Timer3 interrupt
	set_EA;	//enable interrupts
	set_TR3;	//Timer3 run
}

void Timer3_ISR (void) interrupt 16
{
	clr_TF3;
	run();

	feedDogInIsr(2000);	//喂狗
}

void usartInit(void)
{
	set_EA;	//使能总中断
	set_ES;	//使能串口0中断

	P06_Quasi_Mode;		//Setting UART pin as Quasi mode for transmit
	P07_Quasi_Mode;		//Setting UART pin as Quasi mode for transmit
	
	SCON = 0x50;     	//UART0 Mode1,REN=1,TI=1
	TMOD |= 0x20;    	//Timer1 Mode1
	
	set_SMOD;        	//UART0 Double Rate Enable
	set_T1M;
	clr_BRCK;        	//Serial port 0 baud rate clock source = Timer1

	TH1 = 0xe6;	//38400
	set_TR1;
}

void Uart0_ISR() interrupt 4
{
    if(TI)
	{
		TI = 0;
		global.uart.sendIsrFlag = true;	//发送数据前会被赋值为 false
	}

    if(RI)//串口0接收中断标志（有数据时，硬件置1）
    {
		RI = 0;//软件置0
		//SBUF = SBUF;
		uart0_isr(SBUF);
    }
}

void wtd_init(void)
{
    TA=0xAA;
    TA=0x55;

    WDCON = 0x7; //根据手册 [2:0]位表示中断在多少秒后执行，0x7表示1.638s后中断执行。根据需要修改低3位
    set_WDTR;
    set_WDCLR;
    set_EWDT; 
	set_EA;//总中断记得开启，set_EA;   
}

#define feedDog() set_WDCLR;	//喂狗
static unsigned int feedDogTimeCount = 0;	//喂狗标志

void feedDogInMain(void)
{
	feedDogTimeCount = 0;
}

void feedDogInIsr(unsigned int ms)
{
	//再主循环中清除喂狗标志，在中断中喂狗，如果主循环中两秒没有清除标志则认为主循环死了，那就停止喂狗
	//如果中断死了,也会停止喂狗
	if(++feedDogTimeCount < ms)
	{
		feedDogTimeCount = 0;
		feedDog();
	}
}

void main(void)
{
	#ifdef ONLINE
		usartInit();
	#endif
	init();
	initTimer3();
	wtd_init();
	feedDogInMain();	//喂狗

	while(1)
	{
		feedDogInMain();	//喂狗
		runMain();
	}
}
