#include "IO_control.h"


volatile uint32_t shiftRegister[SHIFT_REGISTER_SIZE];
volatile uint8_t inputValueBuffer[INPUT_VALIE_BUFF_SIZE];

volatile uint32_t resetCouner;

void OutputsInit(void)
{
	//Configuration of GPIOA3, GPIOA2, GPIO1 as push-pull output, no pull-up/down, max frequency

	RCC->AHBENR |= 		RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~	GPIO_MODER_MODER3;		//bug

	GPIOA->MODER &= ~(	GPIO_MODER_MODER3
			 	 	 |	GPIO_MODER_MODER1
			 	 	 |	GPIO_MODER_MODER2);
	//output
	GPIOA->MODER |=  (	GPIO_MODER_MODER3_0
				 	 |	GPIO_MODER_MODER1_0
				 	 |	GPIO_MODER_MODER2_0);
	//push-pull output
	GPIOA->OTYPER &= ~(	GPIO_OTYPER_IDR_3
					 |	GPIO_OTYPER_IDR_1
					 |	GPIO_OTYPER_IDR_2);
	//high frequency
	GPIOA->OSPEEDR |= (	GPIO_OSPEEDER_OSPEEDR3
					  |	GPIO_OSPEEDER_OSPEEDR1
					  |	GPIO_OSPEEDER_OSPEEDR2);
	//no pull-up/down
	GPIOA->PUPDR &= ~(	GPIO_PUPDR_PUPDR3
					 |	GPIO_PUPDR_PUPDR1
					 |	GPIO_PUPDR_PUPDR2);
}

void PushOutputData(uint32_t word1, uint32_t word0)
{
	//PA3 - DATA
	//PA1 - latch
	//PA2 - CLK
	int i;
	uint32_t shiftMask = 0x80000000;
	GPIOA->BSRRH =  (	GPIO_BSRR_BS_3
					|	GPIO_BSRR_BS_1
					|	GPIO_BSRR_BS_2);
	for(i = 0; i < 32; i++)
	{
		if(word1 & shiftMask)
		{
			GPIOA->BSRRL = GPIO_BSRR_BS_3;
		}
		else
		{
			GPIOA->BSRRH = GPIO_BSRR_BS_3;
		}
		GPIOA->BSRRL = GPIO_BSRR_BS_2;
		shiftMask >>= 1;
		GPIOA->BSRRH = GPIO_BSRR_BS_2;
	}
	shiftMask = 0x80000000;
	for(i = 0; i < 32; i++)
	{
		if(word0 & shiftMask)
		{
			GPIOA->BSRRL = GPIO_BSRR_BS_3;
		}
		else
		{
			GPIOA->BSRRH = GPIO_BSRR_BS_3;
		}
		GPIOA->BSRRL = GPIO_BSRR_BS_2;
		shiftMask >>= 1;
		GPIOA->BSRRH = GPIO_BSRR_BS_2;
	}
	GPIOA->BSRRL = GPIO_BSRR_BS_1;
	for(i = 0; i < LATCH_DELAY_VALUE; i++)
	{
		__NOP();
	}
	GPIOA->BSRRH = GPIO_BSRR_BS_1;
}

void Tim6Init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
	//clear to reset value
	TIM6->CR2 = 0;
	//set prescaller
	TIM6->PSC = 0x1061;
	//set limit
	TIM6->ARR = 0x3E8;
	//enable interrupt, disable DMA request
	TIM6->DIER = TIM_DIER_UIE & (~TIM_DIER_UDE);

	NVIC_SetPriority (TIM6_IRQn, 2);
	NVIC_EnableIRQ (TIM6_IRQn);

	//enable TIM6
	TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM6_IRQHandler(void)
{
	//clear UIF
	TIM6->SR &= ~TIM_SR_UIF;
	//blink
	GPIOC->ODR ^= GPIO_ODR_ODR_8;
	//Process inputs and outputs
	Tim6InterruptRoutine();
	//blink
	GPIOC->ODR ^= GPIO_ODR_ODR_8;
}

