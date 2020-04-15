/*
 * hmc5883l.c
 *
 * Created: 27.02.2020 16:11:12
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "hmc5883l.h"
#include "../interfaces/i2c.h"
#include "../rtos.h"
#include "md3.h"

// Сырые показания:
static int16_t	hmc5883l_mag_x = 0,
				hmc5883l_mag_y = 0,
				hmc5883l_mag_z = 0;
				
uint8_t __CRA = 0xFF, __CRB = 0xFF, __MODER = 0xFF, __SREG = 0xFF, __ID_A = 0xFF, __ID_B = 0xFF, __ID_C = 0xFF;

/* Область переменных из модуля i2c.c */
extern uint8_t i2c_write_buffer[I2C_MAX_WRITE_BYTES_COUNT];
extern uint8_t i2c_read_buffer[I2C_MAX_READ_BYTES_COUNT];
extern uint8_t i2c_status;
/**************************************/

void hmc5883l_init(void)
{
	uint8_t bytes_count	= 3;	// передадим 3 значения, а именно:
	uint8_t cra		=	(HMC5883L_AVERAGE_SMPL << HMC5883L_MA0) | 
						(HMC5883L_DO_RATE << HMC5883L_DO0) |
						(0 << HMC5883L_MS1) | (0 << HMC5883L_MS0),		// positive self-test
			crb		=	 HMC5883L_GAIN << HMC5883L_GN0,	
			mode	=	(0 << HMC5883L_HS) | (HMC5883L_MODE << HMC5883L_MD0);
//	uint8_t cra = 0x00, crb = 0x00, mode = 0x00;
			
	if (i2c_status & (1 << I2C_STATUS_BUSY))	// здесь смотрим на флаг, т.к. будем писать прямо в буфер
	{
		rtos_set_task (hmc5883l_init, 50,  RTOS_RUN_ONCE);
	}
	else
	{
		// передаём в порядке следования регистров, см. register map
		i2c_write_buffer[0] = cra;
		i2c_write_buffer[1] = crb;
		i2c_write_buffer[2] = mode;
		i2c_write_from_buffer (HMC5883L_ADDR, HMC5883L_CONF_REG_A, bytes_count, hmc5883l_init_exit);
	}
	
	return;
}

void hmc5883l_init_exit(void)
{
	rtos_set_task(hmc5883l_read, HMC5883L_STARTUP_DELAY, HMC5883L_PERIOD);
	
	return;
}

void hmc5883l_read(void)
{
//	uint8_t res = i2c_read_bytes(HMC5883L_ADDR, HMC5883L_DATA_X_MSB, HMC5883L_DATA_BYTES_COUNT, hmc5883l_read_exit);
	uint8_t res = i2c_read_bytes(HMC5883L_ADDR, HMC5883L_CONF_REG_A, 13, hmc5883l_read_exit);
	if(res)
	{
		rtos_set_task(hmc5883l_read, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

void hmc5883l_read_exit(void)
{
// 	hmc5883l_mag_x	= (i2c_read_buffer[0]  << 8)| i2c_read_buffer[1];
// 	hmc5883l_mag_y	= (i2c_read_buffer[2]  << 8)| i2c_read_buffer[3];
// 	hmc5883l_mag_z	= (i2c_read_buffer[4]  << 8)| i2c_read_buffer[5];

	__CRA = i2c_read_buffer[0];
	__CRB = i2c_read_buffer[1];
	__MODER = i2c_read_buffer[2];
	hmc5883l_mag_x	= (i2c_read_buffer[3]  << 8)| i2c_read_buffer[4];
	hmc5883l_mag_z	= (i2c_read_buffer[5]  << 8)| i2c_read_buffer[6];
	hmc5883l_mag_y	= (i2c_read_buffer[7]  << 8)| i2c_read_buffer[8];
	__SREG = i2c_read_buffer[9];
	__ID_A = i2c_read_buffer[10];
	__ID_B = i2c_read_buffer[11];
	__ID_C = i2c_read_buffer[12];
	
	return;
}

void hmc5883l_get_mag(HMC5883L_MAG_DATA *mag_p)
{
	mag_p->mX = (float)hmc5883l_mag_x/HMC5883L_SCALE;
	mag_p->mY = (float)hmc5883l_mag_y/HMC5883L_SCALE;
	mag_p->mZ = (float)hmc5883l_mag_z/HMC5883L_SCALE;
	
	return;
}

void hmc5883l_get_raw(HMC5883L_RAW_DATA *raw_p)
{
	raw_p->mX_raw = hmc5883l_mag_x;
	raw_p->mY_raw = hmc5883l_mag_y;
	raw_p->mZ_raw = hmc5883l_mag_z;
	
	return;
}

void hmc5883l_get_debug(HMC5883L_DEBUG *hmc5883l_debug)
{
	hmc5883l_debug->cra = __CRA;
	hmc5883l_debug->crb = __CRB;
	hmc5883l_debug->moder = __MODER;
	hmc5883l_debug->status = __SREG;
	hmc5883l_debug->ida = __ID_A;
	hmc5883l_debug->idb = __ID_B;
	hmc5883l_debug->idc = __ID_C;
	
	return;
}
