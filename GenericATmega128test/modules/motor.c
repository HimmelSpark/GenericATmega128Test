/*
 * motor.c
 *
 * Created: 13.12.2018 17:40:57
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include <math.h>
#include "motor.h"
#include "../rtos.h"
#include "md3.h"
#include "../interfaces/uart.h"
#include <util/atomic.h>
#include "../displays/_7seg_disp.h"

volatile int32_t __enc_L_pulses = 0;
volatile int32_t __enc_R_pulses = 0;
// u32 хватит на +-1700 километров пути

double __omega_L_raw = 0.0, __omega_R_raw = 0.0;	// оценки производных,
													// не умноженные на масштабирующий коэффициент
													
double __omega_obj_L = 0.0, __omega_obj_R = 0.0;	// уставки для ПИ-регулятора


//TODO: регистр состояния

//static volatile uint8_t motors_status_lo, motors_status_hi;

static uint8_t __estimator_reset = 1;	// флаг сброса оценивателя;
// static потому, что переменная с таким же именем используется в оценивателе в bmp180.c
uint8_t __estimator_diseng_armed = 0;	// флаг подготовки отключения оценивателя

uint8_t __picontroller_reset	 = 1;	// флаг сброса регулятора
uint8_t __motor_L_reached_constr = 0;	// флаг достижения L двигателем ограничения PWM
uint8_t __motor_R_reached_constr = 0;	// флаг достижения R двигателем ограничения PWM

uint8_t __motors_disarmed = 1;			// флаг состояния двигателей
uint8_t __motors_disarm_armed = 0;		// флаг подготовки отключения двигателей

volatile uint8_t __enc_L_en = 1, __enc_R_en = 1;	// флаги разрешения чтения энкодеров

inline void __enc_L (void)
{// ISR
	// Данное прерывание "будит" оцениватель скорости, если он ещё не работает
	
	if (__estimator_reset)
	{	// Если оцениватель подготовлен к запуску, запускаем его
		rtos_set_task (__motors_omega_estimator, RTOS_RUN_ASAP, ESTIM_PERIOD);
	}
	
	if (__enc_L_en)
	{
//		__DEBUG_PIN_SWITCH;
		__enc_L_en = 0;
		
		// Определяем направление движения (см. осцилограммы)
		if ((ENCODERS_PIN & (1 << ENC_L_B_PIN)) == 0x00)
		{
			__enc_L_pulses++;
		}
		else
		{
			__enc_L_pulses--;
		}
	
// 		// Через некоторое время (не более 500 мкс) разрешим чтение:
		OCR3B = TCNT3 + (uint16_t)ENC_FILTER_TIME; // мкс
		ETIMSK |= 1 << OCIE3B;	// прерывание по совпадению с OCR3B
	}

	return;
}

inline void __enc_R (void)
{// ISR
	// Данное прерывание "будит" оцениватель скорости, если он ещё не работает
	
	if (__estimator_reset)
	{	// Если оцениватель подготовлен к запуску, запускаем его
		rtos_set_task (__motors_omega_estimator, RTOS_RUN_ASAP, ESTIM_PERIOD);
	}

	if (__enc_R_en)
	{
		__enc_R_en = 0;
	
		// Определяем направление движения (см. осцилограммы)
		if ((ENCODERS_PIN & (1 << ENC_R_B_PIN)) == 0x00)
		{
			__enc_R_pulses--;
		}
		else
		{
			__enc_R_pulses++;
		}
		
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

void __motors_set_thrust (int16_t thrust_L, int16_t thrust_R)
{
	// Знаки управлений:
	int8_t sgn_L = (thrust_L >= 0) ? MOTORS_THRUST_POS : MOTORS_THRUST_NEG;
	int8_t sgn_R = (thrust_R >= 0) ? MOTORS_THRUST_POS : MOTORS_THRUST_NEG;
	
	// Коммутация железа в соответствии с этими знаками:
	switch (sgn_L)
	{
		case MOTORS_THRUST_POS:
		{
			__MOTORS_CTRL_L_FWD;
			break;
		}
		case MOTORS_THRUST_NEG:
		{
			__MOTORS_CTRL_L_REV;
			break;
		}
	}
	switch (sgn_R)
	{
		case MOTORS_THRUST_POS:
		{
			__MOTORS_CTRL_R_FWD;
			break;
		}
		case MOTORS_THRUST_NEG:
		{
			__MOTORS_CTRL_R_REV;
			break;
		}
	}
	
	// Знаки запомнили, теперь получим модули управлений:
	thrust_L *= sgn_L;
	thrust_R *= sgn_R;
	
	// Проверим необходимость сброса флагов превышения ограничений:
	if (thrust_L < MOTORS_PWM_CONSTR_MAX)
	{
		__motor_L_reached_constr = 0;
		led_y_off ();
	}
	if (thrust_R < MOTORS_PWM_CONSTR_MAX)
	{
		__motor_R_reached_constr = 0;
		led_y_off ();
	}
	
	/*** Управление для двигателя L ***/
	if (thrust_L == 0)
	{	// Остановка только в случае точного 0
		MOTOR_L_OCR = 0;
	}
