/*
 * general.c
 *
 * Created: 19.11.2018 19:24:34
 *  Author: Vsevolod
 */ 

#include "general.h"

inline void general_init (void)
{
//	OSCCAL = eeprom_read_byte (OSC_CAL_BYTE);	// калибровочный бит

	XDIV = (1 << XDIVEN) | (129 - OSC_DIV_FACTOR);	// см. datasheet
	
	_delay_ms (100);
	
	return;
}