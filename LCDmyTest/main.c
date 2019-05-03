/*
 * LCDmyTest.c
 *
 * Created: 04.04.2019 21:05:44
 * Author : Vsevolod
 */ 

#include <avr/io.h>
#include "general.h"
#include "lcd.h"

/* Устройства стандартного вывода */
static FILE _LCD_ = FDEV_SETUP_STREAM (lcd_putc, NULL, _FDEV_SETUP_WRITE);	// стд. устр-во вывода №1


int main(void)
{
	uint8_t count = 0;
	LCD_POS pos;
	
	general_init ();
    lcd_init ();
	
	stdout = &_LCD_;
	
	printf ("ABCDEabcde12345\n");
	printf ("     FGHIJKLMNOPQ\n");
	
 	_delay_ms (3000);
//	 lcd_clr ();
 	
 	printf ("New Line");
	pos = lcd_getpos ();
	printf ("\n");
	lcd_setpos (pos.line, ++pos.pos);
 	
    while (1) 
	{
		_delay_ms (100);
// 		printf ("New Line %d\n", count++);
// 		lcd_setpos (0, 0);
		printf ("%d\n", count++);
		lcd_setpos (pos.line, pos.pos);
	}
}

