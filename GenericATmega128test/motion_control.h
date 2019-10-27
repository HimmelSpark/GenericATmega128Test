/*
 * motion_control.h
 *
 * Created: 03.06.2019 15:00:57
 *  Author: Vsevolod
 */ 


#ifndef MOTION_CONTROL_H_
#define MOTION_CONTROL_H_

typedef struct {
	double lin_vel;	// �������� ��������, �/�
	double ang_vel;	// ������� �������� (�������� ���������), ���/�
	} MOTION_PARAMS;	// ���-��������� ���������� ��������

/* ���������������� ������� */

void mcontrol_init (void);							// ������������� ���������� ���������
void mcontrol_set (float lin_vel, float ang_vel);	// ����� �������� �������� � �������� ���������
void mcontrol_course_reset (void);					// ����� ����������� ������� ��������
MOTION_PARAMS mcontrol_get_mparams (void);			// �������� ���������  ��������� ��������
													// (�������� � ������� ��������)
/****************************/


/* ��������� ������� */

void __motion_controller (void);	// ��������� �������� ������; ������������� �������� ��������
// ���������� � ���, ����� ���������� �������� ��������� ��������

/*********************/


/* ����������� */

#define MCONTROL_PERIOD			100		// ��; �� ������� �������� �������� ����������� ������
										// (�� MPU6050, ������ 100 ��)
										
#define MCONTROL_R			0.0325	// �; ������ ������� ����
#define MCONTROL_B			0.2100	// �; ���������� ����� �������� �������

#define MCONTROL_PI_Kp		4.0							// ���������
#define MCONTROL_PI_Ki		12.0						// ��-
#define MCONTROL_PI_dT		(MCONTROL_PERIOD/1000.0)	// ����������


#define MCONTROL_LIN_VEL_MIN			0.13		// �/�; ������� �������� �� ���������
#define MCONTROL_LIN_VEL_CONSTR_MAX		1.0			// �/�; ����������� ������ �������� ��������
#define MCONTROL_ANG_VEL_CONSTR_MAX		1.0			// ���/�; ����������� ������ �������� ��������


#endif /* MOTION_CONTROL_H_ */