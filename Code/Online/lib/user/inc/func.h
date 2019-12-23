#ifndef _func_h
#define _func_h

#include "global.h"

#define LIGHT_OPEN P11_PushPull_Mode;set_P11
#define LIGHT_CLOSE P11_PushPull_Mode;clr_P11
#define WATER_OPEN P12_PushPull_Mode;set_P12
#define WATER_CLOSE P12_PushPull_Mode;clr_P12
#define NOWATER_INIT P15_Input_Mode
#define IS_NOWATER P15

void init(void);
void run(void);
void runMain(void);
// void uart9_isr(void);
void uart0_isr(unsigned char dat);

#endif