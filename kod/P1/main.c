/*
 * main.c
 * Huvudprogram för dörrlarm
 */

#include <stdint.h>
#include "misc.h"
#include "stm32f4xx.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"

#include "main.h"
#include "settings.h"


// Deklarera dörr-array
Door doors[8];


// --------------- USART ----------------

void _outchar( char c ) {
    /* write character to usart1 */
    while (( USART1->sr & 0x80)==0);
    USART1->dr = (unsigned short) c;
    if( c == '\n')
            _outchar('\r');
}

char _tstchar(void) {
    /* see if character arrived at usart1,
        if not, return 0
        else return character
        */
    if( (USART1->sr & 0x20)==0)
        return 0;
    return (char) USART1->dr;
}

char _getchar(void) {
    /* wait until character arrived at usart1,
        return character
        */
    while( (USART1->sr & 0x20)==0)
        ;
    return (char) USART1->dr;
}

void usart_send(char* s){
   while (*s != '\0')
       _outchar(*(s++));
}

void DUMP(char *s) {
   usart_send(s);
   _outchar('\n');
}

// ---------------- CAN -----------------

CanRxMsg RxMessage;
CanTxMsg TxMessage;

// Avbrottsrutin för CAN
void can1_irq_handler(void)
{
	#ifdef DEBUG_MODE
	DUMP("Message from control center: ");
	#endif
	
	CANMsg msg;
	if(can_receive(&msg)){
		switch(msg.MsgID){
			case(ALARM):			
				door_alarm_reset(msg.Buff[7]-1);
				usart_send("Dismiss alarm at door #");
				_outchar(msg.Buff[7] +'0');
				#ifdef DEBUG_MODE
				DUMP("!\n");
				#endif
				break;
			case(ACK):
				#ifdef DEBUG_MODE
				DUMP("ACK\n");
				#endif
				break;
			case(CMND):
				#ifdef DEBUG_MODE
				DUMP("New command from central unit\n");
				#endif
				activate_deactivate_door_alarm(msg.Buff[6], msg.Buff[7]);
				usart_send("Deactivate/Activate door #");
				_outchar(msg.Buff[6]+1+'0');
				DUMP("!");
				break;
			case(CONF):
				#ifdef DEBUG_MODE
				DUMP("New configuration\n");
				#endif
				if(msg.Buff[5] == 'n'){
				}else if(msg.Buff[5] == TIMER_LOCAL){
					change_timer_local(msg.Buff[6], msg.Buff[7]);
					usart_send("New timer_local for door #");
					_outchar(msg.Buff[6]+1+'0');
					#ifdef DEBUG_MODE
					DUMP("!\n");
					#endif
				}
				else {
					change_timer_central(msg.Buff[6], msg.Buff[7]);
					usart_send("New timer_central for door #");
					_outchar(msg.Buff[6]+1+'0');
					DUMP("!\n");
				}
				break;
			case(REQST):
				#ifdef DEBUG_MODE
				DUMP("PING\n");
				#endif
				ack_ping();
				break;
			default:
				#ifdef DEBUG_MODE
				DUMP("UNKNOWN MESSAGE\n");
				#endif
				break;
		}
	}
}

// Initierar CAN
void can_init(VoidFunction can1_irq_handler)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
	CAN_InitTypeDef CAN_InitStructure;


    GPIO_StructInit(&GPIO_InitStructure);

    GPIO_DeInit(GPIOB);
    CAN_DeInit(CAN1);
	
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // Anslut CAN1 till AF. PB9 - CAN1 TX, PB8 - CAN1 RX
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1);  	
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);  

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);	

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);	

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    
    // Initiering CAN-filter
    CAN_FilterInitStructure.CAN_FilterNumber = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = NODE_ID<<5;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = (NODE_MASK<<5); 
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
    
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;

    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq; 
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq; 
    CAN_InitStructure.CAN_Prescaler = 7; // 750kbps

    if (CAN_Init(CAN1, &CAN_InitStructure) == CAN_InitStatus_Failed){
		#ifdef DEBUG_MODE
        DUMP("");
		DUMP("CAN initialisation failed\n");
		#endif
	} else {
		#ifdef DEBUG_MODE
        DUMP("CAN initiated\n");
		#endif
	}
		
	// Sätt avbrottsrutin
    *((void (**)(void) ) CAN1_IRQ_VECTOR) = can1_irq_handler;
    
	// Aktivera avbrott på FMP0
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
	
	// Konfigurera NVIC
	NVIC_SetPriority(CAN1_RX0_IRQn, CAN1_IRQ_PRIORITY);
    NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

