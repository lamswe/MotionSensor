/**
  ******************************************************************************
  * @file    main.c
  * @author  Grupp 12
  * @version 2019-10-30
  * @brief   Denna fil innehåller funktioner för att hantera det som
  *	       	 centralenheten sköter.
  *
  */

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

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "system_stm32f4xx.h"
#include "misc.h"
#include "stm32f4xx_can.h"
#include "ctype.h"
#include "math.h"
#include "can12.h"
#include "main.h"


uint32_t SystemCoreClock = 168000000;

static __IO uint32_t TimingDelay;

int alarm_flag = 0;

typedef struct command{
	unsigned char name[16];
	CANMsg msg;
};
unsigned char usart_input[16];
struct command commands[4] = {{"calib"}, {"conf"}, {"deact"}, {"act"}};

struct unit units[15];


/**
  * @brief  Delay.
  * @param	'nTime' som är tiden för delay i mikrosekunder.
  * @retval None.
  */
void delay(__IO uint32_t nTime)
{
	TimingDelay = nTime;

	while(TimingDelay != 0);
}

/**
  * @brief  Avbrottsrutin för SysTick.
  * @note  Uppdatera 'counter och 'TimingDelay'.
  * @retval None.
  */
void SysTick_Handler(void)
{                               /* SysTick interrupt Handler. */
	unit_time++;
	TimingDelay--;                          /* See startup file startup_LPC17xx.s for SysTick vector */
}

/**
  * @brief  Hämta 'unit_time'.
  * @param	None.
  * @retval 'unit_time'.
  */
int get_unit_time(void)
{
	return unit_time;
}

/**
  * @brief  Hämta 'last_time_stamp'.
  * @param  None.
  * @retval 'last_time_stamp'.
  */
int get_last_time_stamp(void)
{
	return last_time_stamp;
}

/**
  * @brief  Sätt 'last_time_stamp'.
  * @param  time: Värdet för last_time_stamp.
  * @retval None.
  */
void set_last_time_stamp(int time)
{
	last_time_stamp = time;
}

/**
  * @brief  Berättar vilken enhet samt sensor som larmat.
  * @param  enhets-id och sensor-id.
  * @retval None.
  */
void alarm_received(unsigned char nodeId, unsigned char sensorId)
{
	DUMP("\n***BEGIN ALARM***");
	DUMP("Unit ID:");
	_outchar(nodeId + '0');
	DUMP("");
	DUMP("Sensor ID:");
	_outchar(sensorId + '0');
	DUMP("");
	DUMP("Turn off alarm by typing respective code:\n");
}
/**
  * @brief  Avbrottsrutin för CAN.
  * @param  None.
  * @retval None.
  */
void receiver(void) {
	#ifdef DEBUG_MODE
	DUMP("CAN message received.");
	#endif
    CANMsg msg;
    if (can_receive(&msg)) {
		switch (msg.msgId) {
			case (ALARM):
				units[msg.nodeId - 1].sensors[msg.buff[7]-1].alarm = 1;
				alarm_received(msg.nodeId, msg.buff[7]);
				alarm_flag = 1;
				break;
			case (ACK):
				for(int unit_index = 0; unit_index < ELEMS(units); unit_index++){
					if(unit_index == msg.nodeId - 1){
						units[unit_index].ping_acked = 1;
						units[unit_index].ping_alarm = 0;
					}
				}
				break;
			case (CMND):
				//do stuff
				break;
			case (CONF):
				//do stuff
				break;
			case (REQST):
				//do stuff
				break;
		}
	}
    else
        DUMP("***Error: Something went wrong :(");
}

/**
  * @brief  Skickar ett meddelande för att stänga av larmet på en enhets sensor.
  * @param  unit_index: Enheten som meddelandet ska skickas till, sensor_index: Sensorn vars larm ska stängas av.
  * @retval None.
  */
void send_alarm_turn_off(int unit_index, int sensor_index)
{
	CANMsg msg;
	msg.urgent = NOT_URGENT;
	msg.direction = FROM_CENTRAL_UNIT;	// Från centralenheten
	msg.msgId = ALARM;
	msg.nodeId = unit_index+1;
	msg.buff[7] = sensor_index+1;
	msg.length = 8;
	can_send(&msg);
	DUMP("Message sent");
}

