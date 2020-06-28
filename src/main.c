/****************************************Copyright (c)**************************************************
**                               Guangzou ZLG-MCU Development Co.,LTD.
**                                      graduate school
**                                 http://www.zlgmcu.com
**
**--------------File Info-------------------------------------------------------------------------------
** File name:			main.c
** Last modified Date:  2004-09-16
** Last Version:		1.0
** Descriptions:		The main() function example template
**
**------------------------------------------------------------------------------------------------------
** Created by:			Chenmingji
** Created date:		2004-09-16
** Version:				1.0
** Descriptions:		The original version
**
**------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
********************************************************************************************************/
#include "config.h"

volatile uint8 state[3]={0,0,0};  //3个数码管的状态
volatile uint8 result[3]; //计算出来的状态
volatile uint8 fate;      //根据概率表决定的结果（LOSE,SINGLE,NORMAL,BIG）

const char display1[]={0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9};//使只有第一个数码管亮的0-9
const char display2[]={0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9};
const char display3[]={0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9};
const char display4[]={0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79};

//数字显示模式
const uint8 number1[]={0x07,0x04,0x03,0x08,0x01,0x06,0x05,0x02,0x09};
const uint8 number2[]={0x07,0x01,0x08,0x03,0x09,0x05,0x06,0x02,0x04};
const uint8 number3[]={0x07,0x09,0x02,0x03,0x06,0x05,0x01,0x04,0x08};

int flag[3]={0,0,0};
//三个数码管的指示灯
char LEDTAB=0x38;

#define Start (1<<21)
#define AddCoin (1<<22)
#define AddWager (1<<23)

int coin=0,wager=1;



void SPI_Init(void)
{  
	//-----------引脚初始化------------
	PINSEL0 |= 0x100;	//p0.4	SCK
	PINSEL0 &= ~0xC000;	//p0.7	RCK(GPIO)
	PINSEL0 |= 0x1000;	//p0.6	MOSI
	IO0DIR |= 1<<7;
	
	//----------SPI寄存器初始化-----------  
	S0PCCR = 0x08;		// 设置SPI时钟分频
	S0PCR = 0x30;		// 设置SPI接口模式，MSTR=1，主模式
	//CPOL=0，CPHA=0，LSBF=1，LSB在前
}

void SPI_Write(uint8 ch)
{
	S0PDR = ch;						//写数据
	while(0 == (S0PSR & (1 << 7)));	//等待发送完毕
}

void MessageDisplay(char LED)
{
	int j;
	IO0CLR |= (1 << 7);
	SPI_Write(LED);
	SPI_Write(display1[(coin/100)]);
	IO0SET |= (1 << 7);
	for(j=0;j<10000;j++);
	IO0CLR |= (1 << 7);
	SPI_Write(LED);
	SPI_Write(display2[((coin%100)/10)]);
	IO0SET |= (1 << 7);
	for(j=0;j<10000;j++);
	IO0CLR |= (1 << 7);
	SPI_Write(LED);
	SPI_Write(display3[(coin%10)]);
	IO0SET |= (1 << 7);
	for(j=0;j<10000;j++);
	IO0CLR |= (1 << 7);
	SPI_Write(LED);
	SPI_Write(display4[(wager%10)]);
	IO0SET |= (1 << 7);
	for(j=0;j<10000;j++);
}

void GameInit()
{
	PINSEL2=0;
	IO1DIR|=0xFFF0000;
	IO1CLR|=0xFFF0000;
}

void GameStart()
{
	IO0CLR |= (1 << 7);
	SPI_Write(0x38);
	SPI_Write(0xF0);
	IO0SET |= (1 << 7);
	while((IO0PIN&Start)==0);
}

void Add_Coin()
{
	coin=coin+1;
	while((IO0PIN&AddCoin)==0)
			MessageDisplay(0x00);
}

void Add_Wager()
{
	int j;
	while((IO0PIN&AddWager)==0)
	{
	IO0CLR |= (1 << 7);
	SPI_Write(0x00);
	SPI_Write(display4[wager%10]);
	IO0SET |= (1 << 7);
	for(j=0;j<1000000;j++);
	wager++;
	if(wager==4)
		wager=1;
	}
}

