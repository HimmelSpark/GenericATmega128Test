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
static volatile rtos_fptr_t __rtos_queue[RTOS_QUEUE_SIZE];
static volatile uint8_t __rtos_queue_tail = 0; // первая свободная позиция очереди функций (задач)

//МАССИВ задач (таймеров), ожидающих своего времени выполнения
static volatile rtos_task_t __rtos_pending_tasks[RTOS_MAX_PENDING_TSKs];

uint8_t rtos_i_max = 0;		// для статистики - насколько далеко когда-либо записывалась задача
uint8_t rtos_tail_max = 0;	// для статистики - насколько большой когда-либо была очередь

inline void __rtos_task_manager (void)
{
	if (__rtos_queue_tail == 0)	// очередь пуста
	{
		return;				// валим отсюда
	}
	
	// Есть задачи:
	
	__rtos_queue[0] ();	// хапаем первую и выполняем
	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
	{
		for (int i = 0; i < (__rtos_queue_tail-1); i++)	// сдвигаем всё вперёд (в т.ч. и то, что могло добавиться по ISR!)
		{
			// N.B.: можно считать и до queue_tail, тогда, учитывая, что элемент queue_tail всегда пуст,
			// получим, что бывший предхвостовой элемент обнулится, но лучше делать это явно (после цикла)
			__rtos_queue[i] = __rtos_queue[i+1];
		}
		__rtos_queue_tail--;	// сдвигается хвост
		__rtos_queue[__rtos_queue_tail] = __rtos_idle;	// очистили бывший предхвостовой элемент, теперь это хвост
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
			if (__rtos_queue_tail == RTOS_QUEUE_SIZE)	// если очередь полна,
			{			// то значит, в этом вызове ISR ничего не поменяется
				break;	// и смело можно выходить, в следующий раз повезёт. А пока не удаляем.
			}
			
			//  Если очередь готова принять функцию:
			
			__rtos_queue[__rtos_queue_tail] = __rtos_pending_tasks[i].func;
			__rtos_queue_tail++;
			if (__rtos_queue_tail > rtos_tail_max)
			{
				rtos_tail_max = __rtos_queue_tail;	// обновим статистику
			}
			
			if(__rtos_pending_tasks[i].period != RTOS_RUN_ONCE)	// задача периодическая?
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
	__rtos_pending_tasks[i].func	= __rtos_idle;
	__rtos_pending_tasks[i].delay	= 0;
	__rtos_pending_tasks[i].period	= 0;
}

void __rtos_idle (void)
{
	return;
}

inline void rtos_init (void)
{
	// Вариант инициализации для TIMER0 (НЕ используем XDIV):
// 	OCR0 = 125;		// 125=тиков в 1 мс
// 	TCCR0 |= (1 << WGM01) | (1 << CS02) | (0 << CS01) | (0 << CS00);
// 	// запуск таймера0, prescaler = /64; очитка TCNT при совпадении
// 	TIMSK |= (1 << OCIE0);		// прерывания при совпадении с OCR0

	// Вариант инициализации для TIMER2 (используем XDIV):
	OCR2 = 125;		// 125=тиков в 1 мс
	TCCR2 |= (1 << WGM21) | (0 << CS22) | (1 << CS21) | (1 << CS20);
	// запуск таймера2, prescaler = /64; очитка TCNT при совпадении
	TIMSK |= (1 << OCIE2);		// прерывания при совпадении с OCR2
	
	for (int i = 0; i < RTOS_QUEUE_SIZE; i++)
	{
		__rtos_queue[i] = __rtos_idle;
	}
	for (int i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
	{
		__rtos_null_task (i);
	}
	
	return;
}

void rtos_set_task (rtos_fptr_t func, uint16_t delay, uint16_t period)
{	// ToDo: коды ошибок
	uint8_t i = 0xFF;	// индекс для записи; пока что undefined
	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
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

void rtos_delete_task (rtos_fptr_t func)
{	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
	{
		// ToDo: заменить RTOS_MAX_PENDING_TSKs на (rtos_i_max + 1)
		for (uint8_t i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
		{
			if (__rtos_pending_tasks[i].func == func)	// нашли индекс задачи для удаления
			{
				__rtos_null_task (i);	// очистили задачу по её индексу
				break;					// и больше делать здесь нечего
			}
		}
		
		// Чтобы гарантировать, что после удаления задачи она ни разу не выполнится,
		// нужно также целиком просмотреть очередь задач для выполнения:
		
		for (uint8_t i = 0; i < __rtos_queue_tail; i++)
		{
			if (__rtos_queue[i] == func)		// нашли задачу в очереди
			{
				__rtos_queue[i] = __rtos_idle;	// заменили на простой (время потеряем, но не существенно)
			}
		}
		
	}
// RESTORE INTERRUPTS
	return;
}