/**
  * @brief  Kollar ifall inputen på keypaden är lika med någon sensors kod och ifall larm för sensorn har gått, 
  * i så fall skickas ett CAN-meddelande för avlarmning.
  * @param  number_of_units: Antal enheter som är kopplade till centralenheten.
  * @retval None.
  */
void match_keypad_input(int number_of_units)
{
	int keypad_input_value = calculate_input_value();
	
	for(int unit_index = 0; unit_index < number_of_units; unit_index++){
		for(int sensor_index = 0; sensor_index < units[unit_index].number_of_sensors; sensor_index++){

			if(units[unit_index].sensors[sensor_index].code == keypad_input_value && units[unit_index].sensors[sensor_index].alarm){
				units[unit_index].sensors[sensor_index].alarm = 0;
				send_alarm_turn_off(unit_index, sensor_index);
				//om det inte finns några fler larm
				if(!(alarm_flag = check_alarm_status(number_of_units))){
					DUMP("All alarms turned off.");
				}
			}
		}
	}
}

/**
  * @brief  Kollar larmstatus för samtliga sensorer.
  * @param  number_of_units: Antal enheter som är kopplade till centralenheten.
  * @retval En etta ifall status för någon sensors larm är igång, annars 0.
  */
int check_alarm_status(int number_of_units)
{
	for(int unit_index = 0; unit_index < number_of_units; unit_index++){
		for(int sensor_index = 0; sensor_index < units[unit_index].number_of_sensors; sensor_index++){
			if(units[unit_index].sensors[sensor_index].alarm == 1){
				return 1;
			}
		}
	}
	return 0;
}


/*-------START OF COMMANDS-------*/
/**
  * @brief  Matchar inputen på usart med något av kommandona som finns om dem är lika.
  * @param  number_of_units: Antalet enheter kopplade till centralenheten.
  * @retval None.
  */
void match_usart_command(int number_of_units)
{
	for(int i = 0; i < ELEMS(commands); i++){
		if(!strncmp(usart_input, commands[i].name, sizeof(commands[i].name))){
			unit_sensor_choice(i, number_of_units);
			if(commands[i].msg.msgId == CONF){
				configuration_message(i);
			}
			
			can_send(&commands[i].msg);
			usart_send(&commands[i].name);
			DUMP(" message sent");
		}
	}
}

/**
  * @brief  Val av enhet och sensor som kommandot ska skickas till.
  * @param  index för kommandot och number_of_units.
  * @retval None.
  */
void unit_sensor_choice(int i, int number_of_units)
{
	DUMP("What unit do you want to send to?");
	int number = 0;
	while (!(0 < number && number <= number_of_units)){
		number = _getchar() - '0';
	}
	_outchar(number + '0');
	DUMP("");
	commands[i].msg.nodeId = number;

	DUMP("What sensor do you want to affect?");
	int sensor_number = 0;
	while (!(0 < sensor_number && sensor_number <= units[number-1].number_of_sensors)){
		sensor_number = _getchar() - '0';
	}
	_outchar(sensor_number + '0');
	DUMP("");
	commands[i].msg.buff[6] = sensor_number-1;
}

/**
  * @brief  Tar in värde för konfigurationen, samt ifall det är en timer som konfigureras.
  * @param  index för kommandot.
  * @retval None.
  */
void configuration_message(int i)
{
	DUMP("What is the new value for the configuration?");
	int value = 0;
	int number = 0;
	for(int j = 2; j >= 0; j--){
		number = _getchar() - '0';
		if(number >= 0 && number <= 9){
			_outchar(number + '0');
			value += number*pow(10, j);
		}
	}
	commands[i].msg.buff[7] = value;
	DUMP("");
	
	DUMP("Is it a door timer? (y/n)");
	char input = _getchar();
	_outchar(input);
	DUMP("");
	if(input == 'y'){
		DUMP("Local or central timer? (l/c)");
		input = _getchar();
		_outchar(input);
		DUMP("");
		if(input == 'l'){
			commands[i].msg.buff[5] = 0;
		} else if (input == 'c'){
			commands[i].msg.buff[5] = 1;
		}
	} else {
		commands[i].msg.buff[5] = 'n';
	}
}

/**
  * @brief  Rensar usart_input bufferten.
  * @param  None.
  * @retval None.
  */
void clear_usart_input(void)
{
	for(int i = 0; i < ELEMS(usart_input); i++){
		usart_input[i] = 0;
	}
}

