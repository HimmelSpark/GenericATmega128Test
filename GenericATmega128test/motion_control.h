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
	} MCONTROL_PARAMS;	// ���-��������� ���������� ��������

/* ���������������� ������� */

void mcontrol_init (void);							// ������������� ���������� ���������
void mcontrol_set (float lin_vel, float ang_vel);	// ����� �������� �������� � �������� ���������
MCONTROL_PARAMS mcontrol_get_mparams (void);			// �������� ���������  ��������� ��������
													// (�������� � ������� ��������)
/****************************/


/* ��������� ������� */

void __motion_controller (void);	// ��������� �������� ������; ������������� �������� ��������
// ���������� � ���, ����� ���������� �������� ��������� ��������; ���������� ������������
void __mcontrol_obj_poll (void);	// ����� ��������� �������� �������� (�������)

/*********************/


/* ����������� */

#define MCONTROL_STARTUP_DELAY	1000	// ��
#define MCONTROL_PERIOD			100		// ��; �� ������� �������� �������� ����������� ������
										// (�� MPU6050, ������ 100 ��)
										
										
#define MCONTROL_R	0.0325	// �; ������ ������� ����
#define MCONTROL_B	0.2100	// �; ���������� ����� �������� �������
#define MCONTROL_K	1.0		// �/�; ����������� ����� ������ �� ������� �������� ���������

#define MCONTROL_LIN_VEL_CONSTR_MAX		1.0	// �/�; ����������� ��������
#define MCONTROL_ANG_VEL_CONSTR_MAX		1.0	// ���/�; ����������� �������� ��������


#endif /* MOTION_CONTROL_H_ */