// 	else if (thrust_L < MOTORS_PWM_TRS)
// 	{	// Если управление хотя бы немного больше 0 и меньше порога, то обеспечиваем страгивание:
// 		MOTOR_L_OCR = MOTORS_PWM_TRS;
// 	}
	else if (thrust_L >= MOTORS_PWM_CONSTR_MAX)
	{	// Управление превысило максимум:
		MOTOR_L_OCR = MOTORS_PWM_CONSTR_MAX;
		__motor_L_reached_constr = 1;
		led_y_on ();
	} 
	else
	{	// Управление в заданных рамках:
		MOTOR_L_OCR = thrust_L;
	}
	
	
	/*** Управление для двигателя R ***/
	if (thrust_R == 0)
	{	// Остановка только в случае точного 0
		MOTOR_R_OCR = 0;
	}
// 	else if (thrust_R < MOTORS_PWM_TRS)
// 	{	// Если управление хотя бы немного больше 0 и меньше порога, то обеспечиваем страгивание:
// 		MOTOR_R_OCR = MOTORS_PWM_TRS;
// 	}
	else if (thrust_R >= MOTORS_PWM_CONSTR_MAX)
	{	// Управление превысило максимум:
		MOTOR_R_OCR = MOTORS_PWM_CONSTR_MAX;
		__motor_R_reached_constr = 1;
		led_y_on ();
	}
	else
	{	// Управление в заданных рамках:
		MOTOR_R_OCR = thrust_R;
	}
	
	
	if (__motor_L_reached_constr || __motor_R_reached_constr)
	{	// Если достигнут максимум хотя бы одним двигателем
		if (!__motors_disarm_armed)
		{	// И ещё не подготовили отключение двигателей,
			rtos_set_task (motors_disarm, 1000, RTOS_RUN_ONCE);	// делаем это сейчас
			__motors_disarm_armed = 1;							// подготовили
//			uart_puts ("[ ! ] Constraint exceeded\n");
		}
	}
	else if (__motors_disarm_armed)
	{	// Если не превышаем ограничения, а отключение подготовлено, то отменяем его
		rtos_delete_task (motors_disarm);
		__motors_disarm_armed = 0;
//		uart_puts ("[ OK ] Motors disarm cancelled\n");
	}
	
	
	return;
}


