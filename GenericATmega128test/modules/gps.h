/*
 * gps.h
 *
 * Created: 04.11.2019 16:49:22
 *  Author: Vsevolod
 */ 


#ifndef GPS_H_
#define GPS_H_

/*

GPS висит на UART1

*/

typedef struct {
	float lat;
	float lon;
	float alt;
	} GPS_POS;
	
typedef struct {
	float		vel;
//	float		crs;
	uint16_t	crs;
	} GPS_MOTION;

typedef struct {
	char	status;			// A = valid, V = invalid
	char	mode;			// N = not valid, A = autonomous, D = differential, E = estimated
	uint8_t sats_num;		// GPS sats in use
	float	hdop;			// Horizontal dilution of position
	} GPS_INF;
	
typedef struct {
	uint8_t hh;
	uint8_t mm;
	uint8_t ss;
	} GPS_TIME;
	
typedef struct {
	uint8_t dd;
	uint8_t mm;
	uint8_t yy;
	} GPS_DATE;

typedef struct {
	
// 	uint8_t utc_t_hh;
// 	uint8_t utc_t_mm;
// 	uint8_t utc_t_ss;
// 	
// 	uint8_t lat_deg;
// 	uint8_t lat_min;
// 	uint8_t lat_min_dec;	// десятичная часть; возьмём только старший разряд
// 	char	lat_NS;			// 'N', 'S'
// 	
// 	uint8_t lon_deg;
// 	uint8_t lon_min;
// 	uint8_t lon_min_dec;	// десятичная часть; возьмём только старший разряд
// 	char	lon_EW;			// 'E', 'W'
// 	
// 	uint8_t	q;				// GPS quality indicator

	uint8_t		sats_num;		// GPS sats in use
	
	uint8_t		hdop;			// Horizontal dilution of position
	uint8_t		hdop_dec;		// десятичная часть
	
	uint16_t	alt;
	uint8_t		alt_dec;
	
//	uint8_t checksum;
	
	} NMEA_GGA_MSG;

typedef struct {
	
	uint8_t		utc_t_hh;
	uint8_t		utc_t_mm;
	uint8_t		utc_t_ss;
	
	char		status;			// A = valid, V = invalid
	
	uint8_t		lat_deg;
	uint8_t		lat_min;
	uint16_t	lat_min_dec;	// десятичная часть
	char		lat_NS;			// 'N', 'S'
	
	uint8_t		lon_deg;
	uint8_t		lon_min;
	uint16_t	lon_min_dec;	// десятичная часть
	char		lon_EW;			// 'E', 'W'
	
	uint8_t		vel;
	uint8_t		vel_dec;
	
	uint16_t	crs;
	uint8_t		crs_dec;
	
	uint8_t		utc_d_dd;
	uint8_t		utc_d_mm;
	uint8_t		utc_d_yy;
	
	char		mode;			// N = not valid, A = autonomous, D = differential, E = estimated
	
//	uint8_t checksum;
	
	} NMEA_RMC_MSG;

/* Служебные функции */
void __gps_rx_routine(void);
void __gps_nmea_parser(void);
void __dbg_print_msg(char *msg);
/*********************/

/* Функции для пользователя */
void gps_init(void);
// Передача данных через указатели - экономия стека:
void gps_get_pos(GPS_POS *pos_p);
void gps_get_motion(GPS_MOTION *motion_p);
void gps_get_info(GPS_INF *info_p);
void gps_get_time(GPS_TIME *time_p);
void gps_get_date(GPS_DATE *date_p);
/****************************/


#define GPS_BAUDRATE		9600UL
#define GPS_UBRR			(F_CPU/(16*GPS_BAUDRATE) - 1)

#define NMEA_HEADER_LEN		5
#define NMEA_MSG_MAX_LEN	82
#define GPS_RX_BUF_SIZE		(NMEA_MSG_MAX_LEN + 10)


#endif /* GPS_H_ */