#define TIMER_LOCAL 			0x0
#define	TIMER_CENTRAL			0x1

char usart_ENTER = 13;
char usart_BACK_SPACE = 8;

#define ACTIVATE				0x1
#define DEACTIVATE				0x0

#define SYSTICK_IRQVEC 0x2001C03C

const FIVE_SECONDS = 5000000;

static volatile int unit_time;

static volatile int last_time_stamp;

#define ELEMS(x)  (sizeof(x) / sizeof((x)[0]))

/*------STRUCTS------*/
typedef struct sensor{
	int code;
	int alarm;
	int alarm_acked;
};

typedef struct unit{
	int number_of_sensors;
	struct sensor sensors[15];
	int ping_acked;
	int ping_alarm;
};

