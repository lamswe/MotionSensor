#ifndef MAIN_H
#define MAIN_H

// ---------------- CAN -----------------

typedef void (*VoidFunction)(void);

typedef struct {
	unsigned char Urgent;
	unsigned char Direction;
	unsigned char MsgID;
	unsigned char NodeID;
	unsigned char Length;
	unsigned char Buff[8];
} CANMsg;

typedef void (*VoidFunction)(void);


// Meddelande-fält
#define URGENT 				1
#define NOT_URGENT 			0
#define TO_CENTRAL_UNIT 	0
#define FROM_CENTRAL_UNIT 	1

// Meddelande-typer
#define ALARM 	0b000
#define ACK 	0b0001
#define CMND 	0b0010
#define CONF 	0b0011
#define REQST 	0b0100

// Timers
#define TIMER_LOCAL 	0x0
#define	TIMER_CENTRAL	0x1

#define ACTIVATE		0x1
#define DEACTIVATE		0x0


// --------------- USART ----------------

typedef struct USART {
    volatile unsigned short sr;
    volatile unsigned short Unused0;
    volatile unsigned short dr;
    volatile unsigned short Unused1;
    volatile unsigned short brr;
    volatile unsigned short Unused2;
    volatile unsigned short cr1;
    volatile unsigned short Unused3;
    volatile unsigned short cr2;
    volatile unsigned short Unused4;
    volatile unsigned short cr3;
    volatile unsigned short Unused5;
    volatile unsigned short gtpr;
} USART;

#define USART1  ((USART *) 0x40011000) 


// --------------- Doors ----------------

// Lägen för dörren
typedef enum
{ 
	Door_OPEN = 0,
	Door_CLOSED
}Door_State;

// Lägen för larmet
typedef enum
{
	Alarm_ON = 1,
	Alarm_OFF
}Alarm_State;

// Status för larmet
typedef enum
{
	Alarm_ACTIVE = 1,
	Alarm_INACTIVE = 0
}Alarm_Status;

// Meddelande sänt eller inte
typedef enum
{
	Message_State_SENT = 1,
	Message_State_NOT_SENT
}Message_State;

// Struct för en dörr
typedef struct
{
	uint8_t	Unit_ID;
	uint8_t Door_ID;
	Door_State Door_Status;
	Alarm_Status Alarm_Setting;
	Alarm_State Alarm_Local;
	Alarm_State Alarm_Central;
	uint32_t Timer_Local;
	uint32_t Timer_Central;
	uint32_t Countup;
	Message_State Message_Status;
}Door;



#endif
