
#include "main.h"


//获取编码器数据，在中断函数中刷新，正转时为正数，反转时为负数
volatile int motor_address = 0;
volatile int pendulum_address = 0;
volatile unsigned char turns_num = 0;
volatile unsigned int relative_address = 0;

//摆杆正反转标志,逆时针为1，顺时针为0
unsigned char foreward_flag = 0;


/*---------------------------------------
** 日期：2018.5.8
** 功能：使用基本定时器2的CH2(PA1),CH3(PA2)输出2路pwm驱动无差电机
** 说明：
1、频率：10KHZ,ARR=100,PSC=72，f=72M/ARR/PSC
2、占空比：后面调节的时候通过输出比较寄存器TIM2->CCRn直接赋值
3、直接输出pwm，无需定时器中断
4、pwm输出模式：
模式1：在向上计数时，一旦TIMx_CNT<TIMx_CCR1时通道为有效电平，否则为无效电平；
模式2：在向上计数时，一旦TIMx_CNT<TIMx_CCR1时通道为无效电平，否则为有效电平；
------------------------------------------*/
void Timer2_PWM_Init(void)
{
	GPIO_InitTypeDef  				 GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef        TIM_TimeBaseStructure;
	TIM_OCInitTypeDef              TIM_OCInitStructure;

   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//开启时钟，72M
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); 
	
	//输出引脚配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;			//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
		
	//定时器设置
	TIM_DeInit(TIM2);
   TIM_TimeBaseStructure.TIM_Period = 100 - 1;     //自动重装载值，设置ARR的值为19，从0计数到19，刚好是2ms，500hz
   TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;//分频系数，设置PSC的值为7199，这样的话每计数一次为0.1ms 
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx向上计数模式
   TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //根据TIM_TimeBaseStructure中指定的参数初始化外设TIM2
   
	//输出pwm设置
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 	//选择定时器模式,
   TIM_OCInitStructure.TIM_Pulse = 0; 					//设置待装入捕获比较寄存器的脉冲值，刚开始可以设置为0.
   TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
   TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	
	//chnnel 2
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable); 		//使能TIM2在CCR1上的预装载寄存器
	
	//chnnel 3	
	TIM_OC3Init(TIM2, &TIM_OCInitStructure); 
   TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable); 		//使能TIM2在CCR2上的预装载寄存器
	
	/*
	TIMx_CCRx寄存器能够在任何时候通过软件进行更新以控制输出波形，
	条件是未使用预装载寄存器(TIM_ARRPreloadConfig，否则TIMx_CCRx影子寄存器只能在发生下一次更新事件时被更新)。
	就是为了后面在中断服务子程序可以修改TIMx_CCR实时起作用
	*/
   TIM_ARRPreloadConfig(TIM2, ENABLE); //使能TIM2在ARR上的预装载寄存器
   TIM_Cmd(TIM2, ENABLE);  //使能TIM2外设
}

/*-----------------------------
** 功能：修改pwm占空比
** 说明：注意CCR的值要小于ARR的值
** 利用库函数也是同样的效果：
	TIM_SetCompare2(TIM2,10);
	TIM_SetCompare3(TIM2,70);
--------------------------------*/
void Set_PWM_Value(unsigned int ccr2,unsigned int ccr3)
{
	TIM2->CCR2 = ccr2;
	TIM2->CCR3 = ccr3;
}

/*--------------------------------
** 日期：2018.5.9
** 功能：电机编码器的正交解码
** 说明：
1、使用定时器4通道1和2， CH1(PB6)  CH2(PB7)
2、定时器的分频系数影响编码器转动一圈输出的脉冲，当48分频的时候，编码器一圈输出2000个脉冲
3、24分频输出4000个脉冲
----------------------------------*/
void Timer4_Motor_Encoder(void)
{
	GPIO_InitTypeDef  		 GPIO_InitStructure;
	NVIC_InitTypeDef 			 NVIC_InitStructure; 
	TIM_ICInitTypeDef        TIM4_ICInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	//开启时钟，72M
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); 
	
	//引脚配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		//必须接上拉电阻，不然无法计数。
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//GPIO_ResetBits(GPIOB,GPIO_Pin_6 | GPIO_Pin_7);
	
	//定时器设置
	TIM_DeInit(TIM4);
   TIM_TimeBaseStructure.TIM_Period = 60000 - 1;     //自动重装载值
   TIM_TimeBaseStructure.TIM_Prescaler = 0;			 //分频系数
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx向上计数模式
   TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据TIM_TimeBaseStructure中指定的参数初始化外设TIM2
	
	//正交解码设置,使用编码器模式3，同时在TI1  TI2上计数,且都是上升沿计数
   TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);        
   //TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12,  TIM_ICPolarity_Falling, TIM_ICPolarity_Falling);
	TIM_ICStructInit(&TIM4_ICInitStructure);  
   TIM4_ICInitStructure.TIM_ICFilter = 3;  		//滤波器设置
   TIM_ICInit(TIM4, &TIM4_ICInitStructure);  
  
   TIM_ClearFlag(TIM4, TIM_FLAG_Update);  
   TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);  
	
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
//   NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;	
//   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
//   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//   NVIC_Init(&NVIC_InitStructure);

   TIM4->CNT = 30000;
	
	TIM_Cmd(TIM4, ENABLE); 		//使能定时器4

}


