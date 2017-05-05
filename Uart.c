#include <reg52.h>
#include "Uart.h"
#include "PWM.h"

void Uart_Init()
{
	SCON=0X50;			//设置为工作方式1
	TMOD|=0X20;			//设置计数器工作方式2
	PCON=0X00;			//波特率不加倍
	TH1=0XFA;		    //计数器初始值设置，注意波特率是4800的
	TL1=0XFA;
	ES=1;						//打开接收中断
	EA=1;						//打开总中断
	TR1=1;					    //打开计数器
}

void Usart() interrupt 4
{
	unsigned char receiveData;
    unsigned int plus_num;

	receiveData=SBUF; //出去接收到的数据
	plus_num=36000/receiveData;	  //步进电机驱动器400p/r（减速比2）,平台减速比90，所以：36000p/r
							  //假设电机速度200~300r/min=5r/s，此时f=5*400=2000Hz，平台转速20°/s
	Set_PWM_Num(2000,plus_num);
	RI = 0;           //清除接收中断标志位
	SBUF=receiveData; //将接收到的数据放入到发送寄存器
	while(!TI);		  //等待发送数据完成
	TI=0;			  //清除发送完成标志位
}