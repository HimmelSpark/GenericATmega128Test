/*
 * motion_control.h
 *
 * Created: 03.06.2019 15:00:57
 *  Author: Vsevolod
 */ 


#ifndef MOTION_CONTROL_H_
#define MOTION_CONTROL_H_

typedef struct {
	double lin_vel;	// линейная скорость, м/с
	double ang_vel;	// угловая скорость (поворота платформы), рад/с
	} MCONTROL_PARAMS;	// тип-структура параметров движения

/* Пользовательские функции */

void mcontrol_init (void);							// инициализация управления движением
void mcontrol_set (float lin_vel, float ang_vel);	// задаём скорость движения и поворота платформы
MCONTROL_PARAMS mcontrol_get_mparams (void);			// получить расчётные  параметры движения
													// (линейную и угловую скорости)
/****************************/


/* Служебные функции */

void __motion_controller (void);	// регулятор верхнего уровня; устанавливает скорости вращения
// двигателей с тем, чтобы обеспечить заданные параметры движения; вызывается периодически
void __mcontrol_obj_poll (void);	// опрос задатчика линейной скорости (отладка)

/*********************/


/* Определения */

#define MCONTROL_STARTUP_DELAY	1000	// мс
#define MCONTROL_PERIOD			100		// мс; по периоду наиболее медленно поступающих данных
										// (от MPU6050, каждые 100 мс)
										
										
#define MCONTROL_R	0.0325	// м; радиус ведущих колёс
#define MCONTROL_B	0.2100	// м; расстояние между ведущими колёсами
#define MCONTROL_K	1.0		// б/р; коэффициент учёта ошибки по угловой скорости платформы

#define MCONTROL_LIN_VEL_CONSTR_MAX		1.0	// м/с; ограничение скорости
#define MCONTROL_ANG_VEL_CONSTR_MAX		1.0	// рад/с; ограничение скорости поворота


#endif /* MOTION_CONTROL_H_ */