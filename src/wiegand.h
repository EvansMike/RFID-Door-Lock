/*
 * wiegand.h
 * 
 * Copyright 2014 Mike Evans <mikee@saxicola.co.uk>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifndef WIEGAND_H
#define WIEGAND_H

#include "open-sesame.h"
#include "serial.h"


//uint8_t data_length = 0 ;
//uint32_t card_num = 0;


/**
 * Set up interupts and stuff.
 */
void wiegand_enable(void);
//static void wiegand_on(void);
//static void wiegand_off(void);

void send_data(uint32_t card_num);
int parity_check(uint32_t card_num);
#endif