void ErraseInputBuffer(void)
{
	int i;
	for(i = 0; i < INPUT_VALIE_BUFF_SIZE; i++)
	{
		inputValueBuffer[i] = 0x80;
	}
}

void ErraseShiftRegister(void)
{
	//hardware inversion on
	shiftRegister[0] = SHIFT_REG_DEFAULT_VALUE;
	shiftRegister[1] = SHIFT_REG_DEFAULT_VALUE;
}

void Tim6InterruptRoutine(void)
{
	//В начале некоректнаые состояния (нули, 0), обновляюотя при первом проходе.
	static volatile uint32_t shiftRegCopy0 = (uint32_t)0;
	static volatile uint32_t shiftRegCopy1 = (uint32_t)0;
	//сначала некоторое время сбрасываем регистры.
	if(resetCouner < RESET_COUNTER_VALUE)
	{
		PushOutputData(SHIFT_REG_DEFAULT_VALUE, SHIFT_REG_DEFAULT_VALUE);
		resetCouner++;
	}
	else
	{
		if((shiftRegCopy0 != shiftRegister[0])||(shiftRegCopy1 != shiftRegister[1]))
		{
			shiftRegCopy0 = shiftRegister[0];
			shiftRegCopy1 = shiftRegister[1];
			PushOutputData(shiftRegCopy1, shiftRegCopy0);
		}
	}
	InpitsProcess();
}

