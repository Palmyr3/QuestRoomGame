#include "stm32l1xx.h"
#include "types_P.h"
#include "IO_control.h"
#include "UART_control.h"
#include "WatchDog.h"

void Delay(uint32_t i)
{
	for(; i > 1; i --)
	{
		__NOP();
	}
}

void LedInit(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

	GPIOC->MODER &= ~GPIO_MODER_MODER1;

	GPIOC->MODER &= ~(	GPIO_MODER_MODER9
					 |	GPIO_MODER_MODER8);
	GPIOC->MODER |=  (	GPIO_MODER_MODER9_0
					 |	GPIO_MODER_MODER8_0);
	GPIOC->OTYPER &= ~(	GPIO_OTYPER_IDR_9
					  |	GPIO_OTYPER_IDR_8);
	GPIOC->OSPEEDR |= (	GPIO_OSPEEDER_OSPEEDR9
					  |	GPIO_OSPEEDER_OSPEEDR8); //high frequency
	GPIOC->PUPDR &= ~(	GPIO_PUPDR_PUPDR9
					 |	GPIO_PUPDR_PUPDR8);
}

void GreenLedChangeState(void)
{
	GPIOC->ODR ^= GPIO_ODR_ODR_9;
}

void BlueLedChangeState(void)
{
	GPIOC->ODR ^= GPIO_ODR_ODR_8;
}

int main(void)
{
	LedInit();
	WatchDogInit();
	TransmitBufferErrase();
	IOInit();
	WatchDogReset();
	UART1_Init();
	WatchDogReset();

	while(1)
	{
		GreenLedChangeState();
		Delay(1000000);
		WatchDogReset();
	}
}
