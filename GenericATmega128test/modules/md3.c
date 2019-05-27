/*
 * md3.c
 *
 * Created: 03.12.2018 17:47:17
 *  Author: Vsevolod
 */ 

#include "md3.h"
#include "../general.h"
#include "../rtos.h"

inline void md3_init (void)
{
	LED_DDR		|=	(1 << LED_R) | (1 << LED_Y) | (1 << LED_G);
	
	BUTTON_DDR	&= ~((1 << BUT0) | (1 << BUT2)  | (1 << BUT3));				// и так 0, но на всякий случай
	BUTTON_PORT |=	(1 << BUT0)	| (1 << BUT1) | (1 << BUT2)  | (1 << BUT3);		// подтяжка
	
	led_r_on (); led_y_on (); led_g_on ();
	_delay_ms (MD3_START_DELAY);
	led_r_off (); led_y_off (); led_g_off ();
	
	ADMUX	|= (1 << REFS0) | MD3_MUX_SELECT;
	ADCSRA	|= (1 << ADEN) | (1 << ADSC) | (1 << ADFR) | MD3_ADC_PRESCALER;
	
	DEBUG_DDR |= 1 << DEBUG_PIN;
	
	rtos_set_task (led_g_switch, RTOS_RUN_ASAP, 500);
}

void led_y_on (void)
{
	LED_PORT |= 1 << LED_Y;
	return;
}

void led_y_off (void)
{
	LED_PORT &= ~(1 << LED_Y);
	return;
}

void led_r_on (void)
{
	LED_PORT |= 1 << LED_R;
	return;
}
void led_r_off (void)
{
	LED_PORT &= ~(1 << LED_R);
	return;
}

void led_r_switch (void)
{
	LED_PORT ^= 1 << LED_R;
	return;
}

void led_g_on (void)
{
	LED_PORT |= 1 << LED_G;
	return;
}

void led_g_off (void)
{
	LED_PORT &= ~(1 << LED_G);
	return;
}

void led_g_switch (void)
{
	LED_PORT ^= 1 << LED_G;
	return;
}

inline void led_y_blink (void)
{
	led_y_on();
	rtos_set_task (led_y_off, 100, RTOS_RUN_ONCE);
}

inline void led_r_blink (void)
{
	led_r_on();
	rtos_set_task (led_r_off, 100, RTOS_RUN_ONCE);
}