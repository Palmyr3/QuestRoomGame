#include "UART_control.h"

volatile extern uint32_t shiftRegister[SHIFT_REGISTER_SIZE];
volatile extern uint8_t inputValueBuffer[INPUT_VALIE_BUFF_SIZE];

volatile uint8_t transmitBuffer[TRANSMIT_BUFF_SIZE];
volatile uint8_t recieveBuffer[RECIEVE_BUFF_SIZE];
volatile unsigned int transmitPointer = 0;
volatile unsigned int recievePointer = 0;
volatile unsigned int startByteCatched = FALSE_P;


ErrorStateEnum UART1_Init()
{
	uint32_t tempReg;
	//initiation of GPIOs (PA10;PA9)

	//Enable GPIO "A" port
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	//RX (PA10) init
	GPIOA->MODER &= ~GPIO_MODER_MODER10;
	//alternate function
	GPIOA->MODER |= GPIO_MODER_MODER10_1;
	//speed
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10;
	//NO pull up/down
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR10;

	//Selection of alternate function for PA10
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFRH10;
	GPIOA->AFR[1] |= (ALTER_FUNC_7 & GPIO_AFRH_AFRH10);


	//TX (PA9) init
	GPIOA->MODER &= ~GPIO_MODER_MODER9;
	//alternate function
	GPIOA->MODER |= GPIO_MODER_MODER9_1;
	//push-pull
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_9;
	//speed
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9;
	//pull up
	tempReg = GPIOA->PUPDR;
	tempReg &= ~GPIO_PUPDR_PUPDR9;
	tempReg |= GPIO_PUPDR_PUPDR9_0;
	GPIOA->PUPDR = tempReg;

	//Selection of alternate function for PA9
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFRH9;
	GPIOA->AFR[1] |= (ALTER_FUNC_7 & GPIO_AFRH_AFRH9);


	//UART init
	//USART1 clock enable
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	//enable USART1
	USART1->CR1 |= USART_CR1_UE;
	//8 bit
	USART1->CR1 &= ~USART_CR1_M;
	//1 stop
	USART1->CR2 &= ~USART_CR2_STOP;
	//oversampling by 16
	USART1->CR1 &= ~USART_CR1_OVER8;

	//baud rate = Fclk(4,194MHz)/(16*USARTDIV)
	//baud rate = 38400
	//USART1->BRR = 0x6D;

	//baud rate = Fclk(16MHz)/(16*USARTDIV)
	//baud rate = 38400
	//USART1->BRR = 0x1A1;

	//baud rate = Fclk(32MHz)/(16*USARTDIV)
	//baud rate = 38400
	USART1->BRR = 0x341;

	//RX TE enable
	USART1->CR1 |=	(	USART_CR1_TE
					|	USART_CR1_RE);

	NVIC_SetPriority (USART1_IRQn, 1);
	NVIC_EnableIRQ (USART1_IRQn);


	//enable USART1 interrupt
	USART1->CR1 |=  (	USART_CR1_RXNEIE);


	return SUCCESS_P;
}

void USART1_IRQHandler(void)
{
	volatile int temp;
	if(USART1->SR & USART_SR_RXNE) //data received
	{
		RecieveBufferInterruptRoutine();
	}
	if(USART1->SR & USART_SR_ORE) //data overrun error
	{
		temp = USART1->DR; //reset ORE flag by reading DR register
	}
	if(USART1->SR & USART_SR_TXE)
	{
		TransmitBufferInterruptRoutine();
	}
}


void RecieveBufferInterruptRoutine(void)
{
	uint8_t temp;
	temp = USART1->DR;
	if(temp == PACKAGE_START)
	{
		recievePointer = 1;
		startByteCatched = TRUE_P;
	}
	else
	{
		if(startByteCatched)
		{
			if(recievePointer < (RECIEVE_BUFF_SIZE - 1))
			{
				recieveBuffer[recievePointer] = temp;
				recievePointer++;
			}
			else
			{
				startByteCatched = FALSE_P;
				if(temp == PACKAGE_STOP)
				{
					PackageParse();
				}
			}
		}
	}
}

void TransmitBufferInterruptRoutine(void)
{
	if(transmitPointer < TRANSMIT_BUFF_SIZE)
	{
		USART1->DR = transmitBuffer[transmitPointer];
		transmitPointer++;
	}
	else
	{
		USART1->CR1 &=  ~USART_CR1_TXEIE;
	}
}