// Skickar alarm till centralenheten
int send_alarm(int i)
{
	#ifdef DEBUG_MODE
	DUMP("Sending alarm to C\n");
	#endif
	
	CANMsg msg;
	msg.Urgent = NOT_URGENT;
	msg.Direction = TO_CENTRAL_UNIT;
	msg.MsgID = ALARM;
	msg.NodeID = NODE_ID;
	msg.Buff[7] = doors[i].Door_ID;
	_outchar(msg.Buff[7]+'0');
	msg.Length = 8;
	return can_send(&msg);
}

int can_send(CANMsg *msg){
	#ifdef DEBUG_MODE
	DUMP("Sending CAN message\n");
	#endif
	
    unsigned char index;
    CAN_TypeDef* canport = CAN1;
    CanTxMsg TxMessage;
    uint8_t TransmitMailbox = 0;
    //set the transmit ID, standard identifiers are used, combine IDs
    TxMessage.StdId = (msg->Urgent<<10) | (msg->Direction<<9) | (msg->MsgID<<5) | msg->NodeID;
	TxMessage.RTR = CAN_RTR_Data;
    TxMessage.IDE = CAN_Id_Standard;
    if (msg->Length > 8) 
        msg->Length = 8; 
    TxMessage.DLC = msg->Length; // set number of bytes to send
	
	
    for (index = 0; index < msg->Length; index++) {
        TxMessage.Data[index] = msg->Buff[index]; //copy data to Buffer
    }

    TransmitMailbox = CAN_Transmit(canport, &TxMessage);

    if (TransmitMailbox == CAN_TxStatus_NoMailBox) {
        #ifdef DEBUG_MODE
		DUMP("CAN TX buffer full\n");
		#endif
        return 0;
    }
    while (CAN_TransmitStatus(canport, TransmitMailbox) == CAN_TxStatus_Pending) ;
    
    if (CAN_TransmitStatus(canport, TransmitMailbox) == CAN_TxStatus_Failed) {
        #ifdef DEBUG_MODE
		DUMP("CAN TX failed\n");
		#endif
        return 0;
    }    
    return 1;
}

int can_receive(CANMsg *msg){
    unsigned char index;
    CanRxMsg RxMessage;
    if (CAN_GetFlagStatus( CAN1, CAN_FLAG_FMP0) != SET)  // Data received in FIFO0
        return 0;
	
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
	msg->Urgent = (RxMessage.StdId >> 10) & 0x01;
	msg->Direction = (RxMessage.StdId >> 9) & 0x01;
    msg->MsgID  = (RxMessage.StdId >> 5) & 0x0F;
    msg->NodeID = RxMessage.StdId & 0x1F;
    msg->Length = (RxMessage.DLC & 0x0F);
	
    for (index = 0; index < msg->Length; index++) {
        // Get received data
        msg->Buff[index] = RxMessage.Data[index];
    }
    return 1;
}

void ack_ping(void)
{
	CANMsg msg;
	msg.Urgent = NOT_URGENT;
	msg.Direction = TO_CENTRAL_UNIT;
	msg.MsgID = ACK;
	msg.NodeID = NODE_ID;
	msg.Length = 8;
	can_send(&msg);
	#ifdef DEBUG_MODE
	DUMP("Ping message ACKed\n");
	#endif
}


// --------------- NVIC -----------------

// Initierar avbrottshantering
void nvic_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	// Aktivera NVIC och sätt prioriteter
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	// Initiera NVIC för EXTI
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
    NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_Init(&NVIC_InitStructure);
	
	#ifdef DEBUG_MODE
	DUMP("NVIC initiated\n");
	#endif
}


