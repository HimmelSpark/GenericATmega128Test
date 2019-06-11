/*
 * mpu6050.c
 *
 * Created: 08.12.2018 11:53:38
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "mpu6050.h"
#include "../interfaces/i2c.h"
#include "../rtos.h"
#include "md3.h"

uint8_t mpu6050_addr = (MPU6050_ADDR_MSBs << 1) | MPU6050_ADDR_LSB;

volatile int16_t mpu6050_T = 0xFFFF; // "сырая" температура
// вычисление истинной: Т = (float)mpu6050_T/340.0 + 36.53
// Сырые показания:
static int16_t	mpu6050_accelX	= 0,
				mpu6050_accelY	= 0,
				mpu6050_accelZ	= 0,
				mpu6050_gyroX	= 0,	mpu6050_gyroX_offset = 0,
				mpu6050_gyroY	= 0,	mpu6050_gyroY_offset = 0,
				mpu6050_gyroZ	= 0,	mpu6050_gyroZ_offset = 0;
		
// Сырые фильтрованные показания (ToDo: добавить остальные, + фильтры):
static double	mpu6050_gyroZ_f = 0.0;
					
static uint8_t __filter_reset = 1;	// флаг сброса фильтра угловой скорости Z

uint8_t __gyro_calib = 0;	// выставлены ли нули на гироскопах?

/* Область переменных из модуля i2c.c */
extern uint8_t i2c_write_buffer[I2C_MAX_WRITE_BYTES_COUNT];
extern uint8_t i2c_read_buffer[I2C_MAX_READ_BYTES_COUNT];
extern uint8_t i2c_status;
/**************************************/

inline void mpu6050_init (void)
{
	MPU6050_AD0_DDR |= (1 << MPU6050_AD0);	// выход
	
	#if MPU6050_ADDR_LSB == 0
	MPU6050_AD0_PORT &= ~(1 << MPU6050_AD0);// '0'
	#elif MPU6050_ADDR_LSB == 1
	MPU6050_AD0_PORT |= (1 << MPU6050_AD0);	// '1'
	#endif
	
	mpu6050_init_set ();
}

void mpu6050_init_set (void)
{
	uint8_t bytes_count		= 4;	// передадим 4 значения, а именно:
	uint8_t smplrt_div		= MPU6050_SMPLRT_DIV_VAL,
			config			= MPU6050_DLPF_VAL << MPU6050_DLPF_OFFSET,
			gyro_config		= MPU6050_FS_SEL_VAL << MPU6050_FS_SEL_OFFSET,
			accel_config	= MPU6050_AFS_SEL_VAL << MPU6050_AFS_SEL_OFFSET;
	
	if (i2c_status & (1 << I2C_STATUS_BUSY))	// здесь смотрим на флаг, т.к. будем писать прямо в буфер
	{
		rtos_set_task (mpu6050_init_set, 50,  RTOS_RUN_ONCE);
		return;
	}
	
	// передаём в порядке следования регистров, см. register map
	i2c_write_buffer[0] = smplrt_div;
	i2c_write_buffer[1] = config;
	i2c_write_buffer[2] = gyro_config;
	i2c_write_buffer[3] = accel_config;
	i2c_write_from_buffer (mpu6050_addr, MPU6050_SMPLRT_DIV, bytes_count, init_set_exit);
	
	return;
}

void init_set_exit (void)
{
	rtos_set_task (mpu6050_poweron, MPU6050_STARTUP_DELAY, RTOS_RUN_ONCE);
	return;
}

