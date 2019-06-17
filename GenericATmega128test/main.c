/*
 * main.c
 *
 * Created: 01.05.2018 16:48:56
 * Author : Всеволод
 */ 

/* Ядро */
#include "general.h"
#include "rtos.h"
#include "fifo.h"
/********/

/* Аппаратная часть */
#include "interfaces/i2c.h"
#include "interfaces/uart.h"
#include "modules/md3.h"
#include "modules/bmp180.h"
#include "modules/mpu6050.h"
#include "modules/motor.h"
#include "motion_control.h"
#include "displays/_7seg_disp.h"
#include "displays/lcd.h"
/*******************/

// Вывод отладочной и иной информации
void show_info_7seg (void);
void show_info_uart (void);
void show_info_lcd (void);

// Временно: процедура с приоритетными задачами (для нормальной работы 7seg)
void __top_priority (void);

/* Устройства стандартного вывода */
static FILE _7SEG_	= FDEV_SETUP_STREAM (_7seg_stdputc, NULL, _FDEV_SETUP_WRITE);
static FILE _UART_	= FDEV_SETUP_STREAM (uart_stdputc, NULL, _FDEV_SETUP_WRITE);
static FILE _LCD_	= FDEV_SETUP_STREAM (lcd_stdputc, NULL, _FDEV_SETUP_WRITE);
/**********************************/


ISR (TWI_vect)
{
	__i2c_routine ();
}

ISR (USART0_UDRE_vect)
{
	__uart_tx_routine ();
}

ISR (USART0_RX_vect)
{
	__uart_rx_byte ();
}

// ISR (TIMER0_COMP_vect)
// {
// 	// Служба таймеров ядра (вариант с TIMER0, если не используем XDIV;
// 	// иначе TIMER0 работает только в асинхронном режиме - использовать TIMER2)	
// 
// 	// Временный костыль, решающий проблему равномерной по времени развёртки 7-сегментника:
// 	__top_priority ();
// 	// Сама служба
// 	__rtos_timer_service ();
// }

ISR (TIMER2_COMP_vect)
{
	// Служба таймеров ядра (вариант с TIMER2, если используем XDIV,
	// т.к. TIMER0 в таком случае работает только в асинхронном режиме)
	
	// Временный костыль, решающий проблему равномерной по времени развёртки 7-сегментника:
	__top_priority ();
	// Сама служба
	__rtos_timer_service ();
}

ISR (TIMER3_COMPB_vect) // разрешаем прерывания энкодера L
{
	__enc_L_enable ();
}

ISR (TIMER3_COMPC_vect) // разрешаем прерывания энкодера R
{
	__enc_R_enable ();
}

ISR (INT2_vect)			// прерывание энкодера L
{
	__enc_L ();
}

ISR (INT3_vect)			// прерывание энкодера R
{
	__enc_R ();
}

int main (void)
{
	general_init ();
	rtos_init ();
	
	md3_init ();
 	uart_init ();
 	i2c_init ();
 	lcd_init ();
	_7seg_init ();
	motors_init ();
	mcontrol_init ();
	
// Выбор стандартного вывода (необязательно здесь):
//	stdout = &_7SEG_;
// 	stdout = &_LCD_;
// 	stdout = &_UART_;
	
	rtos_set_task (show_info_7seg, 1000, 200);
 	rtos_set_task (show_info_lcd, 1010, 100);
// 	rtos_set_task (show_info_uart, 1050, 100);
	
// 	rtos_set_task (bmp180_init, 1, RTOS_RUN_ONCE);
 	rtos_set_task (mpu6050_init, 1, RTOS_RUN_ONCE);
	
	sei ();

    while (1) 
    {
		__rtos_task_manager ();
    }
}

void __top_priority (void)
{// Временный костыль, решающий проблему равномерной по времени развёртки 7-сегментника

	_7seg_redraw ();
	
	return;
}

/* Область задач, не описанных в других модулях и заголовочных файлах */

