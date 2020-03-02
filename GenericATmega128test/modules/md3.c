/*
 * md3.c
 *
 * Created: 03.12.2018 17:47:17
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "md3.h"
#include "../rtos.h"

static uint16_t __adc0 = 0x000, __adc1 = 0x000;	// здесь хранятся значения ADC между обновлениями
static uint8_t __adc_sel = 0;

void __adc_routine(void)
{
	switch (__adc_sel)
	{
		case 0:
		{
			__adc0 = ADC;
			break;
		}
		case 1:
		{
			__adc1 = ADC;
			break;
		}
	}
}

void __adc_poll(void)
{
	__adc_sel++;			// следующий ADC
		
	if(__adc_sel == ADC_NUM)
	{						// если перебрали все,
		__adc_sel = 0;		// начинаем сначала
	}

	ADMUX = (1 << REFS0) | (__adc_sel);	// опорное напряжение AVCC=5V; выбор ADC
	ADCSRA |= (1 << ADSC) | ADC_CLK_PRESCALER;
	
	return;
}

inline void md3_init (void)
{
	LED_DDR		|=	(1 << LED_R) | (1 << LED_Y) | (1 << LED_G);
	
	BUTTON_DDR	&= ~((1 << BUT0) | (1 << BUT2)  | (1 << BUT3));
	BUTTON_PORT |=	(1 << BUT0)	| (1 << BUT1) | (1 << BUT2)  | (1 << BUT3);
	
	led_r_on (); led_y_on (); led_g_on ();
	_delay_ms (MD3_START_DELAY);
	led_r_off (); led_y_off (); led_g_off ();
	
	ADCSRA	|= (1 << ADEN) | (1 << ADIE);
	rtos_set_task(__adc_poll, RTOS_RUN_ASAP, ADC_POLL_PERIOD/ADC_NUM);
	
	DEBUG_DDR |= 1 << DEBUG_PIN;
	
	rtos_set_task (led_g_switch, RTOS_RUN_ASAP, 500);
}

inline void led_y_on (void)
{
	LED_PORT |= 1 << LED_Y;
}

inline void led_y_off (void)
{
	LED_PORT &= ~(1 << LED_Y);
}

inline void led_r_on (void)
{
	LED_PORT |= 1 << LED_R;
}
inline void led_r_off (void)
{
	LED_PORT &= ~(1 << LED_R);
}

inline void led_r_switch (void)
{
	LED_PORT ^= 1 << LED_R;
}

inline void led_g_on (void)
{
	LED_PORT |= 1 << LED_G;
}

inline void led_g_off (void)
{
	LED_PORT &= ~(1 << LED_G);
}

inline void led_g_switch (void)
{
	LED_PORT ^= 1 << LED_G;
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

uint16_t md3_get_pot (void)
{	// Возвращает значения от 0 до MD3_POT_MAX (MD3_POT_TRS отводится на мёртвую зону)
	
	return (__adc0 > MD3_POT_TRS) ? (__adc0 - MD3_POT_TRS) : 0;	// мёртвая зона потенциометра

//	return 0;
}

float md3_get_voltage(void)
{
	return (float)__adc1 / (float)ADC_MAX * VOLT_MAX;
}