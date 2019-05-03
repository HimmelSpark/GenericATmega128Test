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
#include "i2c.h"
#include "../rtos.h"
#include "../modules/md3.h"

uint8_t i2c_mode;

uint8_t i2c_sla_addr;				// 7 бит адреса
uint8_t i2c_reg_addr;				// регистр, в который пишем (или начиная с которого читатем)
uint8_t i2c_data;					// байт, который кладём в регистр
uint8_t i2c_bytes2read_count;		// сколько байтов прочитать
uint8_t i2c_byte2read_index = 0;	// какой читаем
uint8_t i2c_bytes2write_count;		// сколько байтов записать
uint8_t i2c_byte2write_index = 0;	// какой пишем

/* Переменные, которые по смыслу должны быть доступны в других файлах */
volatile uint8_t i2c_status		= 0x00;
volatile uint8_t i2c_collisions = 0;	// сколько коллизий произошло
uint8_t i2c_read_buffer[I2C_MAX_READ_BYTES_COUNT];
uint8_t i2c_write_buffer[I2C_MAX_WRITE_BYTES_COUNT];
/****************************************************************/

I2C_EXIT_F i2c_exit_func = i2c_exit_func_idle;	// по умолчанию после выхода из автомата I2C ничего не делаем

inline void i2c_init (void)
{
	// Пины I2C должны быть hi-Z; хотя это происходит по умолчанию после включения,
	// на всякий случай гасим их руками:
	I2C_PORT_DDR &= ~((1 << I2C_SCL_PIN) | (1 << I2C_SDA_PIN));
	I2C_PORT &= ~((1 << I2C_SCL_PIN) | (1 << I2C_SDA_PIN));
	
	TWBR = 33;
	TWSR |= (0 << TWPS1) | (0 << TWPS0); // таким образом получаем f_scl ~= 98 кГц
											
	return;
}

inline void __i2c_routine (void)
{	// Вызов по прерыванию
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
			if (i2c_mode == I2C_WRITE_BYTE2REG)
			{
				if (!(i2c_status & (1 << I2C_STATUS_DATA_SENT)))
				{
					TWDR = i2c_data;
					i2c_status |= 1 << I2C_STATUS_DATA_SENT;
					TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
				}
				else
				{
					TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
					i2c_status &= ~(1 << I2C_STATUS_BUSY); // шина свободна
					
					i2c_exit_func ();	// до обнуления, чтобы функция имела доступ к актуальному i2c_data_sent
					i2c_status &= ~(1 << I2C_STATUS_DATA_SENT);
				}
			}
			else if (i2c_mode == I2C_READ_BYTES)
			{
				TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			}
			else if (i2c_mode == I2C_WRITE_BYTES)
			{
				if (i2c_byte2write_index < i2c_bytes2write_count)
				{
					TWDR = i2c_write_buffer[i2c_byte2write_index];
					i2c_byte2write_index++;
					TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE);
				}
				else
				{
					TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
					i2c_status &= ~(1 << I2C_STATUS_BUSY); // шина свободна
					
					i2c_status |= 1 << I2C_STATUS_DATA_SENT; // ToDo: нафиг не нужно вроде?
					i2c_exit_func ();	// до обнуления, чтобы функция имела доступ к актуальному i2c_data_sent
					i2c_status &= ~(1 << I2C_STATUS_DATA_SENT); // ToDo: нафиг не нужно вроде?
					
					i2c_byte2write_index = 0; // подготовимся к следующей записи
				}
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
			if (i2c_bytes2read_count == 1)
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
			i2c_read_buffer[i2c_byte2read_index] = TWDR;
			i2c_byte2read_index++;
			
//			printf("rd:%d\n", i2c_byte2read_index);	// ОТЛАДКА

			if ((i2c_byte2read_index+1) == i2c_bytes2read_count) // позаботимся о том, чтобы вовремя закончить читать:
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
			i2c_read_buffer[i2c_byte2read_index] = TWDR;
			TWCR = (1 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
			i2c_status &= ~(1 << I2C_STATUS_BUSY); // шина свободна
			
			i2c_status |= 1 << I2C_STATUS_DATA_RECEIVED; // ToDo: нафиг не нужно вроде?
			i2c_exit_func ();
			i2c_status &= ~(1 << I2C_STATUS_DATA_RECEIVED); // ToDo: нафиг не нужно вроде?
			
			i2c_byte2read_index = 0;	// подготовимся к следующему чтению
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

int i2c_write_byte2reg (uint8_t sla_addr, uint8_t reg_addr, uint8_t data, I2C_EXIT_F exit_func)
{
	if (!(i2c_status & (1 << I2C_STATUS_BUSY)))
	{
 		i2c_status |= 1 << I2C_STATUS_BUSY;

		i2c_mode = I2C_WRITE_BYTE2REG;
	
		i2c_sla_addr = sla_addr;
		i2c_reg_addr = reg_addr;
		i2c_data = data;
		i2c_exit_func = exit_func;
	
		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE); // START
		return 0;	// OK
	}
	
	led_y_blink ();
	i2c_collisions++;
	
	return 1;		// не ОК
}

int i2c_read_bytes (uint8_t sla_addr, uint8_t start_reg_addr, uint8_t bytes_count, I2C_EXIT_F exit_func)
{
	if (!(i2c_status & (1 << I2C_STATUS_BUSY)))
	{
		i2c_status |= 1 << I2C_STATUS_BUSY;
	
		i2c_mode = I2C_READ_BYTES;
	
		i2c_sla_addr = sla_addr;
		i2c_reg_addr = start_reg_addr;
		i2c_bytes2read_count = bytes_count;
		i2c_exit_func = exit_func;
	
		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE); // START
		return 0;	// OK
	}
	
	led_y_blink ();
	i2c_collisions++;
	
	return 1;		// не ОК
}

int i2c_write_from_buffer (uint8_t sla_addr, uint8_t start_reg_addr, uint8_t bytes_count, I2C_EXIT_F exit_func)
{
	if (!(i2c_status & (1 << I2C_STATUS_BUSY)))
	{	
		i2c_status |= 1 << I2C_STATUS_BUSY;
		
		i2c_mode = I2C_WRITE_BYTES;
		
		i2c_sla_addr = sla_addr;
		i2c_reg_addr = start_reg_addr;
		i2c_bytes2write_count = bytes_count;
		i2c_exit_func = exit_func;
		
		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) | (1 << TWEN) | (1 << TWIE); // START
		return 0;	// OK
	}
	
	led_y_blink ();
	i2c_collisions++;
	
	return 1;		// не ОК
}

void i2c_exit_func_idle (void)
{
	return;
}