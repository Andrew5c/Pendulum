
#include "main.h"


//��ȡ���������ݣ����жϺ�����ˢ�£���תʱΪ��������תʱΪ����
volatile int motor_address = 0;
volatile int pendulum_address = 0;
volatile unsigned char turns_num = 0;
volatile unsigned int relative_address = 0;

//�ڸ�����ת��־,��ʱ��Ϊ1��˳ʱ��Ϊ0
unsigned char foreward_flag = 0;


/*---------------------------------------
** ���ڣ�2018.5.8
** ���ܣ�ʹ�û�����ʱ��2��CH2(PA1),CH3(PA2)���2·pwm�����޲���
** ˵����
1��Ƶ�ʣ�10KHZ,ARR=100,PSC=72��f=72M/ARR/PSC
2��ռ�ձȣ�������ڵ�ʱ��ͨ������ȽϼĴ���TIM2->CCRnֱ�Ӹ�ֵ
3��ֱ�����pwm�����趨ʱ���ж�
4��pwm���ģʽ��
ģʽ1�������ϼ���ʱ��һ��TIMx_CNT<TIMx_CCR1ʱͨ��Ϊ��Ч��ƽ������Ϊ��Ч��ƽ��
ģʽ2�������ϼ���ʱ��һ��TIMx_CNT<TIMx_CCR1ʱͨ��Ϊ��Ч��ƽ������Ϊ��Ч��ƽ��
------------------------------------------*/
void Timer2_PWM_Init(void)
{
	GPIO_InitTypeDef  				 GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef        TIM_TimeBaseStructure;
	TIM_OCInitTypeDef              TIM_OCInitStructure;

   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//����ʱ�ӣ�72M
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); 
	
	//�����������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;			//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
		
	//��ʱ������
	TIM_DeInit(TIM2);
   TIM_TimeBaseStructure.TIM_Period = 100 - 1;     //�Զ���װ��ֵ������ARR��ֵΪ19����0������19���պ���2ms��500hz
   TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;//��Ƶϵ��������PSC��ֵΪ7199�������Ļ�ÿ����һ��Ϊ0.1ms 
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx���ϼ���ģʽ
   TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //����TIM_TimeBaseStructure��ָ���Ĳ�����ʼ������TIM2
   
	//���pwm����
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 	//ѡ��ʱ��ģʽ,
   TIM_OCInitStructure.TIM_Pulse = 0; 					//���ô�װ�벶��ȽϼĴ���������ֵ���տ�ʼ��������Ϊ0.
   TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //�������:TIM����Ƚϼ��Ը�
   TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
	
	//chnnel 2
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable); 		//ʹ��TIM2��CCR1�ϵ�Ԥװ�ؼĴ���
	
	//chnnel 3	
	TIM_OC3Init(TIM2, &TIM_OCInitStructure); 
   TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable); 		//ʹ��TIM2��CCR2�ϵ�Ԥװ�ؼĴ���
	
	/*
	TIMx_CCRx�Ĵ����ܹ����κ�ʱ��ͨ��������и����Կ���������Σ�
	������δʹ��Ԥװ�ؼĴ���(TIM_ARRPreloadConfig������TIMx_CCRxӰ�ӼĴ���ֻ���ڷ�����һ�θ����¼�ʱ������)��
	����Ϊ�˺������жϷ����ӳ�������޸�TIMx_CCRʵʱ������
	*/
   TIM_ARRPreloadConfig(TIM2, ENABLE); //ʹ��TIM2��ARR�ϵ�Ԥװ�ؼĴ���
   TIM_Cmd(TIM2, ENABLE);  //ʹ��TIM2����
}

/*-----------------------------
** ���ܣ��޸�pwmռ�ձ�
** ˵����ע��CCR��ֵҪС��ARR��ֵ
** ���ÿ⺯��Ҳ��ͬ����Ч����
	TIM_SetCompare2(TIM2,10);
	TIM_SetCompare3(TIM2,70);
--------------------------------*/
void Set_PWM_Value(unsigned int ccr2,unsigned int ccr3)
{
	TIM2->CCR2 = ccr2;
	TIM2->CCR3 = ccr3;
}

