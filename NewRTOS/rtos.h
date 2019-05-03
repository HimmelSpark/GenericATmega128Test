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

typedef void (*rtos_fptr_t)(void);	// ��� ��������� �� �������

// ToDo: Freeze-����
typedef struct {
	rtos_fptr_t func;
	uint16_t delay;
	uint16_t period;
	} rtos_task_t;				// ��� ��������� ������ � ������: �������, ����� �� ����������, ������
								// period == 0 => ������ ����������� (RTOS_RUN_ONCE)

/* ��������� ������� */
extern void __rtos_task_manager (void);
extern void __rtos_timer_service (void);
void __rtos_null_task (uint8_t i);				// ������� ������ �� �������
void __rtos_idle (void);
/*********************/

/* ������� ��� ������������ */
extern void rtos_init (void);	// ������������� ������� � ������� �����
extern void rtos_set_task (rtos_fptr_t func, uint16_t delay, uint16_t period);	// ��������/���������� ������
//extern void rtos_freeze_task(rtos_fptr_t func);	//ToDo ��������� ������
extern void rtos_delete_task (rtos_fptr_t func);	// �������� (�������) ������
/****************************/



#define RTOS_RUN_ONCE				0	// ������ �����������
#define RTOS_RUN_INSTANTLY			0	// � ������� ���������
#define RTOS_MAX_QUEUE_SIZE			32	// ������� � �������
#define RTOS_MAX_PENDING_TSKs		32	// ������, ��������� ����������


#endif /* RTOS_H_ */