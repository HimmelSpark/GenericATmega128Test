/*
 * _7seg_disp.h
 *
 * Created: 05.05.2018 13:31:38
 *  Author: Всеволод
 */ 


#ifndef _7SEG_DISP_H_
#define _7SEG_DISP_H_

/* Служебные функции */
void _7seg_hiZ_all (void);
void _7seg_set_seg (uint8_t seg);
void _7seg_set_pos (uint8_t pos);
void _7seg_clear_buff (void);
void _7seg_disp_char_from_buff (uint8_t pos);
void _7seg_disp_char_from_charset (uint8_t pos, uint8_t index);
/*********************/

/* Функции для пользователя */
void _7seg_init (void);
void _7seg_redraw (void);
int _7seg_stdputc (char c, FILE *stream);
void _7seg_putc (char c);
void _7seg_puts (char *str);
/****************************/

#define _7SEG_POSITIONS		4
#define _7SEG_REDRAW_PERIOD	5	// ms

#define POSPORT PORTC
#define POSDDR	DDRC
#define POS0	PC4
#define POS1	PC5
#define POS2	PC6
#define POS3	PC7

#define POS_OFFSET 4

#define SEGPORT PORTA
#define SEGDDR	DDRA
#define segA	0
#define segB	1
#define segC	2
#define segD	3
#define segE	4
#define segF	5
#define segG	6
#define segDL	7

#define DP1		1
#define DP2		3
#define DASH	2

#define DIG_ADDR_OFFSET (-45)
#define LET_ADDR_OFFSET (-83)

#define CHARSET_SIZE 39

#define chSP 0x00
#define chHYPH (1 << segG)
#define chDP (1 << segDL)
#define ch0 ((1 << segA)|(1 << segB)|(1 << segC)|(1 << segD)|(1 << segE)|(1 << segF))
#define ch1 ((1 << segB) | (1 << segC))
#define ch2 ((1 << segA)|(1 << segB)|(1 << segG)|(1 << segE)|(1 << segD))
#define ch3 ((1 << segA)|(1 << segB)|(1 << segG)|(1 << segC)|(1 << segD))
#define ch4 ((1 << segF)|(1 << segG)|(1 << segB)|(1 << segC))
#define ch5 ((1 << segA)|(1 << segF)|(1 << segG)|(1 << segC)|(1 << segD))
#define ch6 ((1 << segA)|(1 << segF)|(1 << segG)|(1 << segC)|(1 << segD)|(1 << segE))
#define ch7 ((1 << segA)|(1 << segB)|(1 << segC))
#define ch8 ((1 << segA)|(1 << segB)|(1 << segC)|(1 << segD)|(1 << segE)|(1 << segF)|(1 << segG))
#define ch9 ((1 << segA)|(1 << segB)|(1 << segG)|(1 << segF)|(1 << segC)|(1 << segD))
#define chDASH (1 << segDL)
#define chA ((1 << segE)|(1 << segF)|(1 << segA)|(1 << segB)|(1 << segC)|(1 << segG))
#define chB ((1 << segC)|(1 << segD)|(1 << segE)|(1 << segF)|(1 << segG))
#define chC ((1 << segA)|(1 << segF)|(1 << segE)|(1 << segD))
#define chD ((1 << segB)|(1 << segC)|(1 << segD)|(1 << segE)|(1 << segG))
#define chE ((1 << segA)|(1 << segF)|(1 << segG)|(1 << segE)|(1 << segD))
#define chF ((1 << segE)|(1 << segF)|(1 << segA)|(1 << segG))
#define chG ((1 << segA)|(1 << segF)|(1 << segE)|(1 << segD)|(1 << segC))
#define chH ((1 << segF)|(1 << segE)|(1 << segG)|(1 << segC))
#define chI (1 << segE) // (1 << segC)
#define chL ((1 << segF)|(1 << segE)|(1 << segD))
#define chN ((1 << segE)|(1 << segG)|(1 << segC))
#define chO ((1 << segE)|(1 << segG)|(1 << segC)|(1 << segD))
#define chP ((1 << segE)|(1 << segF)|(1 << segA)|(1 << segB)|(1 << segG))
#define chQ ((1 << segA)|(1 << segB)|(1 << segC)|(1 << segF)|(1 << segG))
#define chR ((1 << segE)|(1 << segG))
#define chS ((1 << segA)|(1 << segF)|(1 << segG)|(1 << segC)|(1 << segD))
#define chT ((1 << segF)|(1 << segE)|(1 << segD)|(1 << segG))
#define chU ((1 << segE)|(1 << segD)|(1 << segC))
#define chY ((1 << segF)|(1 << segG)|(1 << segB)|(1 << segC)|(1 << segD))

#define TREAT_UNDEF_CHAR_AS_SP


#endif /* _7SEG_DISP_H_ */