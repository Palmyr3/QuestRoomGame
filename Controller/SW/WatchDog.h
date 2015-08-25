#ifndef WATCHDOG_CONTROL
#define WATCHDOG_CONTROL

#include "stm32l1xx.h"
#include "types_P.h"

#define LSI_TIMOUT_PAL	4000000
#define RELOAD_TIMOUT	1000

#define IWDG_RESET_KEY	0xAAAA
#define IWDG_ACSESS_KEY	0x5555
#define IWDG_START_KEY	0xCCCC

ErrorStateEnum LsiInit(void);
void WatchDogInit(void);
void inline WatchDogReset(void);

#endif
