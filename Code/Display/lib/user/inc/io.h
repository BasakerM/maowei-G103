#ifndef _io_h
#define _io_h

#include "global.h"

///////////////////////////////////////////////LED//////////////////////////////////////////////////////

#define LED_DISEN0 P04_PushPull_Mode;set_P04
#define LED_DISEN1 P03_PushPull_Mode;set_P03
#define LED_DISEN2 P01_PushPull_Mode;set_P01
#define LED_DISEN3 P05_PushPull_Mode;set_P05
#define LED_ALL_DISEN LED_DISEN0;LED_DISEN1;LED_DISEN2;LED_DISEN3;

#define LED_EN0 LED_DISEN1;LED_DISEN2;LED_DISEN3;P04_PushPull_Mode;clr_P04
#define LED_EN1 LED_DISEN0;LED_DISEN2;LED_DISEN3;P03_PushPull_Mode;clr_P03
#define LED_EN2 LED_DISEN0;LED_DISEN1;LED_DISEN3;P01_PushPull_Mode;clr_P01
#define LED_EN3 LED_DISEN0;LED_DISEN1;LED_DISEN2;P05_PushPull_Mode;clr_P05
#define LED_ALL_EN P04_PushPull_Mode;clr_P04;P03_PushPull_Mode;clr_P03;P01_PushPull_Mode;clr_P01;P05_PushPull_Mode;clr_P05;

#define LED_A_ON P11_PushPull_Mode;set_P11
#define LED_B_ON P00_PushPull_Mode;set_P00
#define LED_C_ON P30_PushPull_Mode;set_P30
#define LED_D_ON P13_PushPull_Mode;set_P13
#define LED_E_ON P12_PushPull_Mode;set_P12
#define LED_F_ON P10_PushPull_Mode;set_P10
#define LED_G_ON P15_PushPull_Mode;set_P15
#define LED_H_ON P02_PushPull_Mode;set_P02
#define LED_ALL_ON LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_ON;LED_H_ON

#define LED_A_OFF P11_PushPull_Mode;clr_P11
#define LED_B_OFF P00_PushPull_Mode;clr_P00
#define LED_C_OFF P30_PushPull_Mode;clr_P30
#define LED_D_OFF P13_PushPull_Mode;clr_P13
#define LED_E_OFF P12_PushPull_Mode;clr_P12
#define LED_F_OFF P10_PushPull_Mode;clr_P10
#define LED_G_OFF P15_PushPull_Mode;clr_P15
#define LED_H_OFF P02_PushPull_Mode;clr_P02
#define LED_ALL_OFF LED_A_OFF;LED_B_OFF;LED_C_OFF;LED_D_OFF;LED_E_OFF;LED_F_OFF;LED_G_OFF;LED_H_OFF

