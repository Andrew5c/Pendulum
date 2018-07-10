
#ifndef __PID_H
#define __PID_H

//�ڻ�����
struct Position_PID
{
	float KP;
	float KI;
	float KD;
	int AIM;
	int FEEDBACK;
	int ERROR_1;	//����ƫ��
	int ERROR_2;	//��һ��ƫ��
	int ERROR_SUM; //ƫ����ۼ�
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