void InputsInit(void)
{
	//inputs:
	//PORT A: 4,5,6,0,8,11,12,15
	//PORT B: all
	//PORT C: all, except PC8, PC9
	//PORT D: 2
	//PORT H: 1

	RCC->AHBENR |= (	RCC_AHBENR_GPIOAEN
					|	RCC_AHBENR_GPIOBEN
					|	RCC_AHBENR_GPIOCEN
					|	RCC_AHBENR_GPIODEN
					|	RCC_AHBENR_GPIOHEN);
	GPIOA->MODER &= ~	GPIO_MODER_MODER4;		//bug
	GPIOB->MODER &= ~	GPIO_MODER_MODER4;		//bug

	//Config as input
	GPIOA->MODER &= ~(	GPIO_MODER_MODER4
					|	GPIO_MODER_MODER5
					|	GPIO_MODER_MODER6
					|	GPIO_MODER_MODER0		//A0 (is) <- A7 (was)
					|	GPIO_MODER_MODER8
					|	GPIO_MODER_MODER11
					|	GPIO_MODER_MODER12
					|	GPIO_MODER_MODER15);

	GPIOB->MODER &= ~(	GPIO_MODER_MODER0
					|	GPIO_MODER_MODER1
					|	GPIO_MODER_MODER2
					|	GPIO_MODER_MODER3
					|	GPIO_MODER_MODER4
					|	GPIO_MODER_MODER5
					|	GPIO_MODER_MODER6
					|	GPIO_MODER_MODER7
					|	GPIO_MODER_MODER8
					|	GPIO_MODER_MODER9
					|	GPIO_MODER_MODER10
					|	GPIO_MODER_MODER11
					|	GPIO_MODER_MODER12
					|	GPIO_MODER_MODER13
					|	GPIO_MODER_MODER14
					|	GPIO_MODER_MODER15);

	GPIOC->MODER &= ~(	GPIO_MODER_MODER0
					|	GPIO_MODER_MODER1
					|	GPIO_MODER_MODER2
					|	GPIO_MODER_MODER3
					|	GPIO_MODER_MODER4
					|	GPIO_MODER_MODER5
					|	GPIO_MODER_MODER6
					|	GPIO_MODER_MODER7
					|	GPIO_MODER_MODER10
					|	GPIO_MODER_MODER11
					|	GPIO_MODER_MODER12
					|	GPIO_MODER_MODER13
					|	GPIO_MODER_MODER14
					|	GPIO_MODER_MODER15);

	GPIOD->MODER &= ~	GPIO_MODER_MODER2;

	GPIOH->MODER &= ~(	GPIO_MODER_MODER1);


	//PULL-UP
	GPIOA->PUPDR &= ~(	GPIO_PUPDR_PUPDR4
					|	GPIO_PUPDR_PUPDR5
					|	GPIO_PUPDR_PUPDR6
					|	GPIO_PUPDR_PUPDR0		//A0 (is) <- A7 (was)
					|	GPIO_PUPDR_PUPDR8
					|	GPIO_PUPDR_PUPDR11
					|	GPIO_PUPDR_PUPDR12
					|	GPIO_PUPDR_PUPDR15);

	GPIOB->PUPDR &= ~(	GPIO_PUPDR_PUPDR0
					|	GPIO_PUPDR_PUPDR1
					|	GPIO_PUPDR_PUPDR2
					|	GPIO_PUPDR_PUPDR3
					|	GPIO_PUPDR_PUPDR4
					|	GPIO_PUPDR_PUPDR5
					|	GPIO_PUPDR_PUPDR6
					|	GPIO_PUPDR_PUPDR7
					|	GPIO_PUPDR_PUPDR8
					|	GPIO_PUPDR_PUPDR9
					|	GPIO_PUPDR_PUPDR10
					|	GPIO_PUPDR_PUPDR11
					|	GPIO_PUPDR_PUPDR12
					|	GPIO_PUPDR_PUPDR13
					|	GPIO_PUPDR_PUPDR14
					|	GPIO_PUPDR_PUPDR15);

	GPIOC->PUPDR &= ~(	GPIO_PUPDR_PUPDR0
					|	GPIO_PUPDR_PUPDR1
					|	GPIO_PUPDR_PUPDR2
					|	GPIO_PUPDR_PUPDR3
					|	GPIO_PUPDR_PUPDR4
					|	GPIO_PUPDR_PUPDR5
					|	GPIO_PUPDR_PUPDR6
					|	GPIO_PUPDR_PUPDR7
					|	GPIO_PUPDR_PUPDR10
					|	GPIO_PUPDR_PUPDR11
					|	GPIO_PUPDR_PUPDR12
					|	GPIO_PUPDR_PUPDR13
					|	GPIO_PUPDR_PUPDR14
					|	GPIO_PUPDR_PUPDR15);

	GPIOD->PUPDR &= ~	GPIO_PUPDR_PUPDR2;

	GPIOH->PUPDR &= ~(	GPIO_PUPDR_PUPDR1);

	//SET
	GPIOA->PUPDR |= (	GPIO_PUPDR_PUPDR4_0
					|	GPIO_PUPDR_PUPDR5_0
					|	GPIO_PUPDR_PUPDR6_0
					|	GPIO_PUPDR_PUPDR0_0		//A0 (is) <- A7 (was)
					|	GPIO_PUPDR_PUPDR8_0
					|	GPIO_PUPDR_PUPDR11_0
					|	GPIO_PUPDR_PUPDR12_0
					|	GPIO_PUPDR_PUPDR15_0);

	GPIOB->PUPDR |= (	GPIO_PUPDR_PUPDR0_0
					|	GPIO_PUPDR_PUPDR1_0
					|	GPIO_PUPDR_PUPDR2_0
					|	GPIO_PUPDR_PUPDR3_0
					|	GPIO_PUPDR_PUPDR4_0
					|	GPIO_PUPDR_PUPDR5_0
					|	GPIO_PUPDR_PUPDR6_0
					|	GPIO_PUPDR_PUPDR7_0
					|	GPIO_PUPDR_PUPDR8_0
					|	GPIO_PUPDR_PUPDR9_0
					|	GPIO_PUPDR_PUPDR10_0
					|	GPIO_PUPDR_PUPDR11_0
					|	GPIO_PUPDR_PUPDR12_0
					|	GPIO_PUPDR_PUPDR13_0
					|	GPIO_PUPDR_PUPDR14_0
					|	GPIO_PUPDR_PUPDR15_0);

	GPIOC->PUPDR |= (	GPIO_PUPDR_PUPDR0_0
					|	GPIO_PUPDR_PUPDR1_0
					|	GPIO_PUPDR_PUPDR2_0
					|	GPIO_PUPDR_PUPDR3_0
					|	GPIO_PUPDR_PUPDR4_0
					|	GPIO_PUPDR_PUPDR5_0
					|	GPIO_PUPDR_PUPDR6_0
					|	GPIO_PUPDR_PUPDR7_0
					|	GPIO_PUPDR_PUPDR10_0
					|	GPIO_PUPDR_PUPDR11_0
					|	GPIO_PUPDR_PUPDR12_0
					|	GPIO_PUPDR_PUPDR13_0
					|	GPIO_PUPDR_PUPDR14_0
					|	GPIO_PUPDR_PUPDR15_0);

	GPIOD->PUPDR |= 	GPIO_PUPDR_PUPDR2_0;

	GPIOH->PUPDR |= (	GPIO_PUPDR_PUPDR1_0);

}

