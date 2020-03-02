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
uint8_t __mcontroller_crs_rst = 0;

void mcontrol_init (void)
{
	__motion_params.lin_vel = 0.0;
	__motion_params.ang_vel = 0.0;
	
	return;
}

void mcontrol_set (float lin_vel, float ang_vel)
{
	if (__mcontroller_reset)
	{	// __motion_controller ���� �� �������
		if (lin_vel != 0.0)
		{	// "�����" ���, ���� �������� �������� ������� �� ����
			rtos_set_task (__motion_controller, RTOS_RUN_ASAP, MCONTROL_PERIOD);
		}
		else
		{	// � ��������� ������ ������ ������
			return;
		}
		
	}
	
	// ��������� �������� ��������:
	if (lin_vel > MCONTROL_LIN_VEL_CONSTR_MAX)
	{
		__motion_params.lin_vel = MCONTROL_LIN_VEL_CONSTR_MAX;
	}
	else if (lin_vel < -MCONTROL_LIN_VEL_CONSTR_MAX)
	{
		__motion_params.lin_vel = -MCONTROL_LIN_VEL_CONSTR_MAX;
	}
	else
	{
		__motion_params.lin_vel = lin_vel;
	}
	
	// ��������� ������� �������� (��������)
	if (ang_vel > MCONTROL_ANG_VEL_CONSTR_MAX)
	{
		__motion_params.ang_vel = MCONTROL_ANG_VEL_CONSTR_MAX;
	}
	else if (ang_vel < -MCONTROL_ANG_VEL_CONSTR_MAX)
	{
		__motion_params.ang_vel = -MCONTROL_ANG_VEL_CONSTR_MAX;
	}
	else
	{
		__motion_params.ang_vel = ang_vel;
	}
	
	return;
}

void mcontrol_get_obj(float *lin_vel, float *ang_vel)
{
	*lin_vel = __motion_params.lin_vel;
	*ang_vel = __motion_params.ang_vel;
	
	return;
}

// inline void mcontrol_course_reset(void)
// {
// 	__mcontroller_reset = 1;
// }

MOTION_PARAMS mcontrol_get_mparams (void)
{	// ��������� �������� (�� ������ ��������� �������� ����������)
	
	MOTOR_OMEGA_DATA omega = motors_get_omega ();
	MOTION_PARAMS motion;
	
	motion.lin_vel = (MCONTROL_R/2.0) * (omega.omegaL + omega.omegaR);
	motion.ang_vel = (MCONTROL_R/MCONTROL_B) * (omega.omegaR - omega.omegaL);
	
	return motion;
}

void __motion_controller (void)
{	
	MPU6050_GYRO_DATA gyro;
	double omega1_obj, omega2_obj;
	
	static double	I = 0.0,
					domega = 0.0;
	double eps = 0.0;
	
	if (__mcontroller_reset)
	{
		I = 0.0;
		__mcontroller_reset = 0;
		
		uart_puts ("[ OK ] MCONTR reset\n");
	}
	
	if (__motion_params.lin_vel != 0.0)
	{
		gyro = mpu6050_get_gyro ();
		
//		gyro.gZ = __motion_params.ang_vel;	// ������ ��� �������
		
		// ����� �������� � �������� �������� ���������:
		omega1_obj = omega2_obj = __motion_params.lin_vel / MCONTROL_R;
		
		// ��������� ��������� � ������ �������� ������� �������� �������� ���������
		// (��-���������)
		eps = __motion_params.ang_vel - gyro.gZ;	// ������ �� ������� �������
		I += eps * MCONTROL_PI_Ki * MCONTROL_PI_dT;
		domega = eps * MCONTROL_PI_Kp + I;			// ����������� ����������� ��� �������� � ��������
													// �������� ����������
		
		// ToDo: ���������
		
		omega1_obj += (-domega);	// �������� ��� ������ ���������
		omega2_obj += domega;		// �������� ��� ������� ���������
		
	}
	else
	{	// ���������
		
		omega1_obj = omega2_obj = 0.0;	// � ������ ������� ������� ��������� ��������������
										// ����������� (��� �������������� ��-�����������)
		rtos_delete_task (__motion_controller);
		__mcontroller_reset = 1;
		uart_puts ("[ OK ] MCONTR disengaged\n");
	}
	
	motors_set_omega (omega1_obj, omega2_obj);
	
	return;
}