/**
  * @brief  Initierar samtliga kommandons meddelanden.
  * @param  None.
  * @retval None.
  */
void command_init(void)
{
	commands[0].msg.urgent = NOT_URGENT;
	commands[0].msg.direction = FROM_CENTRAL_UNIT;
	commands[0].msg.msgId = CMND;
	commands[0].msg.length = 8;

	commands[1].msg.urgent = NOT_URGENT;
	commands[1].msg.direction = FROM_CENTRAL_UNIT;
	commands[1].msg.msgId = CONF;
	commands[1].msg.length = 8;

	commands[2].msg.urgent = NOT_URGENT;
	commands[2].msg.direction = FROM_CENTRAL_UNIT;
	commands[2].msg.msgId = CMND;
	commands[2].msg.buff[7] = DEACTIVATE;
	commands[2].msg.length = 8;

	commands[3].msg.urgent = NOT_URGENT;
	commands[3].msg.direction = FROM_CENTRAL_UNIT;
	commands[3].msg.msgId = CMND;
	commands[3].msg.buff[7] = ACTIVATE;
	commands[3].msg.length = 8;
}

/**
  * @brief  Huvudprogram för commands.
  * @param  index för usart_input buffert, usart_in är senaste inmatade tecken och number_of_units.
  * @retval index.
  */
int commands_main(int index, char usart_in, int number_of_units)
{
	if(usart_in == usart_ENTER){
		DUMP("");
		match_usart_command(number_of_units);
		clear_usart_input();
		index = 0;
	} else if(usart_in == usart_BACK_SPACE && index != 0){
		index--;
		usart_input[index] = 0;
		_outchar(usart_in);
	} else if (index < ELEMS(usart_input) && usart_in != usart_BACK_SPACE){
		usart_input[index] = usart_in;
		_outchar(usart_in);
		index++;
	}
	return index;
}
/*-------END OF COMMANDS-------*/


/**
  * @brief  Skickar ping-meddelande över CAN till samtliga enheter.
  * @param  number_of_units: Antal enheter som är kopplade till centralenheten.
  * @retval None.
  */
void ping(int number_of_units)
{
	CANMsg msg;
	msg.urgent = NOT_URGENT;
	msg.direction = FROM_CENTRAL_UNIT;	// Från centralenheten
	msg.msgId = REQST;
	msg.length = 8;
	for(int unit_index = 0; unit_index < number_of_units; unit_index++){
		msg.nodeId = unit_index+1;
		can_send(&msg);
		units[unit_index].ping_acked = 0;
	}
}

/**
  * @brief  Sätter kod för en sensor.
  * @param  None.
  * @retval Returnerar kod.
  */
void set_codes(int i, int number_of_sensors)
{
	unsigned char buff[16];
	int code, index, new_num;
	for (int j = 0; j < number_of_sensors; j++) {
		usart_send("Add a code for sensor ");
		usart_send(itoa(j + 1, buff, 10));
		usart_send(": ");
		code = 0;
		index = 3;
		new_num = 0;
		while (index >= 0) {
			new_num = _getchar() - '0';    // Uppdatera antal sensorer som används
			if(0 <= new_num && new_num <= 9){
				_outchar(new_num + '0');
				code = code + pow(10, index)*new_num;
				index--;
			}
		}
		units[i].sensors[j].code = code;
		usart_send("\n");
	}
	
}

/**
  * @brief  Sätter antalet sensorer som är kopplade till en enhet.
  * @param  Enhet i.
  * @retval Antalet sensorer.
  */
int set_number_of_sensors(int i)
{
	unsigned char buff[16];
	usart_send("How many sensors does unit ");
	usart_send(itoa(i + 1, buff, 10));
	usart_send(" use? ");
	char number_of_sensors = 0;
	while (!(0 < number_of_sensors && number_of_sensors <= ELEMS(units[i].sensors))) {
		number_of_sensors = _getchar() - '0';    // Uppdatera antal sensorer som används
	}
	units[i].number_of_sensors = number_of_sensors;
	units[i].ping_acked = 1;
	units[i].ping_alarm = 0;
	usart_send(itoa(number_of_sensors, buff, 10));
	usart_send("\n");
	return number_of_sensors;
}

