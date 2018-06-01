/**
 *      eeprom.h
 *
 *      Copyright 2010 Mike Evans <mikee@millstreamcomputing.co.uk>
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

#ifndef EEPROM_H
#define EEPROM_H

#include <avr/io.h>
#include <avr/sfr_defs.h>

void EEPROM_write_byte(unsigned int uiAddress, unsigned char ucData);
unsigned char EEPROM_read_byte(unsigned int uiAddress);
void EEPROM_write_string(unsigned int uiAddress, char *ucData);


#endif
