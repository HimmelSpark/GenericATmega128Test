/*
 * md3_defs.h
 *
 * Created: 05.05.2018 15:04:03
 *  Author: Всеволод
 */ 


#ifndef MD3_H_
#define MD3_H_

/* Служебные функции */
void __adc_routine(void);	// обработчик прерываний АЦП
void __adc_poll(void);	// опрос АЦП
/*********************/

/* Функции для пользователя */
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
uint16_t md3_get_pot (void);	// положение потенциометра
float md3_get_voltage(void);	// напряжение первичного источника питания
/****************************/


#define LED_PORT	PORTD
#define LED_DDR		DDRD

#define LED_R		PD5
#define LED_Y		PD6
#define LED_G		PD7

#define BUTTON_PORT	PORTB
#define BUTTON_DDR	DDRB
#define BUTTON_PIN	PINB
#define BUTTON_MSK	0x0F	// для "кнопок" используется не весь порт

#define BUT0		PB0
#define BUT1		PB1 // на PB1 выход SCK, и на него лучше не вешать конденсатор
#define BUT2		PB2	
#define BUT3		PB3

#define VOLT_MAX			15.24	// максимальное измеряемое напряжение первичного источника питания
									// (учитывая, что коэффициент делителя напряжения = 3,
									// максимальное напряжение на входе АЦП составит 5 В)
									// 15.24 - для питания аккумулятором
									// 15.6 - для питания с БП (на его проводах падение напряжения что ли,
									// поэтому другой коэффициент уже....)

#define MD3_START_DELAY		2000	// ms, задержка перед аппаратной инициализацией всех модулей/интерфейсов
									// (предполагается, что инициализация md3 - перед
									// остальными инициализациями)
									
#define ADC_POLL_PERIOD		100	// ms
#define ADC_NUM				2		// сколько ADC, начиная с ADC0, опрашиваем

#define ADC_CLK_PRESCALER	0b111	// XTAL div
#define ADC_MAX				0x3FF	// 10-разрядный АЦП

// #define MD3_MUX_SELECT		0b00000	// выбор входа ADC (выбран ADC0 -> PF0 -> потенциометр)
// #define MD3_ADC_PRESCALER	0b111	// /128

// #define ADC_SEL_0			0
// #define ADC_SEL_1			1

#define MD3_POT_MAX			1000
#define MD3_POT_TRS			23		// мёртвая зона (для исключения дребезга нуля)

#define DEBUG_PORT			PORTF
#define DEBUG_DDR			DDRF
#define DEBUG_PIN			PF3

#define __DEBUG_PIN_HIGH	DEBUG_PORT |= 1 << DEBUG_PIN
#define __DEBUG_PIN_LOW		DEBUG_PORT &= ~(1 << DEBUG_PIN)
#define __DEBUG_PIN_SWITCH	DEBUG_PORT ^= 1 << DEBUG_PIN

#endif /* MD3_DEFS_H_ */