/*
 * nav.h
 *
 * Created: 05.12.2019 13:26:44
 *  Author: Vsevolod
 */ 


#ifndef NAV_H_
#define NAV_H_

typedef struct{
	float lat;
	float lon;
	} NAV_WP;

// Тип структуры, используемый автопилотом
typedef struct{
	uint8_t		id;		// идентификатор маршрута
	
	uint8_t		wp_n;	// точек в маршруте
	uint8_t		wp_tgt;	// текущая точка
	
	NAV_WP		*wps;	// массив точек
	
	uint16_t	brg;	// пеленг на следующую точку
	int16_t		dpsi;	// рассогласование
	float		dst;	// расстояние до точки
	} NAV_ROUTE;

// Тип структуры для использования пользователем
typedef struct{
	uint8_t		rte_id;	// идентификатор маршрута
	uint8_t		wp_n;		// точек в маршруте
	uint8_t		wp_tgt;	// текущая точка
	uint16_t	brg;	// пеленг на следующую точку
	int16_t		dpsi;	// рассогласование
	float		dst;	// расстояние до точки
	} NAV_ROUTE_PROGRESS;

// Инициализация, в т.ч. маршрута
void nav_init(void);

// Включить/выключить автоведение
void nav_autopilot_engage(void);
void nav_autopilot_diseng(void);

// Автоведение по маршруту
void nav_autopilot(void);

// Переключение на следующую точку
uint8_t nav_route_next_wp(void);

// ToDo: получение информации об автоведении
void nav_route_get_progress(NAV_ROUTE_PROGRESS *progress);

// Пеленг от точки 0 на точку 1; градусы
uint16_t nav_brg_p2p(float lat0, float lon0, float lat1, float lon1);

// Расстояние от точки 0 до точки 1; метры
float nav_dst_p2p(float lat0, float lon0, float lat1, float lon1);

// Задать целевую точку
void nav_set_tgt_wp(float lat, float lon);

// Получить целевую точку
void nav_get_tgt_wp(float *lat, float *lon);

// Пеленг на целевую точку (на основе nav_brg_p2p)
uint16_t nav_brg2tgt(float lat, float lon);

// Расстояние до целевой точки (на основе nav_dst_p2p)
float nav_dst2tgt(float lat, float lon);

// Альтернативно: рассогласование курса относительно пеленга на целевую точку
// (на основе векторного и скалярного произведений и тангенса половинного угла)
uint16_t nav_dpsi2tgt(float lat, float lon, uint16_t crs);


// Коэффициенты для вычисления дальности в окрестности базовой точки
// (необходимо вычислять для каждого района применения)
#define K_l			3597865.4	// dl [рад] * K_l = dz
#define K_phi		6372518.7	// dphi [рад] * K_phi = dx
#define K_l__K_phi	0.5645908	// K_l/K_phi

// Коэффициенты автоведения
#define NAV_AUTO_PERIOD			1000	// мс; периодичность вызова алгоритма автоведения

#define NAV_AUTO_V_CRUIZE		0.7		// м/с; скорость следования по маршруту
#define NAV_AUTO_V_APPROACH		0.3		// м/с; скорость подхода к точке

#define NAV_AUTO_D_APPROACH		15.0	// м; окрестность подхода к точке
#define NAV_AUTO_D_CAPTURE		6.0		// м; окрестность захвата точки


#define NAV_AUTO_dPSI_MIN		-45
#define NAV_AUTO_dPSI_MAX		45
#define NAV_AUTO_dPSI_LIM		45	// °; ограничение рассогласования по курсу
#define NAV_OMEGA_OBJ_MAX		20	// °/с; соответствующая заданная угловая скорость
#define NAV_AUTO_K_dPSI			((float)NAV_OMEGA_OBJ_MAX/(float)NAV_AUTO_dPSI_LIM)	// коэффициент угловой скорости по рассогласованию
																		// Omega_tgt = NAV_AUTO_K_dPSI * dpsi


#endif /* NAV_H_ */