void show_info_uart (void)
{
//	static float time = 0.0;
	
	stdout = &_UART_;
	
// 	uart_clrscr ();
// 	uart_home ();
	
// 	MPU6050_ACCEL_DATA mpu6050_accel = mpu6050_get_accel ();
// 	MPU6050_GYRO_DATA mpu6050_gyro = mpu6050_get_gyro ();
// 
 	MOTOR_OMEGA_DATA	omega		= motors_get_omega (),
 						omega_obj	= motors_get_omega_obj ();
// 						
// 	MOTOR_POWER_DATA power = motors_get_power ();
	
	extern __debug_picontr_data picontr;
	
// В идеале надо вывести "фон" и обновлять только изменяющиеся значения

// 	printf ("Elapsed Time		%.1f	[s]\n\n", time);
// 	time += 0.2;
// 
// 	printf ("BMP180 TEMP:		%.1f	[deg. C]\n",  bmp180_get_T ());
// 	printf ("BMP180 PRESSURE:	%.1f	[hPa]\n",  bmp180_get_P_hPa ());
// 	printf ("BMP180 h:			%.1f	[m]\n", bmp180_get_h ());
// 	printf ("BMP180 dh/dt:		%.1f	[m/s]\n\n", bmp180_get_dhdt ());
// 	
//  	printf ("MPU6050 TEMP:		%.1f	[deg. C]\n", mpu6050_get_T ());
// 	printf ("MPU6050 ACCEL:		Wx=%.3f	Wy=%.3f	Wz=%.3f	[m/s^2]\n", \
// 					mpu6050_accel.aX, mpu6050_accel.aY, mpu6050_accel.aZ);
// 	printf ("MPU6050 GYRO:		OMx=%.3f	OMy=%.3f	OMz=%.3f	[deg./s]\n\n", \
// 					mpu6050_gyro.gX, mpu6050_gyro.gY, mpu6050_gyro.gZ);
// 
// 	printf ("MOTOR_L:	TGT_SPEED = %4.1f	ACT_SPEED = %4.1f	[rad/s]\n",\
// 					omega_obj.omegaL, omega.omegaL);
// 	printf ("		USED_POWER = %4.1f	POWER_LIM = %4.1f	[%%]\n",\
// 					power.powL, (MOTORS_PWM_CONSTR_MAX/255.0)*100.0);
// 	printf ("MOTOR_R:	TGT_SPEED = %4.1f	ACT_SPEED = %4.1f	[rad/s]\n",\
// 					omega_obj.omegaR, omega.omegaR);
// 	printf ("		USED_POWER = %4.1f	POWER_LIM = %4.1f	[%%]\n",\
// 					power.powR, (MOTORS_PWM_CONSTR_MAX/255.0)*100.0);

	printf ("%2d %2d %3d %4d %4d %2d %2d %3d %4d %4d\n", \
	(int)omega_obj.omegaL, (int)omega.omegaL, (int)picontr.eps_L, (int)picontr.I_L, picontr.u_L, \
	(int)omega_obj.omegaR, (int)omega.omegaR, (int)picontr.eps_R, (int)picontr.I_R, picontr.u_R);
					
	return;
}

void show_info_lcd (void)
{
//	static float time = -0.1;
//	time += 0.1;

// 	MPU6050_ACCEL_DATA mpu6050_accel = mpu6050_get_accel ();
	MPU6050_GYRO_DATA mpu6050_gyro = mpu6050_get_gyro ();
	MOTION_PARAMS motion = mcontrol_get_mparams ();
//	extern __debug_picontr_data picontr;
	
	stdout = &_LCD_;
	
	printf ("V:%.3f Om:%5.1f\ngZ %5.1f [deg/s]\n", \
			motion.lin_vel, motion.ang_vel * 180.0 / 3.14, mpu6050_gyro.gZ);

//	printf ("Time %.1f [s]\ngZ %5.1f [deg/s]\n", time, mpu6050_gyro.gZ);

//	printf ("Time %.1f [s]\n\n", time);

// 	printf ("L e%3d I%3d u%3d\nR e%3d I%3d u%3d\n", \
// 			(int)picontr.eps_L, (int)picontr.I_L, picontr.u_L, \
// 			(int)picontr.eps_R, (int)picontr.I_R, picontr.u_R);

	
	return;
}

void show_info_7seg (void)
{
// 	MPU6050_ACCEL_DATA accel_data = mpu6050_get_accel ();
// 	MPU6050_GYRO_DATA gyro_data = mpu6050_get_gyro ();
 	MOTOR_OMEGA_DATA omega = motors_get_omega ();
 	MOTOR_OMEGA_DATA omega_obj = motors_get_omega_obj ();
// 	MOTOR_POWER_DATA power = motors_get_power ();

	uint16_t pot_val = md3_get_pot ();
	
	stdout = &_7SEG_;

	switch ((~BUTTON_PIN) & BUTTON_MSK)	// смотрим на "кнопочную" часть порта
	{
		case ((1 << BUT1) | (1 << BUT0)):	// нажата кнопка + провод на PB0
		{
			bmp180_set_P0 (bmp180_get_P_hPa () * 100);
			break;
		}
		case 0:	// ничего не подключено
		{
			/*printf ("%4.1f\n", gyro_data.gX);*/
			/*printf ("a%3d\n", adc_val >> 2);*/
			printf ("%2d:%2d\n", (int)omega_obj.omegaL, (int)omega_obj.omegaR);
			/*printf ("%.1f\n", bmp180_get_P_mmHg ());*/
			break;
		}
		case (1 << BUT0):
		{
			/*printf ("%4.1f\n", bmp180_get_h ());*/
			/*printf ("%4.1f\n", gyro_data.gY);*/
			/*printf ("a%3.1f\n", adc_val / 100.0);*/
			/*printf ("t%3.1f\n", bmp180_get_T());*/
			/*printf ("a%3d\n", adc_val >> 2);*/
			/*printf ("r%4.1f\n", omega.omegaR);*/
			break;
		}
		case (1 << BUT2):
		{
			/*printf ("%d\n", pot_val);*/
			/*printf ("%4.1f\n", gyro_data.gZ);*/
			/*printf ("%4.1f\n", power.powL);*/
			/*printf ("%4.1f\n", bmp180_get_dhdt ());*/
			break;
		}
		case (1 << BUT3):
		{
			/*printf ("%4.1f\n", gyro_data.gZ);*/
			/*printf ("%4.1f\n", accel_data.aZ);*/
			break;
		}
	}

	return;
}