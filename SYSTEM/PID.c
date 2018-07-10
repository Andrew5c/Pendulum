
#include "main.h"

//调试内环宏
#define	TEST_INNER	1

//最终目标位置
#define  PENDULUM_ADDRESS_AIM   500
#define  MOTOR_ADDRESS_AIM   	  0

struct  Position_PID  IN_PID;
struct  Position_PID  OUT_PID;

unsigned char show_pid_buf[30];


/*
** PID参数初始化
** P用于提高响应速度、I用于减小静差、D用于抑制震荡
*/
void PID_Init(void)
{
	memset(&IN_PID, 0, sizeof(struct Position_PID));
	memset(&OUT_PID, 0, sizeof(struct Position_PID));

	//摆杆PID参数
	IN_PID.KP = 3;
	IN_PID.KI = 0;
	IN_PID.KD = 1.2;
	IN_PID.U_MAX = 200;
	IN_PID.U_MIN = -200;
	
	//显示PID参数
	sprintf((char *)show_pid_buf, "I:%.2f,%.2f,%.2f", IN_PID.KP, IN_PID.KI, IN_PID.KD);
	OLED_Show_String(0,4,show_pid_buf);
	
	#if (!TEST_INNER)
	//电机PID参数
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
** 计算位置式PID+积分分离+积分抗饱和
*/
void PID_Operation(struct Position_PID *p, int integration_limit, int PWM_limit)
{
	int index = 0;
	
	p->ERROR_1 = p->AIM - p->FEEDBACK;	//计算偏差,设定值减实际值	
	p->ERROR_SUM += p->ERROR_1;			//累计偏差
	
	if(p->ERROR_SUM >= p->U_MAX)			//积分抗饱和
	{
		if(abs(p->ERROR_1) >= integration_limit)	//积分分离
			index = 0;
		else
		{
			index = 1;
			if(p->ERROR_1 <= 0)		//只累加负偏差
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
			if(p->ERROR_1 >= 0)		//只累加正偏差
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
	
	//p->ERROR_SUM = constrain_int16(p->ERROR_SUM, -integration_limit, integration_limit);		//偏差限幅
	
	p->PWM = (int)(p->KP*p->ERROR_1 + index*p->KI*p->ERROR_SUM + p->KD*(p->ERROR_1 - p->ERROR_2));	//计算输出
	
	p->PWM = constrain_int16(p->PWM, -PWM_limit, PWM_limit);	//输出限幅
	
	p->ERROR_2 = p->ERROR_1;				//迭代偏差
}


/*
** 输出PWM控制电机
*/
void PID_Output(void)
{
	int relative_pendulum_address = 0;
	
	if((-300<=motor_address) && (motor_address<=300))
	{	
		relative_pendulum_address = abs(pendulum_address);
		
		//摆杆处于临近中间位置时电机才开始进行控制
		if((relative_pendulum_address >= 480) && (relative_pendulum_address <= 520))
		{	
			#if (!TEST_INNER)
			//外环PID
			OUT_PID.AIM = MOTOR_ADDRESS_AIM;
			OUT_PID.FEEDBACK = motor_address;					
			PID_Operation(&OUT_PID, 30, 70);
			#endif
			
			//摆杆从左边上去
			if(pendulum_address >= 0)
			{
				//内环PID，外环的输出为内环的输入
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
			
			//摆杆从右边上去
			else
			{
				//内环PID，外环的输出为内环的输入
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
		
		//刚开始的起摆
		else if(start == 1)
		{
			start = 0;
			Start_Launch();
		}
		//其他位置电机不动作
		else
			Set_PWM_Value(0, 0);
	}
	else
		Set_PWM_Value(0, 0);
	
}

/*
** 起摆程序
** 相同的转动时间和PWM，可以使起摆之后，悬臂仍然处于设定值附近
*/
void Start_Launch(void)
{
	//正转甩起摆杆
	Set_PWM_Value(42, 0);
	Delay_ms(290);
	
	//反转使摆倒立
	Set_PWM_Value(0, 43);
	Delay_ms(280);
	
	//起摆完毕
	Set_PWM_Value(0, 0);
		
}


//浮点数限幅,constrain ->约束，限制
//如果输入不是数字，则返回极端值的平均值
//isnan函数检测输入是否是数字，is not a number
float constrain_float(float amt, float low, float high) 
{
	return ((amt)<(low)?(low):((amt)>(high)?(high):(amt)));	
}

//16位整型数限幅
int constrain_int16(int amt, int low, int high)
{
	return ((amt)<(low)?(low):((amt)>(high)?(high):(amt)));
}