//0-9
#define LED_SHOW_NUM0 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_OFF;LED_H_OFF;
#define LED_SHOW_NUM1 LED_A_OFF;LED_B_ON;LED_C_ON;LED_D_OFF;LED_E_OFF;LED_F_OFF;LED_G_OFF;LED_H_OFF;
#define LED_SHOW_NUM2 LED_A_ON;LED_B_ON;LED_C_OFF;LED_D_ON;LED_E_ON;LED_F_OFF;LED_G_ON;LED_H_OFF;
#define LED_SHOW_NUM3 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_OFF;LED_F_OFF;LED_G_ON;LED_H_OFF;
#define LED_SHOW_NUM4 LED_A_OFF;LED_B_ON;LED_C_ON;LED_D_OFF;LED_E_OFF;LED_F_ON;LED_G_ON;LED_H_OFF;
#define LED_SHOW_NUM5 LED_A_ON;LED_B_OFF;LED_C_ON;LED_D_ON;LED_E_OFF;LED_F_ON;LED_G_ON;LED_H_OFF;
#define LED_SHOW_NUM6 LED_A_ON;LED_B_OFF;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_ON;LED_H_OFF;
#define LED_SHOW_NUM7 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_OFF;LED_E_OFF;LED_F_OFF;LED_G_OFF;LED_H_OFF;
#define LED_SHOW_NUM8 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_ON;LED_H_OFF;
#define LED_SHOW_NUM9 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_OFF;LED_F_ON;LED_G_ON;LED_H_OFF;
//0.-9.
#define LED_SHOW_NUM10 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_OFF;LED_H_ON;
#define LED_SHOW_NUM11 LED_A_OFF;LED_B_ON;LED_C_ON;LED_D_OFF;LED_E_OFF;LED_F_OFF;LED_G_OFF;LED_H_ON;
#define LED_SHOW_NUM12 LED_A_ON;LED_B_ON;LED_C_OFF;LED_D_ON;LED_E_ON;LED_F_OFF;LED_G_ON;LED_H_ON;
#define LED_SHOW_NUM13 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_OFF;LED_F_OFF;LED_G_ON;LED_H_ON;
#define LED_SHOW_NUM14 LED_A_OFF;LED_B_ON;LED_C_ON;LED_D_OFF;LED_E_OFF;LED_F_ON;LED_G_ON;LED_H_ON;
#define LED_SHOW_NUM15 LED_A_ON;LED_B_OFF;LED_C_ON;LED_D_ON;LED_E_OFF;LED_F_ON;LED_G_ON;LED_H_ON;
#define LED_SHOW_NUM16 LED_A_ON;LED_B_OFF;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_ON;LED_H_ON;
#define LED_SHOW_NUM17 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_OFF;LED_E_OFF;LED_F_OFF;LED_G_OFF;LED_H_ON;
#define LED_SHOW_NUM18 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_ON;LED_F_ON;LED_G_ON;LED_H_ON;
#define LED_SHOW_NUM19 LED_A_ON;LED_B_ON;LED_C_ON;LED_D_ON;LED_E_OFF;LED_F_ON;LED_G_ON;LED_H_ON;
//符号
#define LED_SHOW_NUM30 LED_A_OFF;LED_B_OFF;LED_C_OFF;LED_D_OFF;LED_E_OFF;LED_F_OFF;LED_G_ON;LED_H_OFF;  //-

void ledEnable(unsigned char ch);
void dispalyNumber(unsigned char num);
void setLed(unsigned char ch);
void resetLed(unsigned char ch);

///////////////////////////////////////////////按键//////////////////////////////////////////////////////

#define KEY_EN P11_Input_Mode;P00_Input_Mode
#define KEY_DISEN P11_PushPull_Mode;P00_PushPull_Mode
#define KEY_PRESS1 (P11)   //按键被按下时为高电平
#define KEY_PRESS2 (P00)
#define KEY_PRESS (P00 | P11)
#define KEY_NOPRESS !(P00 | P11)

enum enum_keyVlaue {noneKey, shortKey1, shortlongKey1, longKey1, shortKey2, shortlongKey2, longKey2, longDoubleKey};

enum enum_keyVlaue keyScanf(unsigned char timeBase);
// enum enum_keyVlaue keyScanf(void);

typedef struct
{
    enum enum_keyVlaue vlaue;
    unsigned char pressTime_100ms;
    unsigned int timeCount;

    enum enum_keyVlaue modeLock;    //模式锁，锁定键值的类型，锁定后将不能切换至其他类型的键值，键值为none时解锁---该功能还不完善
}Struct_Key;

///////////////////////////////////////////////蜂鸣器//////////////////////////////////////////////////////

#define BEEP_ON P14_PushPull_Mode;set_P14
#define BEEP_OFF P14_PushPull_Mode;clr_P14

void Beep(void);
void keyBeep(unsigned char* bool);

///////////////////////////////////////////////结构体数据//////////////////////////////////////////////////////

typedef struct
{
    Struct_Key key;
}Struct_IOData;
extern Struct_IOData ioData;

#endif

///////////////////////////////////////////////ADC//////////////////////////////////////////////////////

void initAdc(void);
unsigned int getAdc(void);
