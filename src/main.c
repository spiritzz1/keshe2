/*
 * File: main.c
 * Project: SLots
 * Created Date: Wednesday June 24th 2020
 * Author: hong
 * -----
 * Last Modified: Sunday, June 28th 2020, 10:43:06 pm
 * Modified By: wei
 * -----
 * Copyright (c) 2020 hong
 */
#include "config.h"

#define Start (1 << 21)
#define AddCoin (1 << 22)
#define AddWager (1 << 23)

#define LOSE 3
#define SINGLE 2
#define NORMAL 1
#define BIG 0

#define n 9

volatile uint8 state[3];  //3个数码管的状态
volatile uint8 result[3]; //计算出来的状态
volatile uint8 fate;      //根据概率表决定的结果（LOSE,SINGLE,NORMAL,BIG）
volatile uint8 stop_flag;
uint8 led_state;
uint8 level;

volatile int coin;
volatile int wager;

long rand_seed;

const uint8 map[9] = {1, 2, 4, 5, 6, 8, 9, 3, 7};
const uint16 fate_tab[5][3] = {
    {19, 91, 1567},
    {18, 86, 1490},
    {17, 82, 1411},
    {16, 77, 1326},
    {5, 28, 496}};

void Set_Rand_seed(unsigned int seed)
{
    rand_seed = (long)seed;
}

int Get_Rand(void)
{
    return (((rand_seed = rand_seed * 214013L + 2531011L) >> 16) & 0x7fff);
}

void Change_Level()
{
    level = (level + 1) % 5;
}

void Fortune_Telling()
{
    int ran = Get_Rand() % 10000;
    if (ran > fate_tab[level][2])
    {
        fate = LOSE;
    }
    else if (ran > fate_tab[level][1])
    {
        fate = SINGLE;
    }
    else if (ran > fate_tab[level][0])
    {
        fate = NORMAL;
    }
    else
    {
        fate = BIG;
    }
}

void Create_State()
{
    switch (fate)
    {
    case LOSE:
        result[1] = Get_Rand() % n;
        result[0] = (result[1] + 1 + Get_Rand() % (n - 1)) % n; //0-1 1-2不同就行,加上随机数[1:n-1]保证不同
        result[2] = (result[1] + 1 + Get_Rand() % (n - 1)) % n;
        break;
    case SINGLE:
        if (Get_Rand() % 2)
        {
            result[0] = Get_Rand() % n;
            result[1] = result[0];
            result[2] = (result[0] + 1 + Get_Rand() % (n - 1)) % n; //加上随机数[1:n-1]保证不同
        }
        else
        {
            result[1] = Get_Rand() % n;
            result[2] = result[1];
            result[0] = (result[1] + 1 + Get_Rand() % (n - 1)) % n; //加上随机数[1:n-1]保证不同
        }
        break;
    case NORMAL:
        result[0] = map[Get_Rand() % (n - 2)];
        result[1] = result[0];
        result[2] = result[0];
        break;
    case BIG:
        result[0] = map[n - (Get_Rand() % 2)];
        result[1] = result[0];
        result[2] = result[0];
        break;
    }
}

void Timer1_Init()
{
    T1TCR = 0;
    T1PR = 0;
    T1TC = 0;
    T1TCR = 1;
}

void Timer0_Init()
{
    T0TCR = 0;
    T0PR = 0;
    T0PC = 0;
    T0TC = 0;
    T0MCR = 0x03;
    T0MR0 = Fosc / 20 - 1;
}

void Timer0_Start()
{
    T0TCR = 1;
}

void Timer0_End()
{
    T0TCR = 2;
}

void SPI_Init(void)
{
    //-----------引脚初始化------------
    PINSEL0 |= 0x100;   //p0.4	SCK
    PINSEL0 &= ~0xC000; //p0.7	RCK(GPIO)
    PINSEL0 |= 0x1000;  //p0.6	MOSI
    IO0DIR |= 1 << 7;

    //----------SPI寄存器初始化-----------
    S0PCCR = 0x08; // 设置SPI时钟分频
    S0PCR = 0x30;  // 设置SPI接口模式，MSTR=1，主模式
                   //CPOL=0，CPHA=0，LSBF=1，LSB在前
}

void SPI_Write(uint8 ch)
{
    S0PDR = ch; //写数据
    while (0 == (S0PSR & (1 << 7)))
        ; //等待发送完毕
}

void Update()
{
    uint8 data1;
    uint8 data2;
    uint8 data3;
    data1 = led_state;
    data2 = (wager << 4) + coin % 10;
    data3 = (coin / 10 % 10 << 4) + coin / 100 % 10;
    IO0CLR |= (1 << 7); //Start
    SPI_Write(data1);
    SPI_Write(data2);
    SPI_Write(data3);
    IO0SET |= (1 << 7); //Done
}

void GameInit()
{
    PINSEL2 = 0;
    IO1DIR |= 0xFFF0000;
    IO1CLR |= 0xFFF0000;
    led_state = 0x6;
    coin = 0;
    wager = 1;
    stop_flag = 0;
    state[0] = 7;
    state[1] = 7;
    state[2] = 7;
    level = 1;
}

