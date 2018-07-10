
#ifndef __TIMER_H
#define __TIMER_H


extern volatile int motor_address;
extern volatile int pendulum_address;
extern volatile unsigned char start;
extern volatile unsigned char turns_num;
extern volatile unsigned int relative_address;

void Timer2_PWM_Init(void);
void Set_PWM_Value(unsigned int ccr2,unsigned int ccr3);
void Timer4_Motor_Encoder(void);
void Timer3_Pendulum_Encoder(void);
void Timer1_PID_T(void);


#endif 

