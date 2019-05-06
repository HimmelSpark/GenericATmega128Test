/*
 * bmp180.c
 *
 * Created: 28.11.2018 15:39:11
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "bmp180.h"
#include "../interfaces/i2c.h"
#include "../rtos.h"
#include "md3.h"
#include <math.h>

volatile short AC1 = 0, AC2 = 0, AC3 = 0;
volatile unsigned short AC4 = 0, AC5 = 0, AC6 = 0;
volatile short B1 = 0, B2 = 0, MB = 0, MC = 0, MD = 0;

/* промежуточные коэффициенты */
volatile long X1, X2, B5;
volatile long B6, X3, B3;
volatile unsigned long B4, B7;
/******************************/

volatile long UT, UP, p;

long bmp180_T = 0, bmp180_P = 0, bmp180_P0 = 101325; // P0 ещё будет меняться по требованию пользователя
float bmp180_H = 0;

/* Область переменных из модуля i2c.c */
extern uint8_t i2c_read_buffer[I2C_MAX_READ_BYTES_COUNT];
/**************************************/

static uint8_t __estimator_reset = 1;	// флаг сброса интеграторов оценивателя;
// static потому, что переменная с таким же именем используется в оценивателе в motor.c

double __bmp180_dpdt = 0.0, __bmp180_dhdt = 0.0;

double C1 = 44330.0, C2 = 0.19029, C3 = 4945.0;
// Коэффициенты для барометрической формулы.
// C3 ещё будет меняться в зависимости от P0; здесь C3 рассчитано для P0 = 101325 Па
// Точное значение C2: 0.19029495718363463368220742150333


inline void bmp180_init (void)
{
	int res = i2c_read_bytes (BMP180_ADDR, BMP180_AC1_ADDR_HI, BMP180_CAL_BYTES_COUNT, read_params_exit);
	if (res)
	{
		rtos_set_task (bmp180_init, 40, RTOS_RUN_ONCE);
	}
	
	return;
}

void read_params_exit (void)
{
	AC1 = (i2c_read_buffer[0]	<< 8)	| i2c_read_buffer[1];
	AC2 = (i2c_read_buffer[2]	<< 8)	| i2c_read_buffer[3];
	AC3 = (i2c_read_buffer[4]	<< 8)	| i2c_read_buffer[5];
	AC4 = (i2c_read_buffer[6]	<< 8)	| i2c_read_buffer[7];
	AC5 = (i2c_read_buffer[8]	<< 8)	| i2c_read_buffer[9];
	AC6 = (i2c_read_buffer[10]	<< 8)	| i2c_read_buffer[11];
	B1	= (i2c_read_buffer[12]	<< 8)	| i2c_read_buffer[13];
	B2	= (i2c_read_buffer[14]	<< 8)	| i2c_read_buffer[15];
	MB	= (i2c_read_buffer[16]	<< 8)	| i2c_read_buffer[17];
	MC	= (i2c_read_buffer[18]	<< 8)	| i2c_read_buffer[19];
	MD	= (i2c_read_buffer[20]	<< 8)	| i2c_read_buffer[21];
	
	rtos_set_task (bmp180_start_UT, BMP180_READ_STARTUP, BMP180_READ_PERIOD);	// теперь готовы читать данные
	
	return;
}