void __motors_omega_estimator (void)
{
/*

	Оценка производной угла поворота колеса - АПЕРИОДИЧЕСКИЙ ФИЛЬТР.
	На 1 импульс с энкодера приходится ~1.5365° поворота колеса.
	Входная последовательность углов заменена последовательностью импульсов.
	
	Замечена особенность: с ростом пройденного расстояния и, как следствие,
	интеграторов и аккумуляторов угла, падает точность вычислений с плавающей точкой.
	Отсюда имеем, что при полной остановке скорости полностью не обнуляются (хотя и
	близки к нулю).
	
	Если принципиально нужны точные значения, можно использовать следующие методы:
	
	1. Алгоритм точного суммирования Кэхэна, учитывающий взникающие погрешности
		машинных вычислений. Очень ресурсоёмкий. Включается дефайном USE_KAHAN_ALGORITHM
	2. "Скручивание" (на ходу) интеграторов и аккумуляторов угла при достижении ими больших значений
		таким образом, что должны сохраниться значения ошибок и скоростей (по факту наблюдается
		небольшой скачок в большую сторону). Включается дефайном USE_OVF_HANDLER


	Фактически использование этих методов не является обязательным, т.к. реализовано автоматическое
	отключение оценивателя с обнулением всех переменных в случае, когда скорость в течение 
	некоторого промежутка времени (порядка секунд) меньше, чем минимально гарантируемая.
	
	Отключение оценивателя принципиально должно быть отложенным, т.к. в случае, например, страгивания
	оценка скорости может не успеть (и скорее всего не успеет) возрасти до минимальной за период
	вызова оценивателя (порядка 10 мс). Необходимо запланировать отключение оценивателя, а в случае,
	если скорость превысит минимальную, отключение отменить.
	
	Теоретически, если не использовать алгоритмы повышения точности вычислений, накопленная ошибка
	определения скорости может достичь значений минимальной скорости, и в таком случае перестанет
	происходить отключение оценивателя с обнулением переменных. Однако это произойдёт только
	при ОЧЕНЬ больших значениях переменных, т.е. с большой вероятностью не произойдёт.++

*/

	static double I1L, I2L, I1R, I2R;	// интеграторы
	double epsL, epsR;					// сигналы ошибок
	
	#ifdef USE_KAHAN_ALGORITHM
		static double c1L, c2L, c1R, c2R;	// погрешности вычислений (по алгоритму Кэхэна)
		double t, y;						// вспомогательные переменные (по алгоритму Кэхэна)
	#endif
	
	if (__estimator_reset)
	{	// Если оцениватель в состоянии сброса......
		
		I1L = 0.0;
		I2L = 0.0;
		I1R = 0.0;
		I2R = 0.0;
		
		#ifdef USE_KAHAN_ALGORITHM
			c1L = 0.0;
			c2L = 0.0;
			c1R = 0.0;
			c2R = 0.0;
		#endif
		
		__estimator_reset = 0;
		
		uart_puts ("[ OK ] Estimator engaged\n");
	}
	else if ((abs((int)__omega_L_raw) <= ESTIM_RAW_OMEGA_TRS) && (abs((int)__omega_R_raw) <= ESTIM_RAW_OMEGA_TRS))
	{	// Если оцениватель не находится в состоянии сброса и если скорость близка к нулю, то 
		// подготовим отключение оценивателя......
		if (!__estimator_diseng_armed)
		{	//.... если этого ещё не сделано
			rtos_set_task (__motors_omega_estimator_diseng, ESTIM_DISENG_DELAY, RTOS_RUN_ONCE);
			__estimator_diseng_armed = 1;
			
// 			uart_puts ("[ ! ]	Estimator is being disengaged if raw omega\n");
// 			uart_puts ("	doesn't exceed 10 pulses/s during next 2 s\n");
		}
	}
	else if (__estimator_diseng_armed)
	{	// Если продолжаем ехать, а отключение оценивателя подготовлено, то отменим это
		rtos_delete_task (__motors_omega_estimator_diseng);
		__estimator_diseng_armed = 0;
//		uart_puts ("[ ! ] Estimator disangage cancelled\n");
	}
	
	// Атомарно вычислим сигналы ошибок, т.к. импульсы меняются в прерываниях
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
	{
		epsL = __enc_L_pulses - I1L;
		epsR = __enc_R_pulses - I1R;
	}
	
	/* Вычисления для двигателя L */
	
	// Алгоритм "скручивания" переменных оценивателя при больших значениях
	#ifdef USE_OVF_HANDLER
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
		{
			if (__enc_L_pulses >= ESTIMATOR_OVF_VAL)
			{
				I2L = 0.0;
				I1L =  __omega_L_raw/ESTIM_CONST_Ki;
				__enc_L_pulses = epsL + I1L;
				led_y_blink ();
			}
		}
	#endif
	
	epsL *= ESTIM_CONST_Ki;
//	__omega_L_raw = epsL;		// если хотим использовать 1 ступень фильтра
	
	#ifdef USE_KAHAN_ALGORITHM	// Алгоритм Кэхэна (точного суммирования чисел с плавающей запятой)
		y = epsL * ESTIM_CONST_dT - c1L;
		t = I1L + y;
		c1L = (t - I1L) - y;
		I1L = t;
	#else
		I1L += epsL * ESTIM_CONST_dT;
	#endif
	
	/* II ступень: */
	epsL = I1L - I2L;
	epsL *= ESTIM_CONST_Ki;
	__omega_L_raw = epsL;
	
	#ifdef USE_KAHAN_ALGORITHM
		y = epsL * ESTIM_CONST_dT - c2L;
		t = I2L + y;
		c2L = (t - I2L) - y;
		I2L = t;
	#else
		I2L += epsL * ESTIM_CONST_dT;
	#endif
	
	/* Вычисления для двигателя R */

	// Алгоритм "скручивания" переменных оценивателя при больших значениях
	#ifdef USE_OVF_HANDLER
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
		{
			if (__enc_R_pulses >= ESTIMATOR_OVF_VAL)
			{
				I2R = 0.0;
				I1R = __omega_R_raw/ESTIM_CONST_Ki;
				__enc_R_pulses = epsR + I1R;
			}
		}
	#endif
	
	epsR *= ESTIM_CONST_Ki;
//	__omega_R_raw = epsR;		// если хотим использовать 1 ступень фильтра

	#ifdef USE_KAHAN_ALGORITHM	// Алгоритм Кэхэна (точного суммирования чисел с плавающей запятой)
		y = epsR * ESTIM_CONST_dT - c1R;
		t = I1R + y;
		c1R = (t - I1R) - y;
		I1R = t;
	#else
		I1R += epsR * ESTIM_CONST_dT;
	#endif
	
	/* II ступень: */
	epsR = I1R - I2R;
	epsR *= ESTIM_CONST_Ki;
	__omega_R_raw = epsR;
	
	#ifdef USE_KAHAN_ALGORITHM
		y = epsR * ESTIM_CONST_dT - c2R;
		t = I2R + y;
		c2R = (t - I2R) - y;
		I2R = t;
	#else
		I2R += epsR * ESTIM_CONST_dT;
	#endif
	
	return;
}


