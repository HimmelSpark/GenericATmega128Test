/*
 * i2c.h
 *
 * Created: 21.11.2018 22:02:01
 *  Author: Vsevolod
 */ 


#ifndef I2C_H_
#define I2C_H_

// Тип указателя на выходную функцию в случае ЗАПИСИ данных на I2C
// (после записи данных в выходную функцию ничего не передаётся)
typedef void (*I2C_EXIT_F_WR)(void);

// Тип указателя на выходную функцию в случае ЧТЕНИЯ данных с I2C
// (после чтения данных в выходную функцию передаётся указатель на
// буфер принятых данных [и его размер?])
typedef void (*I2C_EXIT_F_RD)(uint8_t *rd_buf);

void __i2c_routine (void);

void i2c_init (void);

// Запись из буфера, предоставляемого пользователем, начиная с заданного адреса
int i2c_write(uint8_t slave_addr, uint8_t start_reg_addr, uint8_t *wr_buf, uint8_t bytes_count, I2C_EXIT_F_WR exit_func);

// Запись одного байта по указанному адресу регистра
int i2c_write_byte_to_reg(uint8_t slave_addr, uint8_t reg_addr, uint8_t data, I2C_EXIT_F_WR exit_func);

// Чтение, начиная с заданного адреса. 
int i2c_read(uint8_t slave_addr, uint8_t start_reg_addr, uint8_t bytes_count, I2C_EXIT_F_RD exit_func);

// Чтение одного байта по указанному адресу
int i2c_read_from_byte(uint8_t slave_addr, uint8_t reg_addr, I2C_EXIT_F_RD exit_func);

void i2c_exit_func_wr_idle (void);
void i2c_exit_func_rd_idle (uint8_t *rd_buf);

// Предотвращает занятие шины в случае неответа какого-либо устройства
void i2c_watchdog(void);


#define I2C_SCL_PIN		PD0
#define I2C_SDA_PIN		PD1

#define I2C_PORT_DDR	DDRD
#define I2C_PORT		PORTD

/* Режимы I2C */

#define I2C_IDLE			0x00
#define I2C_WRITE_BYTES		0x01
#define I2C_READ_BYTES		0x02

/**************/

#define I2C_W	0
#define I2C_R	1

/* Флаг I2C_STATUS */
//
// |BIT 7|BIT 6|BIT 5|BIT 4|BIT 3|BIT 2|BIT 1|BIT 0|
// |I 2 C  _  M O D E|I  2  C  _  S  T  A  T  U  S |
//  ^ ^ ^ ^ ^ ^ ^ ^ ^
//     T O   D O (?) |       D   O   N   E		   |

#define I2C_STATUS_BUSY				0

#define I2C_WATCHDOG_DELAY			2000	// мс, в случае неответа после этого времени
											// флаг занятости шины принудительно снимается


#endif /* I2C_H_ */