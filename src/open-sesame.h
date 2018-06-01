/***********************************************************************
 *      @file
 *
 *      Copyright 2009 Mike Evans <mikee@saxicola.idps.co.uk>
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
 *
 * RFID reader and electomagnet controller.
 * Interfaces to PC and/or master atmega for display, control and timer
 * functions.
 * PC would be linked to display and would read via master atmel based
 * controller module.
 * This is a slave.  It reads RFID tags, energises electromagnet and sends
 * code to the master when polled.
 * Master would need to have a multiplexer for at least 20 RFID readers.
 **********************************************************************/

#ifndef SESAME_H
#define SESAME_H
#include <stdint.h>

enum  {READ,LEARN,SEND,MASTER,DELETE,STARTUP,BTTEST} mode;

 
void __premain(void);
void rfidloop(void);
void actionRFID(char* lrfid);
void unlock_door(void);
void handle_twi(unsigned char * data);

void actionRFID(char* id);

#endif
