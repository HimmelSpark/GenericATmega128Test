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

/* Прикладная часть */
#include "interfaces/i2c.h"
#include "interfaces/uart.h"
#include "modules/md3.h"
#include "modules/bmp180.h"
#include "modules/mpu6050.h"
#include "modules/hmc5883l.h"
#include "modules/motor.h"
#include "modules/gps.h"
#include "nav.h"
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

/* Устройства вывода */
static FILE __7SEG__	= FDEV_SETUP_STREAM (_7seg_stdputc, NULL, _FDEV_SETUP_WRITE);
static FILE __UART__	= FDEV_SETUP_STREAM (uart_stdputc, NULL, _FDEV_SETUP_WRITE);
static FILE __LCD__		= FDEV_SETUP_STREAM (lcd_stdputc, NULL, _FDEV_SETUP_WRITE);
/*********************/


ISR(ADC_vect)
{
	__adc_routine();
}

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

ISR(USART1_RX_vect)
{
	__gps_rx_routine();
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

ISR (INT4_vect)			// прерывание энкодера L
{
	__enc_L ();
}

ISR (INT5_vect)			// прерывание энкодера R
{
	__enc_R ();
}

int main (void)
{
	rtos_init ();
	general_init ();
	
	md3_init ();
	uart_init ();
	i2c_init ();
	lcd_init ();
	_7seg_init ();
	motors_init ();
	mcontrol_init ();
	nav_init();
	
// Выбор стандартного вывода:
//	stdout = &_7SEG_;
// 	stdout = &_LCD_;
 	stdout = &__UART__;
	
	rtos_set_task (show_info_7seg, 1000, 200);
 	rtos_set_task (show_info_lcd, 1010, 200);
// 	rtos_set_task (show_info_uart, 1050, 300);
	
// 	rtos_set_task (bmp180_init, 1, RTOS_RUN_ONCE);
// 	rtos_set_task (mpu6050_init, 1, RTOS_RUN_ONCE);
	rtos_set_task (hmc5883l_init, 1, RTOS_RUN_ONCE);
	 
	rtos_set_task(gps_init, 1000, RTOS_RUN_ONCE);
	
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
	
//	stdout = &_UART_;
	
//	uart_clrscr ();
	uart_home ();

	GPS_POS gps_pos;
	GPS_MOTION gps_motion;
	GPS_INF gps_info;
	GPS_DATE gps_date;
	GPS_TIME gps_time;
	
	gps_get_pos(&gps_pos);
	gps_get_motion(&gps_motion);
	gps_get_info(&gps_info);
	gps_get_date(&gps_date);
	gps_get_time(&gps_time);

// 	float lat_ref, lon_ref;
// 	nav_get_tgt_wp(&lat_ref, &lon_ref);
	
// 	float dst = nav_dst2tgt(gps_pos.lat, gps_pos.lon);
// 	uint16_t brg = nav_brg2tgt(gps_pos.lat, gps_pos.lon);

// 	NAV_ROUTE_PROGRESS progress;
// 	nav_route_get_progress(&progress);

// 	float lin_vel_obj, ang_vel_obj;
// 	mcontrol_get_obj(&lin_vel_obj, &ang_vel_obj);
	
	
//	uint16_t dpsi_test = nav_dpsi2tgt(gps_pos.lat, gps_pos.lon, gps_motion.crs);
	
// 	MPU6050_ACCEL_DATA accel = mpu6050_get_accel ();
 	MPU6050_GYRO_DATA gyro = mpu6050_get_gyro ();
//	static float phi_x = 0.0, phi_y = 0.0, phi_z = 0.0;
//	static float  phi_z = 0.0;

// 	HMC5883L_MAG_DATA mag;
// 	hmc5883l_get_mag(&mag);

	HMC5883L_RAW_DATA hmc5883l_raw;
	hmc5883l_get_raw(&hmc5883l_raw);
	
	HMC5883L_DEBUG hmc5883l_debug;
	hmc5883l_get_debug(&hmc5883l_debug);

//	MOTION_PARAMS motion = mcontrol_get_mparams ();

//	MOTOR_OMEGA_DATA	omega		= motors_get_omega ();
// 						omega_obj	= motors_get_omega_obj ();
 						
//	MOTOR_POWER_DATA power = motors_get_power ();
	
//	extern __debug_picontr_data picontr;
	
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

// 	printf ("%2d %2d %3d %4d %4d %2d %2d %3d %4d %4d\n", \
// 	(int)omega_obj.omegaL, (int)omega.omegaL, (int)picontr.eps_L, (int)picontr.I_L, picontr.u_L, \
// 	(int)omega_obj.omegaR, (int)omega.omegaR, (int)picontr.eps_R, (int)picontr.I_R, picontr.u_R);

//	printf ("%4.1f  |  %4.1f      |     %6.1f      |     %6.3f\n", \
				omega_obj.omegaL, omega_obj.omegaR, mpu6050_gyro.gZ, motion.lin_vel);
				
// 	fprintf(&__UART__,
// 		"%02d-%02d-%02d\t%02d:%02d:%02d UTC\nLat %02.5f\tLon %03.5f\tAlt %03.1f\tVel %02.1f\tCrs %03.1f\nSats %2d\tHDOP %02.1f\n\n",
// 		gps_date.dd, gps_date.mm, gps_date.yy, gps_time.hh, gps_time.mm, gps_time.ss,
// 		gps_pos.lat, gps_pos.lon, gps_pos.alt, gps_motion.vel, gps_motion.crs,
// 		gps_info.sats_num, gps_info.hdop);
		
// 	fprintf(&__UART__,
// 		"%02d:%02d:%02d UTC\nLat %02.5f\tLon %03.5f\tVel %02.1f\tCrs %03.1f\nSats %d\tHDOP %02.1f\n\n",
// 		gps_time.hh, gps_time.mm, gps_time.ss,
// 		gps_pos.lat, gps_pos.lon, gps_motion.vel, gps_motion.crs,
// 		gps_info.sats_num, gps_info.hdop);

// 	fprintf(&__UART__,
// 		"%8.5f\t%8.5f\t%5.1f\t%05.2f\t%03d\t%2d\t%4.1f\t%8.5f\t%8.5f\t%3d\t%5.1f\t%5.2f\n",
// 		gps_pos.lat, gps_pos.lon, gps_pos.alt, gps_motion.vel*0.5144, gps_motion.crs,
// 		gps_info.sats_num, gps_info.hdop, 
// 		lat_ref, lon_ref, brg, dst,
// 		md3_get_voltage());
		
// 	fprintf(&__UART__,
// 		"%8.5f\t%8.5f\t%05.2f\t%03d\t%2d\t%4.1f\tR%02d\tW%02d\t%4d\t%3d\t%5.1f\t%5.3f\t%3d\t%3d\t%3d\t%5.1f\t%3d\t%5.1f\t%5.2f\n",
// 		gps_pos.lat, gps_pos.lon, gps_motion.vel*0.5144, gps_motion.crs,
// 		gps_info.sats_num, gps_info.hdop,
// 		progress.rte_id, progress.wp_tgt+1, progress.dpsi, progress.brg, progress.dst,
// 		lin_vel_obj, (int)(ang_vel_obj*180/3.14),
// 		(int)(gyro.gZ*180/3.14),
// 		(uint8_t)power.powL, omega.omegaL, (uint8_t)power.powR, omega.omegaR,
// 		md3_get_voltage());
		
	fprintf(&__UART__,
		"\nGPS:\n\tLAT %8.5f\tLON %8.5f\tVEL %05.2f\tCRS %03d\t\tSATS %2d\t\tHDOP %4.1f\nGYROS:\n\tz %4d\nMAG (raw):\n\tx %5d\t\ty %5d\t\tz %5d\n\nVOLT: %5.2f\n\n",
		gps_pos.lat, gps_pos.lon, gps_motion.vel*0.5144, gps_motion.crs,
		gps_info.sats_num, gps_info.hdop,
		(int)(gyro.gZ*180/3.14),
		hmc5883l_raw.mX_raw, hmc5883l_raw.mY_raw, hmc5883l_raw.mZ_raw,
		md3_get_voltage());
		
	fprintf(&__UART__,
		"HMC5883L debug:\n\tCRA = %d\tCRB = %d\tMODE = %d\tSTATUS = %d\tID_A = %c  ID_B = %c  ID_C = %c\n\n",
		hmc5883l_debug.cra, hmc5883l_debug.crb, hmc5883l_debug.moder, hmc5883l_debug.status,
		hmc5883l_debug.ida, hmc5883l_debug.idb, hmc5883l_debug.idc);

//	fprintf(&__UART__, "%5.1f\t%4.1f\t\t%5.1f\t%4.1f\n", power.powL, omega.omegaL, power.powR, omega.omegaR);
//	#define dT		0.025
// 	phi_x += gyro.gX*dT;
// 	phi_y += gyro.gY*dT;
//	phi_z += gyro.gZ*dT;
// 	fprintf(&__UART__, "%10.6f\t%10.6f\t\t%10.6f\t%10.6f\t\t%10.6f\t%10.6f\n", 
// 				gyro.gX, phi_x, gyro.gY, phi_y, gyro.gZ, phi_z);
//	fprintf(&__UART__, "%10.6f\t%10.6f\n", gyro.gZ*180.0/3.14, phi_z*180.0/3.14);
	
//	fprintf(&__UART__, "%.2f\n", motion.lin_vel);
					
	return;
}

