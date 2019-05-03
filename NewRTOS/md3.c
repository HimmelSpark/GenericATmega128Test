/*
 * md3.c
 *
 * Created: 03.12.2018 17:47:17
 *  Author: Vsevolod
 */ 

#include "md3.h"
#include "general.h"
#include "rtos.h"

inline void md3_init(void)
{
	LED_DDR		|=	(1 << LED_R) | (1 << LED_Y) | (1 << LED_G);
	
	BUTTON_DDR	&= ~((1 << BUT0) | (1 << BUT2)  | (1 << BUT3));		// и так 0, но на всякий случай
	BUTTON_PORT |=	(1 << BUT0)	 | (1 << BUT2)  | (1 << BUT3);		// подтяжка
}

void led_y_on(void)
{
	LED_PORT |= 1 << LED_Y;
	return;
}

void led_y_off(void)
{
	LED_PORT &= ~(1 << LED_Y);
	return;
}

void led_r_on(void)
{
	LED_PORT |= 1 << LED_R;
	return;
}
void led_r_off(void)
{
	LED_PORT &= ~(1 << LED_R);
	return;
}

void led_g_switch (void)
{
	LED_PORT ^= 1 << LED_G;
	return;
}

void led_r_switch (void)
{
	LED_PORT ^= 1 << LED_R;
	return;
}

//inline void led_y_blink(void)
//{
	//led_y_on();
	//rtos_set_r_tsk(R_TSK_LEDY_OFF, 100);
//}
//
//inline void led_r_blink(void)
//{
	//led_r_on();
	//rtos_set_r_tsk(R_TSK_LEDR_OFF, 100);
//}