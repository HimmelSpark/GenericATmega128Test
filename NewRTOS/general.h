/*
 * hardware.h
 *
 * Created: 19.11.2018 19:04:06
 *  Author: Vsevolod
 */ 


#ifndef GENERAL_H_
#define GENERAL_H_

#define F_CPU 4000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>

extern void general_init (void);

//#define OSC_CAL_BYTE 0x00 // F_CPU = 1.0 MHz
//#define OSC_CAL_BYTE 0x01 // F_CPU = 2.0 MHz
#define OSC_CAL_BYTE 0x02 // F_CPU = 4.0 MHz
//#define OSC_CAL_BYTE 0x03 // F_CPU = 8.0 MHz

#define FOREVER 1

#endif /* HARDWARE_H_ */