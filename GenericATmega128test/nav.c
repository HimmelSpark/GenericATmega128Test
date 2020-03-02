/*
 * nav.c
 *
 * Created: 05.12.2019 13:26:26
 *  Author: Vsevolod
 */ 

#include "general.h"
#include "nav.h"
#include <math.h>
#include <stdlib.h>
#include "interfaces/uart.h"
#include "modules/gps.h"
#include "motion_control.h"
#include "rtos.h"
#include <string.h>

static volatile float __tgt_lat = 55.80477, __tgt_lon = 37.50293;		// ������� ����� �� ���������

static volatile NAV_ROUTE MyRoute;

/*	��������, ��� ��������� ����. ����� ��������� �����������/���������� �������������� �� �����:
	
	37.49894,55.79839 - 225�, 360 ������
	37.49857,55.80319 - 315�, 385 ������
	37.50704,55.80299 -  45�, 365 ������
	37.50742,55.79827 - 135�, 393 �����
	
	37.50309,55.80063 -  101�, 10 ������
	37.50308,55.80057 - 135�, 14 ������
	37.51061,55.79386 - 148�, 898 ������
	37.58027,55.77825  - 117�, 5455 ������
	
	37.50293,55.80477 - �����, 458 ������
	37.51292,55.80065 - ������, 627 ������
	37.50100,55.80065 - �����, 121 ����
	
*/

inline void nav_init(void)
{
	// ��������������, ��� ����� ����� ������� �����, ��������, �� EEPROM (?).
	// ���� ��� ��������� ��� �������� sample_route:
/* ROUTE_01 */
// 	#define SAMPLE_ROUTE_N	5
//	#define SAMPLE_ROUTE_ID	1
// 	float sample_route[SAMPLE_ROUTE_N][2] = {
// 		{55.80129,37.50190},
// 		{55.80225,37.50238},
// 		{55.80264,37.49973},
// 		{55.80247,37.49832},
// 		{55.80400,37.49699}
// 	};
/***********/

/* ROUTE_02 */
// 	#define SAMPLE_ROUTE_N	8
// 	#define SAMPLE_ROUTE_ID	2
// 	float sample_route[SAMPLE_ROUTE_N][2] = {
// 		{55.80225,37.50238},
// 		{55.80529,37.50400},
// 		{55.80324,37.50656},
// 		{55.80162,37.50686},
// 		{55.79380,37.49363},
// 		{55.79640,37.48799},
// 		{55.79769,37.49004},
// 		{55.79644,37.49387}
// 	};
/***********/

/* ROUTE_03 */
// 	#define SAMPLE_ROUTE_N	5
//	#define SAMPLE_ROUTE_ID	3
// 	float sample_route[SAMPLE_ROUTE_N][2] = {
// 		{55.79568,37.49709},
// 		{55.80571,37.51079},
// 		{55.80063,37.53119},
// 		{55.78551,37.56576},
// 		{55.77825,37.58031}
// 	};
/***********/

/* ROUTE_04 */
// 	#define SAMPLE_ROUTE_N	8
// 	#define SAMPLE_ROUTE_ID	4
// 	float sample_route[SAMPLE_ROUTE_N][2] = {
// 		{55.80351,37.51850},
// 		{55.79995,37.51562},
// 		{55.79606,37.51247},
// 		{55.79349,37.50928},
// 		{55.78643,37.51272},
// 		{55.77817,37.51709},
// 		{55.77726,37.51765},
// 		{55.77723,37.51955}
// 	};
/***********/

/* ROUTE_05 */
	#define SAMPLE_ROUTE_N	3
	#define SAMPLE_ROUTE_ID	5
	float sample_route[SAMPLE_ROUTE_N][2] = {
		{55.80172,37.50211},
		{55.80226,37.50239},
		{55.80303,37.50281}
	};
/***********/

/* ROUTE_06 */
// 	#define SAMPLE_ROUTE_N	4
// 	#define SAMPLE_ROUTE_ID	6
// 	float sample_route[SAMPLE_ROUTE_N][2] = {
// 		{55.79788,37.51177},
// 		{55.79846,37.51178},
// 		{55.79947,37.51178},
// 		{55.79953,37.51096}
// 	};
/***********/

/* ROUTE_07 */
// 	#define SAMPLE_ROUTE_N	4
// 	#define SAMPLE_ROUTE_ID	7
// 	float sample_route[SAMPLE_ROUTE_N][2] = {
// 		{55.79955,37.51087},
// 		{55.79948,37.51178},
// 		{55.79868,37.51179},
// 		{55.79787,37.51178}
// 	};
/***********/

	NAV_WP wp;
	
	MyRoute.id = SAMPLE_ROUTE_ID;
	MyRoute.wp_n = SAMPLE_ROUTE_N;
	MyRoute.wps = (NAV_WP *) malloc(sizeof(NAV_WP) * MyRoute.wp_n);	// �������� ������ ��� ����� ��������
	
	for(register uint8_t i = 0; i < MyRoute.wp_n; i++)
	{
		wp.lat = sample_route[i][0]; wp.lon = sample_route[i][1];	// ��������� �����
		MyRoute.wps[i] = wp;										// ������ � ������
	}
	// �������� �������:
	MyRoute.wp_tgt = 0;
	nav_set_tgt_wp(MyRoute.wps[0].lat, MyRoute.wps[0].lon);
	
	uart_puts("[ OK ] NAV init completed\n");
}

