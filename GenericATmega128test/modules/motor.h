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
	
typedef struct {
	double eps_L;
	double eps_R;
	double I_L;
	double I_R;
	int16_t u_L;
	int16_t u_R;
	} __debug_picontr_data;
	

/* Служебные функции */
void __enc_L (void);			// событие энкодера L (фаза 1)	ToDo: фаза 2
void __enc_R (void);			// событие энкодера R (фаза 1)	ToDo: фаза 2
void __enc_L_enable (void);		// разрешить чтение с энкодера L
void __enc_R_enable (void);		// разрешить чтение с энкодера R
void __motors_set_thrust (int16_t thrust_L, int16_t thrust_R);	// "тяга"; с учётом знака (на будущее)
void __motors_omega_estimator (void);			// оценка производной угла поворота для двух двигателей
void __motors_omega_estimator_diseng (void);	// отключить оцениватель (для отложенного отключения)
void __motors_pi_controller (void);				// ПИ-регулятор

//void step_test (void);					// снятие ступенчатой характеристики (отладка)

/*********************/

/* Функции для пользователя */
void motors_init (void);						// Инициализация ШИМ, прерываний и т.д.
void motors_arm (void);							// Подключение тактирования, прерываний, ПИ-рег.
void motors_disarm (void);						// Отключение тактирования, прерываний, ПИ-рег.
MOTOR_OMEGA_DATA motors_get_omega (void);		// Возвращает структуру из скоростей вращения колёс
MOTOR_OMEGA_DATA motors_get_omega_obj (void);	// Возвращает структуру из уставок скоростей
MOTOR_POWER_DATA motors_get_power (void);		// Возвращает структуру из текущих мощностей двигателей
void motors_set_omega (double omega_L, double omega_R); // Задать уставки
/****************************/



/* Аппаратные определения */

#define MOTOR_L 0
#define MOTOR_R	1

#define MOTORS_DDR			DDRB
#define MOTORS_PORT			PORTB

#define MOTOR_L_PWM_PIN		PB6
#define MOTOR_R_PWM_PIN		PB7

#define MOTOR_L_OCR			OCR1B
#define MOTOR_R_OCR			OCR1C

#define ENCODERS_PORT		PORTD
#define ENCODERS_DDR		DDRD
#define ENCODERS_PIN		PIND
#define ENC_L_PIN			PIND2
#define ENC_R_PIN			PIND3


/* Программные определения */

#define ENC_FILTER_TIME				500		// us; для фильтра ложных прерываний

#define MOTORS_PWM_TRS				10		// экспериментально установленный порог страгивания
#define MOTORS_PWM_CONSTR_MAX		200		// ограничение сверху

#define MOTORS_THRUST_POS			1
#define MOTORS_THRUST_NEG			(-1)

#define MOTORS_STARTUP_TIME			1000	// ms

/* Определения алгоритмов оценки/управления */

// #define USE_KAHAN_ALGORITHM				// использовать алгоритм суммирования Кэхэна
// #define USE_OVF_HANDLER					// "скручивать" переменные оценивателя при больших значениях
// #define ESTIMATOR_OVF_VAL	100000		// предел счётчика импульсов

#define ESTIM_DISENG_DELAY	2000		// ms; задержка перед отключением оценивателя
#define ESTIM_RAW_OMEGA_TRS		10		// имп/с; меньшие скорости считаются нулевыми

#define ESTIM_PERIOD		10			// ms; период вызова алгоритма оценки
#define ESTIM_CONST_dT		0.01		// s; для алгоритма оценки
#define ESTIM_CONST_Ki		5.00
// #define ESTIM_CONST_Kp		1.9			// только для ПИ-фильтра
// #define ESTIM_SCALE			1.0			// отладка
// #define ESTIM_SCALE			1.5365		// °/имп
#define ESTIM_SCALE			0.0268376	// рад/имп

#define PICONTR_PERIOD		10			// ms
#define PICONTR_CONST_dT	0.01		// s
#define PICONTR_CONST_Kp	3.0
#define PICONTR_CONST_Ki	12.0

#define MOTORS_OMEGA_OBJ_MAX		40.0	// рад/с; максимальная уставка
#define MOTORS_OMEGA_OBJ_MIN		1.0		// рад/с; меньшие скорости не гарантируем


#endif /* MOTOR_H_ */