void __motors_omega_estimator_diseng (void)
{
	__omega_L_raw = 0.0;
	__omega_R_raw = 0.0;
	
	__enc_L_pulses = 0;
	__enc_R_pulses = 0;

	__estimator_reset = 1;
	__estimator_diseng_armed = 0;
		
	rtos_delete_task (__motors_omega_estimator);
	uart_puts ("[ OK ] Estimator disengage and reset done\n");
	
	return;
}

void __motors_pi_controller (void)
{
	static double I_L = 0.0, I_R = 0.0;
	double eps_L, eps_R;
	static int16_t u_L, u_R;
		
	MOTOR_OMEGA_DATA omega = motors_get_omega ();
	
	if (__picontroller_reset)
	{
		I_L = 0.0;
		I_R = 0.0;
		
		__picontroller_reset = 0;
		
		_7seg_puts ("act\n");
		uart_puts ("[ OK ] PI-controller engaged\n");
	}
	else if ((__omega_L_raw == 0.0) && (__omega_R_raw == 0.0) && \
				(__omega_obj_L == 0.0) && (__omega_obj_R == 0.0))
	{	// Если остановились и не хотим больше двигаться, то ПИ-регулятор больше не нужен
		// (точное равенство нулю оценок скоростей при остановке обеспечивается оценивателем)
		__picontroller_reset = 1;
		rtos_delete_task (__motors_pi_controller);
		_7seg_puts ("stby\n");
		uart_puts ("[ OK ] PI-controller disengaged\n");
		
		return;
	}

	/* Вычисления для двигателя L */

	eps_L = __omega_obj_L - omega.omegaL;
		
	if (__motor_L_reached_constr && (eps_L >= 0.0))
	{	// достигли ограничения ШИМ, и снижения не предвидится
		// master caution
		eps_L = 0.0;	// делаем вид, что это и есть целевая скорость
	}
	I_L +=  eps_L * PICONTR_CONST_Ki * PICONTR_CONST_dT;
	if (__omega_obj_L == 0.0)
	{
		u_L = 0;	// наискорейшая остановка
	}
	else
	{
		u_L = (int16_t)(eps_L * PICONTR_CONST_Kp + I_L);
	}
	
	/* Вычисления для двигателя R */
	
	eps_R = __omega_obj_R - omega.omegaR;
		
	if (__motor_R_reached_constr && (eps_R >= 0.0))
	{	// достигли ограничения ШИМ, и снижения не предвидится
		// master caution
		eps_R = 0.0;	// делаем вид, что это и есть целевая скорость
	}
	I_R +=  eps_R * PICONTR_CONST_Ki * PICONTR_CONST_dT;
	if (__omega_obj_R == 0.0)
	{
		u_R = 0;	// наискорейшая остановка
	}
	else
	{
		u_R = (int16_t)(eps_R * PICONTR_CONST_Kp + I_R);
	}
	
	__motors_set_thrust (u_L, u_R);
	
	return;
}

