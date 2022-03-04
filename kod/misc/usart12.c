/****************************************************
 * 		USART
 * 		Beskrivning:
 * 			Används för att skriva ut char till konsolen
 * 			via USART.
 * 		Instruktion:
 * 			Inkluderar usart12.c i projekten och anropar
 * 			metoder nedanför.
 * 
 * **************************************************/
 // Strukten för USART
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

// Adress till Usartvektor.
#define USART1  ((USART *) 0x40011000) 

/**
  * @brief  Skriver en char till USART
  * @param  char c: en char som ska skrivas ut
  * @retval none
  */
  
void _outchar( char c ) {
    /* write character to usart1 */
    while (( USART1->sr & 0x80)==0);
    USART1->dr = (unsigned short) c;
    if( c == '\n')
            _outchar('\r');
}

/**
  * @brief  Ser om en char har kommit till usart1
  * 		Om ja, returnerar den. 
  * 		Annars, returnerar 0.
  * @param  None
  * @retval char: 0 eller den char:n som kommit till usart.
  */
  
char _tstchar(void) {
    /* see if character arrived at usart1,
        if not, return 0
        else return character
        */
    if( (USART1->sr & 0x20)==0)
        return 0;
    return (char) USART1->dr;
}

/**
  * @brief  Väntar till en char kommit till usart1 och returnerar den.
  * @param  None
  * @retval char: 0 eller den char:n som kommit till usart.
  */
  
char _getchar(void) {
    /* wait until character arrived at usart1,
        return character
        */
    while( (USART1->sr & 0x20)==0)
        ;
    return (char) USART1->dr;
}
/**
  * @brief  Skriver ut en char eller sträng till konsolen.
  * @param  Pekare: pekare till char som ska skrivas ut
  * @retval none
  */
void usart_send(char* s){
   while (*s != '\0')
       _outchar(*(s++));
}

/**
  * @brief  Skriver ut en char eller sträng till konsolen.
  * @param  Pekare: pekare till char som ska skrivas ut
  * @retval None
  */
  
void DUMP(char *s) {
   usart_send(s);
   _outchar('\n');
}

