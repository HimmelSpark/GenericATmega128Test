/*
 * hmc5883l.h
 *
 * Created: 27.02.2020 16:11:26
 *  Author: Vsevolod
 */ 


#ifndef HMC5883L_H_
#define HMC5883L_H_

typedef struct {
	double mX;
	double mY;
	double mZ;
	} HMC5883L_MAG_DATA;
	
typedef struct {
	int16_t mX_raw;
	int16_t mY_raw;
	int16_t mZ_raw;
	} HMC5883L_RAW_DATA;
	
typedef struct {
	uint8_t cra, crb, moder, status, ida, idb, idc;
	} HMC5883L_DEBUG;


void hmc5883l_init(void);
void hmc5883l_init_exit(void);
void hmc5883l_read(void);
void hmc5883l_read_exit(uint8_t *buf_rd);

// Получить составляющие вектора магнитной индукции, Гс
void hmc5883l_get_mag(HMC5883L_MAG_DATA *mag_p);

// Получить сырые показания датчика
void hmc5883l_get_raw(HMC5883L_RAW_DATA *raw_p);

// Получить отладочную информацию (содержимое служебных регистров)
void hmc5883l_get_debug(HMC5883L_DEBUG *hmc5883l_debug);

// Получить магнитный курс (heading), °
uint16_t hmc5883l_get_mag_hdg(void);

/* Выбор усреднения измерений */
// Возможные варианты:
// 0: 1 сэмпл
// 1: 2 сэмпла
// 2: 4 сэмпла
// 3: 8 сэмплов
#define HMC5883L_AVERAGE_SMPL	0
/******************************/

/* Выбор частоты data output */
// Возможные варианты:
// 0: 0.75 Hz
// 1: 1.5 Hz
// 2: 3 Hz
// 3: 7.5 Hz
// 4: 15 Hz
// 5: 30 Hz
// 6: 75 Hz
#define HMC5883L_DO_RATE	5

// Периоды в мс в зависимости от выбранного data output rate
#if HMC5883L_DO_RATE == 0
	#define HMC5883L_PERIOD	1334
#elif HMC5883L_DO_RATE == 1
	#define HMC5883L_PERIOD	667
#elif HMC5883L_DO_RATE == 2
	#define HMC5883L_PERIOD	334
#elif HMC5883L_DO_RATE == 3
	#define HMC5883L_PERIOD	134
#elif HMC5883L_DO_RATE == 4
	#define HMC5883L_PERIOD	66
#elif HMC5883L_DO_RATE == 5
	#define HMC5883L_PERIOD	34
#elif HMC5883L_DO_RATE == 6
	#define HMC5883L_PERIOD	14
#endif

//#define HMC5883L_STARTUP_DELAY	(2*HMC5883L_PERIOD)
#define HMC5883L_STARTUP_DELAY		100

// Только отладка: //
#undef	HMC5883L_PERIOD
#define HMC5883L_PERIOD	50
////////////////////

/*****************************/

/* Выбор чувствительности */
// Возможные варианты:
// 0: +/- 0.88 Ga	(наибольшая чувствительность)
// 1: +/- 1.3 Ga
// 2: +/- 1.9 Ga
// 3: +/- 2.5 Ga
// 4: +/- 4.0 Ga
// 5: +/- 4.7 Ga
// 6: +/- 5.6 Ga
// 7: +/- 8.1 Ga	(наименьшая чувствительность)
#define HMC5883L_GAIN	1

// Коэффициенты перевода отсчётов в гауссы:
// Показания HMC5883L / SCALE = [Gauss]

#if HMC5883L_GAIN == 0
	#define HMC5883L_SCALE	1370.0F
#elif HMC5883L_GAIN == 1
	#define HMC5883L_SCALE	1090.0F
#elif HMC5883L_GAIN == 2
	#define HMC5883L_SCALE	820.0F
#elif HMC5883L_GAIN == 3
	#define HMC5883L_SCALE	660.0F
