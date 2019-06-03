/*
 * motor.c
 *
 * Created: 13.12.2018 17:40:57
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "motor.h"
#include "../rtos.h"
#include "md3.h"
#include "../interfaces/uart.h"
#include <util/atomic.h>
#include "../displays/_7seg_disp.h"

volatile uint32_t __enc_L_pulses = 0;
volatile uint32_t __enc_R_pulses = 0;
// u32 хватит на ~3453 километра пути

MOTOR_OMEGA_DATA __omega_objective;		// структура уставок

double __omega_L_raw = 0.0, __omega_R_raw = 0.0;	// оценки производных,
													// не умноженные на масштабирующий коэффициент

static uint8_t __estimator_reset = 1;	// флаг сброса оценивателя;
// static потому, что переменная с таким же именем используется в оценивателе в bmp180.c
uint8_t __picontroller_reset = 1;	// флаг сброса регулятора
uint8_t __motor_L_reached_constr = 0;	// флаг достижения L двигателем ограничения PWM
uint8_t __motor_R_reached_constr = 0;	// флаг достижения R двигателем ограничения PWM

volatile uint8_t __enc_L_en = 1, __enc_R_en = 1;	// разрешение чтения энкодеров

inline void __enc_L (void)
{	// ISR
	__DEBUG_PIN_SWITCH;
	
	// Данное прерывание "будит" оцениватель скорости, если он ещё не работает
	if (__estimator_reset)
	{	// Если оцениватель подготовлен к запуску, запускаем его
		rtos_set_task (__motors_omega_estimator, RTOS_RUN_ASAP, ESTIM_PERIOD);
		// и обнуляем переменные (обе!) импульсов
		__enc_L_pulses = 0;
		__enc_R_pulses = 0;
	}
	
	if (__enc_L_en)
	{
		__enc_L_en = 0;
		__enc_L_pulses++;
	
		// Через некоторое время (не более 500 мкс) разрешим чтение:
		OCR3B = TCNT3 + (uint16_t)ENC_FILTER_TIME; // мкс
		ETIMSK |= 1 << OCIE3B;	// прерывание по совпадению с OCR3B
	}

	return;
}
inline void __enc_R (void)
{	// ISR
	// Данное прерывание "будит" оцениватель скорости, если он ещё не работает
	if (__estimator_reset)
	{	// Если оцениватель подготовлен к запуску, запускаем его
		rtos_set_task (__motors_omega_estimator, RTOS_RUN_ASAP, ESTIM_PERIOD);
		// и обнуляем переменные (обе!) импульсов
		__enc_L_pulses = 0;
		__enc_R_pulses = 0;
	}

	if (__enc_R_en)
	{
		__enc_R_en = 0;
		__enc_R_pulses++;
		
		// Через некоторое время (не более 500 мкс) разрешим чтение:
		OCR3C = TCNT3 + (uint16_t)ENC_FILTER_TIME; // мкс
		ETIMSK |= 1 << OCIE3C;	// прерывание по совпадению с OCR3C
	}

	return;
}

inline void __enc_L_enable (void)
{// ISR
	__enc_L_en = 1;
	ETIMSK &= ~(1 << OCIE3B);	// это прерывание больше не нужно
	
	return;
}

inline void __enc_R_enable (void)
{// ISR
	__enc_R_en = 1;
	ETIMSK &= ~(1 << OCIE3C);	// это прерывание больше не нужно
	
	return;
}

void __motors_set_pwm (uint8_t duty_cycle_l, uint8_t duty_cycle_r)
{
	OCR1BL = duty_cycle_l;	// Только LOW, т.к. разрешение 8 бит
	OCR1CL = duty_cycle_r;	// Только LOW, т.к. разрешение 8 бит

	if (duty_cycle_l >= MOTORS_PWM_CONSTR_MAX)
	{
		__motor_L_reached_constr = 1;
	}
	
	if (duty_cycle_r >= MOTORS_PWM_CONSTR_MAX)
	{
		__motor_R_reached_constr = 1;
	}
	
	return;
}


void __motors_omega_estimator (void)
{
/*

	Оценка производной угла поворота колеса - АПЕРИОДИЧЕСКИЙ ФИЛЬТР.
	На 1 импульс с энкодера приходится ~1.5365° поворота колеса.
	Входная последовательность углов заменена последовательностью импульсов

*/

	static double I1L, I2L, I1R, I2R;
	double epsL, epsR;
	
	if (__estimator_reset)
	{
		I1L = 0;
		I2L = 0;
		I1R = 0;
		I2R = 0;
		
		__estimator_reset = 0;
	}
	else if ((__omega_L_raw <= 0.1) && (__omega_R_raw <= 0.1))
	{	// Если остановились, то оцениватель больше не нужен
		// (в точности нулю эти переменные не будут равны никогда, поэтому сравниваем с 
		// заведомо неосуществимой малой скоростью 0.1 имп/с)
		__estimator_reset = 1;
		rtos_delete_task (__motors_omega_estimator);
		return;
	}
	
	// Атомарно вычислим сигналы ошибок,
	// т.к. импульсы меняются в прерываниях, а далее расчитаем всё остальное
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
	{
		epsL = (double)__enc_L_pulses - I1L;
		epsR = (double)__enc_R_pulses - I1R;
	}
	
	/* Вычисления для двигателя L */
	epsL *= ESTIM_CONST_Ki;
	I1L += epsL * ESTIM_CONST_dT;
	/* II ступень: */
	epsL = I1L - I2L;
	epsL *= ESTIM_CONST_Ki;
	__omega_L_raw = epsL;
	I2L += epsL * ESTIM_CONST_dT;
	
	/* Вычисления для двигателя R */
	epsR *= ESTIM_CONST_Ki;
	I1R += epsR * ESTIM_CONST_dT;
	/* II ступень: */
	epsR = I1R - I2R;
	epsR *= ESTIM_CONST_Ki;
	__omega_R_raw = epsR;
	I2R += epsR * ESTIM_CONST_dT;
	
	return;
}


