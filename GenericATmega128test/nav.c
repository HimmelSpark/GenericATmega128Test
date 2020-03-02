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

static volatile float __tgt_lat = 55.80477, __tgt_lon = 37.50293;		// целевая точка по умолчанию

static volatile NAV_ROUTE MyRoute;

/*	Допустим, что находимся дома. Тогда различные направления/расстояния обеспечиваются на точки:
	
	37.49894,55.79839 - 225°, 360 метров
	37.49857,55.80319 - 315°, 385 метров
	37.50704,55.80299 -  45°, 365 метров
	37.50742,55.79827 - 135°, 393 метра
	
	37.50309,55.80063 -  101°, 10 метров
	37.50308,55.80057 - 135°, 14 метров
	37.51061,55.79386 - 148°, 898 метров
	37.58027,55.77825  - 117°, 5455 метров
	
	37.50293,55.80477 - север, 458 метров
	37.51292,55.80065 - восток, 627 метров
	37.50100,55.80065 - запад, 121 метр
	
*/

inline void nav_init(void)
{
	// Предполагается, что позже будем грузить точки, например, из EEPROM (?).
	// Пока что имитируем это массивом sample_route:
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
	MyRoute.wps = (NAV_WP *) malloc(sizeof(NAV_WP) * MyRoute.wp_n);	// выделяем память под точки маршрута
	
	for(register uint8_t i = 0; i < MyRoute.wp_n; i++)
	{
		wp.lat = sample_route[i][0]; wp.lon = sample_route[i][1];	// формируем точку
		MyRoute.wps[i] = wp;										// пихаем в массив
	}
	// Начинаем маршрут:
	MyRoute.wp_tgt = 0;
	nav_set_tgt_wp(MyRoute.wps[0].lat, MyRoute.wps[0].lon);
	
	uart_puts("[ OK ] NAV init completed\n");
}

inline void nav_autopilot_engage(void)
{
	// ToDo: инициализация и т.д.
	// ......................
	// Подумать, может это должен автопилот делать, а здесь только rtos_set_task
	
	rtos_set_task(nav_autopilot, RTOS_RUN_ASAP, NAV_AUTO_PERIOD);
	uart_puts("[ OK ] AP engaged\n");
}

inline void nav_autopilot_diseng(void)
{
	mcontrol_set(0.0, 0.0);				// гарантированно останавливаемся
	rtos_delete_task(nav_autopilot);
	uart_puts("[ OK ] AP disengaged\n");
}

void nav_autopilot(void)
{// Выполняется периодически, каждые (?) мс
 // Вообще, руководствоваться надо наименьшим значением периодичности
 // обновления всех используемых датчиков. В случае использования только GPS - это 1 секунда
	
	GPS_POS gps_pos;
	GPS_MOTION gps_motion;
	
	gps_get_pos(&gps_pos);
	gps_get_motion(&gps_motion);
	
	// Вырабатываемые уставки - линейная и угловая скорости:
	float V_tgt, Omega_tgt;
	
	// Проверка прохождения к точке (заодно формируем уставки по скорости)
	MyRoute.dst = nav_dst2tgt(gps_pos.lat, gps_pos.lon);
	
// 	if(MyRoute.dst > NAV_AUTO_D_APPROACH)		// На пути к точке
// 	{
// 		V_tgt = NAV_AUTO_V_CRUIZE;
// 	}
// 	else if(MyRoute.dst > NAV_AUTO_D_CAPTURE)	// Подход к точке
// 	{
// 		V_tgt = NAV_AUTO_V_APPROACH;
// 	}
	if(MyRoute.dst > NAV_AUTO_D_CAPTURE)		// На пути к точке
	{
		V_tgt = NAV_AUTO_V_CRUIZE;
	}
	else										// В окрестности точки; захват точки (точка достигнута)
	{
		uint8_t is_last_wp = nav_route_next_wp();	// пробуем переключиться на следующую точку
		
		if(!is_last_wp)		// маршрут не закончился
		{
//			V_tgt = NAV_AUTO_V_CRUIZE;				// едем дальше с крейсерской скоростью
		}
		else				// маршрут закончился
		{
			nav_autopilot_diseng();					// отключаем автоведение
			
			return;									// больше ничего здесь не делаем
		}
	}
	
	// Выработка уставки по угловой скорости
	// ДВА ВАРИАНТА вычисления рассогласования dpsi:
/* ВАРИАНТ 1: с использованием курса crs и пеленга brg */
/*
	uint16_t crs = gps_motion.crs;	// ToDo: вообще, здесь надо учесть и ДУС - напрашивается фильтр Калмана
	MyRoute.brg = nav_brg2tgt(gps_pos.lat, gps_pos.lon);
	MyRoute.dpsi = MyRoute.brg - crs;
	// Приведение к диапазону -180° - 180°
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

/* ВАРИАНТ 2: через векторное и скалярное произведения и тангенс половинного угла */

	uint16_t crs = gps_motion.crs;	// ToDo: вообще, здесь надо учесть и ДУС - напрашивается фильтр Калмана
	MyRoute.dpsi = nav_dpsi2tgt(gps_pos.lat, gps_pos.lon, crs);
	
	MyRoute.brg = nav_brg2tgt(gps_pos.lat, gps_pos.lon);	// по сути, brg здесь уже не нужен, он чисто для информации

/*--------------------------------------------------------------------------------*/
	
	// Ограничение (насыщение)
	float dpsi_constr = MyRoute.dpsi;
	if(MyRoute.dpsi < NAV_AUTO_dPSI_MIN)
	{
		dpsi_constr = NAV_AUTO_dPSI_MIN;
	}
	else if(MyRoute.dpsi > NAV_AUTO_dPSI_MAX)
	{
		dpsi_constr = NAV_AUTO_dPSI_MAX;
	}
	// Преобразование в уставку угловой скорости (N.B.: уставка в рад/с!)
	Omega_tgt = -NAV_AUTO_K_dPSI * dpsi_constr * M_PI/180.0;
	// Обратный знак для приведения в соответствие уставки и рассогласования
	
	// Отрабатываем уставки
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
		MyRoute.wp_tgt = 0;										// прыгаем в начало
		res = 1;												// флаг конца маршрута
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
	float dphi = lat1 - lat0;	// здесь в радианы не переводим
	float dl = lon1 - lon0;		// и здесь - размерности дальше сократятся
//	float phi_m = 0.5*(lat0 + lat1) * M_PI/180.0;	// а здесь переводим!
	
//	float bearing = atan(dl/dphi * cos(phi_m));	// приближенный способ; здесь принята сферическая модель Земли
	float bearing = atan(K_l__K_phi * dl/dphi);	// приближенный способ; привязка к окрестности
	bearing = bearing*180.0/M_PI;				// в градусы

	if((dl > 0.0) && (dphi > 0.0))
	{
		;	// ничего не делаем
	}
	else if(dphi < 0.0)
	{
		bearing += 180.0;
	}
	else if((dl < 0.0) && (dphi > 0.0))
	{
		bearing += 360.0;
	}
	// Дальше вряд ли пойдёт; однако очень небольшая вероятность этого существует
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
	
	bearing = round(bearing);	// в целые градусы
	if (bearing == 360.0)		// если после округления (например, было 359.8) стало 360.0
	{
		bearing = 0.0;			// хотя принципиально это не влияет ни на что
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
			
// Если вдруг Rg == 0.0, ничего страшного не произойдёт, будет dpsi = 0.0
			
	float dpsi = 2 * atan(sin_dpsi / (1 + cos_dpsi)) * 180.0/M_PI;
	
	return (uint16_t)dpsi;
}