// --------------- LEDS -----------------

void leds_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// Konfigurera pins för två bargraphs
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |
								  GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 |
								  GPIO_Pin_6 | GPIO_Pin_7 |	GPIO_Pin_8 |
								  GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 |
								  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 |
								  GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(BAR_PORT, &GPIO_InitStructure);
	#ifdef DEBUG_MODE
	DUMP("LEDs initiated\n");
	#endif
}


// -------------- Systick ---------------

// Avbrottsrutinen jämför dörrens räknare med timers för lokalt och centralt
// larm och sätter på larmen om räknaren har överstigit värdena
void systick_irq_handler(void)
{
	for (int i = 0; i < 8; i++) {
		if (doors[i].Alarm_Setting == Alarm_ACTIVE && doors[i].Door_Status == Door_OPEN) {
			doors[i].Countup++;
			if (doors[i].Countup >= doors[i].Timer_Local && doors[i].Alarm_Local == Alarm_OFF) {
				doors[i].Alarm_Local = Alarm_ON;
			} else if (doors[i].Countup >= doors[i].Timer_Central && doors[i].Alarm_Central == Alarm_OFF) {
				doors[i].Alarm_Central = Alarm_ON;
			}
		}
	}
}

// Initierar systick, genererar ett avbrott per sekund
void init_systick(void)
{
	*((void (**) (void)) SYSTICK_IRQ_VECTOR) = systick_irq_handler;
    SysTick_Config(SYS_CLOCK/100);
	#ifdef DEBUG_MODE
	DUMP("Systick initiated\n");
	#endif
}

// --------------- Doors ----------------

// Initiera dörrar med standard-värden
void init_doors(void)
{
	for (uint8_t d = 0; d < 8; d++) {
		doors[d].Unit_ID = NODE_ID;
		doors[d].Door_ID = d+1;
		doors[d].Door_Status = Door_CLOSED;
		doors[d].Alarm_Setting = Alarm_INACTIVE;
		doors[d].Alarm_Local = Alarm_OFF;
		doors[d].Alarm_Central = Alarm_OFF;
		doors[d].Timer_Local = 300;
		doors[d].Timer_Central = 700;
		doors[d].Countup = 0;
		doors[d].Message_Status = Message_State_NOT_SENT;
		#ifdef DEBUG_MODE
		DUMP("Doors initiated\n");
		#endif
	}
}

// Återställer alarmens status
void door_alarm_reset(uint8_t d)
{
	doors[d].Alarm_Local = Alarm_OFF;
	doors[d].Alarm_Central = Alarm_OFF;
	doors[d].Countup = 0;
	doors[d].Message_Status = Message_State_NOT_SENT;
	#ifdef DEBUG_MODE
	DUMP("Door alarm status reset\n");
	#endif
}

void change_timer_local (uint8_t d, uint8_t timer)
{
	doors[d].Timer_Local = timer*100;
}

// Ändrar tidsintervall innan centralt larm går av. 
// timer räknas i sekund
void change_timer_central (uint8_t d, uint8_t timer)
{
	doors[d].Timer_Central = timer*100;
}

void activate_deactivate_door_alarm (uint8_t d, uint8_t setting)
{
//	doors[d].Alarm_Setting ~= doors[d].Alarm_Setting;
	doors[d].Alarm_Setting = setting;
	#ifdef DEBUG_MODE
	if (setting) {
		DUMP("Door alarm activated\n");
	} else {
		DUMP("Door alarm deactivated\n");
	}
	#endif
}


// ------------ Doorsensors -------------

// Avbrottsrutinen läser GPIO och sätter korrekt dörrstatus, samt återställer
// dörrens timers då dörren har stängts
void doorsensors_irq_handler(void)
{
	#ifdef DEBUG_MODE
	DUMP("Interrupt request from doorsensor\n");
	#endif
	uint16_t sensor_state = GPIO_ReadInputData(SENSOR_PORT);

	for(int i = 0; i < 8; i++) {
		if(sensor_state & 1 << i && doors[i].Alarm_Setting == Alarm_ACTIVE) {
			doors[i].Door_Status = Door_OPEN;
			#ifdef DEBUG_MODE
			DUMP("Door open\n");
			#endif
		} else {
			doors[i].Door_Status = Door_CLOSED;
			door_alarm_reset(i);
			#ifdef DEBUG_MODE
			DUMP("Door closed\n");
			#endif
		}
	}
	EXTI_ClearFlag(EXTI_Line0|EXTI_Line1|EXTI_Line2|EXTI_Line3|
				   EXTI_Line4|EXTI_Line5|EXTI_Line6|EXTI_Line7);
}

