#include "Delay.h"

void Delay10ms(unsigned int c)   //��� 0us
{
    unsigned char a, b;

	//--c�Ѿ��ڴ��ݹ�����ʱ���Ѿ���ֵ�ˣ�������for����һ��Ͳ��ø�ֵ��--//
    for (;c>0;c--)
	{
		for (b=38;b>0;b--)
		{
			for (a=130;a>0;a--);
		}
           
	}
}
