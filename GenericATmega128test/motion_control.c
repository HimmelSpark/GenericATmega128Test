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
#include "modules/md3.h"

MOTION_PARAMS __motion_params;

uint8_t __mcontroller_reset = 1;

void mcontrol_init (void)
{
	__motion_params.lin_vel = 0.0;
	__motion_params.ang_vel = 0.0;
	
	return;
}

void mcontrol_set (float lin_vel, float ang_vel)
{
	if (__mcontroller_reset)
	{	// __motion_controller пока не активен
		if (lin_vel != 0.0)
		{	// "Будим" его, если заданная скорость отлична от нуля
			rtos_set_task (__motion_controller, RTOS_RUN_ASAP, MCONTROL_PERIOD);
		}
		else
		{	// В противном случае уходим отсюда
			return;
		}
		
	}
	
	// Установка линейной скорости:
	if (fabs(lin_vel) > MCONTROL_LIN_VEL_CONSTR_MAX)
	{
		__motion_params.lin_vel = MCONTROL_LIN_VEL_CONSTR_MAX;
	}
	else
	{
		__motion_params.lin_vel = lin_vel;
	}
	
	// Установка угловой скорости (поворота)
	int8_t sgn = (ang_vel >= 0) ? 1 : (-1);
	
	if (fabs(ang_vel) > MCONTROL_ANG_VEL_CONSTR_MAX)
	{
		__motion_params.ang_vel = sgn * MCONTROL_ANG_VEL_CONSTR_MAX;
	}
	else
	{
		__motion_params.ang_vel = ang_vel;
	}
	
	return;
}

MOTION_PARAMS mcontrol_get_mparams (void)
{	// Расчётные скорости (на основе скоростей вращения двигателей)
	
	MOTOR_OMEGA_DATA omega = motors_get_omega ();
	MOTION_PARAMS motion;
	
	motion.lin_vel = (MCONTROL_R/2.0) * (omega.omegaL + omega.omegaR);
	motion.ang_vel = (MCONTROL_R/MCONTROL_B) * (omega.omegaR - omega.omegaL);
	
	return motion;
}

void __motion_controller (void)
{	// Пока что логика этого регулятора нереверсивная (т.к. это аппаратно пока недоступно).
	// В частности, нулевая заданная линейная скорость интерпретируется как безоговорочная остановка
	
	MPU6050_GYRO_DATA gyro;
	double omega1_obj, omega2_obj;
	
	static double I = 0.0, domega = 0.0;
	double eps = 0.0;
	
	if (__mcontroller_reset)
	{
		I = 0.0;
		
		__mcontroller_reset = 0;
		uart_puts ("[ OK ] Motion controller engaged\n");
	}
	
	if (__motion_params.lin_vel != 0.0)
	{
		gyro = mpu6050_get_gyro ();
		
		// Задаём движение с заданной линейной скоростью:
		omega1_obj = omega2_obj = __motion_params.lin_vel / MCONTROL_R;
		
		// Коррекция скоростей с учётом заданной угловой скорости вращения платформы
		// (ПИ-регулятор)
		eps = __motion_params.ang_vel - gyro.gZ;	// ошибка по угловой скрости
		I += eps * MCONTROL_PI_Ki * MCONTROL_PI_dT;
		domega = eps * MCONTROL_PI_Kp + I;			// управляющее воздействие как поправка к скорости
													// вращения двигателей
		
		// ToDo: насыщение
		
		omega1_obj += (-domega);	// поправка для левого двигателя
		omega2_obj += domega;		// поправка для правого двигателя
		
	}
	else
	{	// Остановка
		
		omega1_obj = omega2_obj = 0.0;	// в случае нулевых уставок платформа гарантированно
										// остановится (это обеспечивается ПИ-регулятором)
		rtos_delete_task (__motion_controller);
		__mcontroller_reset = 1;
		uart_puts ("[ OK ] Motion controller disengaged\n");
	}
	
	motors_set_omega (omega1_obj, omega2_obj);
	
	return;
}

void __mcontrol_obj_poll (void)
{
	__motion_params.lin_vel = (MCONTROL_LIN_VEL_CONSTR_MAX * md3_get_pot ())  / MD3_POT_MAX;
	
	return;
}