void InpitsProcess(void)
{
	//inputs: 39pcs
	//PORT A: 4,5,6,7,8,11,12,15
	//PORT B: all
	//PORT C: all, except PC8, PC9
	//PORT D: 2
	//PORT H: 1 : forbidden
	uint8_t inputCounterBuffer[INPUTS_NUMBER + SPECIAL_INPUTS_NUMBER];
	uint8_t inputValueTempBuffer9, inputValueTempBuffer8, inputValueTempBuffer7,
			inputValueTempBuffer6, inputValueTempBuffer5, inputValueTempBuffer4;

	unsigned int i, k;
	for(i = 0; i < (INPUTS_NUMBER + SPECIAL_INPUTS_NUMBER); i++)
	{
		inputCounterBuffer[i] = 0;
	}
	for(i = 0; i < NUMBER_OF_INPUT_READ; i++)
	{


		if(GPIOA->IDR & GPIO_IDR_IDR_4)		{ inputCounterBuffer[0]++;	}
		if(GPIOA->IDR & GPIO_IDR_IDR_5)		{ inputCounterBuffer[1]++;	}
		if(GPIOA->IDR & GPIO_IDR_IDR_6)		{ inputCounterBuffer[2]++;	}
		if(GPIOA->IDR & GPIO_IDR_IDR_0)		{ inputCounterBuffer[3]++;	}		//A0 (is) <- A7 (was)
		if(GPIOA->IDR & GPIO_IDR_IDR_8)		{ inputCounterBuffer[4]++;	}
		if(GPIOA->IDR & GPIO_IDR_IDR_11)	{ inputCounterBuffer[5]++;	}
		if(GPIOA->IDR & GPIO_IDR_IDR_12)	{ inputCounterBuffer[6]++;	}
		if(GPIOA->IDR & GPIO_IDR_IDR_15)	{ inputCounterBuffer[7]++;	}

		if(GPIOB->IDR & GPIO_IDR_IDR_0)		{ inputCounterBuffer[8]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_1)		{ inputCounterBuffer[9]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_2)		{ inputCounterBuffer[10]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_3)		{ inputCounterBuffer[11]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_4)		{ inputCounterBuffer[12]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_5)		{ inputCounterBuffer[13]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_6)		{ inputCounterBuffer[14]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_7)		{ inputCounterBuffer[15]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_8)		{ inputCounterBuffer[16]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_9)		{ inputCounterBuffer[17]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_10)	{ inputCounterBuffer[18]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_11)	{ inputCounterBuffer[19]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_12)	{ inputCounterBuffer[20]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_13)	{ inputCounterBuffer[21]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_14)	{ inputCounterBuffer[22]++;	}
		if(GPIOB->IDR & GPIO_IDR_IDR_15)	{ inputCounterBuffer[23]++;	}

		if(GPIOC->IDR & GPIO_IDR_IDR_0)		{ inputCounterBuffer[24]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_1)		{ inputCounterBuffer[25]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_2)		{ inputCounterBuffer[26]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_3)		{ inputCounterBuffer[27]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_4)		{ inputCounterBuffer[28]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_5)		{ inputCounterBuffer[29]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_6)		{ inputCounterBuffer[30]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_7)		{ inputCounterBuffer[31]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_10)	{ inputCounterBuffer[32]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_11)	{ inputCounterBuffer[33]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_12)	{ inputCounterBuffer[34]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_13)	{ inputCounterBuffer[35]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_14)	{ inputCounterBuffer[36]++;	}
		if(GPIOC->IDR & GPIO_IDR_IDR_15)	{ inputCounterBuffer[37]++;	}

		if(GPIOD->IDR & GPIO_IDR_IDR_2)		{ inputCounterBuffer[38]++;	}

//		if(GPIOH->IDR & GPIO_IDR_IDR_1)		{ inputCounterBuffer[39]++;	}	//PORT H : forbidden
	}
	//подтяжка к питанию
	//при срабатывании сзначения меньше NUMBER_OF_INPUT_READ
	// NORMAL INPUTS PROCESS
	for(i = 0; i < INPUTS_NUMBER; i++)
	{
		if(inputCounterBuffer[i] < INPUT_READ_TRESHOLD)
		{
			inputCounterBuffer[i] = 1;
		}
		else
		{
			inputCounterBuffer[i] = 0;
		}
	}

	//SPECIAL INPUTS PROCESS
	//first sequence
	inputCounterBuffer[SPECIAL_INPUTS_SIN_FIRST] =
			FirstStateNumProcess(	inputCounterBuffer[1],		//PA5 (first)
									inputCounterBuffer[9],		//PB1 (second)
									inputCounterBuffer[10],		//PB2 (third)
									inputCounterBuffer[3],      //PA0 (other)
									inputCounterBuffer[0],		//PA4
									inputCounterBuffer[2],		//PA6
									inputCounterBuffer[8]);		//PB0
	//second sequence
	inputCounterBuffer[SPECIAL_INPUTS_SIN_SECOND] =
			SecondStateNumProcess(	inputCounterBuffer[3],      //PA0 (first)
									inputCounterBuffer[0],		//PA4 (second)
									inputCounterBuffer[2],		//PA6 (third)
									inputCounterBuffer[8],		//PB0 (fourth)
									inputCounterBuffer[1],		//PA5 ()
									inputCounterBuffer[9],		//PB1 ()
									inputCounterBuffer[10]);	//PB2 ()

	//third sequence (PIR sensor inputs process)
	inputCounterBuffer[SPECIAL_INPUTS_PIR_THIRD] =
			ThirdStateNumProcess(	inputCounterBuffer[18],		//PB10
									inputCounterBuffer[19],		//PB11
									inputCounterBuffer[20]);	//PB12



	k = 0;

	inputValueTempBuffer9 	= 0x80
							| inputCounterBuffer[k]
							| inputCounterBuffer[k+1] << 1
							| inputCounterBuffer[k+2] << 2
							| inputCounterBuffer[k+3] << 3
							| inputCounterBuffer[k+4] << 4
							| inputCounterBuffer[k+5] << 5
							| inputCounterBuffer[k+6] << 6;
	k += 7;

	inputValueTempBuffer8 	= 0x80
							| inputCounterBuffer[k]
							| inputCounterBuffer[k+1] << 1
							| inputCounterBuffer[k+2] << 2
							| inputCounterBuffer[k+3] << 3
							| inputCounterBuffer[k+4] << 4
							| inputCounterBuffer[k+5] << 5
							| inputCounterBuffer[k+6] << 6;
	k += 7;

	inputValueTempBuffer7 	= 0x80
							| inputCounterBuffer[k]
							| inputCounterBuffer[k+1] << 1
							| inputCounterBuffer[k+2] << 2
							| inputCounterBuffer[k+3] << 3
							| inputCounterBuffer[k+4] << 4
							| inputCounterBuffer[k+5] << 5
							| inputCounterBuffer[k+6] << 6;
	k += 7;

	inputValueTempBuffer6 	= 0x80
							| inputCounterBuffer[k]
							| inputCounterBuffer[k+1] << 1
							| inputCounterBuffer[k+2] << 2
							| inputCounterBuffer[k+3] << 3
							| inputCounterBuffer[k+4] << 4
							| inputCounterBuffer[k+5] << 5
							| inputCounterBuffer[k+6] << 6;
	k += 7;

	inputValueTempBuffer5 	= 0x80
							| inputCounterBuffer[k]
							| inputCounterBuffer[k+1] << 1
							| inputCounterBuffer[k+2] << 2
							| inputCounterBuffer[k+3] << 3
							| inputCounterBuffer[k+4] << 4
							| inputCounterBuffer[k+5] << 5
							| inputCounterBuffer[k+6] << 6;
	k += 7;

	inputValueTempBuffer4 	= 0x80
							| inputCounterBuffer[k]
							| inputCounterBuffer[k+1] << 1
							| inputCounterBuffer[k+2] << 2
							| inputCounterBuffer[k+3] << 3
							//SPECIAL_INPUTS
							| inputCounterBuffer[k+4] << 4  //PORT H : forbidden, inputCounterBuffer[k+4] now is special input (for PIR sensor)
							| inputCounterBuffer[k+5] << 5
							| inputCounterBuffer[k+6] << 6;

	if(inputValueTempBuffer4 != inputValueBuffer[4]) { inputValueBuffer[4] = inputValueTempBuffer4;	}
	if(inputValueTempBuffer5 != inputValueBuffer[5]) { inputValueBuffer[5] = inputValueTempBuffer5;	}
	if(inputValueTempBuffer6 != inputValueBuffer[6]) { inputValueBuffer[6] = inputValueTempBuffer6;	}
	if(inputValueTempBuffer7 != inputValueBuffer[7]) { inputValueBuffer[7] = inputValueTempBuffer7;	}
	if(inputValueTempBuffer8 != inputValueBuffer[8]) { inputValueBuffer[8] = inputValueTempBuffer8;	}
	if(inputValueTempBuffer9 != inputValueBuffer[9]) { inputValueBuffer[9] = inputValueTempBuffer9;	}
}


