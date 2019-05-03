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

//������� ������� (�����) ��� ����������
volatile static rtos_fptr_t __rtos_queue[RTOS_MAX_QUEUE_SIZE];
volatile static uint8_t queue_tail = 0;	// ������ ��������� ������� ������� ������� (�����)

//������ ����� (��������), ��������� ������ ������� ����������
volatile static rtos_task_t __rtos_pending_tasks[RTOS_MAX_PENDING_TSKs];

uint8_t rtos_i_max = 0;	// ��� ���������� - ��������� ������ �����-���� ������������ ������

inline void __rtos_task_manager (void)
{
	if (queue_tail == 0)	// ������� �����
	{
		return;				// ����� ������
	}
	
	// ���� ������:
	
	__rtos_queue[0]();	// ������ ������ � ���������
	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		for (int i = 0; i < (queue_tail-1); i++)	// �������� �� ����� (� �.�. � ��, ��� ����� ���������� �� ISR!)
		{
			// N.B.: ����� ������� � �� queue_tail, �����, ��������, ��� ������� queue_tail ������ ����,
			// �������, ��� ������ ������������� ������� ���������, �� ����� ������ ��� ���� (����� �����)
			__rtos_queue[i] = __rtos_queue[i+1];
		}
		queue_tail--;	// ���������� �����
		__rtos_queue[queue_tail] = __rtos_idle;	// �������� ������ ������������� �������, ������ ��� �����
	}
// RESTORE INTERRUPTS
	return;
}

inline void __rtos_timer_service (void)
{	// ToDo: �������� RTOS_MAX_PENDING_TSKs �� (i_max + 1)
	for (int i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
	{
		if (__rtos_pending_tasks[i].func == __rtos_idle)
		{				// ���� �������� ��������, �� � ��� �� ��������,
			continue;	// � ����� ������� �� ��������� ��������
		}
		
		if (__rtos_pending_tasks[i].delay == 0)	// ������ ������ � ����������
		{
			if (queue_tail == RTOS_MAX_QUEUE_SIZE)	// ���� ������� �����,
			{			// �� ������, � ���� ������ ISR ������ �� ����������
				break;	// � ����� ����� ��������, � ��������� ��� ������. � ���� �� �������.
			}
			
			//  ���� ������� ������ ������� �������:
			
			__rtos_queue[queue_tail] = __rtos_pending_tasks[i].func;
			queue_tail++;
			
			if(__rtos_pending_tasks[i].period != NULL)	// ������ �������������?
			{	// ��, ����� �� �������, � ��������� �
				__rtos_pending_tasks[i].delay = __rtos_pending_tasks[i].period;
			}
			else
			{	// ���, ����� ������� ������ �� � ������� (��� �� �� ��� �����)
				__rtos_null_task(i);
			}
		}
		else									// ������ �� ������ � ����������
		{
			__rtos_pending_tasks[i].delay--;	// ���������� ������ ����������
		}
	}
	return;
}


inline void __rtos_null_task (uint8_t i)
{
	__rtos_pending_tasks[i].func	= __rtos_idle;	// ���� �� ������
	__rtos_pending_tasks[i].delay	= NULL;			// "���� �� ������" ��� ���������
	__rtos_pending_tasks[i].period	= NULL;			// � ������ �� �����	
}

void __rtos_idle (void)
{
	return;
}

inline void rtos_init (void)
{
	TCCR0 |= (1 << CS01) | (1 << CS00); // ������ �������0, prescaler = /32
	TCNT0 = 255 - 125;					// 255=�������� 8��� �������, 125=����� � 1 ��
	TIMSK = (1 << TOIE0);				// ���������� �� ������������ �������0
	
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
{	// ToDo: ���� ������
	uint8_t i = 0xFF;	// ������ ��� ������; ���� ��� undefined
	
// DISABLE INTERRUPTS
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// ����� ������ ������ �� ����� ������ �� �����; ����� ������ �� �����������
		// �������, ������� �����-���� ��� �����, �� ��� ����� � ��� �����
		// ToDo: �������� RTOS_MAX_PENDING_TSKs �� (i_max + 1)
		for (uint8_t j = 0; j < RTOS_MAX_PENDING_TSKs; j++)				// ���� ����� ��� ����� �� �-���
		{
			if (__rtos_pending_tasks[j].func == func)				// ���� ����� ����� �� �������
			{
				i = j;	// ����� ��������� �
				break;	// ������ ������ ������
			}
			if ((__rtos_pending_tasks[j].func == __rtos_idle) && (i == 0xFF))	// ����������� ���� �����
			{				// ���� ��� ����� � ��� ���� ��� �� ���������
				i = j;		// ���������� � ������ ��� �������� �� ���������
			}
		}
	
		if (i != 0xFF)
		{	// ���� �� ��, ���������/���������:
			__rtos_pending_tasks[i].func	= func;
			__rtos_pending_tasks[i].delay	= delay;
			__rtos_pending_tasks[i].period	= period;
		
			if (i > rtos_i_max)	// ������� ����������
			{
				rtos_i_max = i;
			}
		}
		else
		{	// ���� ����� ���� ������� ������ �� �����, �� ������ �� ����������� (�� ������ ��� ������������)
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
			//if (__rtos_pending_tasks[i].func == func)	// ����� ������ ������ ��� ��������
			//{
				//__rtos_pending_tasks[i].freeze = RTOS_TSK_FREEZE;	// ���������� ������ �� � �������
				//break;											// � ������ ������ ����� ������
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
		// ToDo: �������� RTOS_MAX_PENDING_TSKs �� (rtos_i_max + 1)
		for (uint8_t i = 0; i < RTOS_MAX_PENDING_TSKs; i++)
		{
			if (__rtos_pending_tasks[i].func == func)	// ����� ������ ������ ��� ��������
			{
				__rtos_null_task(i);	// �������� ������ �� � �������
				break;					// � ������ ������ ����� ������
			}
		}
	}
// RESTORE INTERRUPTS
	return;
}
