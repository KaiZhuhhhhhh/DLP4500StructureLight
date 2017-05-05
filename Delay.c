#include "Delay.h"

void Delay10ms(unsigned int c)   //误差 0us
{
    unsigned char a, b;

	//--c已经在传递过来的时候已经赋值了，所以在for语句第一句就不用赋值了--//
    for (;c>0;c--)
	{
		for (b=38;b>0;b--)
		{
			for (a=130;a>0;a--);
		}
           
	}
}
