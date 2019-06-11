/*
 * motion_control.c
 *
 * Created: 03.06.2019 15:01:10
 *  Author: Vsevolod
 */ 

#include "general.h"
#include "motion_control.h"
#include "rtos.h"
#include <math.h>
#include "modules/md3.h"
#include "modules/motor.h"
#include "modules/mpu6050.h"
#include "displays/lcd.h"
#include "interfaces/uart.h"

MCONTROL_PARAMS __mcontrol_params;

uint8_t __mcontroller_active = 0;

void mcontrol_init (void)
{
	__mcontrol_params.lin_vel = 0.0;
	__mcontrol_params.ang_vel = 0.0;
	
//	rtos_set_task (__motion_controller, MCONTROL_STARTUP_DELAY, MCONTROL_PERIOD);
//	rtos_set_task (__mcontrol_obj_poll, MCONTROL_STARTUP_DELAY - 1, MCONTROL_PERIOD);
	
	return;
}

void mcontrol_set (float lin_vel, float ang_vel)
{
	if (!__mcontroller_active)
	{	// __motion_controller пока не активен
		if (lin_vel > 0.0)
		{	// "Будим" его, если заданная скорость отлична от нуля
			rtos_set_task (__motion_controller, RTOS_RUN_ASAP, MCONTROL_PERIOD);
			__mcontroller_active = 1;
			uart_puts ("[ OK ] Motion controller active\n");
		}
		else
		{	// В противном случае уходим отсюда
			return;
		}
		
	}
	
	// Установка линейной скорости:
	if (lin_vel > MCONTROL_LIN_VEL_CONSTR_MAX)
	{
		__mcontrol_params.lin_vel = MCONTROL_LIN_VEL_CONSTR_MAX;
	}
	else if (lin_vel < 0)
	{
		__mcontrol_params.lin_vel = 0.0;
	}
	else
	{
		__mcontrol_params.lin_vel = lin_vel;
	}
	
	// Установка угловой скорости (поворота)
	if (fabs(ang_vel) > MCONTROL_ANG_VEL_CONSTR_MAX)
	{	//									Знак			*		ограничение
		__mcontrol_params.ang_vel = (ang_vel/fabs(ang_vel)) * MCONTROL_ANG_VEL_CONSTR_MAX;
	}
	else
	{
		__mcontrol_params.ang_vel = ang_vel;
	}
	
	return;
}

MCONTROL_PARAMS mcontrol_get_mparams (void)
{	// Расчётные скорости (на основе скоростей вращения двигателей)
	
	MOTOR_OMEGA_DATA omega = motors_get_omega ();
	MCONTROL_PARAMS motion;
	
	motion.lin_vel = (MCONTROL_R/2.0) * (omega.omegaL + omega.omegaR);
	motion.ang_vel = (MCONTROL_R/MCONTROL_B) * (omega.omegaR - omega.omegaL);
	
	return motion;
}

void __motion_controller (void)
{	// Пока что логика этого регулятора нереверсивная (т.к. это аппаратно пока недоступно).
	// В частности, нулевая заданная линейная скорость интерпретируется как безоговорочная остановка
	
	MPU6050_GYRO_DATA gyro;
	double omega1_obj, omega2_obj;
	
	if (__mcontrol_params.lin_vel > 0.0)
	{	// Движение вперёд
		
		gyro = mpu6050_get_gyro ();
		gyro.gZ *= 3.14/180.0;	// перевод из град/с в рад/с

//		gyro.gZ = __mcontrol_params.ang_vel; // отладка; не учитываем показания ДУС
		
		omega1_obj = (2*__mcontrol_params.lin_vel - MCONTROL_B*__mcontrol_params.ang_vel) / (2*MCONTROL_R);
		omega2_obj = (2*__mcontrol_params.lin_vel + MCONTROL_B*__mcontrol_params.ang_vel) / (2*MCONTROL_R);
		
		// Поскольку ПИ-регулятор двигателей из motor.c на вход получает уставки по скоростям
		// вращения колёс, а мы хотим здесь учесть ещё и показания ДУС по оси Z, то проведём
		// коррекцию уставок с тем, чтобы в них уже была учтена ошибка по угловой скорости платформы,
		// и уже скорректированные уставки передадим в set_omega
		// (можно было бы весь регулятор засунуть в motor.c, однако по идее он должен заниматься
		// регулированием только скорости двигателей)
		
		omega1_obj = omega1_obj - MCONTROL_K*(__mcontrol_params.ang_vel - gyro.gZ);
		omega2_obj = omega2_obj + MCONTROL_K*(__mcontrol_params.ang_vel - gyro.gZ);
	}
	else
	{	// Остановка
		
		omega1_obj = omega2_obj = 0.0;	// в случае нулевых уставок платформа гарантированно
										// остановится (это обеспечивается ПИ-регулятором)
		rtos_delete_task (__motion_controller);
		__mcontroller_active = 0;
		uart_puts ("[ OK ] Motion controller inactive\n");
	}
	
	motors_set_omega (omega1_obj, omega2_obj);
	
	return;
}

void __mcontrol_obj_poll (void)
{
	__mcontrol_params.lin_vel = (MCONTROL_LIN_VEL_CONSTR_MAX * md3_get_pot ())  / MD3_POT_MAX;
	
	return;
}