void IOInit(void)
{
	SpecialInputsProcessStateReset();
	SetResetCounterInit();
	ErraseInputBuffer();
	ErraseShiftRegister();
	OutputsInit();
	WatchDogReset(); //
	InputsInit();
	WatchDogReset(); //
	InpitsProcess();
	WatchDogReset(); //
	PushOutputData(0xFFFFFFFF, 0xFFFFFFFF);
	Tim6Init();
}

uint8_t FirstStateNumProcess(uint8_t chanel1, uint8_t chanel2, uint8_t chanel3, uint8_t chanel4, uint8_t chanel5, uint8_t chanel6, uint8_t chanel7)
{
	//FirstStateNumProcess
	static uint32_t timeoutDownCounterFirst = 0;
	static uint32_t stateFirst = 0;
	static uint8_t outValueFirst = INPUT_OFF;

	if(chanel1)
	{
		timeoutDownCounterFirst = SIN_DOWNCOUNTER_VALUE;
		stateFirst = 1;
	}

	if(timeoutDownCounterFirst)
	{
		if(chanel2)
		{
			if(stateFirst == 1)
			{
				timeoutDownCounterFirst = SIN_DOWNCOUNTER_VALUE;
				stateFirst = 2;
			}
		}
	}

	if(timeoutDownCounterFirst)
	{
		if(chanel3)
		{
			if(stateFirst == 2)
			{
				timeoutDownCounterFirst = SIN_DOWNCOUNTER_VALUE;
				stateFirst = 3;
				outValueFirst = INPUT_ON;
			}
		}
	}

	if(chanel4 || chanel5 || chanel6 || chanel7)
	{
		stateFirst = 0;
	}

	//decrement and state reset
	if(timeoutDownCounterFirst)
	{
		timeoutDownCounterFirst--;
	}
	else
	{
		stateFirst = 0;
		outValueFirst = INPUT_OFF;
	}

	return outValueFirst;
}