// void __motors_omega_estimator (void)
// {
// /*
// 
// 	Оценка производной угла поворота колеса - ПИ-ФИЛЬТР.
// 	На 1 импульс с энкодера приходится ~1.5365° поворота колеса.
// 	Входная последовательность углов заменена последовательностью импульсов (x_l, x_r)
// 
// */
// 
// 	static double I11L, I12L, I21L, I22L;
// //	static double I11R, I12R, I21R, I22R;
// 	
// 	double epsL;//, epsR;
// 	
// 	if (__estimator_reset)
// 	{
// 		I11L = 0;
// 		I12L = 0;
// 		I21L = 0;
// 		I22L = 0;
// 				
// // 		I11R = 0;
// // 		I12R = 0;
// // 		I21R = 0;
// // 		I22R = 0;
// 		
// 		__estimator_reset = 0;
// 	}
// 	
// 	// Атомарно вычислим сигналы ошибок,
// 	// т.к. импульсы меняются в прерываниях, а далее расчитаем всё остальное
// 	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
// 	{
// 		epsL = (double)__enc_L_pulses - I12L;
// //		epsR = (double)__enc_R_pulses - I12R;
// 	}
// 	
// 	/* Вычисления для двигателя L */
// 	/* 1 ступень: */
// 	I11L += epsL * ESTIM_CONST_Ki * ESTIM_CONST_dT;
// 	I12L += I11L + epsL * ESTIM_CONST_Kp;
// 	/* 2 ступень: */
// 	epsL = I12L - I22L;
// 	I21L += epsL * ESTIM_CONST_Ki * ESTIM_CONST_dT;
// 	__omega_L_raw = I21L + epsL * ESTIM_CONST_Kp;
// 	I22L += __omega_L_raw;
// 	
// 	
// 	/* Вычисления для двигателя R */
// 	/* 1 ступень: */
// // 	I11R += epsR * ESTIM_CONST_Ki * ESTIM_CONST_dT;
// // 	I12R += I11R + epsR * ESTIM_CONST_Kp;
// // 	/* 2 ступень: */
// // 	epsR = I12R - I22R;
// // 	I21R += epsR * ESTIM_CONST_Ki * ESTIM_CONST_dT;
// // 	__omega_R_raw = I21R + epsR * ESTIM_CONST_Kp;
// // 	I22R += __omega_R_raw;
// 	
// 	return;
// }


