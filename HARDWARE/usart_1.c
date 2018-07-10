/*
** ����ģ�������������
*/
#include "main.h"

//������λ��ͨ��Э��
//���ݷֶη���
#define BYTE0(dwTemp)       (*(char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))
#define BYTE4(dwTemp)       (*((char *)(&dwTemp) + 4))
#define BYTE5(dwTemp)       (*((char *)(&dwTemp) + 5))
#define BYTE6(dwTemp)       (*((char *)(&dwTemp) + 6))
#define BYTE7(dwTemp)       (*((char *)(&dwTemp) + 7))

//���ڷ��͵���Ͱڸ˱����������ݣ�int�ͣ������ɸ�

unsigned char Tx_Buffer[256] ;
unsigned char Tx_Counter = 0;
unsigned char Tx_Counter_Temp = 0;
unsigned char Rx_Buffer[50] ;		//���ܻ���
unsigned char Rx_Counter = 0;          //���ܼ�������
unsigned char data_to_send[50] = {0};  //�������ݴ��
unsigned char CmdRx_Buffer[10] = {0};	//���ϸ�ʽ�Ľ����ַ�����

static void Send_Data(unsigned char *DataToSend ,unsigned char data_num);


/*----------------------------------
**�������ƣ�USART1_Config
**�������������ڲ������ã������ж�����������
**����˵����BaudRate����Ҫ���õĲ�����
**���ߣ�Andrew
**���ڣ�2018.1.24
-----------------------------------*/
void USART1_Config(uint32_t BaudRate)
{
	USART_InitTypeDef USART_InitStructure;					
	NVIC_InitTypeDef  NVIC_InitStructure;	
	GPIO_InitTypeDef GPIO_InitStructure;	
	
	//������Ӧ��ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1,ENABLE);

	//GPIO����
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 				//TX
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	   	//�����������
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	   //���ö˿��ٶ�Ϊ50M
  	GPIO_Init(GPIOA, &GPIO_InitStructure);				   	//���ݲ�����ʼ��GPIOA�Ĵ���	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 				//RX
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������(��λ״̬);
  	GPIO_Init(GPIOA, &GPIO_InitStructure);				   	//���ݲ�����ʼ��GPIOA�Ĵ���	
	
	//ģʽ����
	USART_DeInit(USART1); 														//��λ����
	USART_InitStructure.USART_BaudRate            =BaudRate ;	  			//�������Լ����ã�����������9600
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b; 	//���������ʹ��8λ����
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;	 	//��֡��β����1λֹͣλ
	USART_InitStructure.USART_Parity              = USART_Parity_No ;	 	//��żʧ��
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ����ʧ��
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx; //���պͷ���ģʽ
	USART_Init(USART1, &USART_InitStructure);								//���ݲ�����ʼ�����ڼĴ���
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);							//ʹ�ܴ����жϽ���
	USART_Cmd(USART1, ENABLE);     											//ʹ�ܴ�������
	
	//�����������1���ֽ��޷���ȷ���ͳ�ȥ������ 
	USART_ClearFlag(USART1, USART_FLAG_TC);     /* �巢����Ǳ�־��Transmission Complete flag */
	
	//���ô����ж�����
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //�жϷ���1
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			//�趨�ж�ԴΪ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//�ж�ռ���ȼ�Ϊ0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;			//�����ȼ�Ϊ0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ���ж�
	NVIC_Init(&NVIC_InitStructure);							   	//���ݲ�����ʼ���жϼĴ���
	
}


