/*
 * UARTtest.c
 *
 * Created: 06.04.2019 12:45:37
 * Author : Vsevolod
 */ 

#include "general.h"
#include "uart.h"
#include "lcd.h"
#include "md3.h"

static FILE _UART0_ = FDEV_SETUP_STREAM (uart_putc, NULL, _FDEV_SETUP_WRITE);
static FILE _LCD_ = FDEV_SETUP_STREAM (lcd_putc, NULL, _FDEV_SETUP_WRITE);

ISR (USART0_RX_vect)
{
	__uart_rx_byte ();
}


int main(void)
{
//	uint8_t cnt = 0;
	
	general_init ();
    uart_init ();
//	lcd_init ();
	md3_init ();
	
	stdout = &_UART0_;
	
	uart_clrscr ();
	uart_set_disp_attr (F_BLUE);
	printf ("\n\n*********************\nHello World thru UART0\nWaiting4CMD\n*********************\n\n");
	uart_reset_disp_attr ();
//	stdout = &_LCD_;
	
//	_delay_ms (2000);
	
	sei ();
	
    while (1) 
    {
// 	_delay_ms (1000);
// 	stdout = &_UART0_;
// 	printf ("New Line %d\n", cnt++);

// 	__uart_remote_ctrl ();
// 	_delay_ms (100);
    }
}