inline void motors_init (void)
{
	MOTORS_PWM_DDR	|= (1 << MOTOR_L_PWM_PIN) | (1 << MOTOR_R_PWM_PIN);		// выход
	MOTORS_PWM_PORT	&= ~((1 << MOTOR_L_PWM_PIN) | (1 << MOTOR_R_PWM_PIN));	// кидаем на землю
	
	MOTORS_CTRL_DDR |= (1 << MOTORS_CTRL_IN1) | (1 << MOTORS_CTRL_IN2) | 	// выход
						(1 << MOTORS_CTRL_IN3) | (1 << MOTORS_CTRL_IN4);	// выход
	__MOTORS_CTRL_L_STOP;													// кидаем на землю
	__MOTORS_CTRL_R_STOP;													// кидаем на землю
	
	/* Настройка PWM */
	TCCR1A |= 1 << WGM10;	// Fast
	TCCR1B |= 1 << WGM12;	// PWM
	
	/* Настройка прерываний энкодеров (rising edge INT6,7) */
	EICRB |= (1 << ISC51) | (1 << ISC50) | (1 << ISC41) | (1 << ISC40);
	
	#if MOTORS_ARM_ON_STARTUP == 1
		rtos_set_task (motors_arm, MOTORS_STARTUP_TIME, RTOS_RUN_ONCE);
	#endif
	
	uart_puts ("[ OK ] Motors init completed\n");
	
	return;
}

void motors_arm (void)
{	
	if ((__omega_L_raw != 0.0) || (__omega_R_raw != 0.0))
	{	// Точный нуль обеспечивается сбросом оценивателя при его отключении
		uart_puts ("[ FAIL ] ARM not available in motion\n");
	}
	else if (__motors_disarmed)
	{	// Армируем только если был дизарм; нет смысла несколько раз армировать
		/* Настройка PWM */
		MOTOR_L_OCR = 0;
		MOTOR_R_OCR = 0;
		TCCR1A |= (1 << COM1B1) | (1 << COM1C1);			// подключаем двигатели к OCR1B/C
		TCCR1B |= (0 << CS12) | (1 << CS11) | (1 << CS10);	// запуск таймера ШИМ
		
		/* Запуск таймера фильтра ложных прерываний */
		TCCR3B |= (0 << CS32) | (1 << CS31) | (0 << CS30);	// счёт каждую 1 мкс
		
		/* Включение прерываний датчиков */
		// на всякий случай сбросим флаги (установкой единиц):
		EIFR |= (1 << INTF5) | (1 << INTF4);
		// непосредственно включаем:
		EIMSK |= (1 << INT5) | (1 << INT4);	

		// На всякий случай обнулим уставки
		__omega_obj_L = 0.0;
		__omega_obj_R = 0.0;
		
		__picontroller_reset = 1;	// На всякий случай принудительно сбросим ПИ-регулятор
									// (вообще, он сбрасывается сам при условии остановки
									// и нулевых уставок, однако существует НИЧТОЖНАЯ вероятность того,
									// что пользователь выполнит арм раньше сброса ПИ-регулятора
									// [у меня не получилось], и тогда возможны выбросы управления
		
		__motors_disarmed = 0;
		
		uart_puts ("[ OK ] Motors ARMED\n");
		
		// 	uart_puts ("[ ! ] STEP TEST in 2 sec\n\n");
		// 	rtos_set_task (step_test, 2000, RTOS_RUN_ONCE);
	}
	
	return;
}

