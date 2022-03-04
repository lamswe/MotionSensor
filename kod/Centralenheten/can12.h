#ifndef CAN_H

// Include from standard liberary 
#define		nodeID		0x2
#define		nodeMask	0x200
// Define priority and CAN_irq_vector
#define __IRQ_PRIORITY		2
#define	CAN1_IRQ_VECTOR		(0x2001C000+0x90)
#define __ENABLED_PRIORITY	3

// Meddelande-fält
#define URGENT 1
#define NOT_URGENT 0
#define TO_CENTRAL_UNIT 0
#define FROM_CENTRAL_UNIT 1

// Meddelande-typer
#define ALARM 0b000
#define ACK 0b0001
#define CMND 0b0010
#define CONF 0b0011
#define REQST 0b0100

typedef void (*VoidFunction)(void);


// Define how a message is presented
typedef struct {
	unsigned char urgent; // 0 for urgent message, 1 otherwise
	unsigned char direction; // 0 for sending TO central unit, 1 otherwise
	unsigned char msgId;  //Valid values: 0-127
	unsigned char nodeId; //Valid values: 0-15
	unsigned char length; //Valid values: 0-8
	unsigned char buff[8]; // A message carries at most 8 bytes of data
} CANMsg;

typedef void (*VoidFunction)(void);

// Exported CAN functions
void can1_init(VoidFunction interrupt);
int can_receive(CANMsg *msg);
int can_send(CANMsg *msg);

// Some extra functions for USART
void _outchar(char c);
char _tstchar( void );
char _getchar( void );
void usart_send(char *s);
void DUMP(char* s);

#endif
