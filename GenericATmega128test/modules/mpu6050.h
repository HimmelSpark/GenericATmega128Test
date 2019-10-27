/*
 * mpu6050.h
 *
 * Created: 08.12.2018 11:53:24
 *  Author: Vsevolod
 */ 


#ifndef MPU6050_H_
#define MPU6050_H_

typedef struct {
	double aX;
	double aY;
	double aZ;
	} MPU6050_ACCEL_DATA;
	
typedef struct {
	double gX;
	double gY;
	double gZ;
	} MPU6050_GYRO_DATA;

void mpu6050_init (void);
void mpu6050_init_set (void);
void mpu6050_poweron (void);
void mpu6050_read (void);

MPU6050_ACCEL_DATA mpu6050_get_accel (void);// возвращает структуру из 3-х компонент ускорения
MPU6050_GYRO_DATA mpu6050_get_gyro (void);	// возвращает структуру из 3-х компонент угловой скорости
float mpu6050_get_T (void);

/* exit-функции I2C */
void init_set_exit (void);
void read_exit (void);
void poweron_exit (void);
/****************/


/* Адрес модуля */
// 0b110100X, где X определяется уровнем на AD0 (задаётся далее):
#define MPU6050_ADDR_MSBs		0b110100
#define MPU6050_ADDR_LSB		0			// оно же - уровень на AD0
// Итоговый адрес = (MPU6050_ADDR_MSBs << 1) | MPU6050_ADDR_LSB
/****************/

#define MPU6050_AD0_PORT		PORTD
#define MPU6050_AD0_DDR			DDRD
#define MPU6050_AD0				PD4

/* Настройки модуля */
#define MPU6050_SMPLRT_DIV_VAL	0x1F	// 8 bit; sample rate = 31.25 Hz (раз в 32 мс)
#define MPU6050_DLPF_VAL		6		// 3 bit; DLPF enabled, gyro output rate = 1kHz (см. register map)
#define MPU6050_FS_SEL_VAL		3		// 2 bit
#define MPU6050_AFS_SEL_VAL		3		// 2 bit
/********************/

/* В какой части соответствующих регистров расположены группы битов: */
#define MPU6050_DLPF_OFFSET		0
#define MPU6050_FS_SEL_OFFSET	3
#define MPU6050_AFS_SEL_OFFSET	3
/*********************************************************************/


/* Адреса регистров */
#define MPU6050_SMPLRT_DIV		0x19
#define MPU6050_CONFIG			0x1A
#define MPU6050_GYRO_CONFIG		0x1B
#define MPU6050_ACCEL_CONFIG	0x1C

#define MPU6050_ACCEL_XOUT_H	0x3B
#define MPU6050_ACCEL_YOUT_H	0x3D
#define MPU6050_ACCEL_ZOUT_H	0x3F
#define MPU6050_TEMP_OUT_H		0x41
#define MPU6050_GYRO_XOUT_H		0x43
#define MPU6050_GYRO_YOUT_H		0x45
#define MPU6050_GYRO_ZOUT_H		0x47

#define MPU6050_PWR_MGMT1		0x6B
#define MPU6050_PWR_MGMT2		0x6C

#define MPU6050_FIFO_COUNTH		0x72
#define MPU6050_FIFO_R_W		0x74

#define MPU6050_WHO_AM_I		0x75

/********************/

#define MPU6050_STARTUP_DELAY		300		// ms; от init до poweron
#define MPU6050_READ_STARTUP_DELAY	100		// ms; от poweron до первого read
#define MPU6050_READ_PERIOD			100		// ms

#define MPU6050_WORD_SIZE		2	// байта на слово (для всех измерений)
#define MPU6050_DATA_BLOCK		14	// байт (акс + темп + гиро)

/* Как переводить из битов в °/c и g: масштабные коэф-ты в зав-ти от FS и AFS */
// Необходимо делить показания GYRO на MPU6050_GYRO_SCALE,
// accel - на MPU6050_ACCEL_SCALE

#define MPU6050_GYRO_AS_RAD_S	1		// 1 - угловые скорости в рад/с, 0 - в °/с

#if MPU6050_GYRO_AS_RAD_S == 1
	#if MPU6050_FS_SEL_VAL == 0
		#define MPU6050_GYRO_SCALE	7509.6	// double
	#elif MPU6050_FS_SEL_VAL == 1
		#define MPU6050_GYRO_SCALE	3754.8	// double
	#elif MPU6050_FS_SEL_VAL == 2
		#define MPU6050_GYRO_SCALE	1880.3	// double
	#elif MPU6050_FS_SEL_VAL == 3
		#define MPU6050_GYRO_SCALE	940.1	// double
	#endif
#else
	#if MPU6050_FS_SEL_VAL == 0
		#define MPU6050_GYRO_SCALE	131.0	// double
	#elif MPU6050_FS_SEL_VAL == 1
		#define MPU6050_GYRO_SCALE	65.5	// double
	#elif MPU6050_FS_SEL_VAL == 2
		#define MPU6050_GYRO_SCALE	32.8	// double
	#elif MPU6050_FS_SEL_VAL == 3
		#define MPU6050_GYRO_SCALE	16.4	// double
	#endif
#endif
																						 
#define MPU6050_ACCEL_AS_G	0			// 1 - ускорение в единицах g, 0 - в м/с^2

#if MPU6050_ACCEL_AS_G == 1
	#if MPU6050_AFS_SEL_VAL == 0
		#define MPU6050_ACCEL_SCALE	16384.0	// double
	#elif MPU6050_AFS_SEL_VAL == 1
		#define MPU6050_ACCEL_SCALE	8192.0	// double
	#elif MPU6050_AFS_SEL_VAL == 2
		#define MPU6050_ACCEL_SCALE	4096.0	// double
	#elif MPU6050_AFS_SEL_VAL == 3
		#define MPU6050_ACCEL_SCALE	2048.0	// double
	#endif
#else
	#if MPU6050_AFS_SEL_VAL == 0
		#define MPU6050_ACCEL_SCALE	1670.1	// double
	#elif MPU6050_AFS_SEL_VAL == 1
		#define MPU6050_ACCEL_SCALE	835.1	// double
	#elif MPU6050_AFS_SEL_VAL == 2
		#define MPU6050_ACCEL_SCALE	417.5	// double
	#elif MPU6050_AFS_SEL_VAL == 3
		#define MPU6050_ACCEL_SCALE	208.8	// double
	#endif
#endif
/**************************************************************************/


#endif /* MPU6050_H_ */