uint8_t SecondStateNumProcess(uint8_t chanel1, uint8_t chanel2, uint8_t chanel3, uint8_t chanel4, uint8_t chanel5, uint8_t chanel6, uint8_t chanel7)
{
	//SecondStateNumProcess
	static uint32_t timeoutDownCounterSecond = 0;
	static uint32_t stateSecond = 0;
	static uint8_t outValueSecond = INPUT_OFF;
	
	if(chanel1)
	{
		timeoutDownCounterSecond = SIN_DOWNCOUNTER_VALUE;
		stateSecond = 1;
	}

	if(timeoutDownCounterSecond)
	{
		if(chanel2)
		{
			if(stateSecond == 1)
			{
				timeoutDownCounterSecond = SIN_DOWNCOUNTER_VALUE;
				stateSecond = 2;
			}
		}
	}

	if(timeoutDownCounterSecond)
	{
		if(chanel3)
		{
			if(stateSecond == 2)
			{
				timeoutDownCounterSecond = SIN_DOWNCOUNTER_VALUE;
				stateSecond = 3;
			}
		}
	}

	if(timeoutDownCounterSecond)
	{
		if(chanel4)
		{
			if(stateSecond == 3)
			{
				timeoutDownCounterSecond = SIN_DOWNCOUNTER_VALUE;
				stateSecond = 4;
				outValueSecond = INPUT_ON;
			}
		}
	}

	if(chanel5 || chanel6 || chanel7)
	{
		stateSecond = 0;
	}

	//decrement and state reset
	if(timeoutDownCounterSecond)
	{
		timeoutDownCounterSecond--;
	}
	else
	{
		stateSecond = 0;
		outValueSecond = INPUT_OFF;
	}

	return outValueSecond;
}

