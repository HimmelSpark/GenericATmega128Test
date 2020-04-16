/*
 * mpu6050.c
 *
 * Created: 08.12.2018 11:53:38
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "mpu6050.h"
#include "../interfaces/i2c.h"
#include "../interfaces/uart.h"
#include "../rtos.h"
#include "md3.h"

// Сырые показания:
static int16_t mpu6050_T = 0xFFFF; // "сырая" температура
// вычисление истинной: Т = (float)mpu6050_T/340.0 + 36.53

static int16_t	mpu6050_accelX	= 0,
				mpu6050_accelY	= 0,
				mpu6050_accelZ	= 0,
				mpu6050_gyroX	= 0,	mpu6050_gyroX_offset = 0,
				mpu6050_gyroY	= 0,	mpu6050_gyroY_offset = 0,
				mpu6050_gyroZ	= 0,	mpu6050_gyroZ_offset = 0;
		
uint8_t __gyro_calib = 0;		// выставлены ли нули на гироскопах?

uint8_t __gyro_init_count = 0;	// это нужно для двухкратной инициализации MPU6050 (костыль)


void mpu6050_init (void)
{
	mpu6050_init_hw();
	mpu6050_init_i2c();
	
	return;
}

void mpu6050_init_hw(void)
{
	// Настройка адреса установкой адресного пина
	MPU6050_AD0_DDR |= (1 << MPU6050_AD0);	// выход
		
	#if MPU6050_ADDR_LSB == 0
		MPU6050_AD0_PORT &= ~(1 << MPU6050_AD0);// '0'
	#elif MPU6050_ADDR_LSB == 1
		MPU6050_AD0_PORT |= (1 << MPU6050_AD0);	// '1'
	#endif
	
	uart_puts("[ OK ] MPU6050 HW init\n");
	
	return;
}

void mpu6050_init_i2c(void)
{
	uint8_t mpu6050_init_settings[4] = {
		MPU6050_SMPLRT_DIV_VAL,							// smplrt_div
		MPU6050_DLPF_VAL << MPU6050_DLPF_OFFSET,		// config
		MPU6050_FS_SEL_VAL << MPU6050_FS_SEL_OFFSET,	// gyro_config
		MPU6050_AFS_SEL_VAL << MPU6050_AFS_SEL_OFFSET	// accel_config
	};

	int res = i2c_write(MPU6050_ADDR, MPU6050_SMPLRT_DIV, mpu6050_init_settings, 4, mpu6050_init_set_exit);
	if(res)
	{
		rtos_set_task(mpu6050_init_i2c, 5, RTOS_RUN_ONCE);
	}
	
	uart_puts("[ OK ] MPU6050 I2C init\n");
	
	return;
}

void mpu6050_init_set_exit (void)
{
	rtos_set_task (mpu6050_poweron, MPU6050_STARTUP_DELAY, RTOS_RUN_ONCE);
	
	return;
}

void mpu6050_poweron (void)
{
	int res = i2c_write_byte_to_reg(MPU6050_ADDR, MPU6050_PWR_MGMT1, MPU6050_DEFAULT_PWR, mpu6050_poweron_exit);
	if (res)
	{
		rtos_set_task (mpu6050_poweron, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	__gyro_init_count++;
	
	return;
}

void mpu6050_poweron_exit (void)
{
	// Это костыль
	if (__gyro_init_count == 1)
	{
		rtos_set_task(mpu6050_init_i2c, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	else if (__gyro_init_count == 2)
	{
		rtos_set_task (mpu6050_read, MPU6050_READ_STARTUP_DELAY, RTOS_RUN_ONCE);	// теперь готовы читать данные
		
		uart_puts("[ OK ] MPU6050 poweron&ready\n");
	}
	
	return;
}

void mpu6050_read (void)
{
	// Сразу запланируем следующее чтение через период:
	rtos_set_task(mpu6050_read, MPU6050_READ_PERIOD, RTOS_RUN_ONCE);
	
	int res = i2c_read(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, MPU6050_DATA_BYTES_COUNT, mpu6050_read_exit);
	if (res)
	{
		rtos_set_task (mpu6050_read, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

void mpu6050_read_exit(uint8_t *buf_rd)
{
	mpu6050_accelX	= (buf_rd[0]  << 8)| buf_rd[1];
	mpu6050_accelY	= (buf_rd[2]  << 8)| buf_rd[3];
	mpu6050_accelZ	= (buf_rd[4]  << 8)| buf_rd[5];
	mpu6050_T		= (buf_rd[6]  << 8)| buf_rd[7];
	mpu6050_gyroX	= (buf_rd[8]  << 8)| buf_rd[9];
	mpu6050_gyroY	= (buf_rd[10] << 8)| buf_rd[11];
	mpu6050_gyroZ	= (buf_rd[12] << 8)| buf_rd[13];

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
	
//	uart_puts("[ OK ] MPU6050 rd\n");
	
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
	gyro_data.gZ = ((double)mpu6050_gyroZ)/MPU6050_GYRO_SCALE;
	
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