
#include "main.h"


unsigned char motor[] = {"Motor:"};
unsigned char pendulum[] = {"Pendulum:"};

volatile unsigned char start = 0;
volatile unsigned char start_pid = 0;

int main(void)
{
	OLED_Init();
	
	LED_Init();
	
	Key_Init();
	
	Beep_Init();
	
	USART1_Config(115200);
	
	Timer2_PWM_Init();  
	
	Timer4_Motor_Encoder();
	
	Timer3_Pendulum_Encoder();
	
	OLED_Show_String(0,0,motor);
	OLED_Show_String(0,2,pendulum);
	
	//������ʾ
	Beep_Warning(3,100);
	LED_Start();
	
	//PID������ʼ��
	PID_Init();
	
	//��ʼ����PID����
	Timer1_PID_T();
	
	
	while(1)
	{
		OLED_Show_encoder_address();
		
//		printf("motor:%d\tpendulum:%d\n",motor_address,pendulum_address);
//		//���ݻ�ͼ
//		Send_Senser();
		
	}
	
}

