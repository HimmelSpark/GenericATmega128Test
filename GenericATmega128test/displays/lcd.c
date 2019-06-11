/*
 * lcd.c
 *
 * Created: 30.03.2019 14:17:53
 *  Author: Vsevolod
 */ 

#include "../general.h"
#include "lcd.h"
#include "../fifo.h"
#include "../rtos.h"

uint8_t __lcd_ac = 0x00;

uint8_t __lcd_bf_readable = 0;	// во время Initializing by Instruction Busy Flag нельзя считать;
// поэтому в этот интервал времени запись данных будет осуществляться без его проверки
uint8_t __lcd_4bit_enabled = 0; // изначально дисплей работает в 8-битном режиме, и команды
// 0x30 Initializing by Instruction отсылаются "по-восьмибитному", т.е. в один приём;
// это будет учтено в __lcd_write_byte ()

FIFO_BUFFER_t __lcd_data_buf, __lcd_mode_buf;

void __lcd_write_byte (uint8_t mode, uint8_t data)
{
	// Режимы (запись данных/команд) и данные (байты) всегда пишем парами в два буфера:
	fifo_push (mode, &__lcd_mode_buf);
	fifo_push (data, &__lcd_data_buf);
	
	if (!__lcd_bf_readable)
	{	// на этапе инициализации нельзя использовать RTOS, т.к. выдерживаем
		// тупые задержки, не зная BUSY FLAG; поэтому сразу вызываем процедуру передачи
		__lcd_tx_routine ();
	}
	else
	{	
		// ToDo: если не можем достучаться до дисплея достаточно долго, то
		// нужно уже не ASAP, а например каждую секунду
		rtos_set_task (__lcd_tx_routine, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

void __lcd_tx_routine (void)
{
	uint8_t mode, data, data_l, data_h;
	register uint8_t bf;

	// если можно прочитать флаг занятости,
	if (__lcd_bf_readable)
	{
		bf = __lcd_read_bf ();	// читаем его
		if (bf)	// и в случае занятости планируем эту процедуру ASAP
		{
			rtos_set_task (__lcd_tx_routine, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
			return;		// и уходим отсюда до лучших времён
		}
	}
	
	// в противном случае полагаем, что необходимые интервалы времени выдержаны,
	// забираем данные из буферов и выставляем их на шину данных
	
	// предполагается, что раз вызвали эту процедуру, то есть что передать
	data = fifo_pop (&__lcd_data_buf);
	mode = fifo_pop (&__lcd_mode_buf);
	
	data_l = data & 0x0F;	// младшие 4 бита
	data_h = data >> 4;		// старшие 4 бита

	__LCD_RW_LO;	// запись
	
	switch (mode)
	{
		case LCD_MODE_WR_CMD:
		{
			__LCD_RS_LO;
			break;
		}
		case LCD_MODE_WR_DATA:
		{
			__LCD_RS_HI;
			break;
		}
	}
	
	__LCD_DB_OUT;
	LCD_DB_PORT |= data_h;
	__lcd_strobe ();
	
	// если перешли в 4-битный режим, передаём младшую тетраду
	if (__lcd_4bit_enabled)
	{
		__LCD_DB_PORT_LO;
		LCD_DB_PORT |= data_l;
		__lcd_strobe ();
	}
	
	__LCD_DB_HiZ;
	
	// Если ещё осталось что-то для записи, планируем эту процедуру ещё раз:
	if (__lcd_data_buf.idxIn != __lcd_data_buf.idxOut)
	{
		rtos_set_task (__lcd_tx_routine, RTOS_RUN_ASAP, RTOS_RUN_ONCE);
	}
	
	return;
}

uint8_t __lcd_read_bf (void)
{
	uint8_t bf = 0;

	__LCD_RS_LO;
	__LCD_RW_HI;
	
	__LCD_DB_HiZ;
	
	_delay_us (1);
	// строб вверх
	__LCD_E_HI;
	_delay_us (1);
	
	// читаем флаг:
	bf = (LCD_DB_PIN & (1 << LCD_DB7_PIN)) >> 3;	// в принципе можно и не сдвигать,
													// т.к. потом всё равно в if используем
	
	// строб вниз:
	__LCD_E_LO;

	_delay_us (1);
				
	// строб вверх:
	__LCD_E_HI;

	_delay_us (1);

	// строб вниз:
	__LCD_E_LO;

	return bf;
}

uint8_t __lcd_read_ac (void)
{
	uint8_t ac = 0;

	__LCD_RS_LO;
	__LCD_RW_HI;
	
	__LCD_DB_HiZ;
	
	_delay_us (1);
	// строб вверх
	__LCD_E_HI;
	_delay_us (1);
	
	// читаем AC6-AC4
	ac = (LCD_DB_PIN & \
	((1 << LCD_DB6_PIN)|(1 << LCD_DB5_PIN)|(1 << LCD_DB4_PIN))) << 4;
	
	// строб вниз:
	__LCD_E_LO;

	_delay_us (1);
	
	// строб вверх:
	__LCD_E_HI;

	_delay_us (1);
	
	// читаем AC3-AC0
	ac |= (LCD_DB_PIN & \
	((1 << LCD_DB7_PIN)|(1 << LCD_DB6_PIN)|(1 << LCD_DB5_PIN)|(1 << LCD_DB4_PIN)));

	// строб вниз:
	__LCD_E_LO;

	return ac;
}

void __lcd_set_ddram (uint8_t ad)
{
	ad &= 0x7F;	// чтобы точно осталось 7 младших бит
	__lcd_write_byte (LCD_MODE_WR_CMD, (1 << LCD_DDRAM_SET) | ad);
	
	return;
}

inline void __lcd_strobe (void)
{
	_delay_us (1);
	// строб вверх:
	__LCD_E_HI;
	_delay_us(1);
	// строб вниз:
	__LCD_E_LO;
	_delay_us(1);

	return;
}

inline void lcd_init (void)
{
	fifo_init (&__lcd_data_buf, LCD_BUF_SIZE);
	fifo_init (&__lcd_mode_buf, LCD_BUF_SIZE);
	
	LCD_E_DDR |= 1 << LCD_E;	// порт строба всегда на вывод
	LCD_RS_DDR |= 1 << LCD_RS;	// RS всегда на вывод
	LCD_RW_DDR |= 1 << LCD_RW;	// RW всегда на вывод
	
//	_delay_ms(40);
	
	/**** Initializing by Instruction: ****/

// Выполнение этого цикла практически оказалось необязательным, но на всякий случай оставил его
	for (uint8_t i = 0; i < 3; i++)
	{
		__lcd_write_byte (LCD_MODE_WR_CMD, (1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT));
		// передаём 0x30 согласно datasheet
		_delay_ms (5);
	}
	
	// Function Set:
 	__lcd_write_byte (LCD_MODE_WR_CMD, 1 << LCD_FUNCTION);
	
	_delay_us (40);
	
	// Костыль из hd44780.c, без которого ничего не работает (в даташите не описан!):
	__lcd_write_byte (LCD_MODE_WR_CMD, 0b10100000);
	_delay_us (40);
	
	// теперь 4-битный режим активен, с этого момента байты будут выставляться в 2 захода:
	__lcd_4bit_enabled = 1;
	
// UPD 05.04.2019: этот участок кода оказался ненужным, хотя согласно даташиту, он как раз нужен...
// 	// Function Set:
// //	__lcd_write_buffer = (1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_2LINES);
// 	__lcd_write_byte (LCD_MODE_WR_CMD, (1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_2LINES));
// 	_delay_us (100);
	
	// Display on/off control:
	__lcd_write_byte (LCD_MODE_WR_CMD, 1 << LCD_DISPLAYMODE);
	_delay_us (40);
	
	// Clear Display:
	__lcd_write_byte (LCD_MODE_WR_CMD, 1 << LCD_CLR);
	_delay_ms (2);
	
	// Entry Mode Set:
	__lcd_write_byte (LCD_MODE_WR_CMD, (1 << LCD_ENTRY_MODE) | (1 << LCD_ENTRY_INC));
	_delay_us (40);
	// с этого момента флаг занятости может быть прочитан:
	__lcd_bf_readable = 1;
	
	/****  Initializing by Instruction завершён  ****/

	/* Далее врубаем дисплей */
	
	// Display on/off control:
	__lcd_write_byte (LCD_MODE_WR_CMD, (1 << LCD_DISPLAYMODE) | (1 << LCD_DISPLAYMODE_ON));

	lcd_puts ("LCD Alive\n12345678\n");
	
	return;
}

int lcd_stdputc (char c, FILE *stream)
{
	if (c == '\n')
	{
		for (; (__lcd_ac % LCD_LINEWIDTH) < LCD_LINEWIDTH_USED; __lcd_ac++)
		{	// Очистим всё, что дальше последнего символа строки
			// до конца видимого дисплея (скорее всего, там что-то есть)
			__lcd_write_byte (LCD_MODE_WR_DATA, ' ');
		}
		
		if ((__lcd_ac / LCD_LINEWIDTH) == 0)
		{	// если это была первая строка
			__lcd_ac = 40; // перенос на начало второй строки
		}
		else
		{	// если это была вторая строка
			__lcd_ac = 0; // перенос на начало первой строки
		}
		
		__lcd_set_ddram (__lcd_ac);
		
		return 0;
	}
	
	__lcd_write_byte (LCD_MODE_WR_DATA, c);
	__lcd_ac++;
	
	return 0;
}


inline void lcd_putc (char c)
{
	lcd_stdputc (c, NULL);
	
	return;
}


void lcd_puts (char *str)
{
	register char c;
	
	while ((c = *str++))	// да, здесь присвоение
	{
		lcd_putc (c);
	}
	
	return;
}

void lcd_setpos (uint8_t line, uint8_t pos)
{
	uint8_t ad = line * LCD_LINEWIDTH + pos;
	__lcd_set_ddram (ad);
	
	return;
}

LCD_POS lcd_getpos (void)
{
	LCD_POS pos;
	pos.line = __lcd_ac / LCD_LINEWIDTH;
	pos.pos = __lcd_ac % LCD_LINEWIDTH;
	
	return pos;
}

inline void lcd_clr (void)
{
	__lcd_write_byte (LCD_MODE_WR_CMD, 1 << LCD_CLR);
	
	return;
}

inline void lcd_home (void)
{
	__lcd_write_byte (LCD_MODE_WR_CMD, 1 << LCD_HOME);
	
	return;
}