// Initierar GPIO och EXTI för dörrsensorer
void doorsensors_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	
	// Aktivera klocka för GPIO
    RCC_AHB1PeriphClockCmd(SENSOR_RCC, ENABLE);
	// Aktivera klocka för SYSCFG
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	// Konfigurera pins för dörrsensorer
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |
								   GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 |
								   GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(SENSOR_PORT, &GPIO_InitStructure);
	
	// Koppla avbrottslinor till dörrsensor-pins
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource0);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource1);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource2);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource3);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource4);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource5);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource6);
	SYSCFG_EXTILineConfig(SENSOR_PORTSOURCE, EXTI_PinSource7);
	
	// Konfigurera avbrottslinor
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    // Generera avbrott både på positiv och negativ flank
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0|EXTI_Line1|EXTI_Line2|EXTI_Line3|
								   EXTI_Line4|EXTI_Line5|EXTI_Line6|EXTI_Line7;	
    EXTI_Init(&EXTI_InitStructure);
	
	// Sätt avbrottsvektorer
	*((void (**) (void)) EXTI0_IRQ_VECTOR) = doorsensors_irq_handler;
	*((void (**) (void)) EXTI1_IRQ_VECTOR) = doorsensors_irq_handler;
	*((void (**) (void)) EXTI2_IRQ_VECTOR) = doorsensors_irq_handler;
	*((void (**) (void)) EXTI3_IRQ_VECTOR) = doorsensors_irq_handler;
	*((void (**) (void)) EXTI4_IRQ_VECTOR) = doorsensors_irq_handler;
	*((void (**) (void)) EXTI9_5_IRQ_VECTOR) = doorsensors_irq_handler;
	
	#ifdef DEBUG_MODE
	DUMP("Doorsensors initiated\n");
	#endif
}


// --------------- Main -----------------

// Assembler-direktiv för att relokera avbrottsvektorer i MD407
void startup(void) __attribute__((naked)) __attribute__((section (".start_section")) );
void startup ( void )
{
__asm volatile(
    " LDR R0,=0x2001C000\n"     // set stack 
    " MOV SP,R0\n"
    " BL main\n"                // call main 
    "_exit: B .\n"              // never return 
    ) ;
}


// Huvudprogram för dörrlarm
void main(void)
{ 
	DUMP("");
	DUMP("--------------- Starting P1! ---------------");
	uint8_t alarm_setting;
	uint16_t led_bargraph_local;
	
	doorsensors_init();
	leds_init();
	init_doors();
	init_systick();
	nvic_init();
	can_init(can1_irq_handler);
	GPIO_Write(BAR_PORT, 0x0);

	alarm_setting = 0;
	led_bargraph_local = 0;
	GPIO_Write(BAR_PORT, led_bargraph_local);
	
	#ifdef DEBUG_MODE
	DUMP("Initiations finished\n");
	#endif

	while(1) {
		for(int i = 0; i < 8; i++){
			if(doors[i].Alarm_Setting == Alarm_INACTIVE){
			
				led_bargraph_local |= (1<<(i+8));
				
			} else {
				led_bargraph_local &= ~(1<<(i+8));
			}
			if(doors[i].Alarm_Local == Alarm_ON) {
				led_bargraph_local |= (1<<i);
				
			} else {
				led_bargraph_local &= ~(1<<i);
				
			}
			if(doors[i].Alarm_Central == Alarm_ON && doors[i].Message_Status == Message_State_NOT_SENT){
				doors[i].Message_Status = send_alarm(i);
			}
			GPIO_Write(BAR_PORT, led_bargraph_local);
		}
	}
}
