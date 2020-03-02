/*
 * general.h
 *
 * Created: 19.11.2018 19:04:06
 *  Author: Vsevolod
 */ 


#ifndef GENERAL_H_
#define GENERAL_H_

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <stdlib.h>

void general_init (void);
void check_voltage(void);

//#define OSC_CAL_BYTE (void*) 0x000 // F_CPU = 1.0 MHz
//#define OSC_CAL_BYTE (void*) 0x001 // F_CPU = 2.0 MHz
//#define OSC_CAL_BYTE (void*) 0x002 // F_CPU = 4.0 MHz
//#define OSC_CAL_BYTE (void*) 0x003 // F_CPU = 8.0 MHz

#define OSC_DIV_FACTOR		2

#define CHECK_VOLT_PERIOD	1000
#define VOLT_WARNING		10.0


#endif /* GENERAL_H_ */