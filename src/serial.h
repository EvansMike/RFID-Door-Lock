/*
 *      serial.h
 *
 *      Copyright 2009 Mike Evans <mikee@saxicola.saxicola.idps.co.uk>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef SERIAL_H
#define SERIAL_H

#ifndef F_CPU
#error "CPU speed is not defined"
#endif

#include <stdio.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/portpins.h>
#include <avr/common.h>
#include <avr/pgmspace.h>



void USARTInit(void);
char USARTReadChar(void);
void USARTputc(char data);
void USARTputs(const char * str);
void USARTputs_P(const char * str);
int  USARTgets(char * buffer, char eos);
void ansi_cl(void);
void USARTputnum(uint32_t uint32);
char int2string (int number, int decimalPlaces, char * string);
#endif
