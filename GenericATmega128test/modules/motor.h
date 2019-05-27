/*
 * motor.h
 *
 * Created: 13.12.2018 17:40:46
 *  Author: Vsevolod
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_

/*
Работа ШИМ - на Timer/Counter1

Двигатель L: PB6, OC1B;
Двигатель R: PB7, OC1C

Энкодер L: PD2, INT2 (запасной PE4/INT4 с диодом)
Энкодер R: PD3, INT3 (запасной PE5/INT5 с диодом)

*/

typedef struct {
	double omegaL;
	double omegaR;
	} MOTOR_OMEGA_DATA;

typedef struct {
	float powL;
	float powR;
	} MOTOR_POWER_DATA;

/* Функции для пользователя */
void motors_init (void);					// Инициализация ШИМ, прерываний и т.д.
void motors_arm (void);						// Подключение тактирования, прерываний, ПИ-рег.
void motors_disarm (void);					// Отключение тактирования, прерываний, ПИ-рег.
MOTOR_OMEGA_DATA motors_get_omega (void);	// Возвращает структуру из скоростей вращения колёс
MOTOR_OMEGA_DATA motors_get_omega_obj (void);	// Возвращает структуру из уставок скоростей
MOTOR_POWER_DATA motors_get_power (void);		// Возвращает структуру из текущих мощностей двигателей
void motors_set_omega (double omega_L, double omega_R); // Задать уставки

/* Служебные функции */
void __enc_L (void);		// событие энкодера L (фаза 1)	ToDo: фаза 2
void __enc_R (void);		// событие энкодера R (фаза 1)	ToDo: фаза 2
void __enc_L_enable (void);		// разрешить чтение с энкодера L
void __enc_R_enable (void);		// разрешить чтение с энкодера R
void __motors_set_pwm (uint8_t duty_cycle_l, uint8_t duty_cycle_r);	// Задание скважности, 0 - 255
void __motors_omega_estimator (void);	// оценка производной угла поворота для двух двигателей
void __motors_pi_controller (void);		// ПИ-регулятор для двух двигателей
void __motors_obj_poll (void);			// опрос задатчика скорости двигателей (отладка)


/* Аппаратная область */

#define MOTOR_L 0
#define MOTOR_R	1

#define MOTORS_DDR		DDRB
#define MOTORS_PORT		PORTB

#define MOTOR_L_PWM_PIN	PB6
#define MOTOR_R_PWM_PIN	PB7

// #define ENCODERS_PORT	PORTE
// #define ENCODERS_DDR	DDRE
// #define ENCODERS_PIN	PINE
// #define ENC_L_PIN		PINE4
// #define ENC_R_PIN		PINE5

#define ENCODERS_PORT	PORTD
#define ENCODERS_DDR	DDRD
#define ENCODERS_PIN	PIND
#define ENC_L_PIN		PIND2
#define ENC_R_PIN		PIND3

#define ENC_FILTER_TIME	500		// us; для фильтра ложных прерываний

// /* Макросы управления прерываниями */
// #define __ENC_L_ISR_ON		EIMSK |= 1 << INT2
// #define __ENC_L_ISR_OFF		EIMSK &= ~(1 << INT2)
// #define __ENC_R_ISR_ON		EIMSK |= 1 << INT3
// #define __ENC_R_ISR_OFF		EIMSK &= ~(1 << INT3)

/* Программная область */
#define MOTORS_OBJ_POLL_PERIOD	100	// опрос задатчика скорости каждые 100 мс

#define MOTORS_PWM_CONSTR_MIN	25		// экспериментально установленный порог страгивания
#define MOTORS_PWM_CONSTR_MAX	200		// ограничение сверху

#define MOTORS_SPEED_OBJ_MAX		40.0	// рад/с
#define MOTORS_SPEED_OBJ_ADC_TRS	10		// мёртвая зона в единицах ADC

#define MOTORS_STARTUP_TIME		1000	// ms

/* Область определений алгоритмов оценки/управления */

#define ESTIM_PERIOD		10		// ms; период вызова алгоритма оценки
#define ESTIM_CONST_dT		0.01	// s; для алгоритма оценки
#define ESTIM_CONST_Ki		5.0
#define ESTIM_CONST_Kp		1.9		// только для ПИ-фильтра
//#define ESTIM_SCALE			1.5365	// °/имп
#define ESTIM_SCALE			0.0268376	// рад/имп

#define PICONTR_PERIOD		10		// ms
#define PICONTR_CONST_dT	0.01	// s
#define PICONTR_CONST_Kp	3.0
#define PICONTR_CONST_Ki	10.0


#endif /* MOTOR_H_ */