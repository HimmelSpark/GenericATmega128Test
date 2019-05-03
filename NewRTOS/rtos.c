/*
 * rtos.c
 *
 * Created: 11.12.2018 11:45:17
 *  Author: Vsevolod
 */ 

#include "rtos.h"
#include "general.h"
#include <util/atomic.h>

uint8_t	__debug__set_task_errors = 0,
		__debug__set_queue_errors = 0;

//ОЧЕРЕДЬ функций (задач) для выполнения
volatile static rtos_fptr_t __rtos_queue[RTOS_MAX_QUEUE_SIZE];
volatile static uint8_t queue_tail = 0;	// первая свободная позиция очереди функций (задач)

//МАССИВ задач (таймеров), ожидающих своего времени выполнения
volatile static rtos_task_t __rtos_pending_tasks[RTOS_MAX_PENDING_TSKs];

uint8_t rtos_i_max = 0;	// для статистики - насколько далеко когда-либо записывалась задача

inline void __rtos_task_manager (void)
{
	if (queue_tail == 0)	// очередь пуста
	{
		return;				// валим отсюда
	}
	
	// Есть задачи:
	
	__rtos_queue[0]();	// хапаем первую и выполняем
	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		for (int i = 0; i < (queue_tail-1); i++)	// сдвигаем всё вперёд (в т.ч. и то, что могло добавиться по ISR!)
		{
			// N.B.: можно считать и до queue_tail, тогда, учитывая, что элемент queue_tail всегда пуст,
			// получим, что бывший предхвостовой элемент обнулится, но лучше делать это явно (после цикла)
			__rtos_queue[i] = __rtos_queue[i+1];
		}
		queue_tail--;	// сдвигается хвост
		__rtos_queue[queue_tail] = __rtos_idle;	// очистили бывший предхвостовой элемент, теперь это хвост
	}
// RESTORE INTERRUPTS
	return;
}

inline void __rtos_timer_service (void)
{	// ToDo: заменить RTOS_MAX_PENDING_TSKs на (i_max + 1)
	for (int i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
	{
		if (__rtos_pending_tasks[i].func == __rtos_idle)
		{				// если попалась пустышка, то с ней не работаем,
			continue;	// а сразу прыгаем на следующую итерацию
		}
		
		if (__rtos_pending_tasks[i].delay == 0)	// задача готова к выполнению
		{
			if (queue_tail == RTOS_MAX_QUEUE_SIZE)	// если очередь полна,
			{			// то значит, в этом вызове ISR ничего не поменяется
				break;	// и смело можно выходить, в следующий раз повезёт. А пока не удаляем.
			}
			
			//  Если очередь готова принять функцию:
			
			__rtos_queue[queue_tail] = __rtos_pending_tasks[i].func;
			queue_tail++;
			
			if(__rtos_pending_tasks[i].period != NULL)	// задача периодическая?
			{	// да, тогда не удаляем, а обновляем её
				__rtos_pending_tasks[i].delay = __rtos_pending_tasks[i].period;
			}
			else
			{	// нет, тогда очищаем задачу по её индексу (раз уж мы его знаем)
				__rtos_null_task(i);
			}
		}
		else									// задача не готова к выполнению
		{
			__rtos_pending_tasks[i].delay--;	// приближаем момент готовности
		}
	}
	return;
}


inline void __rtos_null_task (uint8_t i)
{
	__rtos_pending_tasks[i].func	= __rtos_idle;	// ничё не делаем
	__rtos_pending_tasks[i].delay	= NULL;			// "ничё не делаем" уже выполнили
	__rtos_pending_tasks[i].period	= NULL;			// и больше не нужно	
}

void __rtos_idle (void)
{
	return;
}

inline void rtos_init (void)
{
	TCCR0 |= (1 << CS01) | (1 << CS00); // запуск таймера0, prescaler = /32
	TCNT0 = 255 - 125;					// 255=максимум 8бит таймера, 125=тиков в 1 мс
	TIMSK = (1 << TOIE0);				// прерывания по переполнению таймера0
	
	for (int i = 0; i < RTOS_MAX_QUEUE_SIZE; i++)
	{
		__rtos_queue[i] = __rtos_idle;
	}
	for (int i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
	{
		__rtos_null_task(i);
	}
	return;
}

void rtos_set_task (rtos_fptr_t func, uint16_t delay, uint16_t period)
{	// ToDo: коды ошибок
	uint8_t i = 0xFF;	// индекс для записи; пока что undefined
	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Здесь вообще говоря не нужно искать до конца; нужно искать до наибольшего
		// индекса, который когда-либо был занят, но для теста и так сойдёт
		// ToDo: заменить RTOS_MAX_PENDING_TSKs на (i_max + 1)
		for (uint8_t j = 0; j < RTOS_MAX_PENDING_TSKs; j++)				// ищем место или такую же ф-цию
		{
			if (__rtos_pending_tasks[j].func == func)				// если нашли такую же функцию
			{
				i = j;	// будем обновлять её
				break;	// искать больше нечего
			}
			if ((__rtos_pending_tasks[j].func == __rtos_idle) && (i == 0xFF))	// параллельно ищем место
			{				// если его нашли и при этом ещё не запомнили
				i = j;		// запоминаем и больше это значение не обновляем
			}
		}
	
		if (i != 0xFF)
		{	// если всё ок, планируем/обновляем:
			__rtos_pending_tasks[i].func	= func;
			__rtos_pending_tasks[i].delay	= delay;
			__rtos_pending_tasks[i].period	= period;
		
			if (i > rtos_i_max)	// обновим статистику
			{
				rtos_i_max = i;
			}
		}
		else
		{	// если после всех поисков ничего не нашли, то задача не добавляется (но вообще это маловероятно)
			__debug__set_task_errors++;
		}
	}
// RESTORE INTERRUPTS
	return;
}

//void rtos_freeze_task(rtos_fptr_t func)
//{
//// DISABLE INTERRUPTS
	//ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	//{
		//for (uint8_t i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
		//{
			//if (__rtos_pending_tasks[i].func == func)	// нашли индекс задачи для удаления
			//{
				//__rtos_pending_tasks[i].freeze = RTOS_TSK_FREEZE;	// заморозили задачу по её индексу
				//break;											// и больше делать здесь нечего
			//}
		//}		
	//}
//// RESTORE INTERRUPTS
//}

void rtos_delete_task (rtos_fptr_t func)
{	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// ToDo: заменить RTOS_MAX_PENDING_TSKs на (rtos_i_max + 1)
		for (uint8_t i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
		{
			if (__rtos_pending_tasks[i].func == func)	// нашли индекс задачи для удаления
			{
				__rtos_null_task(i);	// очистили задачу по её индексу
				break;					// и больше делать здесь нечего
			}
		}
	}
// RESTORE INTERRUPTS
	return;
}
