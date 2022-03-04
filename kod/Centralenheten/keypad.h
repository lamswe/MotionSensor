#define keypad_ENTER			0xD
#define	no_keypad_input_value 	0xFF

#define ELEMS(x)  (sizeof(x) / sizeof((x)[0]))

unsigned char keypad_input_buffer[5] = {0, 0, 0, 0};
unsigned char ascii_input_buffer[4];

const QUARTER_SECOND = 250000;
