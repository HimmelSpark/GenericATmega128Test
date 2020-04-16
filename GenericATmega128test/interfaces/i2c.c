/*
 * i2c.c
 *
 * Created: 21.11.2018 22:04:06
 *  Author: Vsevolod
 */ 

// заглушка для настройки TWCR:
// TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);

#include "../general.h"
#include <util/twi.h>
#include <stdlib.h>
#include "i2c.h"
#include "../rtos.h"
#include "../modules/md3.h"

uint8_t i2c_mode				= I2C_IDLE;	// режим

uint8_t i2c_sla_addr	= 0x00;		// 7 битов адреса
uint8_t i2c_reg_addr	= 0x00;		// адрес первого регистра, в который пишем/читаем
uint8_t *i2c_buf;					// буфер записи/чтения
uint8_t i2c_bytes_count	= 0;		// сколько байтов записать/прочитать
uint8_t i2c_byte_index	= 0;		// какой по счёту

volatile uint8_t i2c_status		= 0x00;
volatile uint8_t i2c_collisions = 0;	// сколько коллизий произошло

// Выходные функции для записи и для чтения с шины I2C
// (по умолчанию после выхода из автомата I2C ничего не делаем)
I2C_EXIT_F_WR i2c_exit_func_wr = i2c_exit_func_wr_idle;
I2C_EXIT_F_RD i2c_exit_func_rd = i2c_exit_func_rd_idle;

inline void i2c_init (void)
{
	// Пины I2C должны быть hi-Z
	I2C_PORT_DDR &= ~((1 << I2C_SCL_PIN) | (1 << I2C_SDA_PIN));
	I2C_PORT &= ~((1 << I2C_SCL_PIN) | (1 << I2C_SDA_PIN));
	
	TWBR = 33;
	TWSR |= (0 << TWPS1) | (0 << TWPS0); // таким образом получаем f_scl ~= 98 кГц
											
	return;
}