/*--------------------------
** 日期：2018.5.9
** 功能：摆杆编码器的正交解码
** 说明：
1、使用定时器3的通道1和2  CH1(PA6)  CH2(PA7)
2、2000的16进制是 7D0
---------------------------*/
void Timer3_Pendulum_Encoder(void)
{
	GPIO_InitTypeDef  		 GPIO_InitStructure;
	NVIC_InitTypeDef 			 NVIC_InitStructure; 
	TIM_ICInitTypeDef        TIM3_ICInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	//开启时钟，72M
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); 
	
	//引脚配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//GPIO_ResetBits(GPIOA,GPIO_Pin_6 | GPIO_Pin_7);
	
	//定时器设置
	TIM_DeInit(TIM3);
   TIM_TimeBaseStructure.TIM_Period = 0xFF;	    //自动重装载值
   TIM_TimeBaseStructure.TIM_Prescaler = 0;			 //分频系数
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx向上计数模式
   TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseStructure中指定的参数初始化外设TIM2
	
	//正交解码设置,使用编码器模式3，同时在TI1  TI2上计数,且都是上升沿计数
   TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);   
	//TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,  TIM_ICPolarity_Falling, TIM_ICPolarity_Falling);	
   TIM_ICStructInit(&TIM3_ICInitStructure);  
   TIM3_ICInitStructure.TIM_ICFilter = 3;  			//滤波系数
   TIM_ICInit(TIM3, &TIM3_ICInitStructure);  
  
   TIM_ClearFlag(TIM3, TIM_FLAG_Update);  			//清除标志位
   TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);  		//允许中断更新
	
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
//   NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;	
//   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
//   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//   NVIC_Init(&NVIC_InitStructure);
	
   TIM3->CNT = 30000;  
	
	TIM_Cmd(TIM3, ENABLE);
}

/*
** 编码器模式中断
*/
void TIM2_IRQHandler(void)
{ 
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  
    {   
    }  
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  
}

void TIM3_IRQHandler(void)  
{  
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  
    {   
    }  
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  
}  


/*--------------------------
** 日期：2018.5.9
** 功能：PID运算周期定时设置
** 说明：
	定时10ms中断
----------------------------*/
void Timer1_PID_T(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	
	//定时器设置
	TIM_DeInit(TIM1);
   TIM_TimeBaseStructure.TIM_Period = 1000 - 1;     	//自动重装载值
   TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;		//分频系数
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx向上计数模式
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;	//重复计数设置  
   TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); 	//根据TIM_TimeBaseStructure中指定的参数初始化外设TIM2
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);			// 清除溢出中断标志 
   TIM_ITConfig(TIM1, TIM_IT_Update,ENABLE);		//使能定时器中断
    
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
	
   NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;	
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);	
	 
	TIM_Cmd(TIM1, ENABLE);	// 开启时钟
	 
}


/*----------------------------
** 日期：2018.5.10
** 功能：定时器1中断处理函数，100hz频率刷新正交解码的结果
** 说明：
1、注意高级定时器和普通定时器中断函数名的写法不同
------------------------------*/
void TIM1_UP_IRQHandler(void)
{
	if(TIM_GetITStatus( TIM1, TIM_IT_Update) != RESET ) 
	{	
		motor_address = TIM4->CNT/2 - 15000;
		pendulum_address = TIM3->CNT/2 - 15000;
		
//		if((TIM3->CR1 & TIM_CounterMode_Down) == TIM_CounterMode_Down)//向下计数，此时摆杆逆时针旋转
//			foreward_flag = 1;
//		else
//			foreward_flag = 0;				//摆杆顺时针旋转
//		
//		//判断圈数是否需要加
//		if((pendulum_address > 1200) && (pendulum_address < 1300) && (foreward_flag == 0))
//			turns_num++;
//		if((pendulum_address > -1300) && (pendulum_address < -1200) && (foreward_flag == 1))
//			turns_num--;
//		
//		relative_address = abs(pendulum_address) - turns_num * 2000;
				
		//定时进行pid计算
		PID_Output();
		
		TIM_ClearITPendingBit(TIM1 , TIM_FLAG_Update);  
	}		
}

