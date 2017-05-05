#include <reg52.h>
#include "Uart.h"
#include "PWM.h"

void Uart_Init()
{
	SCON=0X50;			//����Ϊ������ʽ1
	TMOD|=0X20;			//���ü�����������ʽ2
	PCON=0X00;			//�����ʲ��ӱ�
	TH1=0XFA;		    //��������ʼֵ���ã�ע�Ⲩ������4800��
	TL1=0XFA;
	ES=1;						//�򿪽����ж�
	EA=1;						//�����ж�
	TR1=1;					    //�򿪼�����
}

void Usart() interrupt 4
{
	unsigned char receiveData;
    unsigned int plus_num;

	receiveData=SBUF; //��ȥ���յ�������
	plus_num=36000/receiveData;	  //�������������400p/r�����ٱ�2��,ƽ̨���ٱ�90�����ԣ�36000p/r
							  //�������ٶ�200~300r/min=5r/s����ʱf=5*400=2000Hz��ƽ̨ת��20��/s
	Set_PWM_Num(2000,plus_num);
	RI = 0;           //��������жϱ�־λ
	SBUF=receiveData; //�����յ������ݷ��뵽���ͼĴ���
	while(!TI);		  //�ȴ������������
	TI=0;			  //���������ɱ�־λ
}