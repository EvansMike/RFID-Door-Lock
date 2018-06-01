/**
 *      spi.c
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

#include "spi.h"

// SPI macros
#define SPI_CLEAR_SS PORT_SPI &= ~(1<<DD_SS)
#define SPI_SET_SS PORT_SPI |= (1<<DD_SS)

void SPI_MasterInit(void)
{
   // Set MOSI, SS and SCK output, all others input
   DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
   // set SS signal
   SPI_SET_SS;
   // Enable SPI, Master, set clock rate
   SPCR = (1<<SPE)|(1<<MSTR);
   SPSR |= (1<<SPI2X);
}

void SPI_MasterTransmit(unsigned char cData)
{
   // Start transmission
   SPDR = cData;
   // Wait for transmission complete
   while(!(SPSR & (1<<SPIF)));
}

unsigned char SPI_MasterReceive()
{
   // Start transmission
   SPDR = 0x00;
   // Wait for transmission complete
   while(!(SPSR & (1<<SPIF)));
   // return received data
   return SPDR;
}