/*----------------------------------
**�������ƣ�USART1_IRQHandler
**���������������жϷ�����
**����˵������
**���ߣ�Andrew
**���ڣ�2018.1.24
-----------------------------------*/
void USART1_IRQHandler()
{
	/*�쳣�жϴ���*/
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)	//
    {
       USART_ReceiveData(USART1);										//��������
		 USART_ClearFlag(USART1, USART_FLAG_ORE);
    }
	 
	 /*����*/
	if(USART_GetITStatus(USART1,USART_IT_TXE) != RESET)
	{
		USART1->DR = Tx_Buffer[Tx_Counter_Temp++];    	//��DR�Ĵ���д���ݵ�ʱ�򣬴��ھͻ��Զ�����
			
		if(Tx_Counter == Tx_Counter_Temp)					//���ݰ��������
		{
		   USART_ITConfig(USART1,USART_IT_TXE,DISABLE);	//�ر�TXE�ж�
			Tx_Counter = 0;		//������ɼ�������
			Tx_Counter_Temp = 0;
		}
	}
	  
	 /*���ܣ�����������������͹�����У׼ʱ��*/
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)	//��ȡ�����жϱ�־λUSART_IT_RXNE 
	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);	//����жϱ�־λ
		
		Rx_Buffer[Rx_Counter++] = USART_ReceiveData(USART1);//�������ݵ�������������������Զ�����жϱ�־λ
		
		if((Rx_Buffer[Rx_Counter-2] == 0x0d) && (Rx_Buffer[Rx_Counter-1] == 0x0a))//������Ļس����в�������
		{
			memcpy(CmdRx_Buffer,Rx_Buffer,Rx_Counter);	//ֱ��ʹ���ڴ濽����Ч�ʱȽϸ�
			
//			for(i = 0;i<Rx_Counter;i++)
//			{
//				CmdRx_Buffer[i] = Rx_Buffer[i];  //�������������
//			}
			
			CmdRx_Buffer[Rx_Counter] = '\0';		//������'\0',������������
			Rx_Counter = 0;							//��������
			printf("CMD OK!");						//���ճɹ������ͱ�־
		}
	}
}


/*----------------------------------
**�������ƣ�fputc
**�����������ض���fputc����������ʹ��printf
**����˵������
**���ߣ�Andrew
**���ڣ�2018.1.24
-----------------------------------*/
int fputc(int Data, FILE *f)
{   
	while(!USART_GetFlagStatus(USART1,USART_FLAG_TXE));	 	//USART_GetFlagStatus���õ�����״̬λ
																				//USART_FLAG_TXE:���ͼĴ���Ϊ�� 1��Ϊ�գ�0��æ״̬
	USART_SendData(USART1,Data);						  				//����һ���ַ�	
	return Data;										  					//���ط��͵�ֵ
}


/*�жϷ��ͷ�ʽ�������ַ�*/
static void Send_Data(unsigned char *DataToSend ,unsigned char data_num)
{
	uint8_t i;
	
	for(i = 0;i < data_num;i++)	//Ҫ���͵����ݷŵ����ͻ�����
		Tx_Buffer[Tx_Counter++] = *(DataToSend+i);

	if(!(USART1->CR1 & USART_CR1_TXEIE))					//�жϷ�ʽ����
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE); 	//ֻҪ���ͼĴ����գ��ͻ�һֱ���ж�
}


/*----------------------------------
**�������ƣ�Send_Senser
**�������������ʹ����6050����������
**����˵������
**���ߣ�Andrew
**���ڣ�2018.1.24
-----------------------------------*/
void Send_Senser(void)
{
	u8 cnt = 0;
	uint16_t sum = 0;
	u8 i = 0;
		
	data_to_send[cnt++]=0xAA;	 //֡ͷ��AAAA
	data_to_send[cnt++]=0xAA;
	data_to_send[cnt++]=0x02;	 //�����֣�OXF2	��0x02һ����
	data_to_send[cnt++]=0;	     //��Ҫ�������ݵ��ֽ�������ʱ��0�������ڸ�ֵ��
	
	//��һ�����ݶ�
	data_to_send[cnt++] = BYTE1(motor_address);//ȡdata[0]���ݵĸ��ֽڣ�
	data_to_send[cnt++] = BYTE0(motor_address);
	data_to_send[cnt++] = BYTE1(pendulum_address);
	data_to_send[cnt++] = BYTE0(pendulum_address);
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	//�ڶ������ݶ�
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	//���������ݶ�
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	data_to_send[cnt++] = 0;
	
	data_to_send[3] = cnt-4;//���������ݵ��ֽ�����

	for(i=0;i<cnt;i++)
		sum += data_to_send[i];
	
	data_to_send[cnt++]=sum;  //���һλ��У���

	Send_Data(data_to_send, cnt);   //����һ�����ݰ�
		
}



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
End:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D:-D
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