void bmp180_start_UT (void)	// вызов этой функции даёт начало цепочке измерения температуры и давления
{
	int res = i2c_write_byte2reg (BMP180_ADDR, BMP180_CTRL_REG_ADDR, BMP180_REQ_UT, start_UT_exit);
	if (res)
	{
		rtos_set_task (bmp180_start_UT, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

 void start_UT_exit (void)
 {
	rtos_set_task (bmp180_read_UT, BMP180_UT_READ_DELAY, RTOS_RUN_ONCE);
 	return;
 }

void bmp180_read_UT (void)
{
	int res = i2c_read_bytes (BMP180_ADDR, BMP180_MEASUREMENT_HI, BMP180_UT_WORD_SIZE, read_UT_exit);
	if (res)
	{
		rtos_set_task (bmp180_read_UT, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

 void read_UT_exit (void)
 {
	UT = (i2c_read_buffer[0] << 8) | i2c_read_buffer[1];
	rtos_set_task (bmp180_start_UP, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	
 	return;
 }

void bmp180_start_UP (void)
{
	int res = i2c_write_byte2reg (BMP180_ADDR, BMP180_CTRL_REG_ADDR, (BMP180_REQ_UP | (BMP180_OSS << 6)), start_UP_exit);
	if (res)
	{
		rtos_set_task(bmp180_start_UP, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

 void start_UP_exit (void)
 {
	rtos_set_task (bmp180_read_UP, BMP180_UP_READ_DELAY, RTOS_RUN_ONCE);
	return;
 }

void bmp180_read_UP (void)
{
	int res = i2c_read_bytes (BMP180_ADDR, BMP180_MEASUREMENT_HI, BMP180_UP_WORD_SIZE, read_UP_exit);
	if (res)
	{
		rtos_set_task (bmp180_read_UP, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

 void read_UP_exit (void)
 {
 	uint32_t MSB, LSB, XLSB;
 	MSB		= i2c_read_buffer[0];
 	LSB		= i2c_read_buffer[1];
 	XLSB	= i2c_read_buffer[2];
 	UP = ((MSB << 16) | (LSB << 8) | XLSB) >> (8 - BMP180_OSS);
	rtos_set_task (bmp180_calc_T, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	
 	return;
 }

void bmp180_calc_T (void)
{
	X1 = ((UT - AC6) * AC5) >> 15;
	X2 = ((long)MC << 11)/(X1 + MD);
	B5 = X1 + X2;
	bmp180_T = (B5 + 8) >> 4;	// в 0.1 °С
	
	rtos_set_task (bmp180_calc_P, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	
	return;
}

void bmp180_calc_P (void)
{
	B6 =  B5 - 4000;
	X1 = (B2 * ((B6*B6) >> 12)) >> 11;
	X2 = (AC2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = ((((long)AC1 * 4 + X3) << BMP180_OSS) + 2) >> 2;
	X1 = (AC3 * B6) >> 13;
	X2 = (B1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = (AC4 * (unsigned long)(X3 + 32768)) >> 15;
	B7 = ((unsigned long)UP - B3) * (50000 >> BMP180_OSS);
	if (B7 < 0x80000000)
	{
		p = (B7 * 2) / B4;
	}
	else
	{
		p = (B7 / B4) * 2;
	}
	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;
	bmp180_P = p + ((X1 + X2 + 3791) >> 4);	// в Па
	
	// Получили давление - прогоняем оцениватель dp/dt
	rtos_set_task (bmp180_dpdt_estimator, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	
	return;
}


void bmp180_dpdt_estimator (void)
{
// Запускается следом за bmp180_calc_P.

// Здесь оцениваем dP/dt. Это необходимо для дальнейшей оценки dh/dt в связи с тем, что
// значения давления у нас уже есть, а каждый раз запрашивать bmp180_get_H - слишком сложно вычислительно.
// Зная dp/dt, можем (по требованию пользователя) найти dh/dt:
// dh/dt [t=t0] = -C3 * C2 * (P(t) [t=t0])^(C2-1) * dP/dt [t=t0], где
// C1 = 44330;	C2 = 0.19029495718363463368220742150333...;	C3 = C1/(P0^C2); где
// P0 - QNH/QFE/что душе угодно.

	static double I1, I2;
	double eps;

	if (__estimator_reset)
	{
		// Инициализируем I1 = I2 = bmp180_P, чтобы в момент включения не было скачка;
		// да и вообще полагаем, что в начале мы стоим на месте
		
		I1 = bmp180_P;
		I2 = bmp180_P;
	
		__estimator_reset = 0;
	}

	/* I ступень: */
	eps = (double)bmp180_P - I1;
	eps *= BMP180_ESTIM_CONST_Ki;
	I1 += eps * BMP180_ESTIM_CONST_dT;
	/* II ступень: */
	eps = I1 - I2;
	eps *= BMP180_ESTIM_CONST_Ki;
	__bmp180_dpdt = eps;
	I2 += eps * BMP180_ESTIM_CONST_dT;
	
	return;
}

void bmp180_set_P0 (long P0)
{
	bmp180_P0 = P0;
	
	// изменение P0 требует пересчёта C3:
	C3 = C1 / pow (bmp180_P0, C2);
	
	return;
}

double bmp180_get_h (void) 
{	// P0 - стандартное давление, Па
	if (bmp180_P)	// отдаём только если состоялось первое измерение, о чём говорит bmp180_P != 0
	{
		return (44330 * (1 - pow ((double)bmp180_P / (double)bmp180_P0, 0.19029)));
	}
	else
	{
		return 0.0;
	}
}

double bmp180_get_dhdt (void)
{
	if (bmp180_P)	// отдаём только если состоялось первое измерение, о чём говорит bmp180_P != 0,
	{				// хотя для dh/dt это недостаточно сильное условие
		return -C3 * C2 * pow ((double)bmp180_P, C2 - 1) * __bmp180_dpdt;
	}
	else
	{
		return 0.0;
	}
}

double bmp180_get_P_hPa (void)
{
	if (bmp180_P)	// отдаём только если состоялось первое измерение, о чём говорит bmp180_P != 0
	{
		return ((double)bmp180_P / 100.0);
	}
	else
	{
		return 0.0;
	}
}

double bmp180_get_P_mmHg (void)
{
	if (bmp180_P)	// отдаём только если состоялось первое измерение, о чём говорит bmp180_P != 0
	{
		return ((double)bmp180_P / 133.28);
	}
	else
	{
		return 0.0;
	}
}

float bmp180_get_T (void)
{
	if (bmp180_P)	// отдаём только если состоялось первое измерение, о чём говорит bmp180_P != 0
	{
		return ((float)bmp180_T / 10.0);
	}
	else
	{
		return 0.0;
	}
}