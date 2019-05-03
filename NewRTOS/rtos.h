/*
 * rtos.h
 *
 * Created: 11.12.2018 11:45:31
 *  Author: Vsevolod
 */ 


#ifndef RTOS_H_
#define RTOS_H_

#include "rtos.h"
#include "general.h"

typedef void (*rtos_fptr_t)(void);	// тип указателя на функцию

// ToDo: Freeze-флаг
typedef struct {
	rtos_fptr_t func;
	uint16_t delay;
	uint16_t period;
	} rtos_task_t;				// тип структуры данных о задаче: функция, время до выполнения, период
								// period == 0 => задача одноразовая (RTOS_RUN_ONCE)

/* Сервисные функции */
extern void __rtos_task_manager (void);
extern void __rtos_timer_service (void);
void __rtos_null_task (uint8_t i);				// очищает задачу по индексу
void __rtos_idle (void);
/*********************/

/* Функции для пользователя */
extern void rtos_init (void);	// Инициализация очереди и массива задач
extern void rtos_set_task (rtos_fptr_t func, uint16_t delay, uint16_t period);	// Создание/обновление задачи
//extern void rtos_freeze_task(rtos_fptr_t func);	//ToDo Заморозка задачи
extern void rtos_delete_task (rtos_fptr_t func);	// Удаление (очистка) задачи
/****************************/



#define RTOS_RUN_ONCE				0	// задача одноразовая
#define RTOS_RUN_INSTANTLY			0	// с нулевой задержкой
#define RTOS_MAX_QUEUE_SIZE			32	// функции в очереди
#define RTOS_MAX_PENDING_TSKs		32	// задачи, ожидающие выполнения


#endif /* RTOS_H_ */