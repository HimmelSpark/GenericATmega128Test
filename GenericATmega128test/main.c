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
static FILE _LCD_	= FDEV_SETUP_STREAM (lcd_stdputc, NULL, _FDEV_SETUP_WRITE);
static FILE _UART_	= FDEV_SETUP_STREAM (uart_stdputc, NULL, _FDEV_SETUP_WRITE);
/**********************************/

ISR (TWI_vect)
{
	__i2c_routine ();
}

ISR (USART0_UDRE_vect)
{
	__uart_tx_routine ();
}

ISR (TIMER0_COMP_vect)	// служба таймеров; вызывается каждую 1 мс
{// ToDo: решить на уровне RTOS проблему приоритетных задач!
	// Временный костыль, решающий проблему равномерной по времени развёртки 7-сегментника:
	__top_priority ();
	// Сама служба
	__rtos_timer_service ();
}

// ISR (TIMER1_COMPA_vect) // грубый фильтр событий энкодеров (необязателен)
// {
// 	__enc_filter ();
// }

ISR (INT2_vect)			// прерывание энкодера motor_l
{
	__enc_L ();
}

// ISR (INT3_vect)			// прерывание энкодера motor_r
// {
// 	__enc_R ();
// }

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
	
// Выбор стандартного вывода (необязательно здесь):
//	stdout = &_7SEG_;
// 	stdout = &_LCD_;
// 	stdout = &_UART_;
//	printf ("This is a STDOUT\ndev\n");
	
	rtos_set_task (show_info_7seg, 1000, 100);
	rtos_set_task (show_info_uart, 1000, 200);
	rtos_set_task (show_info_lcd, 1000, 500);
	
	rtos_set_task (bmp180_init, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	rtos_set_task (mpu6050_init, 200, RTOS_RUN_ONCE);
	
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
	static float time = 0.0;
	
	stdout = &_UART_;
	
	uart_clrscr ();
	uart_home ();
	
	MPU6050_ACCEL_DATA mpu6050_accel = mpu6050_get_accel ();
	MPU6050_GYRO_DATA mpu6050_gyro = mpu6050_get_gyro ();

	MOTOR_OMEGA_DATA	omega		= motors_get_omega (),
						omega_obj	= motors_get_omega_obj ();
						
	MOTOR_POWER_DATA power = motors_get_power ();
	
// В идеале надо вывести "фон" и обновлять только изменяющиеся значения

	printf ("Elapsed Time		%.1f	[s]\n\n", time);
	time += 0.2;

	printf ("BMP180 TEMP:		%.1f	[deg. C]\n",  bmp180_get_T ());
	printf ("BMP180 PRESSURE:	%.1f	[hPa]\n",  bmp180_get_P_hPa ());
	printf ("BMP180 h:			%.1f	[m]\n", bmp180_get_h ());
	printf ("BMP180 dh/dt:		%.1f	[m/s]\n\n", bmp180_get_dhdt ());
	
 	printf ("MPU6050 TEMP:		%.1f	[deg. C]\n", mpu6050_get_T ());
	printf ("MPU6050 ACCEL:		Wx=%.3f	Wy=%.3f	Wz=%.3f	[m/s^2]\n", \
					mpu6050_accel.aX, mpu6050_accel.aY, mpu6050_accel.aZ);
	printf ("MPU6050 GYRO:		OMx=%.3f	OMy=%.3f	OMz=%.3f	[deg./s]\n\n", \
					mpu6050_gyro.gX, mpu6050_gyro.gY, mpu6050_gyro.gZ);

	printf ("MOTOR_L:	TGT_SPEED = %4.1f	ACT_SPEED = %4.1f	[rad/s]\n",\
					omega_obj.omegaL, omega.omegaL);
	printf ("		USED_POWER = %4.1f	POWER_LIM = %4.1f	[%%]\n",\
					power.powL, (MOTORS_PWM_MAX/255.0)*100.0);
	
	return;
}

void show_info_lcd (void)
{
	static float time = 0.0;
 	MPU6050_ACCEL_DATA mpu6050_accel = mpu6050_get_accel ();
	
	stdout = &_LCD_;
	
	printf ("Time %.1f [s]\naZ %.2f [m/s^2]\n", time, mpu6050_accel.aZ);
//	printf ("Time %.1f [s]\n\n", time);
	time += 0.5;
	
	return;
}

void show_info_7seg (void)
{
 	MPU6050_ACCEL_DATA accel_data = mpu6050_get_accel ();
// 	MPU6050_GYRO_DATA gyro_data = mpu6050_get_gyro ();
// 	MOTOR_OMEGA_DATA omega = motors_get_omega ();
// 	MOTOR_OMEGA_DATA omega_obj = motors_get_omega_obj ();
// 	MOTOR_POWER_DATA power = motors_get_power ();
	
	stdout = &_7SEG_;

//	__motors_set_pwm (adc_val >> 2, 0);


	switch ((~BUTTON_PIN) & BUTTON_MSK)	// смотрим на "кнопочную" часть порта
	{
		case 0:	// ничего не подключено
		{
			/*printf ("a%3d\n", adc_val >> 2);*/
			/*printf ("%4.1f\n", omega.omegaL);*/
			/*printf ("o%4.1f\n", omega_obj.omegaL);*/
			printf ("%4.1f\n", bmp180_get_dhdt ());
			break;
		}
		case (1 << BUT0):
		{
			/*printf ("t%3.1f\n", bmp180_get_T());*/
			/*printf ("a%3d\n", adc_val >> 2);*/
			/*printf ("%4.1f\n", omega.omegaL);*/
			printf ("%4.1f\n", bmp180_get_h ());
			break;
		}
		case (1 << BUT2):	// не ошибка, BUT1 пока не используется
		{
			/*printf ("%4.1f\n", bmp180_get_P_mmHg());*/
			/*printf ("%4.1f\n", power.powL);*/
			printf ("%d\n", (uint16_t)bmp180_get_P_hPa ());
			break;
		}
		case (1 << BUT3):
		{
			/*printf ("%4.1f\n", gyro_data.gZ);*/
			printf ("%4.1f\n", accel_data.aZ);
			break;
		}
	}

	return;
}