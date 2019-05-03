/*
 * uart.c
 *
 * Created: 28.03.2019 13:45:09
 *  Author: Vsevolod
 */ 

#include "general.h"
#include "uart.h"
#include "md3.h"
#include "fifo.h"
#include <util/atomic.h>

// Буфер передачи:
FIFO_BUFFER_t __uart_tx_buf;


 void uart_init (void)
{
	UBRR0L = (uint8_t) UART_UBRR;
	UBRR0H = (uint8_t) (UART_UBRR >> 8);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // Char size 8 bit
	UCSR0B |= (UART_TX_ENABLE << TXEN0) | (UART_RX_ENABLE << RXEN0) | \
				(UART_RXC_INT_ENABLE << RXCIE0);
				
	// Инициализируем FIFO-буфер (очередь) кольцевого типа
	fifo_init (&__uart_tx_buf, UART_TX_BUF_SIZE);
	
	uart_clrscr ();
	uart_home ();
	uart_puts ("[ OK ] UART Alive\n");
	
	return;
}

int uart_stdputc (char c, FILE *stream)
{	
	#if UART_INSERT_CR == 1
		if (c == '\n')
		{
			uart_stdputc ('\r', stream);
		}
	#endif
	
//	__uart_tx_byte (c);			// это передача с тупым ожиданием

// Передача через буфер:
	fifo_push (c, &__uart_tx_buf);	// пихаем в буфер
	UCSR0B |= 1 << UDRE0;			// когда UDR будет готов, в прерывании будем писать в UDR
	// (даже если это прерывание уже разрешено, то ничего не изменится)
	
	return 0;
}

inline void uart_putc (char c)
{
	uart_stdputc (c, NULL);
	return;
}

void uart_puts (char *str)
{
	register char c;
	
	while ((c = *str++))
	{
		uart_putc (c);
	}
	
	return;
}

void uart_clrscr (void)
{
//	printf (UART_ESC "[2J");
	uart_puts (UART_ESC);
	uart_puts ("[2J");
	return;
}

void uart_home (void)
{
//	printf (UART_ESC "[H");
	uart_puts (UART_ESC);
	uart_puts ("[H");
	return;
}

inline void uart_goto_xy (int x, int y)
{
	printf (UART_ESC "[%d;%dH", y, x);
	return;
}

void uart_reset_disp_attr (void)
{
//	printf (UART_ESC "[0m");
	uart_puts (UART_ESC);
	uart_puts ("[0m");
	return;
}

inline void uart_set_disp_attr (int attr)
{
	printf (UART_ESC "[%dm", attr);
	return;
}

void __uart_tx_byte (uint8_t data)
{	
// Тупая передача байта с ожиданием
	led_r_on ();
	while(!(UCSR0A & (1 << UDRE0)))
		;
	led_r_off ();
	UDR0 = data;
	
	return;
}

inline void __uart_rx_byte (void)
{	// Вызов по прерыванию
	uint8_t buff = UDR0;
	 
	switch (buff)
	{
		case 'l':
		{
			led_g_switch ();
			uart_set_disp_attr (F_GREEN);
			printf ("[OK] LED \"G\" TOGGLED\n");
			uart_reset_disp_attr ();
			break;
		}
		default:
		{
			led_r_switch ();
// 			uart_set_disp_attr (F_RED);
// 			printf ("[FAIL] UNKNOWN COMMAND\n");
// 			uart_reset_disp_attr ();
	
			printf (ATTR "%d" ATTR_END "[FAIL]" ATTR_RST "UNKNOWN COMMAND\n", F_RED);
			
			break;
		}
	} 
		
	return;
}