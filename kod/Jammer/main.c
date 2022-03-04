/*
 * 
 * 		JAMMER
 * 		Used to test network's handling of spam
 * 		
 * 		Lägg till dessa filer i projekten:
 * 			can12.c och can12.h (hämtas på github under koder/Jammer)
 * 			usart12.c (hämtas på github under koder/Jammer)
 * 			misc.c
 * 			stm32f4xx_can.c
 * 			stm32f4xx_gpio.c
 * 			stm32f4xx_rcc.c
 * 			stm32f4xx_syscfg.c
 * 
 * // cirka 55 meddelande med 8 bytes data per sekund
 * */
//#define TEST

// #include <stdint.h>
#include "can12.h"
#include "misc.h"


CANMsg msg;

static __IO uint32_t TimingDelay = 1000000;
unsigned char testID = 0x0;
static unsigned int test_status = 1;	// 1 för aktiv 
uint32_t spam_counter;

//------------------------SYSTICK--------------------------
void interrupt(void)
{
	if(TimingDelay != 0x0){
		TimingDelay--;
	}else{
		test_status = 0;
	}
}
//---------------------------------------------------------

void copy_message(CANMsg new_msg) {
	if(new_msg.StdId < msg.StdId){
		msg.StdId = new_msg.StdId;
		DUMP("Copied message!");
		DUMP("Start jamming!");
		msg.buff[0] = 'L';
		msg.buff[1] = 'O';
		msg.buff[2] = 'L';
		msg.buff[3] = '\0';
		msg.length = 4;
	}
}

void receiver(void) {
	DUMP("CAN message received.");
	CANMsg new_msg;
	if (can_receive(&new_msg)){
		copy_message(new_msg);
		jam_CAN();
	} else
        DUMP("***Error: Something went wrong :(");
}

void jam_CAN () {
	while(1)
		can_send(&msg);
}

#ifdef TEST
void test_network_capability(void){
		msg.StdId = testID;
		msg.buff[0] = 'F';
		msg.buff[1] = 'F';
		msg.buff[2] = 'F';
		msg.buff[3] = 'F';
		msg.buff[4] = 'F';
		msg.buff[5] = 'F';
		msg.buff[6] = 'F';
		msg.buff[7] = 'F';
		msg.length = 8;
		while(test_status){
			can_send(&msg);
			DUMP("Message sent!");
			spam_counter++;
		}
//		for(int i = 0; i < 10000; i ++)
//		{
//			can_send(&msg);
//		}
}
#endif	

void main()
{
	DUMP("\n----------------------Starting Jammer---------------------");
	can1_init(receiver);
	systick_init(interrupt);
#ifdef TEST	
	test_network_capability();
#endif
	DUMP("-----------------------------Stop-------------------------");
}

void startup(void) __attribute__((naked)) __attribute__((section (".start_section")) );

void startup ( void )
{
__asm volatile(
	" LDR R0,=0x2001C000\n"		/* set stack */
	" MOV SP,R0\n"
	" BL main\n"				/* call main */
	"_exit: B .\n"				/* never return */
	) ;
}

