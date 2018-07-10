
#include "main.h"

//�����ڻ���
#define	TEST_INNER	1

//����Ŀ��λ��
#define  PENDULUM_ADDRESS_AIM   500
#define  MOTOR_ADDRESS_AIM   	  0

struct  Position_PID  IN_PID;
struct  Position_PID  OUT_PID;

unsigned char show_pid_buf[30];


/*
** PID������ʼ��
** P���������Ӧ�ٶȡ�I���ڼ�С���D����������
*/
void PID_Init(void)
{
	memset(&IN_PID, 0, sizeof(struct Position_PID));
	memset(&OUT_PID, 0, sizeof(struct Position_PID));

	//�ڸ�PID����
	IN_PID.KP = 3;
	IN_PID.KI = 0;
	IN_PID.KD = 1.2;
	IN_PID.U_MAX = 200;
	IN_PID.U_MIN = -200;
	
	//��ʾPID����
	sprintf((char *)show_pid_buf, "I:%.2f,%.2f,%.2f", IN_PID.KP, IN_PID.KI, IN_PID.KD);
	OLED_Show_String(0,4,show_pid_buf);
	
	#if (!TEST_INNER)
	//���PID����
	OUT_PID.KP = 0.2;
	OUT_PID.KI = 0;
	OUT_PID.KD = 7;
	OUT_PID.U_MAX = 200;
	OUT_PID.U_MIN = -200;

	sprintf((char *)show_pid_buf, "O:%.2f,%.2f,%.2f", OUT_PID.KP, OUT_PID.KI, OUT_PID.KD);
	OLED_Show_String(0,6,show_pid_buf);
	#endif
}


/*
** ����λ��ʽPID+���ַ���+���ֿ�����
*/
void PID_Operation(struct Position_PID *p, int integration_limit, int PWM_limit)
{
	int index = 0;
	
	p->ERROR_1 = p->AIM - p->FEEDBACK;	//����ƫ��,�趨ֵ��ʵ��ֵ	
	p->ERROR_SUM += p->ERROR_1;			//�ۼ�ƫ��
	
	if(p->ERROR_SUM >= p->U_MAX)			//���ֿ�����
	{
		if(abs(p->ERROR_1) >= integration_limit)	//���ַ���
			index = 0;
		else
		{
			index = 1;
			if(p->ERROR_1 <= 0)		//ֻ�ۼӸ�ƫ��
				p->ERROR_SUM += p->ERROR_1;
		}
	}
	else if(p->ERROR_SUM <= p->U_MIN)
	{
		if(abs(p->ERROR_1) >= integration_limit)
			index = 0;
		else
		{
			index = 1;
			if(p->ERROR_1 >= 0)		//ֻ�ۼ���ƫ��
				p->ERROR_SUM += p->ERROR_1;
		}
	}
	else
	{
		if(abs(p->ERROR_1) >= integration_limit)
			index = 0;
		else
		{
			index = 1;
			p->ERROR_SUM += p->ERROR_1;
		}	
	}
	
	//p->ERROR_SUM = constrain_int16(p->ERROR_SUM, -integration_limit, integration_limit);		//ƫ���޷�
	
	p->PWM = (int)(p->KP*p->ERROR_1 + index*p->KI*p->ERROR_SUM + p->KD*(p->ERROR_1 - p->ERROR_2));	//�������
	
	p->PWM = constrain_int16(p->PWM, -PWM_limit, PWM_limit);	//����޷�
	
	p->ERROR_2 = p->ERROR_1;				//����ƫ��
}


/*
** ���PWM���Ƶ��
*/
void PID_Output(void)
{
	int relative_pendulum_address = 0;
	
	if((-300<=motor_address) && (motor_address<=300))
	{	
		relative_pendulum_address = abs(pendulum_address);
		
		//�ڸ˴����ٽ��м�λ��ʱ����ſ�ʼ���п���
		if((relative_pendulum_address >= 480) && (relative_pendulum_address <= 520))
		{	
			#if (!TEST_INNER)
			//�⻷PID
			OUT_PID.AIM = MOTOR_ADDRESS_AIM;
			OUT_PID.FEEDBACK = motor_address;					
			PID_Operation(&OUT_PID, 30, 70);
			#endif
			
			//�ڸ˴������ȥ
			if(pendulum_address >= 0)
			{
				//�ڻ�PID���⻷�����Ϊ�ڻ�������
				#if TEST_INNER
				IN_PID.AIM = PENDULUM_ADDRESS_AIM;
				#else
				IN_PID.AIM = PENDULUM_ADDRESS_AIM + OUT_PID.PWM;
				#endif
				
				IN_PID.FEEDBACK = pendulum_address;		
				PID_Operation(&IN_PID, 30, 70);
				if(IN_PID.PWM >= 0)
					Set_PWM_Value(IN_PID.PWM, 0);
				else
					Set_PWM_Value(0, -IN_PID.PWM);
			}
			
			//�ڸ˴��ұ���ȥ
			else
			{
				//�ڻ�PID���⻷�����Ϊ�ڻ�������
				#if TEST_INNER
				IN_PID.AIM = PENDULUM_ADDRESS_AIM;
				#else
				IN_PID.AIM = PENDULUM_ADDRESS_AIM - OUT_PID.PWM;
				#endif
				
				IN_PID.FEEDBACK = -pendulum_address;		
				PID_Operation(&IN_PID, 200, 70);
				if(IN_PID.PWM >= 0)
					Set_PWM_Value(0, IN_PID.PWM);
				else
					Set_PWM_Value(-IN_PID.PWM, 0);
			}			
		}
		
		//�տ�ʼ�����
		else if(start == 1)
		{
			start = 0;
			Start_Launch();
		}
		//����λ�õ��������
		else
			Set_PWM_Value(0, 0);
	}
	else
		Set_PWM_Value(0, 0);
	
}

/*
** ��ڳ���
** ��ͬ��ת��ʱ���PWM������ʹ���֮��������Ȼ�����趨ֵ����
*/
void Start_Launch(void)
{
	//��ת˦��ڸ�
	Set_PWM_Value(42, 0);
	Delay_ms(290);
	
	//��תʹ�ڵ���
	Set_PWM_Value(0, 43);
	Delay_ms(280);
	
	//������
	Set_PWM_Value(0, 0);
		
}


//�������޷�,constrain ->Լ��������
//������벻�����֣��򷵻ؼ���ֵ��ƽ��ֵ
//isnan������������Ƿ������֣�is not a number
float constrain_float(float amt, float low, float high) 
{
	return ((amt)<(low)?(low):((amt)>(high)?(high):(amt)));	
}

//16λ�������޷�
int constrain_int16(int amt, int low, int high)
{
	return ((amt)<(low)?(low):((amt)>(high)?(high):(amt)));
}