void motors_disarm (void)
{	
	// Отключим пины ШИМ от OCR1B/C
	TCCR1A &= ~((1 << COM1B1) | (1 << COM1C1));
	// Гарантированно остановим двигатели:
	__MOTORS_CTRL_L_STOP;
	__MOTORS_CTRL_R_STOP;
	MOTORS_PWM_PORT	&= ~((1 << MOTOR_L_PWM_PIN) || (1 << MOTOR_R_PWM_PIN));
	// Обнулим регистры сравнения ШИМ:
	MOTOR_L_OCR = 0;
	MOTOR_R_OCR = 0;
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));	// отключаем любые источники тактирования ШИМ
	
	// Выключение прерываний датчиков 
	EIMSK &= ~((1 << INT5) | (1 << INT4));
	// Остановка таймера фильтра ложных прерываний
	TCCR3B &= ~((1 << CS32) | (1 << CS31) | (1 << CS30));
	
	// Необходимое условие отключения ПИ-регулятора
	__omega_obj_L = 0.0;
	__omega_obj_R = 0.0;
	// ПИ-регулятор и оцениватель скорости отключатся автоматически;
	// нет необходимости делать это вручную
	
	__motors_disarmed = 1;		// дизармировали
	__motors_disarm_armed = 0;	// дизарм не подготовлен (сброс на тот случай, если 
								// мы сюда попали по автоматическому отключению)
	
	uart_puts ("[ OK ] Motors DISARMED\n");
	
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
	omega.omegaL = __omega_obj_L;
	omega.omegaR = __omega_obj_R;
	
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
	// Задание уставки при DISARM запрещено
	if (!__motors_disarmed)
	{	
		if (__picontroller_reset && ((fabs(omega_L) >= MOTORS_OMEGA_OBJ_MIN) || 
										(fabs(omega_R) >= MOTORS_OMEGA_OBJ_MIN)))
		{// "Будим" ПИ-регулятор, если он ещё не работает:
			rtos_set_task (__motors_pi_controller, RTOS_RUN_ASAP, PICONTR_PERIOD);
		}
		
		if (fabs(omega_L) >= MOTORS_OMEGA_OBJ_MIN)
		{
			__omega_obj_L = omega_L;
		}
		else
		{
			__omega_obj_L = 0.0;
		}
		
		if (fabs(omega_R) >= MOTORS_OMEGA_OBJ_MIN)
		{
			__omega_obj_R = omega_R;
		}
		else
		{
			__omega_obj_R = 0.0;
		}
	}
// 	else
// 	{
// 		uart_puts ("[ FAIL ] Unable to set motor omega in disarm mode\n");
// 	}
	
 	return;
}

/*
void step_test (void)
{
	#define REC_COUNT	200	// количество записей
	#define REC_PERIOD	10	// пишем каждые 10 мс
	
	static uint8_t phase = 0;
	static uint8_t idx = 0;		// индекс записи
	static double omega_rec[REC_COUNT];	// пишем каждые 10 мс
	
	switch (phase)
	{
		case 0:
		{
			uart_puts ("[ ! ] Step test begin\n");
			__motors_set_thrust (190, 190);
			//motors_set_omega (30, 30);
			omega_rec[idx] = __omega_R_raw * ESTIM_SCALE;
			idx++;
			phase = 1;
			rtos_set_task (step_test, REC_PERIOD, REC_PERIOD);
			
			break;
		}
		case 1:
		{
			omega_rec[idx] = __omega_R_raw * ESTIM_SCALE;
			idx++;
			if (idx == REC_COUNT)
			{
				idx = 0;
				__motors_set_thrust (0, 0);
				//motors_set_omega (0, 0);
				uart_puts ("[ OK ] Step test completed. Res avail in 3 sec\n");
				phase = 2;
				rtos_set_task (step_test, 3000, RTOS_RUN_ONCE);
			}
			
			break;
		}
		case 2:
		{
			uart_puts ("STEP TEST RESULTS:\n\n");
			rtos_set_task (step_test, 5, 5);
			phase = 3;
			
			break;
		}
		case 3:
		{
			printf ("%.1f\n", omega_rec[idx]);
			idx++;
			
			if (idx == REC_COUNT)
			{
				phase = 4;
				rtos_set_task (step_test, RTOS_RUN_ASAP, RTOS_RUN_ONCE);	// переконфигурируем задачу
			}
			
			break;	
		}
		case  4:
		{
			uart_puts ("END\n\n");
			
			break;
		}
	}
	
	return;
}
*/