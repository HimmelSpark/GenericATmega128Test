/*
 * uart.c
 *
 * Created: 28.03.2019 13:45:09
 *  Author: Vsevolod
 */ 

#include "general.h"
#include "uart.h"
#include "lcd.h"
#include "md3.h"
#include <util/atomic.h>


// volatile uint8_t rx_buff [UART_RX_BUF_SIZE][UART_CMD_SIZE],	tx_buff [UART_TX_BUFF_SIZE];
// volatile uint8_t rx_cmd_index = 0, rx_ch_index = 0,			tx_ch_index = 0;
// volatile uint8_t rx_err_cnt = 0,							tx_err_cnt = 0;


 void uart_init (void)
{
	UBRR0L = (uint8_t) UART_UBRR;
	UBRR0H = (uint8_t) (UART_UBRR >> 8);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // Char size 8 bit
	UCSR0B |= (UART_TX_ENABLE << TXEN0) | (UART_RX_ENABLE << RXEN0) | \
				(UART_RXC_INT_ENABLE << RXCIE0);
	
	return;
}

int uart_putc (char c, FILE *stream)
{	
	#if UART_INSERT_CR == 1
		if (c == '\n')
		{
			uart_putc ('\r', stream);
		}
	#endif
	
	__uart_tx_byte (c);
	
	return 0;
}

inline void uart_clrscr(void)
{
	printf (UART_ESC "[2J");
	return;
}

inline void uart_home (void)
{
	printf (UART_ESC "[H");
	return;
}

inline void uart_goto_xy (int x, int y)
{
	printf (UART_ESC "[%d;%dH", y, x);
	return;
}

inline void uart_reset_disp_attr (void)
{
	printf (UART_ESC "[0m");
	return;
}

inline void uart_set_disp_attr (int attr)
{
	printf (UART_ESC "[%dm", attr);
	return;
}

void __uart_tx_byte (uint8_t data)
{	// Пока что так. А вообще надо как-то через буфер писать,
	// чтобы без задержек
		
	while(!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = data;
	
	return;
}

inline void __uart_rx_byte (void)
{	// Вызов по прерыванию
	uint8_t buff = UDR0;
	
// 	if ((rx_ch_index < UART_CMD_SIZE) && (rx_cmd_index < UART_RX_BUF_SIZE))
// 	{
// 		rx_buff [rx_cmd_index][rx_ch_index] = buff;
// 		rx_ch_index++;
// 	}
// 	else
// 	{
// 		rx_err_cnt++;
// 	}
// 	
// 	if (buff == UART_CMD_TERM)
// 	{
// 		rx_cmd_index++;		// следующая команда
// 		rx_ch_index = 0;	// пишем с первой позиции
// 	}

	
// 	lcd_putc (buff, NULL);
	 
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

// void __uart_remote_ctrl(void)
// {	// Требуется RTOS !!!
// 	// Обработка всего буфера команд (до последней записанной)
// 	
// 	static uint16_t count = 0;
// //ATOMIC BEGIN
// 	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
// 	{
// 		for (uint8_t cmd = 0; cmd < rx_cmd_index; cmd++)
// 		{	// cmd ~ command
// 				
// 			for (uint8_t ch = 0; ch < UART_CMD_SIZE; ch++)
// 			{	// l ~ letter
// 				if (rx_buff [cmd][ch] == UART_CMD_TERM)
// 				{	// Команда принята. Выполняем
// 					// TODO: обработка команды и её выполнение
// 					// Тест: команды одним символом
// 						
// 					switch (rx_buff [cmd][ch-1])
// 					{
// 						case 'L':
// 						{
// 							led_g_switch ();
// 							count++;
// 							printf ("[OK] Command %d\n", count);
// 							break;
// 						}
// 						default:
// 						{
// 							// ToDo: обработка неверной посылки
// 						}
// 					}
// 						
// 					break; // выход из цикла, т.к. достигли конца команды	
// 				}
// 			}
// 				
// 		}
// 		// команды обработаны, сбрасываем счётчики
// 		rx_cmd_index = 0;
// 		rx_ch_index = 0;	// хотя по идее он и так должен быть 0
// 	}
// // ATOMIC END
// 	
// 	
// 	
// 	return;
// }