/**
  * @brief  Sätter antalet enheter som är kopplade till centralenheten.
  * @param  None.
  * @retval Antalet enheter.
  */
int set_number_of_units(void)
{
	unsigned char buff[16];
	usart_send("How many units are being used?");
	char number_of_units = 0;
	while (!(0 < number_of_units && number_of_units <= ELEMS(units))) {
		number_of_units = _getchar() - '0';
	}
	usart_send(itoa(number_of_units, buff, 10));
	usart_send("\n");
	return number_of_units;
}

/**
  * @brief  Konfigurerar antalet enheter och sensorer på varje enhet, samt varje sensors pinkod.
  * @param  None.
  * @retval None.
  */
int config_mode(void) {
    unsigned char buff[16];
	int number_of_sensors;
	int number_of_units;
    while(1) {
		DUMP("");
        DUMP("***BEGIN CONFIG MODE***\n");

        number_of_units = set_number_of_units();

        for (int i = 0; i < number_of_units; i ++) {
            number_of_sensors = set_number_of_sensors(i);
			set_codes(i, number_of_sensors);
        }

        DUMP("***END CONFIG MODE***\n");

        return number_of_units;
    }

}

/**
  * @brief  Aktiverar samtliga sensorer.
  * @param  None.
  * @retval None.
  */
void activate_sensors(void)
{
	for(int i = 0; i < ELEMS(units); i++){
		for(int j = 0; j < units[i].number_of_sensors; j++){
			CANMsg msg;
			msg.urgent = NOT_URGENT;
			msg.direction = FROM_CENTRAL_UNIT;
			msg.msgId = CMND;
			msg.nodeId = i+1;
			msg.buff[6] = j;
			msg.buff[7] = ACTIVATE;
			msg.length = 8;
			can_send(&msg);
			delay(10000);
		}
	}
}

/**
  * @brief  Kollar ifall alla enheter svarat på ping.
  * @param 	number_of_units: Antal enheter som är kopplade till centralenheten.
  * @retval None.
  */
void check_acked_ping(int number_of_units)
{
	for(int i = 0; i < number_of_units; i++){
		if(!units[i].ping_acked){
			units[i].ping_alarm = 1;
		}
		if(units[i].ping_alarm){
			DUMP("***ALARM***");
			usart_send("Unit ");
			_outchar((i+1) + '0');
			DUMP(" didn't answer ping!");

		}
	}
}

/**
  * @brief  Initiering av SysTick.
  * @param 	None.
  * @retval None.
  */
void systick_init(void)
{
	*((void (**) (void)) SYSTICK_IRQVEC) = SysTick_Handler;


    SysTick_Config(SystemCoreClock / 1000000); // avbrott varje mikrosekund
}

/**
  * @brief  Initieringar för centralenheten samlade.
  * @param 	None.
  * @retval None.
  */
void app_init(void)
{
	keypad_GPIO_init();
	#ifdef DEBUG_MODE
	DUMP("keypad_GPIO_init done.");
	#endif
	systick_init();
	#ifdef DEBUG_MODE
	DUMP("systick_init done.");
	#endif
	can1_init(receiver);
	#ifdef DEBUG_MODE
	DUMP("can1_init done.");
	#endif
	command_init();
	#ifdef DEBUG_MODE
	DUMP("command_init done.");
	#endif
}

/**
  * @brief  Huvudprogrammet för centralenheten.
  * @param 	None.
  * @retval None.
  */
void main(void)
{
	int number_of_units, index, ping_time_stamp;
	char usart_in;
	
	app_init();
	
	index = 0;
	ping_time_stamp = unit_time;
	
	number_of_units = config_mode();
	
	activate_sensors();
	#ifdef DEBUG_MODE
	DUMP("Sensors activated.");
	#endif
	
	while( 1 )
	{
		if(alarm_flag){//ska inte kunna göra någon input via keypaden ifall inget larm finns
			clear_usart_input();
			keypad_main(number_of_units);
		}
		
		usart_in = _tstchar();
		if(usart_in && !alarm_flag){
			index = commands_main(index, usart_in, number_of_units);
		}

		if(unit_time - ping_time_stamp > FIVE_SECONDS){
			check_acked_ping(number_of_units);
			ping(number_of_units);
			ping_time_stamp = unit_time;
			#ifdef DEBUG_MODE
			DUMP("Ping sent.");
			#endif
		}

	}
}