inline void nav_autopilot_engage(void)
{
	// ToDo: ������������� � �.�.
	// ......................
	// ��������, ����� ��� ������ ��������� ������, � ����� ������ rtos_set_task
	
	rtos_set_task(nav_autopilot, RTOS_RUN_ASAP, NAV_AUTO_PERIOD);
	uart_puts("[ OK ] AP engaged\n");
}

inline void nav_autopilot_diseng(void)
{
	mcontrol_set(0.0, 0.0);				// �������������� ���������������
	rtos_delete_task(nav_autopilot);
	uart_puts("[ OK ] AP disengaged\n");
}

void nav_autopilot(void)
{// ����������� ������������, ������ (?) ��
 // ������, ����������������� ���� ���������� ��������� �������������
 // ���������� ���� ������������ ��������. � ������ ������������� ������ GPS - ��� 1 �������
	
	GPS_POS gps_pos;
	GPS_MOTION gps_motion;
	
	gps_get_pos(&gps_pos);
	gps_get_motion(&gps_motion);
	
	// �������������� ������� - �������� � ������� ��������:
	float V_tgt, Omega_tgt;
	
	// �������� ����������� � ����� (������ ��������� ������� �� ��������)
	MyRoute.dst = nav_dst2tgt(gps_pos.lat, gps_pos.lon);
	
// 	if(MyRoute.dst > NAV_AUTO_D_APPROACH)		// �� ���� � �����
// 	{
// 		V_tgt = NAV_AUTO_V_CRUIZE;
// 	}
// 	else if(MyRoute.dst > NAV_AUTO_D_CAPTURE)	// ������ � �����
// 	{
// 		V_tgt = NAV_AUTO_V_APPROACH;
// 	}
	if(MyRoute.dst > NAV_AUTO_D_CAPTURE)		// �� ���� � �����
	{
		V_tgt = NAV_AUTO_V_CRUIZE;
	}
	else										// � ����������� �����; ������ ����� (����� ����������)
	{
		uint8_t is_last_wp = nav_route_next_wp();	// ������� ������������� �� ��������� �����
		
		if(!is_last_wp)		// ������� �� ����������
		{
//			V_tgt = NAV_AUTO_V_CRUIZE;				// ���� ������ � ����������� ���������
		}
		else				// ������� ����������
		{
			nav_autopilot_diseng();					// ��������� �����������
			
			return;									// ������ ������ ����� �� ������
		}
	}
	
	// ��������� ������� �� ������� ��������
	// ��� �������� ���������� ��������������� dpsi:
/* ������� 1: � �������������� ����� crs � ������� brg */
/*
	uint16_t crs = gps_motion.crs;	// ToDo: ������, ����� ���� ������ � ��� - ������������� ������ �������
	MyRoute.brg = nav_brg2tgt(gps_pos.lat, gps_pos.lon);
	MyRoute.dpsi = MyRoute.brg - crs;
	// ���������� � ��������� -180� - 180�
	if(MyRoute.dpsi > 180)
	{
		MyRoute.dpsi -= 360;
	}
	else if(MyRoute.dpsi < -180)
	{
		MyRoute.dpsi += 360;
	}
*/
/*-----------------------------------------------------*/

/* ������� 2: ����� ��������� � ��������� ������������ � ������� ����������� ���� */

	uint16_t crs = gps_motion.crs;	// ToDo: ������, ����� ���� ������ � ��� - ������������� ������ �������
	MyRoute.dpsi = nav_dpsi2tgt(gps_pos.lat, gps_pos.lon, crs);
	
	MyRoute.brg = nav_brg2tgt(gps_pos.lat, gps_pos.lon);	// �� ����, brg ����� ��� �� �����, �� ����� ��� ����������

/*--------------------------------------------------------------------------------*/
	
	// ����������� (���������)
	float dpsi_constr = MyRoute.dpsi;
	if(MyRoute.dpsi < NAV_AUTO_dPSI_MIN)
	{
		dpsi_constr = NAV_AUTO_dPSI_MIN;
	}
	else if(MyRoute.dpsi > NAV_AUTO_dPSI_MAX)
	{
		dpsi_constr = NAV_AUTO_dPSI_MAX;
	}
	// �������������� � ������� ������� �������� (N.B.: ������� � ���/�!)
	Omega_tgt = -NAV_AUTO_K_dPSI * dpsi_constr * M_PI/180.0;
	// �������� ���� ��� ���������� � ������������ ������� � ���������������
	
	// ������������ �������
	mcontrol_set(V_tgt, Omega_tgt);
	
	return;
}