void __motors_pi_controller (void)
{
	static double I_L = 0.0, I_R = 0.0;
	double eps_L, eps_R;
	
	static int16_t u_L, u_R;	// учитываем знак, т.к. он может появиться
		
	MOTOR_OMEGA_DATA omega = motors_get_omega ();
	
	if (__picontroller_reset)
	{
		// Ничего не нужно обнулять, только сбрасываем флаг	
		__picontroller_reset = 0;
		_7seg_puts ("act\n");
	}
	else if ((__omega_L_raw <= 0.2) && (__omega_R_raw <= 0.2) && \
				(__omega_objective.omegaL == 0.0) && (__omega_objective.omegaR == 0.0))
	{	// Если остановились, то ПИ-регулятор больше не нужен
		__picontroller_reset = 1;
		rtos_delete_task (__motors_pi_controller);
		_7seg_puts ("stby\n");
		return;
	}

	/* Вычисления для двигателя L */
	if (__omega_objective.omegaL == 0)
	{
		u_L = 0;	// лучший способ остановиться
		I_L = 0;	// обнулим интегратор перед следующим страгиванием
	}
	else
	{
		eps_L = __omega_objective.omegaL - omega.omegaL;
		if (__motor_L_reached_constr && (eps_L >= 0))
		{	// достигли ограничения ШИМ, и снижения не предвидится
			led_y_on ();	// master caution
		}
		else
		{
			if (__motor_L_reached_constr)
			{	// сбрасываем ошибку:
				__motor_L_reached_constr = 0;
				led_y_off ();
			}
			
			I_L +=  eps_L * PICONTR_CONST_dT;
			u_L = (int16_t)(eps_L * PICONTR_CONST_Kp + I_L * PICONTR_CONST_Ki);
			
			if (u_L < MOTORS_PWM_CONSTR_MIN)
			{
				u_L = MOTORS_PWM_CONSTR_MIN;
			}
			else if (u_L > MOTORS_PWM_CONSTR_MAX)
			{
				u_L = MOTORS_PWM_CONSTR_MAX;
			}
		}
	}
	
	/* Вычисления для двигателя R */
	if (__omega_objective.omegaR == 0)
	{
		u_R = 0;	// лучший способ остановиться
		I_R = 0;	// обнулим интегратор перед следующим страгиванием
	}
	else
	{
		eps_R = __omega_objective.omegaR - omega.omegaR;
		if (__motor_R_reached_constr && (eps_R >= 0))
		{	// достигли ограничения ШИМ, и снижения не предвидится
			led_y_on();	// master caution
		}
		else
		{
			if (__motor_R_reached_constr)
			{	// сбрасываем ошибку:
				__motor_R_reached_constr = 0;
				led_y_off ();
			}
			
			I_R +=  eps_R * PICONTR_CONST_dT;
			u_R = (int16_t)(eps_R * PICONTR_CONST_Kp + I_R * PICONTR_CONST_Ki);
			
			if (u_R < MOTORS_PWM_CONSTR_MIN)
			{
				u_R = MOTORS_PWM_CONSTR_MIN;
			}
			else if (u_R > MOTORS_PWM_CONSTR_MAX)
			{
				u_R = MOTORS_PWM_CONSTR_MAX;
			}
		}
	}
	
	__motors_set_pwm ((uint8_t)u_L, (uint8_t)u_R);
	
	return;
}

void __motors_obj_poll (void)
{
	uint16_t adc_val = ADC;
	
	if (__picontroller_reset && (adc_val > MOTORS_SPEED_OBJ_ADC_TRS))
	{// "Будим" ПИ-регулятор, если он ещё не работает:
		rtos_set_task (__motors_pi_controller, RTOS_RUN_ASAP, PICONTR_PERIOD);
	}
	
	// Задаём уставку скорости (двух двигателей одновременно, отладка)
	// в зависимости от положения потенциометра:
	
	if (adc_val <= MOTORS_SPEED_OBJ_ADC_TRS)
	{// Если меньше порога, то ноль
		__omega_objective.omegaL = 0.0;
		__omega_objective.omegaR = 0.0;
	}
	else
	{
// 		__omega_objective.omegaL = (MOTORS_SPEED_OBJ_MAX/(ADC_MAX - MOTORS_SPEED_OBJ_ADC_TRS)) * \
// 		((double)adc_val - MOTORS_SPEED_OBJ_ADC_TRS);

		__omega_objective.omegaL = (MOTORS_SPEED_OBJ_MAX/ADC_MAX) * ((double)adc_val);
		__omega_objective.omegaR = __omega_objective.omegaL; // только отладка
	}
	
	motors_set_omega (__omega_objective.omegaL, __omega_objective.omegaR);

//	__motors_set_pwm (adc_val >> 2, adc_val >> 2);
	
	return;
}


