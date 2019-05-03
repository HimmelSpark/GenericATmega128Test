/*
 * _7seg_disp.c
 *
 * Created: 05.05.2018 13:32:40
 *  Author: Всеволод
 */ 

#include "../general.h"
#include "_7seg_disp.h"
#include "../rtos.h"

uint8_t charset[CHARSET_SIZE] = {
	chSP, chHYPH, chDP,
	ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, chDASH,
	chA, chB, chC, chD, chE, chF, chG, chH, chI, chSP, chSP, chL, chSP,
	chN, chO, chP, chQ, chR, chS, chT, chU, chSP, chSP, chSP, chY
};

uint8_t buff[_7SEG_POSITIONS] = {0x00, 0x00, 0x00, 0x00}; // что выводим (маски символов); младший разряд слева

uint8_t dl_flag = 0; // младший разряд не оказывает влияния, т.к. у позиции0 нет разделителя;
// могут быть заняты разряды 1, 2, 3 в силу наличия у соответствующих позиций разделителей
// Разряд 1 -> DP1, разряд 2 -> DASH, разряд 3 -> DP2

inline void _7seg_init (void)
{
	_7seg_hiZ_all ();
//	rtos_set_task (_7seg_redraw, RTOS_RUN_ASAP, _7SEG_REDRAW_PERIOD);
	_7seg_puts ("hola\n");
	
	return;
}

inline void _7seg_hiZ_all (void)
{
	SEGDDR = 0x00; // весь порт, можно присвоением
	SEGPORT = 0x00;
	POSDDR &= 0x0F; // гасим пины 7, 6, 5, 4
	POSPORT &= 0x0F;
	
	return;
}

inline void _7seg_set_seg (uint8_t seg)
{
	SEGPORT |= 1 << seg;
	SEGDDR |= 1 << seg;
	
	return;
}

inline void _7seg_set_pos (uint8_t pos)
{
	POSDDR |= 1 << (pos);
	return;
}

inline void _7seg_clear_buff (void)
{
	dl_flag = 0x00; // сброс флага разделителей, т.к. он по смыслу связан с буфером
	for (int i = 0; i < _7SEG_POSITIONS; i++)
	{
		buff[i] = 0x00;
	}

	return;
}

inline void _7seg_disp_char_from_charset (uint8_t pos, uint8_t index)
{
	_7seg_hiZ_all ();
	_7seg_set_pos (pos + POS_OFFSET);
	SEGDDR = SEGPORT = charset[index];
	
	return;
}

void _7seg_redraw (void)
{
	static uint8_t pos = _7SEG_POSITIONS;
	_7seg_disp_char_from_buff (--pos);
	
	if (pos == 0)
	{
		pos = _7SEG_POSITIONS;
	}
	
	return;
}

inline void _7seg_disp_char_from_buff (uint8_t pos)
{
	_7seg_hiZ_all ();
	_7seg_set_pos (pos + POS_OFFSET);
	SEGDDR = SEGPORT = buff[pos];
	
	if (dl_flag & (1 << pos)) // если на этой позиции оказался разделитель
	{
		_7seg_set_seg (segDL); // (на 0 позиции не окажет влияния)
	}
	
	return;
}

int _7seg_stdputc (char c, FILE *stream)
{
	uint8_t index = 0;
	static uint8_t pos = _7SEG_POSITIONS - 1; // пишем со старшего разряда, т.к. printf
	// выдаёт символы строки слева направо

	if ((pos == _7SEG_POSITIONS - 1) && (c != '\n')) // когда начали писать новую строку
	{
		_7seg_clear_buff ();	// очищаем буфер
	}
	
	if (c == '\n')
	{
		pos = _7SEG_POSITIONS - 1; // возврат каретки
		return 0;
	}
	
	/********* ОБРАБОТКА РАЗДЕЛИТЕЛЕЙ *********/
	if (c == '.')
	{
		if ((pos+1) == 1)
		{
			dl_flag |= 1 << DP1;
		}
		else if ((pos+1) == 2)
		{
			dl_flag |= 1 << DP2;
		}
		return 1;
	}
	else if (c == ':')
	{
		if ((pos+1) == 2)
		{
			dl_flag |= 1 << DASH;
		}
		return 1;
	}
	// здесь везде (pos+1), т.к. pos декрементируется после вывода символа, а т.к. **неожиданно** следующим
	// символом может последовать разделитель, относящийся к **предыдущему** разряду, нужно с предыдущим
	// и сравнивать
	
	/*******************************************/
	
	if (c == ' ')
	{
		index = 0;
	}
	else if (c == '-')
	{
		index = 1;
	}
	else if ((c >= '0') && (c <= '9'))
	{
		index = c + DIG_ADDR_OFFSET;
	}
	else if ((c >= 'a') && (c <= 'y'))
	{
		index = c + LET_ADDR_OFFSET;
	}
	else // не знаю такого символа!
	{
		#ifdef TREAT_UNDEF_CHAR_AS_SP
			index = 0;	// получите пробел
		#else
			return 1;	// идите на фиг
		#endif
	}

	buff[pos] = charset[index];
	
	pos--;
	
	if (pos == 0xFF) // конец строки
	{
		pos = _7SEG_POSITIONS - 1;
	}
	
	return 0;
}

inline void _7seg_putc (char c)
{
	_7seg_stdputc (c, NULL);
	return;
}

inline void _7seg_puts (char *str)
{
	register char c;
	
	while ((c = *str++))
	{
		_7seg_putc (c);
	}
	
	return;
}