void SetResetCounterInit(void)
{
	resetCouner = 0;
}

//срабатывает если в течении небольшого промежутка времени пропадёт сигнал на всех трёх
//входах (не обязательно одновременно, последовательность не важна) (разомкнутся три PIR
//датчика)
uint8_t ThirdStateNumProcess(uint8_t chanel1, uint8_t chanel2, uint8_t chanel3)
{
	//max timeout output is TRUE after event end
	
	//ThirdStateNumProcess
	static uint32_t timeoutDownCounterThird = 0;
	static uint8_t outValueThird = INPUT_OFF;
	static uint32_t chanelDelta1 = 0, chanelDelta2 = 0, chanelDelta3 = 0;

	//inverted 1
	if(!chanel1)
	{
		chanelDelta1 = PIR_CHANEL_DELTA_VALUE;
	}
	else
	{
		if(chanelDelta1)
		{
			chanelDelta1--;
		}
	}

	//inverted 2
	if(!chanel2)
	{
		chanelDelta2 = PIR_CHANEL_DELTA_VALUE;
	}
	else
	{
		if(chanelDelta2)
		{
			chanelDelta2--;
		}
	}

	//inverted 3
	if(!chanel3)
	{
		chanelDelta3 = PIR_CHANEL_DELTA_VALUE;
	}
	else
	{
		if(chanelDelta3)
		{
			chanelDelta3--;
		}
	}

	if((chanelDelta1 > 0) && (chanelDelta2 > 0) && (chanelDelta3 > 0))
	{
		outValueThird = INPUT_ON;
		timeoutDownCounterThird = PIR_DOWNCOUNTER_VALUE;
	}

	//decrement and state reset
	if(timeoutDownCounterThird)
	{
		timeoutDownCounterThird--;
	}
	else
	{
		outValueThird = INPUT_OFF;
	}

	return outValueThird;
}

void SpecialInputsProcessStateReset(void)
{
	//FirstStateNumProcess
	timeoutDownCounterFirst = 0;
	stateFirst = 0;
	outValueFirst = INPUT_OFF;

	//SecondStateNumProcess
	timeoutDownCounterSecond = 0;
	stateSecond = 0;
	outValueSecond = INPUT_OFF;

	//ThirdStateNumProcess
	timeoutDownCounterThird = 0;
	outValueThird = INPUT_OFF;
	chanelDelta1 = 0, chanelDelta2 = 0, chanelDelta3 = 0;
}
