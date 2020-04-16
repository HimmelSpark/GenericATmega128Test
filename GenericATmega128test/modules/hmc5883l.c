/*
 * hmc5883l.c
 *
 * Created: 27.02.2020 16:11:12
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "hmc5883l.h"
#include "../interfaces/i2c.h"
#include "../interfaces/uart.h"
#include "../rtos.h"
#include <math.h>
#include "md3.h"

// —ырые показани€:
static int16_t	hmc5883l_mag_x = 0,
				hmc5883l_mag_y = 0,
				hmc5883l_mag_z = 0;
				
uint8_t __CRA = 0xFF, __CRB = 0xFF, __MODER = 0xFF, __SREG = 0xFF, __ID_A = 0xFF, __ID_B = 0xFF, __ID_C = 0xFF;

void hmc5883l_init(void)
{
	uint8_t hmc5883l_init_settings[3]	=	{
		(HMC5883L_AVERAGE_SMPL << HMC5883L_MA0) |  (HMC5883L_DO_RATE << HMC5883L_DO0) | (0 << HMC5883L_MS1) | (0 << HMC5883L_MS0),	// CR_A
		HMC5883L_GAIN << HMC5883L_GN0,																								// CR_B
		(0 << HMC5883L_HS) | (HMC5883L_MODE << HMC5883L_MD0)																		// MODE
	};																		

	int res = i2c_write(HMC5883L_ADDR, HMC5883L_CONF_REG_A, hmc5883l_init_settings, 3, hmc5883l_init_exit);
	if(res)
	{
		rtos_set_task (hmc5883l_init, 5,  RTOS_RUN_ONCE);
	}
	
	return;
}

void hmc5883l_init_exit(void)
{
	rtos_set_task(hmc5883l_read, HMC5883L_STARTUP_DELAY, RTOS_RUN_ONCE);
	
	return;
}

void hmc5883l_read(void)
{
	// —разу запланируем следующее чтение через период:	
	rtos_set_task(hmc5883l_read, HMC5883L_PERIOD, RTOS_RUN_ONCE);
	
	uint8_t res = i2c_read(HMC5883L_ADDR, HMC5883L_CONF_REG_A, 13, hmc5883l_read_exit);
	if(res)
	{
		rtos_set_task(hmc5883l_read, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

void hmc5883l_read_exit(uint8_t *buf_rd)
{
	__CRA = buf_rd[0];
	__CRB = buf_rd[1];
	__MODER = buf_rd[2];
	hmc5883l_mag_x	= (buf_rd[3]  << 8)| buf_rd[4];
	hmc5883l_mag_z	= (buf_rd[5]  << 8)| buf_rd[6];
	hmc5883l_mag_y	= (buf_rd[7]  << 8)| buf_rd[8];
	__SREG = buf_rd[9];
	__ID_A = buf_rd[10];
	__ID_B = buf_rd[11];
	__ID_C = buf_rd[12];
	
//	uart_puts("[ OK ] HMC5883L rd\n");
	
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

uint16_t hmc5883l_get_mag_hdg(void)
{
	HMC5883L_RAW_DATA hmc5883l_raw;
	hmc5883l_get_raw(&hmc5883l_raw);
	
	float mag_hdg = atan2f((float)hmc5883l_raw.mY_raw, (float)hmc5883l_raw.mX_raw);
	if(mag_hdg < 0)
	{
		mag_hdg += 2*M_PI;
	}
	mag_hdg *= 180/M_PI;
	
	mag_hdg = roundf(mag_hdg);
	if(mag_hdg == 360.0)
	{
		mag_hdg = 0.0;
	}
	
	return (uint16_t)mag_hdg;
}