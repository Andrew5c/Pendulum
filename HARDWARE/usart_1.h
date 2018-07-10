
#ifndef __USART1_H
#define __USART1_H

#include <stm32f10x.h>

//方便在其他文件中检查串口发送过来的命令
extern unsigned char CmdRx_Buffer[10];	


void USART1_Config(uint32_t BaudRate);
void Send_Senser(void);

#endif