void TransmitBufferStart(void)
{
	transmitPointer = 0;
	TransmitBufferInterruptRoutine();
	//enable USART1 interrupt
	USART1->CR1 |=  USART_CR1_TXEIE;
}

void inline TransmitBufferSetMagicNumbers(void)
{
	transmitBuffer[PAC_START_BYTE] = PACKAGE_START;
	transmitBuffer[PAC_STOP_BYTE] = PACKAGE_STOP;
}

void TransmitBufferErrase(void)
{
	int i;
	for(i=0; i<TRANSMIT_BUFF_SIZE; i++)
	{
		transmitBuffer[i] = (uint8_t)0;
	}
}

void UpdateShiftRegister(void)
{
	//shiftRegister[1]; shiftRegister[0]
	//||||||||||||||||||||||||||||||||||
	//VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
	//shift IC outputs (MAX is 64 outputs)

	uint32_t sReg0 = 0, sReg1 = 0;

	sReg0 =	  (recieveBuffer[PAC_LAST_DATA_BYTE] & ~DATA_MULT)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 1] & ~DATA_MULT)<<7)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 2] & ~DATA_MULT)<<14)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 3] & ~DATA_MULT)<<21)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 4] & ~DATA_MULT)<<28);

	sReg1 =   ((recieveBuffer[PAC_LAST_DATA_BYTE - 4] & ~DATA_MULT) >> 4)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 5] & ~DATA_MULT) << 3)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 6] & ~DATA_MULT) << 10)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 7] & ~DATA_MULT) << 17)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 8] & ~DATA_MULT) << 24)
			| ((recieveBuffer[PAC_LAST_DATA_BYTE - 9] & ~DATA_MULT) << 31);

	//shiftRegister[0] = sReg0;
	//shiftRegister[1] = sReg1;

	//hardware inversion on
	shiftRegister[0] = ~ sReg0;
	shiftRegister[1] = ~ sReg1;
}

void CreateInputValuePackage(void)
{
	int i;
	uint8_t parityCalc = COMAND_READ_INPUTS;
	transmitBuffer[PAC_COMAND_BYTE] = COMAND_READ_INPUTS;
	for(i = PAC_FIRST_DATA_BYTE; i < PAC_PARITY_BYTE; i++)
	{
		transmitBuffer[i] = inputValueBuffer[i-PAC_FIRST_DATA_BYTE];
		parityCalc ^= transmitBuffer[i];
	}
	transmitBuffer[PAC_PARITY_BYTE] = (parityCalc | DATA_MULT);
	TransmitBufferSetMagicNumbers();
}

void CreateDeviceIdPackage(void)
{
	int i;
	transmitBuffer[PAC_COMAND_BYTE] = COMAND_SEND_DEVICE_ID;
	for(i = PAC_FIRST_DATA_BYTE; i < PAC_PARITY_BYTE; i++)
	{
		transmitBuffer[i] = DEVICE_ID;
	}
	//parity = command ^ [id^id] | 0x80; (==COMAND_SEND_DEVICE_ID)
	transmitBuffer[PAC_PARITY_BYTE] = COMAND_SEND_DEVICE_ID;
	TransmitBufferSetMagicNumbers();
}


ErrorStateEnum PackageParse(void)
{
	int i;
	uint8_t temp;
	temp = recieveBuffer[PAC_COMAND_BYTE];
	for(i = (PAC_COMAND_BYTE + 1); i <(RECIEVE_BUFF_SIZE-2); i++)
	{
		temp ^= recieveBuffer[i];
	}
	temp |= 0x80;
	if (temp == recieveBuffer[PAC_PARITY_BYTE])
	{
		switch(recieveBuffer[PAC_COMAND_BYTE])
		{
		case COMAND_READ_INPUTS:
			CreateInputValuePackage();
			TransmitBufferStart();
			return SUCCESS_P;
			break;

		case COMAND_UPDATE_OUPUTS:
			UpdateShiftRegister();
			return SUCCESS_P;
			break;

		case COMAND_SEND_DEVICE_ID:
			CreateDeviceIdPackage();
			TransmitBufferStart();
			return SUCCESS_P;
			break;

		default:
			return ERROR_P;
			break;
		}
	}
	else
	{
		return ERROR_P;
	}
}