#elif HMC5883L_GAIN == 4
	#define HMC5883L_SCALE	440.0F
#elif HMC5883L_GAIN == 5
	#define HMC5883L_SCALE	390.0F
#elif HMC5883L_GAIN == 6
	#define HMC5883L_SCALE	330.0F
#elif HMC5883L_GAIN == 7
	#define HMC5883L_SCALE	230.0F
#endif

/**************************/

/* Выбор режима измерений */
// Возможные варианты:
// 0: Continuos measurement
// 1: Single measurement
// 2: Idle
#define HMC5883L_MODE	0
/**************************/

/* Адрес модуля (7 бит) */
#define HMC5883L_ADDR	0b0011110

/* Адреса регистров и положения их битов */

#define HMC5883L_CONF_REG_COUNT	3

#define HMC5883L_CONF_REG_A		0x00	// R/W
#define HMC5883L_CRA7			7
#define HMC5883L_CRA6			6
#define HMC5883L_CRA5			5
#define HMC5883L_CRA4			4
#define HMC5883L_CRA3			3
#define HMC5883L_CRA2			2
#define HMC5883L_CRA1			1
#define HMC5883L_CRA0			0
#define HMC5883L_MA1			HMC5883L_CRA6
#define HMC5883L_MA0			HMC5883L_CRA5
#define HMC5883L_DO2			HMC5883L_CRA4
#define HMC5883L_DO1			HMC5883L_CRA3
#define HMC5883L_DO0			HMC5883L_CRA2
#define HMC5883L_MS1			HMC5883L_CRA1
#define HMC5883L_MS0			HMC5883L_CRA0

#define HMC5883L_CONF_REG_B		0x01	// R/W
#define HMC5883L_CRB7			7
#define HMC5883L_CRB6			6
#define HMC5883L_CRB5			5
#define HMC5883L_CRB4			4
#define HMC5883L_CRB3			3
#define HMC5883L_CRB2			2
#define HMC5883L_CRB1			1
#define HMC5883L_CRB0			0
#define HMC5883L_GN2			HMC5883L_CRB7
#define HMC5883L_GN1			HMC5883L_CRB6
#define HMC5883L_GN0			HMC5883L_CRB5

#define HMC5883L_MODE_REG		0x02	// R/W
#define HMC5883L_MR7			7
#define HMC5883L_MR6			6
#define HMC5883L_MR5			5
#define HMC5883L_MR4			4
#define HMC5883L_MR3			3
#define HMC5883L_MR2			2
#define HMC5883L_MR1			1
#define HMC5883L_MR0			0
#define HMC5883L_HS				HMC5883L_MR7
#define HMC5883L_MD1			HMC5883L_MR1
#define HMC5883L_MD0			HMC5883L_MR0


#define HMC5883L_DATA_BYTES_COUNT	6

#define HMC5883L_DATA_X_MSB		0x03
#define HMC5883L_DATA_X_LSB		0x04
#define HMC5883L_DATA_Z_MSB		0x05
#define HMC5883L_DATA_Z_LSB		0x06
#define HMC5883L_DATA_Y_MSB		0x07
#define HMC5883L_DATA_Y_LSB		0x08

#define HMC5883L_SREG			0x09
#define HMC5883L_SR7			7
#define HMC5883L_SR6			6
#define HMC5883L_SR5			5
#define HMC5883L_SR4			4
#define HMC5883L_SR3			3
#define HMC5883L_SR2			2
#define HMC5883L_SR1			1
#define HMC5883L_SR0			0
#define HMC5883L_LOCK			HMC5883L_SR1
#define HMC5883L_RDY			HMC5883L_SR0

#define HMC5883L_ID_REG_A		0x0A
#define HMC5883L_ID_REG_B		0x0B
#define HMC5883L_ID_REG_C		0x0C
/********************/

#endif /* HMC5883L_H_ */