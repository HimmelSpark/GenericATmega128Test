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
#include "../displays/lcd.h"
#include "../nav.h"
#include "../modules/gps.h"

extern void show_info_uart(void);

// Буфер передачи:
FIFO_BUFFER_t __uart_tx_buf;

uint8_t __uart_ready = 0;		// если UART не инициализирован, передача байтов будет запрещена

 void uart_init (void)
{
	// Инициализируем кольцевой FIFO-буфер
	fifo_init (&__uart_tx_buf, UART_TX_BUF_SIZE);
	
	UBRR0L = (uint8_t) UART_UBRR;
	UBRR0H = (uint8_t) (UART_UBRR >> 8);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // Char size 8 bit
	UCSR0B |= (UART_TX_ENABLE << TXEN0) | (UART_RX_ENABLE << RXEN0) | \
				(UART_RXC_INT_ENABLE << RXCIE0);
				
	__uart_ready = 1;
	
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
}

inline void __uart_tx_routine (void)
{// ISR
	// UDR готов, пишем в него из буфера (в нём заведомо что-то есть):
	UDR0 = fifo_pop (&__uart_tx_buf);
	if (!fifo_pop_avail(&__uart_tx_buf))
	{	// если на данный момент передать больше нечего,
		UCSR0B &= ~(1 << UDRIE0);	// отключаем данное прерывание
	}
}

inline void __uart_rx_byte (void)
{// ISR
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
// 		case 'c':
// 		{
// 			mcontrol_course_reset();
// 			
// 			break;
// 		}
// 		case 'r':
// 		{
// 			gps_get_pos(&gps_pos);
// 			nav_set_tgt_wp(gps_pos.lat, gps_pos.lon);
// 			uart_puts("[ OK ] New reference set\n");
// 			
// 			break;
// 		}
		case 'n':
		{
			nav_route_next_wp();
			
			break;
		}
		case 'g':
		{
			nav_autopilot_engage();
			
			break;
		}
		case 's':
		{
			nav_autopilot_diseng();
			
			break;
		}
		case '0':
		{
// 			motors_set_omega (0.0, 0.0);
// 			uart_puts ("[ OK ] 0.0; 0.0 SET\n");
			mcontrol_set (0.0, 0.0);
			uart_puts ("[ OK ] 0.0 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '1':
		{
// 			motors_set_omega (3.0, 3.0);
// 			uart_puts ("[ OK ] 3.0; 3.0 SET\n");
			mcontrol_set (0.1, 0.0);
			uart_puts ("[ OK ] 0.1 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '2':
		{
// 			motors_set_omega (6.0, 6.0);
// 			uart_puts ("[ OK ] 6.0; 6.0 SET\n");
			mcontrol_set (0.2, 0.0);
			uart_puts ("[ OK ] 0.2 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '3':
		{
// 			motors_set_omega (9.0, 9.0);
// 			uart_puts ("[ OK ] 9.0; 9.0 SET\n");
			mcontrol_set (0.3, 0.0);
			uart_puts ("[ OK ] 0.3 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '4':
		{
// 			motors_set_omega (12.0, 12.0);
// 			uart_puts ("[ OK ] 12.0; 12.0 SET\n");
			mcontrol_set (0.4, 0.0);
			uart_puts ("[ OK ] 0.4 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '5':
		{
// 			motors_set_omega (15.0, 15.0);
// 			uart_puts ("[ OK ] 15.0; 15.0 SET\n");
			mcontrol_set (0.5, 0.0);
			uart_puts ("[ OK ] 0.5 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '6':
		{
// 			motors_set_omega (18.0, 18.0);
// 			uart_puts ("[ OK ] 18.0; 18.0 SET\n");
			mcontrol_set (0.8, 0.0);
			uart_puts ("[ OK ] 0.8 m/s; 0.0 rad/s SET\n");
			
			break;
		}
		case '7':
		{
			__motors_set_thrust(254, 254);
			uart_puts("[ OK ] Thrust 99%,99% set\n");
			
			break;
		}
		case '[':	// поворачиваем налево
		{
// 			motors_set_omega (3.0, 6.0);
// 			uart_puts ("[ OK ] 3.0; 6.0 SET. Target OMEGA = 28 deg/s\n");
			mcontrol_set (0.1, 0.5);
			uart_puts ("[ OK ] 0.1 m/s; 0.5 rad/s SET\n");
			
			break;
		}
		case ']':	// поворачиваем направо
		{
// 			motors_set_omega (6.0, 3.0);
// 			uart_puts ("[ OK ] 6.0; 3.0 SET. Target OMEGA = -28 deg/s\n");
			mcontrol_set (0.1, -0.5);
			uart_puts ("[ OK ] 0.1 m/s; -0.5 rad/s SET\n");
			
			break;
		}
		case 'b':	// отладочная информация
		{
			if (!dbg)
			{
				dbg = 1;
				uart_clrscr ();
				uart_home ();
				uart_puts ("[ OK ] Debug mode begin\n");
//				uart_puts ("obj_L omega_L eps_L I_L u_L obj_R omega_R eps_R I_R u_R\nevery 0.3 s\n");
//				uart_puts ("eng1 [rad/s]|eng2 [rad/s]  |  gZ [deg/s] |  Vel [m/s]\n\nEvery 0.3 s\n\n");
//				uart_puts("POW_L\tOMEGA_L\t\tPOW_R\tOMEGA_R\n\n");
//				uart_puts("LAT\t\tLON\t\tVEL\tCRS\tSATS\tHDOP\tRTE_ID\tWP_TGT\tdPSI\tBRG\tDST\tV_obj\tOm_obj\tgZ\tENG_L_P\tomega\tENG_R_P\tomega\tVOLT\n\n");
//				uart_puts("STEP-TEST BEGIN: Omega_obj = 15 deg/s, dt = 0.1 s\n\n");
//				uart_puts("STEP-TEST BEGIN: lin_vel_obj = 1.0, dt = 0.1 s\n\n");
//				uart_puts("gX\tint(gX)\t\tgY\tint(gY)\t\tgZ\tint(gZ)\n");
//				uart_puts("degrees\ngZ\tint(gZ)\n");
				rtos_set_task (show_info_uart, 1000, 300);
//				mcontrol_set(0.8, 15*3.14/180.0);
//				mcontrol_set(1.0, 0.0);
			}
			else
			{
				dbg = 0;
				uart_puts ("[ OK ] Debug mode end\n");
				rtos_delete_task (show_info_uart);
				
//				mcontrol_set(0.0, 0.0);
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
}