void show_info_lcd (void)
{
//	static float time = -0.1;
//	time += 0.1;

// 	MPU6050_ACCEL_DATA mpu6050_accel = mpu6050_get_accel ();
	MPU6050_GYRO_DATA mpu6050_gyro = mpu6050_get_gyro();
//	MOTION_PARAMS motion = mcontrol_get_mparams ();
//	extern __debug_picontr_data picontr;
	GPS_INF gps_info;
	gps_get_info(&gps_info);

	MOTOR_OMEGA_DATA omega = motors_get_omega();
		
//	stdout = &_LCD_;
	
	fprintf(&__LCD__, "L%5.1f R%5.1f\ngZ%4.0f Sat %2d %c\n", 
				omega.omegaL, omega.omegaR, mpu6050_gyro.gZ*180.0/3.14, gps_info.sats_num, gps_info.status);
	
// 	printf ("V:%.3f Om:%5.1f\ngZ %5.1f [deg/s]\n", \
// 			motion.lin_vel, motion.ang_vel * 180.0 / 3.14, mpu6050_gyro.gZ);

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
//	MOTOR_OMEGA_DATA omega = motors_get_omega ();
//  MOTOR_OMEGA_DATA omega_obj = motors_get_omega_obj ();
// 	MOTOR_POWER_DATA power = motors_get_power ();

//	uint16_t pot_val = md3_get_pot ();
	
//	double setval = ((double)pot_val/(double)MD3_POT_MAX) * 0.5;

	switch ((~BUTTON_PIN) & BUTTON_MSK)	// смотрим на "кнопочную" часть порта
	{
		case 0:	// ничего не подключено
		{
			fprintf(&__7SEG__, "u%4.1f\n", md3_get_voltage());
			/*printf ("%4.1f\n", gyro_data.gZ * 180/3.14);*/
			/*printf ("%4d\n", pot_val >> 2);*/
			/*printf ("%2d:%2d\n", (int)omega.omegaL, (int)omega.omegaR);*/
			/*printf ("%.1f\n", bmp180_get_P_mmHg ());*/
			/*printf("%4.1f\n", setval);*/
			
			/*__motors_set_thrust((int16_t)(pot_val >> 2), (int16_t)(pot_val >> 2));*/
			
			/*motors_set_omega(5.0, 5.0);*/
			
			/*motors_set_omega(setval, setval);*/
			
			/*mcontrol_set(setval, 0.0);*/
			
			break;
		}
		case ((1 << BUT1) | (1 << BUT0)):	// нажата кнопка + провод на PB0
		{
			/*bmp180_set_P0 (bmp180_get_P_hPa () * 100);*/
			
			break;
		}
		case (1 << BUT0):
		{
			/*fprintf(&__7SEG__, "%2d:%2d\n", (int)omega.omegaL, (int)omega.omegaR);		*/
			/*printf ("%4.1f\n", bmp180_get_h ());*/
			/*printf ("%4.1f\n", gyro_data.gY);*/
			/*printf ("%4d\n", -1*(pot_val >> 2));*/
			/*printf ("t%3.1f\n", bmp180_get_T());*/
			/*printf ("a%3d\n", adc_val >> 2);*/
			/*printf ("r%4.1f\n", omega.omegaR);*/
			/*printf ("%2d:%2d\n", (int)omega.omegaL, (int)omega.omegaR);*/
			
			/*__motors_set_thrust(-1*(pot_val >> 2), -1*(pot_val >> 2));*/
			
			/*motors_set_omega(-5.0, -5.0);*/
			
			/*motors_set_omega(-setval, -setval);*/
			
			/*mcontrol_set(-setval, 0.0);*/
			
			break;
		}
		case (1 << BUT2):
		{
			/*printf ("%d\n", pot_val);*/
			/*printf ("%4.1f\n", gyro_data.gZ);*/
			/*printf ("%4.1f\n", power.powL);*/
			/*printf ("%4.1f\n", bmp180_get_dhdt ());*/
			
			/*__MOTORS_CTRL_L_STOP; __MOTORS_CTRL_R_STOP;*/
			
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