uint8_t nav_route_next_wp(void)
{
	uint8_t res = 0;
	
	MyRoute.wp_tgt++;
	if(MyRoute.wp_tgt < MyRoute.wp_n)
	{
		uart_puts("[ OK ] Next WP\n");
	}
	else
	{
		MyRoute.wp_tgt = 0;										// ������� � ������
		res = 1;												// ���� ����� ��������
		uart_puts("[ ! ] RTE end, sw to start WP\n");
	}
	
	nav_set_tgt_wp(MyRoute.wps[MyRoute.wp_tgt].lat, MyRoute.wps[MyRoute.wp_tgt].lon);
	
	return res;
}

void nav_route_get_progress(NAV_ROUTE_PROGRESS *progress)
{
	progress->rte_id	= MyRoute.id;
	progress->wp_n		= MyRoute.wp_n;
	progress->wp_tgt	= MyRoute.wp_tgt;
	progress->brg		= MyRoute.brg;
	progress->dpsi		= MyRoute.dpsi;
	progress->dst		= MyRoute.dst;
	
	return;
}

uint16_t nav_brg_p2p(float lat0, float lon0, float lat1, float lon1)
{
	float dphi = lat1 - lat0;	// ����� � ������� �� ���������
	float dl = lon1 - lon0;		// � ����� - ����������� ������ ����������
//	float phi_m = 0.5*(lat0 + lat1) * M_PI/180.0;	// � ����� ���������!
	
//	float bearing = atan(dl/dphi * cos(phi_m));	// ������������ ������; ����� ������� ����������� ������ �����
	float bearing = atan(K_l__K_phi * dl/dphi);	// ������������ ������; �������� � �����������
	bearing = bearing*180.0/M_PI;				// � �������

	if((dl > 0.0) && (dphi > 0.0))
	{
		;	// ������ �� ������
	}
	else if(dphi < 0.0)
	{
		bearing += 180.0;
	}
	else if((dl < 0.0) && (dphi > 0.0))
	{
		bearing += 360.0;
	}
	// ������ ���� �� �����; ������ ����� ��������� ����������� ����� ����������
	else if((dl == 0.0) && (dphi > 0.0))
	{
		bearing = 0.0;
	}
	else if(dphi == 0.0)
	{
		if(dl > 0.0)
		{
			bearing = 90.0;
		}
		if(dl < 0.0)
		{
			bearing = 270.0;
		}
	}
	// dphi = dl = 0.0
	else
	{
		bearing = 0.0;
	}
	
	bearing = round(bearing);	// � ����� �������
	if (bearing == 360.0)		// ���� ����� ���������� (��������, ���� 359.8) ����� 360.0
	{
		bearing = 0.0;			// ���� ������������� ��� �� ������ �� �� ���
	}

	return (uint16_t)bearing;	
}

float nav_dst_p2p(float lat0, float lon0, float lat1, float lon1)
{
	float dphi = (lat1 - lat0)*M_PI/180.0;
	float dl = (lon1 - lon0)*M_PI/180.0;
	
	float l = sqrt(square(dl*K_l) + square(dphi*K_phi));
	
	return l;
}

inline void nav_set_tgt_wp(float lat, float lon)
{
	__tgt_lat = lat;
	__tgt_lon = lon;
}

void nav_get_tgt_wp(float *lat, float *lon)
{
	*lat = __tgt_lat;
	*lon = __tgt_lon;
	
	return;
}

inline uint16_t nav_brg2tgt(float lat, float lon)
{
	return nav_brg_p2p(lat, lon, __tgt_lat, __tgt_lon);
}

inline float nav_dst2tgt(float lat, float lon)
{
	return nav_dst_p2p(lat, lon, __tgt_lat, __tgt_lon);
}

uint16_t nav_dpsi2tgt(float lat, float lon, uint16_t crs)
{
	float	crs_rad = crs * M_PI/180.0;
	float	sin_alpha = sin(crs_rad),
			cos_alpha = cos(crs_rad),
			dx = K_phi*(__tgt_lat - lat) * M_PI/180.0,
			dz = K_l*(__tgt_lon - lon) * M_PI/180.0,
			Rg = nav_dst2tgt(lat, lon);

	float	sin_dpsi = (cos_alpha * dz - sin_alpha * dx)/Rg,
			cos_dpsi = (cos_alpha * dx + sin_alpha * dz)/Rg;
			
// ���� ����� Rg == 0.0, ������ ��������� �� ���������, ����� dpsi = 0.0
			
	float dpsi = 2 * atan(sin_dpsi / (1 + cos_dpsi)) * 180.0/M_PI;
	
	return (uint16_t)dpsi;
}
