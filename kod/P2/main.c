/**
  ******************************************************************************
  * @file    main.c
  * @author  Grupp 12
  * @version 2019-10-30
  * @brief   Denna fil innehåller funktioner för att hantera det som P2 sköter.
  *
  */
 
#include "system_stm32f4xx.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "misc.h"

#include "main.h"
#include "can12.h"

static volatile int counter;
static __IO uint32_t timingDelay;

static volatile int ultrasound_sensor_limit = 100;
static volatile int calibrate_flag = 0;

struct unit {
	int sensorId;	// Ett unikt id för noden
	int alarm;	// 1 om larmet gått, 0 annars
	int message_sent; // 1 om meddelande skickats till centralenheten, 0 annars
};

struct unit vibration_sensor;
struct unit ultrasound_sensor;

void startup(void) __attribute__((naked)) __attribute__((section (".start_section")) );

void startup ( void )
{
__asm volatile(
    " LDR R0,=0x2001C000\n"     // sätt stacken
    " MOV SP,R0\n"
    " BL main\n"                // anropa main
    "_exit: B .\n"              // returnera aldrig
    ) ;
}

/**
  * @brief  Nödvändiga initieringar för vibrationssensorn.
  * @param  None.
  * @retval None.
  */
void vibration_sensor_irq_handler(void)
{	
    vibration_sensor.alarm = 1;
    ack_vibration_sensor_interrupt();
}

/**
  * @brief  Nödvändiga initieringar för ultraljudsmätaren.
  * @param  None.
  * @retval None.
  */
void ultrasound_sensor_irq_handler(void)
{
    ultrasound_sensor.alarm = 1;
	ack_ultrasound_sensor_interrupt(); // Kvittera avbrott
}


/**
  * @brief  Avbrott för vibrationssensorn kvitteras.
  * @param  None.
  * @note   Vibrationssensorn är ansluten till PD0 och kopplad till
  *         avbrottslina EXTI 0.
  * @retval None.
  */
void ack_vibration_sensor_interrupt(void)
{
    ack_interrupt(EXTI_Line0);
}

/**
  * @brief  Avbrott kvitters för ultraljudsmätaren.
  * @param  None.
  * @note   Ultraljudsmätaren är ansluten till PD2 och kopplad till
  * 		avbrottslina EXTI 2.
  * @retval None
  */
void ack_ultrasound_sensor_interrupt(void) 
{
    ack_interrupt(EXTI_Line2);
}

/**
  * @brief  Avbrott kvitteras.
  * @param  None.
  * @retval None.
  */
void ack_interrupt(uint32_t EXTI_Line) 
{
    // Kvittera avbrottet genom att kolla flagstatus och nollställ flaggan.
    if (EXTI_GetFlagStatus(EXTI_Line)) {
        EXTI_ClearFlag(EXTI_Line);
    }
}

/**
  * TODO använd Lams delay-bibliotek
  */
void delay(__IO uint32_t nTime)
{ 
	timingDelay = nTime;

	while(timingDelay != 0);
}

/**
  * @brief  Avbrottsrutin för SysTick.
  * @note   Uppdatera `counter` och `TImingDelay`.
  */
void systick_irq_handler(void)  
{  
	counter++;     
	timingDelay--; 
}

/**
  * @brief  `ultrasound_sensor_limit` uppdateras.
  * @param  new_limit: Nytt maxvärde.
  * @retval None.
  */
void set_limit(int new_limit)
{
	unsigned char buff[16];
	itoa(new_limit, buff, 10);
	ultrasound_sensor_limit = new_limit;
	usart_send("New limit set: ");
	DUMP(buff);
	DUMP("");
}

/**
  * @brief  Triggar puls för ultraljudsmätaren.
  * @param  None.
  * @retval None.
  */
void trigger_pulse(void) 
{
    GPIO_SetBits(GPIOD, GPIO_Pin_1);
    delay(10);
    GPIO_ResetBits(GPIOD, GPIO_Pin_1);
}

/**
  * @brief  Distansen till närmsta föremål beräknas.
  * @param  microseconds: Antal mikrosekunder som ultraljudet rört sig under.
  * @note   Distansen, d(t), beräknas genom d(t) = (0.00034 * t).
  * @retval Distansen till närmsta föremål.
  */
int distance(unsigned int microseconds)
{
	int distance = 0.034*microseconds;
	return distance;
}

/**
  * @brief  Ultraljudsmätaren triggas och skillnaden av ekots längd jämfört
  * 		med en tidigare mätning beräknas.
  * @param  echo_time: Tidigare värde av ekotiden.
  * @retval Nytt värde av ekotiden.
  */
