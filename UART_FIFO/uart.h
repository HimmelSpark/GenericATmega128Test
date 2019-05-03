/*
 * uart.h
 *
 * Created: 28.03.2019 13:44:50
 *  Author: Vsevolod
 */ 

/*

Используем UART0

*/


#ifndef UART_H_
#define UART_H_


/* Пользовательские функции */
void uart_init (void);
int uart_stdputc (char c, FILE *stream);	// для стд вывода
void uart_putc (char c);					// для обычного вывода
void uart_puts (char *str);					// вывод строки

// Следующие функии используют ESC-последовательности:
void uart_clrscr (void);
void uart_home (void);
void uart_goto_xy (int x, int y);
void uart_reset_disp_attr (void);
void uart_set_disp_attr (int attr);
/****************************/

/* Служебные функции */
void __uart_tx_byte (uint8_t data);	// передача байта
void __uart_rx_byte (void);			// приём байта (вызов по прерыванию)
/**********************/


#define UART_BAUDRATE	9600UL
#define	UART_UBRR		(F_CPU/(16*UART_BAUDRATE) - 1)

 
#define UART_TX_BUF_SIZE	256

#define UART_TX_ENABLE		1
#define UART_RX_ENABLE		0
#define UART_RXC_INT_ENABLE	0

#define UART_INSERT_CR		1	// после \n вставляется \r



/*************************/
// ФОРМАТИРОВАННЫЙ ВЫВОД //
/*************************/

#define UART_ESC	"\033"

#define ATTR		"\033["
#define ATTR_END	"m"
#define ATTR_RST	"\033[0m"

//Format text
#define RESET 		0
#define BRIGHT 		1
#define DIM			2
#define UNDERSCORE	3
#define BLINK		4
#define REVERSE		5
#define HIDDEN		6

//Foreground Colours (text)
#define F_BLACK 	30
#define F_RED		31
#define F_GREEN		32
#define F_YELLOW	33
#define F_BLUE		34
#define F_MAGENTA 	35
#define F_CYAN		36
#define F_WHITE		37

//Background Colours
#define B_BLACK 	40
#define B_RED		41
#define B_GREEN		42
#define B_YELLOW	44
#define B_BLUE		44
#define B_MAGENTA 	45
#define B_CYAN		46
#define B_WHITE		47


#endif /* UART_H_ */