void GameStart()
{
    led_state = 0x38;
    Update();
    Set_Rand_seed(T1TC & 0xFFF);
    Fortune_Telling();
    Create_State();
    while ((IO0PIN & Start) == 0)
        ;
}

void Add_Coin()
{
    coin = coin + 1;
    if (coin == 1)
        led_state = 0x7;
    Update();
    while ((IO0PIN & AddCoin) == 0)
        ;
}

void Add_Wager()
{
    wager++;
    if (wager == 4)
        wager = 1;
    Update();
    while ((IO0PIN & AddWager) == 0)
        ;
}

//能判断出哪些数码管不需要滚动（被按停）
//更新state（滚动数值），并刷新给3个数码管
void Refresh()
{
    if ((stop_flag & (1 << 0)) == 0)
        state[0] = (state[0] + 1) % 9 + 1;
    if ((stop_flag & (1 << 1)) == 0)
        state[1] = (state[1] + 1) % 9 + 1;
    if ((stop_flag & (1 << 2)) == 0)
        state[2] = (state[2] + 1) % 9 + 1;
    IO1CLR |= 0xFFF0000;
    IO1SET |= state[0] << 16;
    IO1SET |= state[1] << 20;
    IO1SET |= state[2] << 24;
}

void Show_Time(int num)
{
    int i;
    int j;
    int d = result[num] - state[num];
    uint8 trun_tick[4] = {1, 5, 14, 30};
    d = (d + 9) % 9 + 18 - 6; //滚动至目标前第四个数
    for (i = 0; i != 4; i++)
        trun_tick[i] += d;
    d += 30; //非线性停止
    for (i = 0, j = 0; i != d; i++)
    {
        if (i == trun_tick[j])
            j++;
        else //未到转变点，预先减掉Reflesh()即将加的
            state[num]--;
        while ((T0IR & 1) == 0) //50ms
            ;
        T0IR |= 1;
        Refresh();
    }
    //标记该数码管已停（Refresh()使用）,退出
    stop_flag |= 1 << num;
}

//关闭所有中断
//关闭本身按停按钮的指示灯
//调用Show_Time()
void __irq EINT0_ISR()
{
    Show_Time(0);
    stop_flag |= (1 << 0);
    if (stop_flag == 0x7)
        stop_flag |= 1 << 4;
    EXTINT = 0x0F;
    VICVectAddr = 0;
}

void __irq EINT1_ISR()
{
    Show_Time(1);
    stop_flag |= (1 << 1);
    if (stop_flag == 0x7)
        stop_flag |= 1 << 4;
    EXTINT = 0x0F;
    VICVectAddr = 0;
}

void __irq EINT2_ISR()
{
    Show_Time(2);
    stop_flag |= (1 << 2);
    if (stop_flag == 0x7)
        stop_flag |= 1 << 4;
    EXTINT = 0x0F;
    VICVectAddr = 0;
}

void EINT_Init()
{
    PINSEL0 |= 0xA0000000;
    PINSEL0 &= ~0x50000000;
    PINSEL1 |= 0x1;
    PINSEL1 &= ~0x2;

    EXTMODE |= 0x7;
    EXTPOLAR &= ~0x7;

    VICIntSelect = 0x00;
    VICVectAddr7 = (uint32)EINT0_ISR;
    VICVectCntl7 = 0x20 | 14;
    VICVectAddr8 = (uint32)EINT1_ISR;
    VICVectCntl8 = 0x20 | 15;
    VICVectAddr9 = (uint32)EINT2_ISR;
    VICVectCntl9 = 0x20 | 16;
    VICIntEnable = 7 << 14;
}

void Wind_Up()
{
    switch (fate)
    {
    case LOSE:
        coin -= 1;
        break;
    case SINGLE:
        coin += 3;
        break;
    case NORMAL:
        coin += 20;
        break;
    case BIG:
        coin += 100;
        break;
    }
    Update();
}

int main(void)
{
    SPI_Init();
    EINT_Init();
    GameInit();
    Timer0_Init();
    Timer1_Init();
    Update();
    while (1)
    {
        /*********************** STANDBY MODE **********************/
        while (1)
        {
            //游戏币足够才打开开始按键的指示灯
            if ((IO0PIN & Start) == 0)
            {
                //币不够无法开始
                if (coin <= 0)
                {
                    while ((IO0PIN & Start) == 0)
                        ;
                    continue;
                }
                GameStart();
                break;
            }
            if ((IO0PIN & AddCoin) == 0)
                Add_Coin();
            if ((IO0PIN & AddWager) == 0)
                Add_Wager();
        }
        /************************ WORK MODE ************************/
        Timer0_Start();
        while (1)
        {
            while ((T0IR & 1) == 0) //50ms
                ;
            T0IR |= 1;
            Refresh();
            if ((stop_flag & (1 << 4)) != 0)
            {
                stop_flag = 0;
                Wind_Up();
                break;
            }
        }
        Timer0_End();
    }
    return 0;
}
