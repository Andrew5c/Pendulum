
#ifndef __MAIN_H
#define __MAIN_H

#include <stm32f10x.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usart_1.h"
#include "timer.h"
#include "OLED.h"
#include "LED.h"
#include "PID.h"
#include "delay.h"
#include "beep.h"
#include "key.h"

extern volatile unsigned char start;
extern volatile unsigned char start_pid;

#endif