void Refresh()
{
	int j;
	IO1CLR|=0xFFF0000;
	IO1SET|=(number3[state[2]]<<24);
	IO1SET|=(number2[state[1]]<<20);
	IO1SET|=(number1[state[0]]<<16);
	for(j=0;j<1000000;j++);
	if(flag[0]==0)
		state[0]=(state[0]+1)%9;
	if(flag[1]==0)
		state[1]=(state[1]+1)%9;
	if(flag[2]==0)
		state[2]=(state[2]+1)%9;
}

/*void Show_Time(int num)
{
    int d = result[num] - state[num];
    uint8 trun_tick[4] = {1, 5, 14, 30};
    d = (d + 9) % 9 + 18; //滚动至目标前第四个数
    for (int i = 0; i != 4; i++)
        trun_tick[i] += d;
    d += 30; //非线性停止
    for (int i = 0, j = 0; i != d; d++)
    {
        if (i == trun_tick[j])
            j++;
        else                    //未到转变点，预先减掉Reflesh()即将加的
            state[num]--;
        if ((T0IR & 1) != 0) //50ms
            Refresh();
    }
    //标记该数码管已停
    flag[num]=1;
    （Refresh()使用）,退出
}*/

//关闭所有中断
//关闭本身按停按钮的指示灯
//调用Show_Time()
void __irq EINT0_ISR()
{
	flag[0]=1;
	LEDTAB=LEDTAB&0xF0;
	IO0CLR |= (1 << 7);
	SPI_Write(LEDTAB);
	SPI_Write(0xF0);
	IO0SET |= (1 << 7);
	//Show_Time(0);
	EXTINT =0x0F;
	VICVectAddr =0;
}

void __irq EINT1_ISR()
{
	flag[1]=1;
	LEDTAB=LEDTAB&0xEF;
	IO0CLR |= (1 << 7);
	SPI_Write(LEDTAB);
	SPI_Write(0xF0);
	IO0SET |= (1 << 7);
	//Show_Time(1);
	EXTINT =0x0F;
	VICVectAddr =0;
}

/*void __irq EINT2_ISR()
{
	flag[2]=1;
	LEDTAB=LEDTAB&0xDF;
	IO0CLR |= (1 << 7);
	SPI_Write(LEDTAB);
	SPI_Write(0xF0);
	IO0SET |= (1 << 7);
	//Show_Time(2);
	EXTINT =0x0F;
	VICVectAddr =0;
}*/
//中断我怎么加都只有两个能用，就是在p0.7后的一用SPI就用不了，你看看哪里有问题
void IrqInit()
{
	//这是p0.1和p0.3作为外部中断
	PINSEL0|=0xCC;
	//PINSEL0|=0xA0000000;
	//PINSEL1|=0x
	
	EXTMODE =EXTMODE&0xFC;
	EXTPOLAR=EXTPOLAR&0xFC;
	
	VICIntSelect=0x00;
	VICVectAddr7=(uint32)EINT0_ISR;
	VICVectCntl7=0x20|14;
	VICVectAddr8=(uint32)EINT1_ISR;
	VICVectCntl8=0x20|15;
	/*VICVectAddr9=(uint32)EINT2_ISR;
	VICVectCntl9=0x20|16;*/
	VICIntEnable=(1<<14)|(1<<15);
	//VICIntEnable=(1<<14)|(1<<15)|(1<<16);
}

int main (void)
{// add user source code 
	SPI_Init();
	IrqInit();
	GameInit();
	while(1)
	{
		/*********************** STANDBY MODE **********************/
		while(1)
		{
			//游戏币足够才打开开始按键的指示灯
		 	if(coin>0)
				MessageDisplay(0x07);
			else
				MessageDisplay(0x06);
			if((IO0PIN&Start)==0)
			{
			//币不够无法开始
				if(coin<=0)
				{
					while((IO0PIN&Start)==0);
					continue;
				}
				GameStart();
				break;
			}
			if((IO0PIN&AddCoin)==0)
				Add_Coin();
			if((IO0PIN&AddWager)==0)
				Add_Wager();
		}
		/************************ WORK MODE ************************/	
		while(1)
		{
			Refresh();
		}
	}
    return 0;
}
/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/