/*--------------------------------
** ���ڣ�2018.5.9
** ���ܣ��������������������
** ˵����
1��ʹ�ö�ʱ��4ͨ��1��2�� CH1(PB6)  CH2(PB7)
2����ʱ���ķ�Ƶϵ��Ӱ�������ת��һȦ��������壬��48��Ƶ��ʱ�򣬱�����һȦ���2000������
3��24��Ƶ���4000������
----------------------------------*/
void Timer4_Motor_Encoder(void)
{
	GPIO_InitTypeDef  		 GPIO_InitStructure;
	NVIC_InitTypeDef 			 NVIC_InitStructure; 
	TIM_ICInitTypeDef        TIM4_ICInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	//����ʱ�ӣ�72M
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); 
	
	//��������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		//������������裬��Ȼ�޷�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//GPIO_ResetBits(GPIOB,GPIO_Pin_6 | GPIO_Pin_7);
	
	//��ʱ������
	TIM_DeInit(TIM4);
   TIM_TimeBaseStructure.TIM_Period = 60000 - 1;     //�Զ���װ��ֵ
   TIM_TimeBaseStructure.TIM_Prescaler = 0;			 //��Ƶϵ��
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx���ϼ���ģʽ
   TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //����TIM_TimeBaseStructure��ָ���Ĳ�����ʼ������TIM2
	
	//������������,ʹ�ñ�����ģʽ3��ͬʱ��TI1  TI2�ϼ���,�Ҷ��������ؼ���
   TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);        
   //TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12,  TIM_ICPolarity_Falling, TIM_ICPolarity_Falling);
	TIM_ICStructInit(&TIM4_ICInitStructure);  
   TIM4_ICInitStructure.TIM_ICFilter = 3;  		//�˲�������
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
	
	TIM_Cmd(TIM4, ENABLE); 		//ʹ�ܶ�ʱ��4

}


/*--------------------------
** ���ڣ�2018.5.9
** ���ܣ��ڸ˱���������������
** ˵����
1��ʹ�ö�ʱ��3��ͨ��1��2  CH1(PA6)  CH2(PA7)
2��2000��16������ 7D0
---------------------------*/
void Timer3_Pendulum_Encoder(void)
{
	GPIO_InitTypeDef  		 GPIO_InitStructure;
	NVIC_InitTypeDef 			 NVIC_InitStructure; 
	TIM_ICInitTypeDef        TIM3_ICInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	//����ʱ�ӣ�72M
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); 
	
	//��������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//GPIO_ResetBits(GPIOA,GPIO_Pin_6 | GPIO_Pin_7);
	
	//��ʱ������
	TIM_DeInit(TIM3);
   TIM_TimeBaseStructure.TIM_Period = 0xFF;	    //�Զ���װ��ֵ
   TIM_TimeBaseStructure.TIM_Prescaler = 0;			 //��Ƶϵ��
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx���ϼ���ģʽ
   TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����TIM_TimeBaseStructure��ָ���Ĳ�����ʼ������TIM2
	
	//������������,ʹ�ñ�����ģʽ3��ͬʱ��TI1  TI2�ϼ���,�Ҷ��������ؼ���
   TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);   
	//TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,  TIM_ICPolarity_Falling, TIM_ICPolarity_Falling);	
   TIM_ICStructInit(&TIM3_ICInitStructure);  
   TIM3_ICInitStructure.TIM_ICFilter = 3;  			//�˲�ϵ��
   TIM_ICInit(TIM3, &TIM3_ICInitStructure);  
  
   TIM_ClearFlag(TIM3, TIM_FLAG_Update);  			//�����־λ
   TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);  		//�����жϸ���
	
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
** ������ģʽ�ж�
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
** ���ڣ�2018.5.9
** ���ܣ�PID�������ڶ�ʱ����
** ˵����
	��ʱ10ms�ж�
----------------------------*/
void Timer1_PID_T(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	
	//��ʱ������
	TIM_DeInit(TIM1);
   TIM_TimeBaseStructure.TIM_Period = 1000 - 1;     	//�Զ���װ��ֵ
   TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;		//��Ƶϵ��
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIMx���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;	//�ظ���������  
   TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); 	//����TIM_TimeBaseStructure��ָ���Ĳ�����ʼ������TIM2
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);			// �������жϱ�־ 
   TIM_ITConfig(TIM1, TIM_IT_Update,ENABLE);		//ʹ�ܶ�ʱ���ж�
    
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
	
   NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;	
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);	
	 
	TIM_Cmd(TIM1, ENABLE);	// ����ʱ��
	 
}


/*----------------------------
** ���ڣ�2018.5.10
** ���ܣ���ʱ��1�жϴ�������100hzƵ��ˢ����������Ľ��
** ˵����
1��ע��߼���ʱ������ͨ��ʱ���жϺ�������д����ͬ
------------------------------*/
void TIM1_UP_IRQHandler(void)
{
	if(TIM_GetITStatus( TIM1, TIM_IT_Update) != RESET ) 
	{	
		motor_address = TIM4->CNT/2 - 15000;
		pendulum_address = TIM3->CNT/2 - 15000;
		
//		if((TIM3->CR1 & TIM_CounterMode_Down) == TIM_CounterMode_Down)//���¼�������ʱ�ڸ���ʱ����ת
//			foreward_flag = 1;
//		else
//			foreward_flag = 0;				//�ڸ�˳ʱ����ת
//		
//		//�ж�Ȧ���Ƿ���Ҫ��
//		if((pendulum_address > 1200) && (pendulum_address < 1300) && (foreward_flag == 0))
//			turns_num++;
//		if((pendulum_address > -1300) && (pendulum_address < -1200) && (foreward_flag == 1))
//			turns_num--;
//		
//		relative_address = abs(pendulum_address) - turns_num * 2000;
				
		//��ʱ����pid����
		PID_Output();
		
		TIM_ClearITPendingBit(TIM1 , TIM_FLAG_Update);  
	}		
}

