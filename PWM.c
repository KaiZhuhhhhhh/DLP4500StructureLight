#include <reg52.h>
#include "PWM.h"

sbit PWM=P2^0;
sbit Dir=P3^2;

u8  c_TH;
u8  c_TL;

u16 cnt_PWM;
u16 cnt_timer1;
u16 PWM_Num;

void PWM_Init()   //16Hz~1MHz
{
     TMOD|= 0x01;   //设置定时计数器0工作方式2为定时器 

	 
	TH0 = c_TH; 
	TL0 = c_TL;
	PT0=1;        //设置为高优先级
	ET0= 1; 	 //开启定时器1中断
	EA = 1;
//	TR1 = 1;	 //开启定时器

    PWM=0;
}

void Set_PWM_Num(u16 f_PWM,u16 step)
{
    PWM_Num=step;
   	c_TH = (65536-460800/f_PWM)>>8;	 //12MHz晶振，每2次为一周期，11.0592M晶振应为460800
    c_TL = ((65536-460800/f_PWM)<<8)>>8;
	TR0=1;
}

void Time1(void) interrupt 1    //3 为定时器1的中断号  1 定时器0的中断号 0 外部中断1 2 外部中断2  4 串口中断
{
	PWM=~PWM;
	TH0 = c_TH; 
	TL0 = c_TL;
	cnt_timer1++; 
	if((cnt_timer1>>1)>=PWM_Num)
	{
  	  PWM=0;
	  cnt_timer1=0;
	  TR0=0;
	}   
}
