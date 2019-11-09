/*
 * fifo.c
 *
 * Created: 25.04.2019 12:30:07
 *  Author: Vsevolod
 */ 

#include "general.h"
#include "fifo.h"
#include <stdlib.h>	// для malloc


void fifo_init (FIFO_BUFFER_t *buf_p, uint16_t size)
{
	buf_p->size = size;
	buf_p->buffer = (uint8_t *) malloc (size);
	fifo_flush (buf_p);
	
	return;
}

void fifo_push (uint8_t c, FIFO_BUFFER_t* buf_p)
{
	buf_p->buffer[buf_p->idxIn++] = c;
	if (buf_p->idxIn >= buf_p->size)
	{
		buf_p->idxIn = 0;
	}
	return;
}

uint8_t fifo_pop (FIFO_BUFFER_t *buf_p)
{
	uint8_t retval;

	if(fifo_pop_avail(buf_p))					// Если есть чего доставать, 
	{
		retval = buf_p->buffer[buf_p->idxOut++];// достаём
		if (buf_p->idxOut >= buf_p->size)
		{
			buf_p->idxOut = 0;
		}
	}
	else										// Если доставать нечего,
	{
		retval = FIFO_NULL;						// возвращаем NULL и больше ничего не делаем
	}
	
	return retval;
}

inline uint8_t fifo_pop_avail(FIFO_BUFFER_t *buf_p)
{
	// Если можем достать элемент, то возвращаем 1; в противном случае 0;
	return (buf_p->idxIn != buf_p->idxOut) ? 1 : 0;
}

void fifo_flush (FIFO_BUFFER_t *buf_p)
{
	buf_p->idxIn = 0;
	buf_p->idxOut = 0;
	
	return;
}
