#include "keypad.h"
#include "stm32f4xx_gpio.h"

/**
  * @brief  GPIO-initiering för keypaden.
  * @param	None.
  * @retval None.
  */
void keypad_GPIO_init (void)
{

    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

}

/**
  * @brief  Dumpar inputen från keypaden.
  * @param	None.
  * @retval None.
  */
void dump_input(void)
{
	for(int i = 0; i < ELEMS(keypad_input_buffer)-1; i++){
		ascii_input_buffer[i] = keypad_input_buffer[i] + '0';//char görs om till ascii-tecken
	}
	ascii_input_buffer[ELEMS(keypad_input_buffer)-1] = '\0';
	DUMP(ascii_input_buffer);
}

/**
  * @brief  Rensar keypad_input_buffer.
  * @param	None.
  * @retval None.
  */
void clear_keypad_input(void)
{
	for(int i = 0; i < ELEMS(keypad_input_buffer); i++){
		keypad_input_buffer[i] = 0;
		ascii_input_buffer[i] = 0;
	}
}

/**
  * @brief  Kollar ifall en knapp är nedtryckt på keypaden.
  * @param	None.
  * @retval Värdet på knappen eller 0xFF om ingen knapp är nedtryckt.
  */
unsigned char keyb( void )
{
	unsigned char key[] = {1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
	int row, col;
	for(row = 1; row <= 4; row++)
	{
		activateRow(row);
		if(col = readCol())
		{
			activateRow(0);
			return key[(4*(row-1) + (col-1))];
		}

	}
	activateRow(0);
	return 0xFF;
}

/**
  * @brief  Aktiverar rad på keypaden.
  * @param	row: Vilken rad som ska aktiveras.
  * @retval None.
  */
void activateRow(unsigned int row)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
	switch(row)
	{
		case 1: GPIO_SetBits(GPIOD, GPIO_Pin_12); break;
		case 2: GPIO_SetBits(GPIOD, GPIO_Pin_13); break;
		case 3: GPIO_SetBits(GPIOD, GPIO_Pin_14); break;
		case 4: GPIO_SetBits(GPIOD, GPIO_Pin_15); break;
		default: GPIO_ResetBits(GPIOD, GPIO_Pin_12|GPIO_Pin_3|GPIO_Pin_14|GPIO_Pin_15); break;
	}
}

/**
  * @brief  Läs av kolumn.
  * @param	None.
  * @retval Vilken kolumn som har den nedtryckta knappen.
  */
int readCol(void)
{
	if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11)) return 4;
	if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_10)) return 3;
	if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_9)) return 2;
	if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_8)) return 1;
	return 0;
}

/**
  * @brief  Lägger till inmatning på keypad i bufferten.
  * @param	keypad_input: Inputen.
  * @retval None.
  */
void add_to_buffer( unsigned char keypad_input)
{
	if(keypad_input < 16 && keypad_input != keypad_ENTER)
	{
		for(int i = 0; i < ELEMS(keypad_input_buffer)-1; i++){
			keypad_input_buffer[i] = keypad_input_buffer[i+1];
		}
		keypad_input_buffer[ELEMS(keypad_input_buffer)-2] = keypad_input;
		usart_send("Current code in buffer: ");
		dump_input();
	}
}

/**
  * @brief  Beräknar värdet av keypad_input_buffer.
  * @param	None.
  * @retval Värdet.
  */
int calculate_input_value(void)
{
	int input_value = 0;
	for(int i = 0; i < ELEMS(keypad_input_buffer)-1; i++){
		input_value = input_value + keypad_input_buffer[i]*pow(10, 3-i);
	}
	return input_value;
}

/**
  * @brief  Huvudprogrammet för keypad.
  * @note	Inmatning på keypad ska inte kunna ske två gånger inom loppet av en kvarts sekund.
  * @param	number_of_units: Antal enheter som är kopplade till centralenheten.
  * @retval None.
  */
void keypad_main(int number_of_units)
{
	unsigned char keypad_input = keyb();
	if(keypad_input != no_keypad_input_value && keypad_input < 10 && get_unit_time() - get_last_time_stamp() > QUARTER_SECOND){
		add_to_buffer(keypad_input);
		set_last_time_stamp(get_unit_time());
	} else if (keypad_input == keypad_ENTER && get_unit_time() - get_last_time_stamp() > QUARTER_SECOND){
		match_keypad_input(number_of_units);
		clear_keypad_input();
		DUMP("Input reset");
		set_last_time_stamp(get_unit_time());
	}
}