int ultrasound_emission(int last_echo_time)
{
	unsigned int diff_time;
	unsigned int diff_distance;
	int new_echo_time;

	unsigned char buff[16];
	trigger_pulse();
	while(!(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2)));	// Vänta på ekot
	counter = 0;
	while(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2));	// Vänta tills ekot är slut
	new_echo_time = counter/2;
	
	diff_distance = abs(distance(new_echo_time) - distance(last_echo_time)); // Beräkna skillnad i distans från tidigare mätning
	
	#ifdef DEBUG_MODE
	itoa(diff_distance, buff, 10);
	DUMP(buff);	// Printa diff_distans
	#endif // DEBUG_MODE
	
	if (ultrasound_sensor_limit <= diff_distance && diff_distance < 1000) {
		EXTI_GenerateSWInterrupt(EXTI_Line2);	// Generera avbrott
	}

	return new_echo_time;
}

/**
  * @brief  Skickar ett ACK för ping-meddelande till centralenheten.
  * @param  None.
  * @retval None.
  */
void ack_ping(void)
{
	CANMsg msg;
	msg.urgent = NOT_URGENT;
	msg.direction = TO_CENTRAL_UNIT;
	msg.msgId = ACK;
	msg.nodeId = nodeID;
	msg.length = 8;
	can_send(&msg);
	
	#ifdef DEBUG_MODE
	DUMP("Ping message acked");
	#endif // DEBUG_MODE
}

/**
  * @brief  Nödvändiga GPIO-initieringar för vibrationssensorn.
  * @param  None.
  * @retval None.
  */
void vibration_GPIO_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// Konfigurera PD0 till inport
    GPIO_InitStructure.GPIO_Pin = D0_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/**
  * @brief  Nödvändiga GPIO-initieringar för ultraljudsmätaren.
  * @param  None.
  * @retval None.
  */
void ultrasound_GPIO_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// Konfigurera PD1 till utport
    GPIO_InitStructure.GPIO_Pin = D1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    // Konfigurera PD2 till inport
    GPIO_InitStructure.GPIO_Pin = D2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/**
  * @brief  Nödvändiga initieringar för vibrationssensorn.
  * @param  None.
  * @retval None.
  */
void vibration_sensor_init(void) 
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	vibration_GPIO_init();
	
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource0);
	
	// Konfigurera EXTI Line 0
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // Avbrottsvektor
    *((void (**) (void)) EXTI0_IRQVEC) = vibration_sensor_irq_handler;
	
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	// Uppdatera structen
	vibration_sensor.sensorId = 1;
	vibration_sensor.alarm = 0;
	vibration_sensor.message_sent = 0;
}

/**
  * @brief  Nödvändiga initieringar för ultraljudsmätaren.
  * @param  None.
  * @retval None.
  */
void ultrasound_sensor_init(void) 
{
    EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    
	ultrasound_GPIO_init();
	
	// Configure EXTI Line2 
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
	
    // Avbrottsvektor
    *((void (**) (void)) EXTI2_IRQVEC) = ultrasound_sensor_irq_handler;
	
    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	// Uppdatera structen
	ultrasound_sensor.sensorId = 2;
	ultrasound_sensor.alarm = 0;
	ultrasound_sensor.message_sent = 0;
}

/**
  * @brief  Nödvändiga initieringar för SysTick.
  * @param  None.
  * @retval None.
  */
void systick_init(void)
{
    SysTick_Config(SystemCoreClock / 1000000); 	// Generera avbrott varje mikrosekund
	*((void (**) (void)) SYSTICK_IRQVEC) = systick_irq_handler;	// Sätt avbrottsvektor
}

/**
  * @brief  Sköter det som ska hända om enheten mottager ett ALARM-meddelande. 
  * @param  data: datan i buffern.
  * @retval None.
  */
 void receiver_ALARM(char data) {
	 if (data == 1) {
		vibration_sensor.alarm = 0;
		vibration_sensor.message_sent = 0;
		DUMP("Vibration sensor OK!");
	}
	else if (data == 2) {
		ultrasound_sensor.alarm = 0;
		ultrasound_sensor.message_sent = 0;
		calibrate_flag = 1;
		DUMP("Ultra sound sensor OK!");
	}
 }
 
 /**
  * @brief  Sköter det som ska hända om enheten mottager ett ACK-meddelande. 
  * @param  None.
  * @retval None.
  */
 void receiver_ACK(void) {
	// Gör saker
 }
 
