/*---------------------------------------------------
 * 
 * 			Fördröjningsfunktioner
 *      Olika intervall beroende på situation
 * 
 * --------------------------------------------------*/
#include "misc.h"
static __IO uint32_t TimingDelay;
uint32_t SystemCoreClock = 168000000;
#define SYSTICK_IRQVEC 0x2001C03C


void SysTick_Handler(void)
{
        if (TimingDelay != 0x0)
			TimingDelay--;
}
//delay i mikrosekunder
void delay_in_micro(__IO uint32_t nTime)
{ 
	TimingDelay = nTime;
	while(TimingDelay != 0);
}
//delay i millisekunder
void delay_in_milli(__IO uint32_t nTime)
{ 
	delay_in_micro(nTime*1000);
}
//delay i sekunder
void delay_in_sec(__IO uint32_t nTime)
{ 
	delay_in_milli(nTime*1000);
}

void systick_init(void)
{
	*((void (**) (void)) SYSTICK_IRQVEC) = SysTick_Handler;
    SysTick_Config(SystemCoreClock/1000000); // avbrott varje mikrosekund
}

