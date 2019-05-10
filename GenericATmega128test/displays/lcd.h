/*
 * lcd.h
 *
 * Created: 30.03.2019 14:18:03
 *  Author: Vsevolod
 */ 

/*

Здесь предусматривается 4-битный режим работы!

Подключение проводов (слева направо): зелёный, 3 белых, кремовый, синий, фиолетовый

*/


#ifndef LCD_H_
#define LCD_H_

typedef struct {
	uint8_t line;
	uint8_t pos;
	} LCD_POS;


/*  Служебные функции  */
void __lcd_write_byte (uint8_t mode, uint8_t data);		// запись команд/данных в буфер
void __lcd_tx_routine (void);		// непосредственная запись в LCD
uint8_t __lcd_read_bf (void);		// возвращает  флаг занятости
uint8_t __lcd_read_ac (void);		// возвращает adress counter
void __lcd_set_ddram (uint8_t ad);	// прыг
void __lcd_busy_wait (void);		// тупое ожидание, но вроде быстрое; скорее всего, так и оставить
void __lcd_strobe (void);			// строб E 
/*************************/


/*  Функции для пользователя  */
void lcd_init (void);
int lcd_stdputc (char c, FILE *stream);		// для работы со стандартным выводом
void lcd_putc (char c);						// для простого вывода символа
void lcd_puts (char *str);					// вывод строки
void lcd_setpos (uint8_t line, uint8_t pos);// установить курсор в заданную позицию
LCD_POS lcd_getpos (void);					// возвращает структуру: строка+позиция курсора
void lcd_clr (void);						// очистка экрана. ВНИМАНИЕ! Выполняется ок. 1.6 мс!
void lcd_home (void);						// AC=0, shift=0. ВНИМАНИЕ! Выполняется ок. 1.6 мс!
/******************************/


// Команды HD44780 (позиция единицы, см. datasheet)

#define LCD_CLR                 0    // DB0: clear display

#define LCD_HOME                1    // DB1: return to home position

#define LCD_ENTRY_MODE          2    // DB2: set entry mode
#define LCD_ENTRY_INC           1    // DB1: 1=increment, 0=decrement
#define LCD_ENTRY_SHIFT         0    // DB0: 1=display shift on

#define LCD_DISPLAYMODE         3    // DB3: turn lcd/cursor on
#define LCD_DISPLAYMODE_ON      2    // DB2: turn display on
#define LCD_DISPLAYMODE_CURSOR  1    // DB1: turn cursor on
#define LCD_DISPLAYMODE_BLINK   0    // DB0: blinking cursor

#define LCD_MOVE                4    // DB4: move cursor/display
#define LCD_MOVE_DISP           3    // DB3: move display (0-> cursor)
#define LCD_MOVE_RIGHT          2    // DB2: move right (0-> left)

#define LCD_FUNCTION            5    // DB5: function set
#define LCD_FUNCTION_8BIT       4    // DB4: set 8BIT mode (0->4BIT mode)
#define LCD_FUNCTION_2LINES     3    // DB3: two lines (0->one line)
#define LCD_FUNCTION_10DOTS     2    // DB2: 5x10 font (0->5x7 font)

#define LCD_CGRAM_SET           6    // DB6: set CG RAM address
#define LCD_DDRAM_SET           7    // DB7: set DD RAM address

// Общие

#define LCD_DDRAM_SIZE			80
#define LCD_LINEWIDTH			40	// 80/2
#define LCD_LINEWIDTH_USED		16	// сколько символов в строке будем использовать	

#define LCD_BUF_SIZE			128	


// Режимы передачи/чтения
#define LCD_MODE_WR_CMD		0	// RS=0, RW=0
#define LCD_MODE_RD_BF		1	// RS=0, RW=1
#define LCD_MODE_WR_DATA	2	// RS=1, RW=0
#define LCD_MODE_RD_DATA	3	// RS=1, RW=1


#define LCD_DB_PORT		PORTG
#define LCD_DB_DDR		DDRG
#define LCD_DB4			PG0
#define LCD_DB5			PG1
#define LCD_DB6			PG2
#define LCD_DB7			PG3

#define LCD_DB_PIN		PING
#define LCD_DB4_PIN		PING0
#define LCD_DB5_PIN		PING1
#define LCD_DB6_PIN		PING2
#define LCD_DB7_PIN		PING3

#define LCD_E_PORT		PORTG
#define LCD_E_DDR		DDRG
#define LCD_E			PG4

#define LCD_RS_PORT		PORTC
#define LCD_RS_DDR		DDRC
#define LCD_RS			PC0

#define LCD_RW_PORT		PORTC
#define LCD_RW_DDR		DDRC
#define LCD_RW			PC1

// установка строба:
#define __LCD_E_LO		LCD_E_PORT &= ~(1 << LCD_E)
#define __LCD_E_HI		LCD_E_PORT |= (1 << LCD_E)

// установка RS
#define __LCD_RS_LO		LCD_RS_PORT &= ~(1 << LCD_RS)
#define __LCD_RS_HI		LCD_RS_PORT |= (1 << LCD_RS)

// установка RW
#define __LCD_RW_LO		LCD_RW_PORT &= ~(1 << LCD_RW)
#define __LCD_RW_HI		LCD_RW_PORT |= (1 << LCD_RW)

#define __LCD_DB_DDR_LO		LCD_DB_DDR	&= ~((1 << LCD_DB4)|(1 << LCD_DB5)|(1 << LCD_DB6)|(1 << LCD_DB7))
#define __LCD_DB_DDR_HI		LCD_DB_DDR |= (1 << LCD_DB4)|(1 << LCD_DB5)|(1 << LCD_DB6)|(1 << LCD_DB7)
#define __LCD_DB_PORT_LO	LCD_DB_PORT &= ~((1 << LCD_DB4)|(1 << LCD_DB5)|(1 << LCD_DB6)|(1 << LCD_DB7))
#define __LCD_DB_PORT_HI	LCD_DB_PORT |= (1 << LCD_DB4)|(1 << LCD_DB5)|(1 << LCD_DB6)|(1 << LCD_DB7)

// шина данных на ввод/отключена
#define __LCD_DB_HiZ	__LCD_DB_DDR_LO; __LCD_DB_PORT_LO
// шина данных на вывод
#define __LCD_DB_OUT	__LCD_DB_DDR_HI; __LCD_DB_PORT_LO


#endif /* LCD_H_ */