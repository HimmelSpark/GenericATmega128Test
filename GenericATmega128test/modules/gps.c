/*
 * gps.c
 *
 * Created: 04.11.2019 16:49:11
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "gps.h"
#include "../rtos.h"
#include <string.h>
#include "../fifo.h"
#include "../interfaces/uart.h"
#include "md3.h"

FIFO_BUFFER_t	__gps_rx_buf;
uint8_t			__idx_read_up_to;	// парсер будет читать буфер до этого индекса,
// фиксируемого в момент получения символа конца строки, т.к. есть вероятность (мизерная),
// что до вызова парсера в буфер может добавиться хотя один или (что ещё менее вероятно)
// несколько символов

GPS_POS		__gps_position;
GPS_MOTION	__gps_motion;
GPS_TIME	__gps_time;
GPS_DATE	__gps_date;
GPS_INF		__gps_info;

inline void __gps_rx_routine(void)
{	// ISR
 	register char buff = UDR1;
	fifo_push(buff, &__gps_rx_buf);
	
	if(buff == '\n')
	{
		__idx_read_up_to = __gps_rx_buf.idxIn;							// фиксируем индекс конца строки
		rtos_set_task(__gps_nmea_parser, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
//	 	uart_putc(buff);
}

void __gps_nmea_parser(void)
{
	char nmea_msg[NMEA_MSG_MAX_LEN];
	register uint8_t nmea_msg_offset;	// если хотим парсить строку не с начала
	register uint8_t parse_res;

	NMEA_GGA_MSG gga;
	NMEA_RMC_MSG rmc;
	
	// Ищем начало сообщения:
	while(fifo_pop(&__gps_rx_buf) != '$')
	{
		if(__gps_rx_buf.idxOut == __idx_read_up_to)	// если всё перебрали, а $ так и не нашли,
		{
			uart_puts("[ERR] NMEA msg corrupt\n\n");
			return;	// выходим
			// Такая ситуация может возникнуть при первом запуске парсера, если
			// от GPS успели принять только последнюю часть строки
		}
	}
	// Нашли начало. Читаем строку сообщения:
	for(register uint8_t i = 0; (nmea_msg[i] = fifo_pop(&__gps_rx_buf)) != '\n'; i++)
		;
		
	// Полученную строку проверяем на соответствие заголовкам NMEA
	// и предпринимаем соответствующие действия:
	if(strncmp(nmea_msg, "GNGGA", NMEA_HEADER_LEN) == 0)
	{
//		uart_puts("[GGA]\n");
//		__dbg_print_msg(nmea_msg);

		// Проскакиваем всё до количества спутников (7 запятых)
		nmea_msg_offset = 0;
		for(register uint8_t i = 0; i < 7; nmea_msg_offset++)
		{
			if(*(nmea_msg + nmea_msg_offset) == '\r')
			{
				uart_puts("[ERR] Reached end of line\n\n");
//				__dbg_print_msg(nmea_msg);
				return;		// если наткнулись на конец строки (практически невозможное событие),
							// то искать больше здесь нечего
			}
			else if(*(nmea_msg + nmea_msg_offset) == ',')
			{
				i++;
			}
		}
		parse_res = sscanf(nmea_msg + nmea_msg_offset,
				"%2hhd,%hhd.%1hhd%*d,%3d.%1hhd",
				&gga.sats_num, &gga.hdop, &gga.hdop_dec,
				&gga.alt, &gga.alt_dec);
		
//		printf("[%d parsed]\n", parse_res);
	
		if(parse_res < 3)	// SATS и HDOP должны быть всегда; если это не так,
		{	
			uart_puts("[ERR] GGA msg corrupt\n");	// считаем, что сообщение испорчено
//			__dbg_print_msg(nmea_msg);
		}
		else
		{
			__gps_info.hdop = (float)gga.hdop + gga.hdop_dec/10.0;
			__gps_info.sats_num = gga.sats_num;
			
			// На всякий случай проверка корректности парсинга
			if(parse_res == 5)
			{
				__gps_position.alt = (float)gga.alt + gga.alt_dec/10.0;
			}
			else
			{
				uart_puts("[ERR] GGA Alt not parsed\n");
//				__dbg_print_msg(nmea_msg);
			}	
		}
		
//		printf("Sats %d  HDOP %.1f  Alt %.1f\n\n", __gps_info.sats_num, __gps_info.hdop, __gps_position.alt);
	}
	else if(strncmp(nmea_msg, "GNRMC", NMEA_HEADER_LEN) == 0)
	{
//		uart_puts("[RMC]\n");
//		__dbg_print_msg(nmea_msg);
		
		// Пробуем парсить всё до курса включительно. Остальное - потом, т.к. в случае
		// ошибки ssacnf оно может не запарситься
		parse_res = sscanf(nmea_msg,
				"GNRMC,%2hhd%2hhd%2hhd.%*d,%c,%2hhd%2hhd.%3d%*d,%c,%3hhd%2hhd.%3d%*d,%c,%hhd.%1hhd%*d,%d.%1hhd",
				&rmc.utc_t_hh, &rmc.utc_t_mm, &rmc.utc_t_ss, &rmc.status,
				&rmc.lat_deg, &rmc.lat_min, &rmc.lat_min_dec, &rmc.lat_NS,
				&rmc.lon_deg, &rmc.lon_min, &rmc.lon_min_dec, &rmc.lon_EW,
				&rmc.vel, &rmc.vel_dec,	&rmc.crs, &rmc.crs_dec);
		// Дальше возможны случаи:
		if(parse_res < 4)	// не пропарсилось время (и символ валидности)
		{
			__gps_info.mode = 'N';
			__gps_info.status = 'V';
			uart_puts("[ERR] RMC msg empty\n");
//			__dbg_print_msg(nmea_msg);
		}
		else				// точно пропарсилось время и символ валидности
		{
			__gps_time.hh = rmc.utc_t_hh;
			__gps_time.mm = rmc.utc_t_mm;
			__gps_time.ss = rmc.utc_t_ss;
			
			__gps_info.status = rmc.status;
						
			if(rmc.status == 'V')
			{
				uart_puts("[WARN] RMC NOT valid\n");
			}
			else if(rmc.status == 'A')
			{
//				uart_puts("RMC valid\n");
				// На всякий случай проверим, пропарсились ли координаты
				// (статус A предполагает их наличие, однако есть мизерная вероятность,
				// что произойдёт ошибка на линии и испортится какой-нибудь символ)
				if(parse_res >= 12)
				{
					__gps_position.lat = (float)rmc.lat_deg + ((float)rmc.lat_min + rmc.lat_min_dec/1000.0)/60.0;
					// знак не учитываем, т.к. только N
					__gps_position.lon = (float)rmc.lon_deg + ((float)rmc.lon_min + rmc.lon_min_dec/1000.0)/60.0;
					// знак не учитываем, т.к. только E
					
					// Аналогично для скорости
					if(parse_res >= 14)
					{
						__gps_motion.vel = (float)rmc.vel + rmc.vel_dec/10.0;
						// Аналогично для курса
						if(parse_res == 16)
						{
//							__gps_motion.crs = (float)rmc.crs + (float)rmc.crs_dec/10.0;

							__gps_motion.crs = rmc.crs;	// целая часть курса
							if(rmc.crs_dec >= 5)		// округление по общеизвестному правилу
							{
								__gps_motion.crs += 1;
								if(__gps_motion.crs == 360)	// если после округления получили 360
								{
									__gps_motion.crs = 0;
								}
							}
						}
					}
					else
					{
						uart_puts("[ERR] Vel and Crs not parsed\n");
//						__dbg_print_msg(nmea_msg);
					}
				}
				else
				{
					uart_puts("[ERR] Coords, Vel and Crs not parsed\n");
//					__dbg_print_msg(nmea_msg);
				}
			}
			// Проскакиваем 9 запятых до даты
			nmea_msg_offset = 0;
			for(register uint8_t i = 0; i < 9; nmea_msg_offset++)
			{
				if(*(nmea_msg + nmea_msg_offset) == '\r')
				{
					uart_puts("[ERR] Reached end of line\n\n");
					//					__dbg_print_msg(nmea_msg);
					return;		// если наткнулись на конец строки (практически невозможное событие)
				}
				else if(*(nmea_msg + nmea_msg_offset) == ',')
				{
					i++;
				}
			}
			// Пробуем парсить дату и режим:
			parse_res = sscanf(nmea_msg + nmea_msg_offset,
			"%2hhd%2hhd%2hhd,,,%c",
			&rmc.utc_d_dd, &rmc.utc_d_mm, &rmc.utc_d_yy, &rmc.mode);
			
			if(parse_res < 3)
			{
				uart_puts("[WARN] RMC date not parsed\n");
			}
			else
			{
				__gps_date.dd = rmc.utc_d_dd;
				__gps_date.mm = rmc.utc_d_mm;
				__gps_date.yy = rmc.utc_d_yy;
				
				if(parse_res == 4)
				{
					__gps_info.mode = rmc.mode;
				}
				else
				{
					uart_puts("[WARN] RMC mode not parsed\n");
				}
			}
		}
//		printf("[%d parsed]\n", parse_res);
// 		printf("%02d:%02d:%02d %c  %.5f  %03.5f  V%.1f Crs%3.1f %02d-%02d-%02d  %c\n\n",
// 				__gps_time.hh, __gps_time.mm, __gps_time.ss, __gps_info.status,
// 				__gps_position.lat, __gps_position.lon, __gps_motion.vel, __gps_motion.crs,
// 				__gps_date.dd, __gps_date.mm, __gps_date.yy, __gps_info.mode);
	}
// 	else if(strstr(nmea_header, "GNVTG") != NULL)
// 	{
// 		uart_puts("[VTG]\n");
// 	}
	
	return;
}

void __dbg_print_msg(char *msg)
{
	uart_puts("===========================================\nMessage:\n");
	for(; *msg != '\n'; msg++)
	{
		uart_putc(*msg);
	}
	uart_putc('\n');
	
	return;
}

inline void gps_init(void)
{
	fifo_init(&__gps_rx_buf, GPS_RX_BUF_SIZE);
	
	UBRR1L = (uint8_t) GPS_UBRR;
	UBRR1H = (uint8_t) (GPS_UBRR >> 8);
	
	UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10);				// Char size 8 bit
	UCSR1B |= (0 << TXEN1) | (1 << RXEN1) | (1 << RXCIE1);	// (будем только слушать, а также включим прерывание)
	
	__gps_position.alt = 0.0; __gps_position.lat = 55.80172; __gps_position.lon = 37.50211;
	__gps_motion.crs = 0.0; __gps_motion.vel = 0.0;
	__gps_date.dd = 0; __gps_date.mm = 0; __gps_date.yy = 0;
	__gps_time.hh = 0; __gps_time.mm = 0; __gps_time.ss = 0;
	__gps_info.hdop = 99.0; __gps_info.mode = 'N'; __gps_info.sats_num = 0; __gps_info.status = 'V';
	
	uart_puts("[ OK ] GPS init completed\n");
}

void gps_get_pos(GPS_POS *pos_p)
{
	pos_p->lat = __gps_position.lat;
	pos_p->lon = __gps_position.lon;
	pos_p->alt = __gps_position.alt;
	
	return;
}

void gps_get_motion(GPS_MOTION *motion_p)
{
	motion_p->vel = __gps_motion.vel;
	motion_p->crs = __gps_motion.crs;
	
	return;
}

void gps_get_info(GPS_INF *info_p)
{
	info_p->status = __gps_info.status;
	info_p->mode = __gps_info.mode;
	info_p->sats_num = __gps_info.sats_num;
	info_p->hdop = __gps_info.hdop;
	
	return;
}

void gps_get_time(GPS_TIME *time_p)
{
	time_p->hh = __gps_time.hh;
	time_p->mm = __gps_time.mm;
	time_p->ss = __gps_time.ss;
	
	return;
}

void gps_get_date(GPS_DATE *date_p)
{
	date_p->dd = __gps_date.dd;
	date_p->mm = __gps_date.mm;
	date_p->yy = __gps_date.yy;
	
	return;
}