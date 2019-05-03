/*
 * md3_defs.h
 *
 * Created: 05.05.2018 15:04:03
 *  Author: Всеволод
 */ 


#ifndef MD3_H_
#define MD3_H_

extern void md3_init(void);
extern void led_y_on(void);
extern void led_y_off(void);
extern void led_r_on(void);
extern void led_r_off(void);
extern void led_g_switch(void);
extern void led_r_switch(void);
// extern void led_y_blink(void);
// extern void led_r_blink(void);

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

#endif /* MD3_DEFS_H_ */