void mpu6050_poweron (void)
{
	uint8_t pwr_mgmt1 = 0x00;	// выходим из sleep + не используем cycle, temp_dis и clksel
	uint8_t res = i2c_write_byte2reg (mpu6050_addr, MPU6050_PWR_MGMT1, pwr_mgmt1, poweron_exit);
	if (res)
	{
		rtos_set_task (mpu6050_poweron, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

void poweron_exit (void)
{
	rtos_set_task (mpu6050_read, MPU6050_READ_STARTUP_DELAY, MPU6050_READ_PERIOD);	// теперь готовы читать данные
	rtos_set_task (mpu6050_gyro_filter, \
					MPU6050_READ_STARTUP_DELAY + MPU6050_READ_PERIOD, \
					MPU6050_READ_PERIOD);	// запускаем фильтр
	
	return;
}

void mpu6050_read (void)
{
	uint8_t res = i2c_read_bytes (mpu6050_addr, MPU6050_ACCEL_XOUT_H, MPU6050_DATA_BLOCK, read_exit);
	if (res)
	{
		rtos_set_task (mpu6050_read, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

void read_exit (void)
{
	mpu6050_accelX	= (i2c_read_buffer[0]  << 8)| i2c_read_buffer[1];
	mpu6050_accelY	= (i2c_read_buffer[2]  << 8)| i2c_read_buffer[3];
	mpu6050_accelZ	= (i2c_read_buffer[4]  << 8)| i2c_read_buffer[5];
	mpu6050_T		= (i2c_read_buffer[6]  << 8)| i2c_read_buffer[7];
	mpu6050_gyroX	= (i2c_read_buffer[8]  << 8)| i2c_read_buffer[9];
	mpu6050_gyroY	= (i2c_read_buffer[10] << 8)| i2c_read_buffer[11];
	mpu6050_gyroZ	= (i2c_read_buffer[12] << 8)| i2c_read_buffer[13];

	if (!__gyro_calib)
	{
		mpu6050_gyroX_offset = mpu6050_gyroX;
		mpu6050_gyroY_offset = mpu6050_gyroY;
		mpu6050_gyroZ_offset = mpu6050_gyroZ;
		
		__gyro_calib = 1;
	}
	
	mpu6050_gyroX -= mpu6050_gyroX_offset;
	mpu6050_gyroY -= mpu6050_gyroY_offset;
	mpu6050_gyroZ -= mpu6050_gyroZ_offset;
	
	return;
}

MPU6050_ACCEL_DATA mpu6050_get_accel (void)
{
	MPU6050_ACCEL_DATA accel_data;
	accel_data.aX = ((double)mpu6050_accelX)/MPU6050_ACCEL_SCALE;
	accel_data.aY = ((double)mpu6050_accelY)/MPU6050_ACCEL_SCALE;
	accel_data.aZ = ((double)mpu6050_accelZ)/MPU6050_ACCEL_SCALE;
	
	return accel_data;
}

MPU6050_GYRO_DATA mpu6050_get_gyro (void)
{
	MPU6050_GYRO_DATA gyro_data;
	gyro_data.gX = ((double)mpu6050_gyroX)/MPU6050_GYRO_SCALE;
	gyro_data.gY = ((double)mpu6050_gyroY)/MPU6050_GYRO_SCALE;
	/*gyro_data.gZ = ((double)mpu6050_gyroZ)/MPU6050_GYRO_SCALE;*/
	gyro_data.gZ = ((double)mpu6050_gyroZ_f)/MPU6050_GYRO_SCALE;
	
	return gyro_data;
}

float mpu6050_get_T (void)
{
	if (mpu6050_T != 0xFFFF)
	{
		return (((float)mpu6050_T)/340.0 + 36.53);
	}
	else
	{
		return 0.0;
	}
}

void mpu6050_gyro_filter (void)
{	// Пока что фильтруем только Z-составляющую
	
	static double I1;//, I2;
	double eps;
	
	if (__filter_reset)
	{
		I1 = mpu6050_gyroZ;
//		I2 = mpu6050_gyroZ;
		__filter_reset = 0;
	}
	
	eps = ((double)mpu6050_gyroZ - I1) * FILT_CONST_Ki;
	I1 += eps * FILT_CONST_dT;
	
// 	eps = (I1 - I2) * FILT_CONST_Ki;
// 	I2 += eps * FILT_CONST_dT;
	
	mpu6050_gyroZ_f = I1;
	
	return;
}