/**
  * @brief  Sköter det som ska hända om enheten mottager ett CMND-meddelande. 
  * @param  None.
  * @retval None.
  */
 void receiver_CMND(void) {
	calibrate_flag = 1;
 }
 
/**
  * @brief  Sköter det som ska hända om enheten mottager ett CONF-meddelande. 
  * @param  data: datan i buffern.
  * @retval None.
  */
 void receiver_CONF(char data) {
	set_limit(data);
	calibrate_flag = 1;
 }
 
 /**
  * @brief  Sköter det som ska hända om enheten mottager ett REQST-meddelande. 
  * @param  data: datan i buffern.
  * @retval None.
  */
 void receiver_REQST(void) {
	ack_ping();
 }
 
 /**
  * @brief  Avbrottsrutin för CAN. 
  * @param  None.
  * @retval None.
  */
 void receiver(void) {
    CANMsg msg;
    if (can_receive(&msg)) {
		switch (msg.msgId) {
			case (ALARM):
				receiver_ALARM(msg.buff[7]);
				break;
			case (ACK):
				receiver_ACK();
				break;
			case (CMND):
				receiver_CMND();
				break;
			case (CONF):
				receiver_CONF(msg.buff[7]);
				break;
			case(REQST):
				receiver_REQST();
				break;
			default:
				DUMP("VARNING: Invalid message type");
				break;
		}
	}
    else
        DUMP("***Error: Something went wrong :(");
}

/**
  * @brief  Ultraljudsmätaren kalibreras.
  * @param  None.
  * @retval Ekotiden för ultraljudsmätaren.
  */
int calibrate_ultrasound_sensor(void) {
    trigger_pulse();
	while(!(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2)));	// Vänta på ekot
	counter = 0;
	while(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2));	// Vänta tills ekot är slut
    return counter/2;
}

/**
  * @brief  Nödvändiga initieringar för programmet.
  * @param  None.
  * @retval None.
  */
void app_init(void)
{	
	can1_init(receiver);
	vibration_sensor_init();
	ultrasound_sensor_init();
	systick_init();
}

/**
  * @brief  Centralenheten larmas om larm genereras av vibrationssensorn.
  * @param  None.
  * @retval 1 om CAN-meddelande lyckades skickas
  * 		0 annars
  */
int vibration_sensor_alert(void) {
	CANMsg msg;
	msg.urgent = NOT_URGENT;
	msg.direction = TO_CENTRAL_UNIT;
	msg.msgId = ALARM;
	msg.nodeId = nodeID;
	msg.buff[7] = vibration_sensor.sensorId;
	msg.length = 8;
	return can_send(&msg);
}

/**
  * @brief  Centralenheten larmas om larm genereras av ultraljudsmätaren.
  * @param  None.
  * @retval 1 om CAN-meddelande lyckades skickas
  * 		0 annars
  */
int ultrasound_sensor_alert(void) {
	CANMsg msg;
	msg.urgent = NOT_URGENT;
	msg.direction = TO_CENTRAL_UNIT;	// Till centralenheten
	msg.msgId = ALARM;
	msg.nodeId = nodeID;
	msg.buff[7] = ultrasound_sensor.sensorId;
	msg.length = 8;
	return can_send(&msg);
}

/**
  * @brief  Huvudprogrammet för P2.
  * @param  None.
  * @retval None.
  */
void main(void)
{	
	int echo_time;
	
	// Gör nödvändiga initieringar
	app_init();
	
	// Kalibrera ultraljudsmätaren
	echo_time = calibrate_ultrasound_sensor();
	
	while(1)
    {
		if (vibration_sensor.alarm && !vibration_sensor.message_sent) {
			vibration_sensor_alert();
			vibration_sensor.message_sent = 1;
			
			#ifdef DEBUG_MODE
			DUMP("Vibration sensor: Message sent.");
			#endif // DEBUG_MODE
		}

		if (!ultrasound_sensor.alarm && !calibrate_flag) {
			echo_time = ultrasound_emission(echo_time);
		} else if(ultrasound_sensor.alarm && !ultrasound_sensor.message_sent) {
			ultrasound_sensor.message_sent = ultrasound_sensor_alert();
			
			#ifdef DEBUG_MODE
			DUMP("Ultrasound sensor: Message sent.");
			#endif // DEBUG_MODE
		}
		
		if(calibrate_flag){
			echo_time = calibrate_ultrasound_sensor();
			calibrate_flag = 0;
			DUMP("Ultrasound sensor calibrated!");
		}
		
		delay(250000);	// Vänta 0,25 sekunder
    }
}
