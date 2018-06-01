/**
 *      eeprom.c
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
 *
 *      Functions for reading and writing EEPROM on Atmega8.
 *
 **********************************************************************/


#include "eeprom.h"


/***********************************************************************
 * Write a byte to eeprom at the specified address
 * @param unsigned int uiAddress.  Address of data
 * @param unsigned char ucData.  Byte to be written
 **********************************************************************/
void
EEPROM_write_byte (unsigned int uiAddress, unsigned char ucData)
{
/* Wait for completion of previous write */
  while (EECR & (1 << EEWE))
    ;
/* Set up address and data registers */
  EEAR = uiAddress;
  EEDR = ucData;
/* Write logical one to EEMWE */
  EECR |= (1 << EEMWE);
/* Start eeprom write by setting EEWE */
  EECR |= (1 << EEWE);
}

/***********************************************************************
 * Read a byte from eeprom at the specified address
 * @param unsigned int uiAddress.  Address of data

 **********************************************************************/
unsigned char
EEPROM_read_byte (unsigned int uiAddress)
{
/* Wait for completion of previous write */
  while (EECR & (1 << EEWE))
    ;
/* Set up address register */
  EEAR = uiAddress;
/* Start eeprom read by writing EERE */
  EECR |= (1 << EERE);
/* Return data from data register */
  return EEDR;
}


/***********************************************************************
 * Write a byte to eeprom at the specified address
 * @param unsigned int uiAddress.  Start address of data
 * @param unsigned char ucData.  String to be written
 **********************************************************************/
void
EEPROM_write_string (unsigned int uiAddress, char *ucData)
{

}



/***********************************************************************
 * Read a string from eeprom starting at the specified address.
 * If lenght is zero, read until we see '\0'
 * Buffer MUST of sufficient size to contain data.
 * @param unsigned int uiAddress.  Start address of data
 * @param int length of string
 * @param char* buffer space to contain result of read
 **********************************************************************/
int
EEPROM_read_string (unsigned int uiAddress, int length, char *buffer)
{
  int chars = 0, n = 0;
  while (EEPROM_read_byte (uiAddress + n) != 0)
    {
      buffer[n] = EEPROM_read_byte ();
      uiAddress += 1;
      n++;
    }

  return int;
}
