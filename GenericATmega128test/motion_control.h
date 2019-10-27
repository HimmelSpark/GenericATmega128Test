/*
 * motion_control.h
 *
 * Created: 03.06.2019 15:00:57
 *  Author: Vsevolod
 */ 


#ifndef MOTION_CONTROL_H_
#define MOTION_CONTROL_H_

typedef struct {
	double lin_vel;	// линейна€ скорость, м/с
	double ang_vel;	// углова€ скорость (поворота платформы), рад/с
	} MOTION_PARAMS;	// тип-структура параметров движени€

/* ѕользовательские функции */

void mcontrol_init (void);							// инициализаци€ управлени€ движением
void mcontrol_set (float lin_vel, float ang_vel);	// задаЄм скорость движени€ и поворота платформы
void mcontrol_course_reset (void);					// сброс интегратора угловой скорости
MOTION_PARAMS mcontrol_get_mparams (void);			// получить расчЄтные  параметры движени€
													// (линейную и угловую скорости)
/****************************/


/* —лужебные функции */

void __motion_controller (void);	// регул€тор верхнего уровн€; устанавливает скорости вращени€
// двигателей с тем, чтобы обеспечить заданные параметры движени€

/*********************/


/* ќпределени€ */

#define MCONTROL_PERIOD			100		// мс; по периоду наиболее медленно поступающих данных
										// (от MPU6050, каждые 100 мс)
										
#define MCONTROL_R			0.0325	// м; радиус ведущих колЄс
#define MCONTROL_B			0.2100	// м; рассто€ние между ведущими колЄсами

#define MCONTROL_PI_Kp		4.0							// параметры
#define MCONTROL_PI_Ki		12.0						// ѕ»-
#define MCONTROL_PI_dT		(MCONTROL_PERIOD/1000.0)	// регул€тора


#define MCONTROL_LIN_VEL_MIN			0.13		// м/с; меньшие скорости не обеспечим
#define MCONTROL_LIN_VEL_CONSTR_MAX		1.0			// м/с; ограничение модул€ линейной скорости
#define MCONTROL_ANG_VEL_CONSTR_MAX		1.0			// рад/с; ограничение модул€ скорости поворота


#endif /* MOTION_CONTROL_H_ */