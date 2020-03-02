/*
 * general.c
 *
 * Created: 19.11.2018 19:24:34
 *  Author: Vsevolod
 */ 

#include "general.h"
#include "rtos.h"
#include "modules/md3.h"
#include "interfaces/uart.h"

static uint8_t volt_warning = 0;

inline void general_init (void)
{
//	OSCCAL = eeprom_read_byte (OSC_CAL_BYTE);	// калибровочный бит
	XDIV = (1 << XDIVEN) | (129 - OSC_DIV_FACTOR);	// см. datasheet
	_delay_ms (100);
	
	rtos_set_task(check_voltage, CHECK_VOLT_PERIOD, CHECK_VOLT_PERIOD);
	
	return;
}

void check_voltage(void)
{
	if(md3_get_voltage() < VOLT_WARNING)
	{
		uart_puts("[ ! ] CHECK SUPPLY VOLTAGE\n");
		if(!volt_warning)
		{
			led_r_on();
		}
	}
	
	return;
}
