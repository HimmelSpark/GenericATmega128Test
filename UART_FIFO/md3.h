/*
 * md3_defs.h
 *
 * Created: 05.05.2018 15:04:03
 *  Author: Всеволод
 */ 


#ifndef MD3_H_
#define MD3_H_

void md3_init (void);
void led_y_on (void);
void led_y_off (void);
void led_r_on (void);
void led_r_off (void);
void led_r_switch (void);
void led_g_on (void);
void led_g_off (void);
void led_g_switch (void);
void led_y_blink (void);
void led_r_blink (void);

#define LED_PORT	PORTD
#define LED_DDR		DDRD

#define LED_R PD5
#define LED_Y PD6
#define LED_G PD7

#define BUTTON_PORT	PORTB
#define BUTTON_DDR	DDRB
#define BUTTON_PIN	PINB

#define BUT0	PB0
#define BUT2	PB2	// не ошибка; на PB1 выход SCK, и на него лучше не вешать конденсатор
#define BUT3	PB3

#define MD3_START_DELAY		2000	// ms, задержка перед аппаратной инициализацией всех модулей/интерфейсов
									// (предполагается, что инициализация md3 - перед
									// остальными инициализациями)

#define MD3_MUX_SELECT		0b00000	// выбор входа ADC (выбран ADC0 -> PF0 -> потенциометр)
#define MD3_ADC_PRESCALER	0b111	// /128

#define ADC_MAX				1024

#endif /* MD3_DEFS_H_ */