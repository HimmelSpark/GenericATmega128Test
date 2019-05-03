/*
 * i2c.h
 *
 * Created: 21.11.2018 22:02:01
 *  Author: Vsevolod
 */ 


#ifndef I2C_H_
#define I2C_H_

typedef void (*I2C_EXIT_F)(void); // тип указателя на функцию

void i2c_init (void);
void __i2c_routine (void);
int i2c_write_byte2reg (uint8_t sla_addr, uint8_t reg_addr, uint8_t data, I2C_EXIT_F exit_func);
int i2c_read_bytes (uint8_t sla_addr, uint8_t start_reg_addr, uint8_t bytes_count, I2C_EXIT_F exit_func); 
int i2c_write_from_buffer (uint8_t sla_addr, uint8_t start_reg_addr, uint8_t bytes_count, I2C_EXIT_F exit_func);\
// в i2c_write_from_buffer данные класть через массив i2c_write_buffer

void i2c_exit_func_idle (void);


#define I2C_SCL_PIN PD0
#define I2C_SDA_PIN PD1

#define I2C_PORT_DDR	DDRD
#define I2C_PORT		PORTD

#define I2C_MAX_READ_BYTES_COUNT	22
#define I2C_MAX_WRITE_BYTES_COUNT	8

/* Режимы I2C */

#define I2C_WRITE_BYTE2REG	0x00
#define I2C_READ_BYTES		0x01
#define I2C_WRITE_BYTES		0x02

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
#define I2C_STATUS_DATA_SENT		1
#define I2C_STATUS_DATA_RECEIVED	2


#endif /* I2C_H_ */