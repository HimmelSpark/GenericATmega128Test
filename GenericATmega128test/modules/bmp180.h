/*
 * bmp180.h
 *
 * Created: 28.11.2018 15:38:53
 *  Author: Vsevolod
 */ 


#ifndef BMP180_H_
#define BMP180_H_

void bmp180_init (void);		// запуск последовательности считывания калибровочных параметров
void bmp180_start_UT (void);	// запуск последовательности считывания некомпенсированной температуры
void bmp180_read_UT (void);		// считывание некомпенсированной температуры
void bmp180_start_UP (void);	// запуск последовательности считывания некомпенсированного давления
void bmp180_read_UP (void);		// считывание некомпенсированного давления
void bmp180_calc_T (void);		// расчёт температуры
void bmp180_calc_P (void);	// расчёт давления

double bmp180_get_H (long P0);	// приведённая высота, метры
double bmp180_get_P_hPa (void);	// последнее измеренное давление, гПа
double bmp180_get_P_mmHg (void);// последнее измеренное давление, мм рт. ст.
float bmp180_get_T (void);		// последняя измеренная температура, °C

/* Функции выхода */
void read_params_exit (void);
void start_UT_exit (void);
void read_UT_exit (void);
void start_UP_exit (void);
void read_UP_exit (void);
/******************/


/* Выбор оверсэмплинга */
#define BMP180_OSS 0 // 0, 1, 2, 3
/***********************/

#define BMP180_CAL_BYTES_COUNT	22

#define BMP180_ADDR				0x77	// 7-битный адрес!
#define BMP180_CTRL_REG_ADDR	0xF4

/* Значения CTRL_REG_ADDR */
#define BMP180_REQ_UT			0x2E
#define BMP180_REQ_UP			0x34
/**************************/

/* Адреса регистров */
#define BMP180_AC1_ADDR_HI	0xAA	// хотя для массового считывания нужен только этот адрес
#define BMP180_AC1_ADDR_LO	0xAB
#define BMP180_AC2_ADDR_HI	0xAC
#define BMP180_AC2_ADDR_LO	0xAD
#define BMP180_AC3_ADDR_HI	0xAE
#define BMP180_AC3_ADDR_LO	0xAF
#define BMP180_AC4_ADDR_HI	0xB0
#define BMP180_AC4_ADDR_LO	0xB1
#define BMP180_AC5_ADDR_HI	0xB2
#define BMP180_AC5_ADDR_LO	0xB3
#define BMP180_AC6_ADDR_HI	0xB4
#define BMP180_AC6_ADDR_LO	0xB5
#define BMP180_B1_ADDR_HI	0xB6
#define BMP180_B1_ADDR_LO	0xB7
#define BMP180_B2_ADDR_HI	0xB8
#define BMP180_B2_ADDR_LO	0xB9
#define BMP180_MB_ADDR_HI	0xBA
#define BMP180_MB_ADDR_LO	0xBB
#define BMP180_MC_ADDR_HI	0xBC
#define BMP180_MC_ADDR_LO	0xBD
#define BMP180_MD_ADDR_HI	0xBE
#define BMP180_MD_ADDR_LO	0xBF

#define BMP180_MEASUREMENT_HI	0xF6	// хотя для массового считывания нужен только этот адрес
#define BMP180_MEASUREMENT_LO	0xF7
#define BMP180_MEASUREMENT_XLO	0xF8
/*********************/

#define BMP180_READ_STARTUP			1000	// ms
#define BMP180_READ_PERIOD			100		// ms


#define BMP180_UT_READ_DELAY		5	// ms

#if BMP180_OSS == 0
	#define BMP180_UP_READ_DELAY	5	// ms;	точность 6 Па /		0,5 м	/ 0,045 мм рт ст
#elif BMP180_OSS == 1
	#define BMP180_UP_READ_DELAY	8	// ms;	точность 5 Па /		0,4 м	/ 0,038 мм рт с
#elif BMP180_OSS == 2
	#define BMP180_UP_READ_DELAY	14	// ms;	точность 4 Па /		0,3 м	/ 0,030 мм рт ст
#elif BMP180_OSS == 3
	#define BMP180_UP_READ_DELAY	26	// ms;	точность 3 Па /		0,25 м	/ 0,023 мм рт ст
#endif

#define BMP180_UT_WORD_SIZE 2
#define BMP180_UP_WORD_SIZE 3

#endif /* BMP180_H_ */