inline void motors_init (void)
{
	/* Настройка PWM (PWM out) */
	__motors_set_pwm ((uint8_t)0, (uint8_t)0);
	
	MOTORS_DDR |= (1 << MOTOR_L_PWM_PIN) | (1 << MOTOR_R_PWM_PIN);	// output
	
	TCCR1A |=  (1 << COM1B1) | (1 << COM1C1) | (1 << WGM10);		// Fast
	TCCR1B |= (1 << WGM12);											// PWM
	
	/* Настройка прерываний энкодеров (rising edge) */
	EICRA |= (1 << ISC31) | (1 << ISC30) | (1 << ISC21) | (1 << ISC20);
	
	rtos_set_task (motors_arm, MOTORS_STARTUP_TIME, RTOS_RUN_ONCE);
	
	uart_puts ("[ OK ] Motors init completed\n");
	
	return;
}

void motors_arm (void)
{
	/* Настройка PWM */
	__motors_set_pwm (0, 0);
	TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);	// запуск таймера ШИМ
	
	/* Запуск таймера фильтра ложных прерываний */
	TCCR3B |= (0 << CS32) | (1 << CS31) | (0 << CS30);	// счёт каждую 1 мкс
	
	/* Включение прерываний датчиков */
	EIFR |= (1 << INTF3) | (1 << INTF2);	// на всякий случай сбросим флаги (установкой единиц)
	EIMSK |= (1 << INT3) | (1 << INT2);		// непосредственно включаем
	
	__estimator_reset = 1;	// сброс интеграторов оценивателя; подготовка к запуску
//	rtos_set_task (__motors_omega_estimator, RTOS_RUN_ASAP, ESTIM_PERIOD);
	
	rtos_set_task (__motors_obj_poll, RTOS_RUN_ASAP, MOTORS_OBJ_POLL_PERIOD);
	
	__picontroller_reset = 1;	// сброс интеграторов ПИ-регулятора; подготовка к запуску
//	rtos_set_task (__motors_pi_controller, RTOS_RUN_ASAP, PICONTR_PERIOD);

	uart_puts ("[ OK ] Motors armed\n");
	
	return;
}

void motors_disarm (void)
{	
	rtos_delete_task (__motors_obj_poll);
	motors_set_omega (0.0, 0.0);
	
	rtos_delete_task (__motors_pi_controller);
	
	rtos_delete_task (__motors_omega_estimator);
	
	/* Выключение прерываний датчиков */
	EIMSK &= ~((1 << INT3) | (1 << INT2));
	
	/* Остановка таймера ШИМ */
	TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12));
	
	/* Остановка таймера фильтра ложных прерываний */
	TCCR3B &= ~((1 << CS32) | (1 << CS31) | (1 << CS30));
	
	__motors_set_pwm (0, 0);
	
	return;
}

MOTOR_OMEGA_DATA motors_get_omega (void)
{
	MOTOR_OMEGA_DATA omega;
	omega.omegaL = __omega_L_raw * ESTIM_SCALE;
	omega.omegaR = __omega_R_raw * ESTIM_SCALE;
	
	return omega;
}

MOTOR_OMEGA_DATA motors_get_omega_obj (void)
{
	MOTOR_OMEGA_DATA omega;
	omega.omegaL = __omega_objective.omegaL;
	omega.omegaR = __omega_objective.omegaR;
	
	return omega;
}

MOTOR_POWER_DATA motors_get_power (void)
{	// Мощность - в % от абсолютного максимума PWM (255 для 8-битного ШИМ)
	MOTOR_POWER_DATA power;
	power.powL = ((float)OCR1BL / 255.0) * 100.0;
	power.powR = ((float)OCR1CL / 255.0) * 100.0;
	
	return power;
}

 void motors_set_omega (double omega_L, double omega_R)
{
	__omega_objective.omegaL = omega_L;
	__omega_objective.omegaR = omega_R;
	
 	return;
}