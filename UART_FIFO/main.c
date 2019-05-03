/*
 * UART_FIFO.c
 *
 * Created: 25.04.2019 12:26:59
 * Author : Vsevolod
 */ 

#include "general.h"
#include "md3.h"
#include "uart.h"
#include "fifo.h"

static FILE _UART_	= FDEV_SETUP_STREAM (uart_stdputc, NULL, _FDEV_SETUP_WRITE);

ISR (USART0_UDRE_vect)
{
	// UDR �����, ����� � ���� �� ������:
	volatile extern FIFO_BUFFER_t __uart_tx_buf;	// ����� �� ������� uart.c
	UDR0 = fifo_pop (&__uart_tx_buf);
// ��������! ������� ��������, �.�. �������� � ������� ��������, � �� ����� ���������:
	if (__uart_tx_buf.idxOut == __uart_tx_buf.idxIn)	// ���� �� ������ ������ �������� ������ ������,
	{
		UCSR0B &= ~(1 << UDRIE0);	// ��������� ������ ����������
	}
}


int main(void)
{
	uint8_t i = 0;
	
	general_init ();
	md3_init ();
	uart_init ();
	
	sei ();
	
	stdout = &_UART_;
	
	printf ("This is a stdout device\n\n");
	
    
    while (1) 
    {
		_delay_ms (1000);
		printf ("New Line %d\n", i++);
    }
}

