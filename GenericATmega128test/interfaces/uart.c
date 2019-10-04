/*
 * uart.c
 *
 * Created: 28.03.2019 13:45:09
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "uart.h"
#include "../modules/md3.h"
#include "../fifo.h"
#include "../rtos.h"

#include "../modules/motor.h"
#include "../motion_control.h"

extern void show_info_uart(void);

// Буфер передачи:
FIFO_BUFFER_t __uart_tx_buf;

uint8_t __uart_ready = 0;		// если UART не инициализирован, передача байтов будет запрещена

 void uart_init (void)
{
	UBRR0L = (uint8_t) UART_UBRR;
	UBRR0H = (uint8_t) (UART_UBRR >> 8);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // Char size 8 bit
	UCSR0B |= (UART_TX_ENABLE << TXEN0) | (UART_RX_ENABLE << RXEN0) | \
				(UART_RXC_INT_ENABLE << RXCIE0);
				
	__uart_ready = 1;
				
	// Инициализируем кольцевой FIFO-буфер
	fifo_init (&__uart_tx_buf, UART_TX_BUF_SIZE);
	
	uart_clrscr ();
	uart_home ();
	uart_puts ("[ OK ] UART Alive\n");
	
	return;
}

int uart_stdputc (char c, FILE *stream)
{	
	if (!__uart_ready)
	{
		return 0;
	}
	
	#if UART_INSERT_CR == 1
		if (c == '\n')
		{
			uart_stdputc ('\r', stream);
		}
	#endif

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
	uart_puts (UART_ESC);
	uart_puts ("[2J");
	return;
}

void uart_home (void)
{
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
	uart_puts (UART_ESC);
	uart_puts ("[0m");
	return;
}

inline void uart_set_disp_attr (int attr)
{
	printf (UART_ESC "[%dm", attr);
	return;
}

inline void __uart_tx_routine (void)
{	// Вызов по прерыванию
	// UDR готов, пишем в него из буфера:
	UDR0 = fifo_pop (&__uart_tx_buf);
	// Внимание! Нотация точечная, т.к. работаем с буфером напрямую, а не через указатель:
	if (__uart_tx_buf.idxOut == __uart_tx_buf.idxIn)
	{	// если на данный момент передать больше нечего,
		UCSR0B &= ~(1 << UDRIE0);	// отключаем данное прерывание
	}
}

inline void __uart_rx_byte (void)
{	// Вызов по прерыванию
	// Пока что примитивная система команд
	uint8_t buff = UDR0;
	
	static uint8_t dbg = 0;
	 
	switch (buff)
	{
		case 'a':
		{
			motors_arm ();
			break;
		}
		case 'd':
		{
			motors_disarm ();
			break;
		}
		case '0':
		{
			motors_set_omega (0.0, 0.0);
			uart_puts ("[ OK ] 0.0; 0.0 SET\n");
// 			mcontrol_set (0.0, 0.0);
// 			uart_puts ("[ OK ] 0.0 m/s; 0.0 rad/s SET\n");
			break;
		}
		case '1':
		{
			motors_set_omega (3.0, 3.0);
			uart_puts ("[ OK ] 3.0; 3.0 SET\n");
// 			mcontrol_set (0.1, 0.0);
// 			uart_puts ("[ OK ] 0.1 m/s; 0.0 rad/s SET\n");
			break;
		}
		case '2':
		{
			motors_set_omega (6.0, 6.0);
			uart_puts ("[ OK ] 6.0; 6.0 SET\n");
// 			mcontrol_set (0.2, 0.0);
// 			uart_puts ("[ OK ] 0.2 m/s; 0.0 rad/s SET\n");
			break;
		}
		case '3':
		{
			motors_set_omega (9.0, 9.0);
			uart_puts ("[ OK ] 9.0; 9.0 SET\n");
// 			mcontrol_set (0.3, 0.0);
// 			uart_puts ("[ OK ] 0.3 m/s; 0.0 rad/s SET\n");
			break;
		}
		case '4':
		{
			motors_set_omega (12.0, 12.0);
			uart_puts ("[ OK ] 12.0; 12.0 SET\n");
// 			mcontrol_set (0.4, 0.0);
// 			uart_puts ("[ OK ] 0.4 m/s; 0.0 rad/s SET\n");
			break;
		}
		case '5':
		{
			motors_set_omega (15.0, 15.0);
			uart_puts ("[ OK ] 15.0; 15.0 SET\n");
// 			mcontrol_set (0.5, 0.0);
// 			uart_puts ("[ OK ] 0.5 m/s; 0.0 rad/s SET\n");
			break;
		}
		case '6':
		{
			motors_set_omega (18.0, 18.0);
			uart_puts ("[ OK ] 18.0; 18.0 SET\n");
// 			mcontrol_set (0.6, 0.0);
// 			uart_puts ("[ OK ] 0.6 m/s; 0.0 rad/s SET\n");
			break;
		}
		case 'l':	// поворачиваем налево
		{
			motors_set_omega (3.0, 6.0);
			uart_puts ("[ OK ] 3.0; 6.0 SET. Target OMEGA = 28 deg/s\n");
// 			mcontrol_set (0.1, 0.5);
// 			uart_puts ("[ OK ] 0.1 m/s; 0.5 rad/s SET\n");
			break;
		}
		case 'r':	// поворачиваем направо
		{
			motors_set_omega (6.0, 3.0);
			uart_puts ("[ OK ] 6.0; 3.0 SET. Target OMEGA = -28 deg/s\n");
// 			mcontrol_set (0.1, -0.5);
// 			uart_puts ("[ OK ] 0.1 m/s; -0.5 rad/s SET\n");
			break;
		}
		case 'b':	// отладочная информация
		{
			if (!dbg)
			{
				dbg = 1;
				uart_clrscr ();
				uart_home ();
				uart_puts ("~~~DEBUG MODE BEGIN~~~\n");
//				uart_puts ("obj_L omega_L eps_L I_L u_L obj_R omega_R eps_R I_R u_R\nevery 0.1 s\n");
				uart_puts ("eng1 [rad/s]|eng2 [rad/s]  |  gZ [deg/s] |  Vel [m/s]\n\nEvery 0.1 s\n\n");
				rtos_set_task (show_info_uart, 1000, 100);
			}
			else
			{
				dbg = 0;
				uart_puts ("~~~DEBUG MODE END~~~\n");
				rtos_delete_task (show_info_uart);
			}
			break;
		}
		default:
		{
			uart_puts ("[ FAIL ] Unknown command. Performing DISARM\n");
			motors_disarm ();
			break;
		}
	} 
		
	return;
}