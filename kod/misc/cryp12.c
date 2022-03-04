/*
 * 	Encryption for G12
 *
 */
 #include "misc.h"
// crypterar en char genom att roterar alla bitar key steg åt höger
char encrypt_char (uint8_t c, uint32_t key)
{
	uint8_t shifted_c = 0;
	uint32_t c_bit[8];
	
	//Sätter alla bitar i c_bit till 0
	for(int i = 0; i < 8; i++)
		c_bit[i]=0;
		
	//Skiftar alla bitar key steg till höger
	for(int i = 0; i < 8; i ++){
		c_bit[(i+key)%8] = (c >> i) & 0x1;
	}
	// Array of bitar till en char
	for(int i = 0; i < 8 ; i++)
		shifted_c += c_bit[i]<<i;
		
	return shifted_c;
}

// decrypterar en char genom att flytta alla bitar key steg åt vänster 
char decrypt_char (uint8_t c, uint32_t key) 
{
	encrypt_char(c,8-key);
}
