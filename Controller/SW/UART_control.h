#ifndef UART_CONTROL
#define UART_CONTROL

#include "stm32l1xx.h"
#include "types_P.h"

#define ALTER_FUNC_7 0x77777777

#define TRANSMIT_BUFF_SIZE		(10+1+1+1+1)
#define RECIEVE_BUFF_SIZE		(10+1+1+1+1)

#ifndef SHIFT_REGISTER_SIZE
#define SHIFT_REGISTER_SIZE		2
#endif

#ifndef INPUT_VALIE_BUFF_SIZE
#define INPUT_VALIE_BUFF_SIZE	10
#endif

#define PACKAGE_START	0x7F
#define PACKAGE_STOP	0x05
#define DATA_MULT		0x80

#define PAC_PARITY_BYTE		12
#define PAC_STOP_BYTE		13
#define PAC_COMAND_BYTE		1
#define PAC_START_BYTE		0
#define PAC_FIRST_DATA_BYTE	2
#define PAC_LAST_DATA_BYTE	11

#define COMAND_UPDATE_OUPUTS		0xF0
#define COMAND_READ_INPUTS			0xF1
#define COMAND_SEND_DEVICE_ID		0xF2

#define COMAND_UPDATE_LEDS			0xF3

#define DEVICE_ID		0x81


ErrorStateEnum UART1_Init();

void TransmitBufferInterruptRoutine(void);
void RecieveBufferInterruptRoutine(void);

void TransmitBufferStart(void);
void TransmitBufferErrase(void);
void inline TransmitBufferSetMagicNumbers(void);

ErrorStateEnum PackageParse(void);

void CreateDeviceIdPackage(void);
void CreateInputValuePackage(void);
void UpdateShiftRegister(void);


#endif
