/*
 * fifo.h
 *
 * Created: 25.04.2019 12:30:18
 *  Author: Vsevolod
 */ 

/* Функции для работы с кольцевым буфером */


#ifndef FIFO_H_
#define FIFO_H_

typedef struct
{
	uint8_t		*buffer;
	uint16_t	idxIn;
	uint16_t	idxOut;
	uint16_t	size;
	} FIFO_BUFFER_t;

void fifo_init (FIFO_BUFFER_t *buf_p, uint16_t size);	// инициализация, выделение памяти
void fifo_push (uint8_t c, FIFO_BUFFER_t* buf_p);		// кладём байт
uint8_t fifo_pop (FIFO_BUFFER_t *buf_p);				// достаём байт
void fifo_flush (FIFO_BUFFER_t *buf_p);					// очистка буфера

// buf_p ~ buffer pointer



#endif /* FIFO_H_ */