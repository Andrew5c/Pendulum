
#ifndef __PID_H
#define __PID_H

//内环参数
struct Position_PID
{
	float KP;
	float KI;
	float KD;
	int AIM;
	int FEEDBACK;
	int ERROR_1;	//本次偏差
	int ERROR_2;	//上一次偏差
	int ERROR_SUM; //偏差的累加
	int PWM;
	
	int U_MAX;
	int U_MIN;
		
};


extern struct  Position_PID  IN_PID;
extern struct  Position_PID  OUT_PID;


void PID_Init(void);
void PID_Operation(struct Position_PID *p, int integration_limit, int PWM_limit);
void PID_Output(void);
void Start_Launch(void);


float constrain_float(float amt, float low, float high);
int constrain_int16(int amt, int low, int high);

#endif