inline void __i2c_routine (void)
{// ISR
	switch (TW_STATUS)
	{
		case TW_BUS_ERROR:
		{
//			printf ("errb\n");
			led_r_blink ();
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			break;
		}
		case TW_START:
		{	
			TWDR = (i2c_sla_addr << 1) | I2C_W;
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			break;
		}
		case TW_MT_SLA_ACK:
		{
			TWDR = i2c_reg_addr;
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			break;
		}
		case TW_MT_DATA_ACK:
		{ 
			if (i2c_mode == I2C_WRITE_BYTES)
			{
				if (i2c_byte_index < i2c_bytes_count)
				{
					TWDR = i2c_buf[i2c_byte_index];
					i2c_byte_index++;
					TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
				}
				else
				{
					TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
					
					i2c_exit_func_wr();
					i2c_status &= ~(1 << I2C_STATUS_BUSY); // шина свободна
					
					// Подготовимся к следующему чтению/записи
					i2c_bytes_count = 0;
					i2c_byte_index = 0;
					free(i2c_buf);
					
					rtos_delete_task(i2c_watchdog);
				}
			}
			else if (i2c_mode == I2C_READ_BYTES)
			{
				TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			}
			
			break;
		}
		case TW_REP_START:
		{ 
			TWDR = (i2c_sla_addr << 1) | I2C_R;
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			
			break;
		}
		case TW_MR_SLA_ACK:
		{
			if (i2c_bytes_count == 1)
			{	// если читаем 1 байт, сразу позаботимся о том, чтобы вовремя закончить читать:
				// "Data byte will be received and NOT ACK will be returned":
				TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			}
			else
			{	// если более одного, то сделаем это позднее, а пока продолжим:
				// "Data byte will be received and ACK will be returned":
				TWCR = (1 << TWINT) | (1 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			}
			
			break;
		}
		case TW_MR_DATA_ACK:
		{ 
			i2c_buf[i2c_byte_index] = TWDR;
			i2c_byte_index++;

			if ((i2c_byte_index+1) == i2c_bytes_count) // позаботимся о том, чтобы вовремя закончить читать:
			{	// на следующем "такте" I2C:
				TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			}
			else
			{	// хз когда:
				TWCR = (1 << TWINT) | (1 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			}
			
			break;
		}
		case TW_MR_DATA_NACK:
		{
			i2c_buf[i2c_byte_index] = TWDR;
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			
			i2c_exit_func_rd(i2c_buf);
			i2c_status &= ~(1 << I2C_STATUS_BUSY); // шина свободна
			
			// Подготовимся к следующему чтению/записи
			i2c_bytes_count = 0;
			i2c_byte_index = 0;
			free(i2c_buf);
			
			rtos_delete_task(i2c_watchdog);
			
			break;
		}
		default:
		{
//			printf ("errd\n");
			led_y_blink ();
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			
			break;
		}
	}
	
	return;
}

int i2c_write(uint8_t slave_addr, uint8_t start_reg_addr, uint8_t *wr_buf, uint8_t bytes_count, I2C_EXIT_F_WR exit_func)
{
	if (!(i2c_status & (1 << I2C_STATUS_BUSY)))
	{
		i2c_status |= 1 << I2C_STATUS_BUSY;
		rtos_set_task(i2c_watchdog, I2C_WATCHDOG_DELAY, RTOS_RUN_ONCE);
		
		i2c_mode = I2C_WRITE_BYTES;
		
		i2c_sla_addr = slave_addr;
		i2c_reg_addr = start_reg_addr;
		i2c_bytes_count = bytes_count;
		
		i2c_buf = (uint8_t *) malloc(i2c_bytes_count);			// выделяем память под буфер, который будет сохраняться
																// между вызовами i2c_routine
		for(register uint8_t i = 0; i < i2c_bytes_count; i++)	// и копируем в него байты для передачи
		{
			i2c_buf[i] = wr_buf[i];
		}
		
		i2c_exit_func_wr = exit_func;
		
		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE); // START
		
		return 0;	// успешно
	}
	
	led_y_blink ();
	i2c_collisions++;
	
	return 1;	// шина занята	
}

int i2c_write_byte_to_reg(uint8_t slave_addr, uint8_t reg_addr, uint8_t data, I2C_EXIT_F_WR exit_func)
{
	int res = i2c_write(slave_addr, reg_addr, &data, 1, exit_func);
	
	return res;
}

int i2c_read(uint8_t slave_addr, uint8_t start_reg_addr, uint8_t bytes_count, I2C_EXIT_F_RD exit_func)
{
	if (!(i2c_status & (1 << I2C_STATUS_BUSY)))
	{
		i2c_status |= 1 << I2C_STATUS_BUSY;
		rtos_set_task(i2c_watchdog, I2C_WATCHDOG_DELAY, RTOS_RUN_ONCE);

		i2c_mode = I2C_READ_BYTES;

		i2c_sla_addr = slave_addr;
		i2c_reg_addr = start_reg_addr;
		i2c_bytes_count = bytes_count;
		
		i2c_buf = (uint8_t *) malloc(i2c_bytes_count);		// выделяем память под буфер, который будет
															// сохраняться между вызовами i2c_routine
		i2c_exit_func_rd = exit_func;

		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE); // START
		
		return 0;	// успешно
	}

	led_y_blink ();
	i2c_collisions++;

	return 1;	// шина занята
}

int i2c_read_from_byte(uint8_t slave_addr, uint8_t reg_addr, I2C_EXIT_F_RD exit_func)
{
	int res = i2c_read(slave_addr, reg_addr, 1, exit_func);
	
	return res;
}

void i2c_exit_func_wr_idle (void)
{
	return;
}

void i2c_exit_func_rd_idle(uint8_t *rd_buf)
{
	return;
}

void i2c_watchdog(void)
{
	// ToDo: Вообще, в идеале надо сделать, чтобы вотчдог удалял "проблемную" задачу,
	// которая вешает шину. Текущая реализация позволяет только раз в период
	// освободить шину, после чего она снова будет занята "проблемной" задачей
	
	// Принудительно снимаем флаг занятонсти шины
	i2c_status &= ~(1 << I2C_STATUS_BUSY);
	
	return;
}