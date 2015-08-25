#ifndef IO_CONTROL
#define IO_CONTROL

#include "stm32l1xx.h"
#include "types_P.h"
#include "WatchDog.h"

#define RESET_COUNTER_VALUE		60
#define SHIFT_REG_DEFAULT_VALUE	(uint32_t)0xFFFFFFFF


#ifndef SHIFT_REGISTER_SIZE
#define SHIFT_REGISTER_SIZE		2
#endif
#ifndef INPUT_VALIE_BUFF_SIZE
#define INPUT_VALIE_BUFF_SIZE	10
#endif

#define INPUTS_NUMBER			39
#define SPECIAL_INPUTS_NUMBER	3

#define SPECIAL_INPUTS_PIR_THIRD	39
#define SPECIAL_INPUTS_SIN_FIRST	40
#define SPECIAL_INPUTS_SIN_SECOND	41

#define INPUT_ON	1
#define INPUT_OFF	0


#define NUMBER_OF_INPUT_READ	60 
#define INPUT_READ_TRESHOLD		8 


#define SIN_DOWNCOUNTER_VALUE	400
#define PIR_DOWNCOUNTER_VALUE	500
#define PIR_CHANEL_DELTA_VALUE	500

#define LATCH_DELAY_VALUE		700

void OutputsInit(void);
void InputsInit(void);
void ErraseInputBuffer(void);
void ErraseShiftRegister(void);
void PushOutputData(uint32_t word1, uint32_t word0);
void InpitsProcess(void);

//грехи 1 (3 элемента)
uint8_t FirstStateNumProcess(uint8_t chanel1, uint8_t chanel2, uint8_t chanel3, uint8_t chanel4, uint8_t chanel5, uint8_t chanel6, uint8_t chanel7);
//грехи 2 (4 элемента)
uint8_t SecondStateNumProcess(uint8_t chanel1, uint8_t chanel2, uint8_t chanel3, uint8_t chanel4, uint8_t chanel5, uint8_t chanel6, uint8_t chanel7);
//pir-sensor (3 элемента)
uint8_t ThirdStateNumProcess(uint8_t chanel1, uint8_t chanel2, uint8_t chanel3);

void IOInit(void);

void Tim6Init(void);

void Tim6InterruptRoutine(void);

void SetResetCounterInit(void);

void SpecialInputsProcessStateReset(void);

#endif
