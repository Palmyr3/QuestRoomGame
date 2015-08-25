#include "WatchDog.h"

ErrorStateEnum LsiInit(void)
{
	uint32_t timeoutInit = 0;
	//try to use LSI (RC 37kHz)
	RCC->CSR |= RCC_CSR_LSION;
	do
	{
		timeoutInit++;
	}
	while (!((RCC->CSR & RCC_CSR_LSIRDY)||(timeoutInit > LSI_TIMOUT_PAL)));
	if(timeoutInit > LSI_TIMOUT_PAL)
	{
		return ERROR_P;
	}
	else
	{
		return SUCCESS_P;
	}
}

void WatchDogInit(void)
{
	uint32_t timeoutInit = 0;
	if(LsiInit() == SUCCESS_P)
	{
		IWDG->KR = IWDG_ACSESS_KEY;
		do
		{
			timeoutInit++;
		}
		while ((IWDG->SR & IWDG_SR_PVU)&&(timeoutInit < LSI_TIMOUT_PAL));
		//Prescaler divider = 37Khz/32
		IWDG->PR = 0x3;
		timeoutInit = 0;
		do
		{
			timeoutInit++;
		}
		while ((IWDG->SR & IWDG_SR_RVU)&&(timeoutInit < LSI_TIMOUT_PAL));
		//Counter value ~3 seconds, if divider = /32
		IWDG->RLR = 0xfff;
		//start
		IWDG->KR = IWDG_START_KEY;
	}
}

void inline WatchDogReset(void)
{
	IWDG->KR = IWDG_RESET_KEY;
}
