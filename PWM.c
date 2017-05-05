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
     TMOD|= 0x01;   //���ö�ʱ������0������ʽ2Ϊ��ʱ�� 

	 
	TH0 = c_TH; 
	TL0 = c_TL;
	PT0=1;        //����Ϊ�����ȼ�
	ET0= 1; 	 //������ʱ��1�ж�
	EA = 1;
//	TR1 = 1;	 //������ʱ��

    PWM=0;
}

void Set_PWM_Num(u16 f_PWM,u16 step)
{
    PWM_Num=step;
   	c_TH = (65536-460800/f_PWM)>>8;	 //12MHz����ÿ2��Ϊһ���ڣ�11.0592M����ӦΪ460800
    c_TL = ((65536-460800/f_PWM)<<8)>>8;
	TR0=1;
}

void Time1(void) interrupt 1    //3 Ϊ��ʱ��1���жϺ�  1 ��ʱ��0���жϺ� 0 �ⲿ�ж�1 2 �ⲿ�ж�2  4 �����ж�
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
