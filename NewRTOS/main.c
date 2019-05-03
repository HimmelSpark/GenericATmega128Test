/*
 * NewRTOS.c
 *
 * Created: 11.12.2018 11:32:22
 * Author : Vsevolod
 */ 

#include "general.h"
#include "md3.h"
#include "rtos.h"

ISR(TIMER0_OVF_vect)
{	// вызывается каждую 1 мс (prescaler = /32, F_CPU=4000000UL)
	TCNT0 = 255 - 125;
	__rtos_timer_service();
}

int main(void)
{
	general_init();
	md3_init();
	rtos_init();
	
	rtos_set_task(led_y_on, 1000, RTOS_RUN_ONCE);
	rtos_set_task(led_y_off, 5000, RTOS_RUN_ONCE);
	rtos_set_task(led_r_switch, RTOS_RUN_INSTANTLY, 95);
	rtos_delete_task(md3_init);
	rtos_set_task(led_y_on, 2000, RTOS_RUN_ONCE);
	rtos_set_task(led_g_switch, 50, 95);
	
	sei();
	
    while (FOREVER) 
    {
		__rtos_task_manager();
    }
}

