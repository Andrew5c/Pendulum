
#ifndef __USART1_H
#define __USART1_H

#include <stm32f10x.h>

//�����������ļ��м�鴮�ڷ��͹���������
extern unsigned char CmdRx_Buffer[10];	


void USART1_Config(uint32_t BaudRate);
void Send_Senser(void);

#endif



