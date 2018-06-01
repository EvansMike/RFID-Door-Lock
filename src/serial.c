/*
 *      serial.c
 *
 * 		$Id: serial.c,v 1.3 2010/07/19 20:55:33 mikee Exp $
 *
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
 */




#include "serial.h"
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>


void ansi_cl(void)
{
    // ANSI clear screen: cl=\E[H\E[J
    USARTputc(27);
    USARTputc('[');
    USARTputc('H');
    USARTputc(27);
    USARTputc('[');
    USARTputc('J');
}

//This function is used to initialize the USART
//at a given UBRR value
void USARTInit()
{
#define BAUD 9600
#include <util/setbaud.h>
    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;
#if USE_2X
    UCSRA |= (1 << U2X);
#else
    UCSRA &= ~(1 << U2X);
#endif
#if 0
    char ubrr = 12; // From manual
    // set baud rate
    UBRRH = (uint8_t)(UART_BAUD_CALC(UART_BAUD_RATE,F_OSC)>>8);
    UBRRL = (uint8_t)UART_BAUD_CALC(UART_BAUD_RATE,F_OSC);
    /* Set baud rate */
    UBRRH = (unsigned char)(ubrr>>8);
    UBRRL = (unsigned char)ubrr;

    UCSRA |= (1 << U2X);
#endif
    // Enable receiver and transmitter; enable RX interrupt
    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);

    //asynchronous 8N1
    UCSRC = (1 << URSEL) | (3 << UCSZ0);

    return;

}


/**
 * This function is used to read the available data
 * from USART. This function will wait until data is available.
 * @return A single char.
*/
char USARTReadChar()
{
    //Wait until a data is available

    while(!(UCSRA & (1<<RXC)))
    {
        //Do nothing
    }

    //Now USART has got data from host
    //and is available is buffer

    return UDR;
}


/**
 * This fuction writes the given "data" to
 * the USART which then transmit it via TX line
 * @param char Single character to write.
 */
void USARTputc(char data)
{
    //Wait until the transmitter is ready

    while(!(UCSRA & (1<<UDRE)))
    {
        //Do nothing
    }
    //Now write the data to USART buffer
    UDR=data;
    data = UDR;
    data = 0;
}
/**
 * Send a string out the serial port
 * @param const char*. The string to send
 * */
void USARTputs(const char * str)
// loop until *str = 0, end of string
{   while (*str) {
        USARTputc(*str);
        str++;
    }
}

/**
 * Get a string from the serial port.
 * Loop until end_of_string char received
 * DANGER!  Will block untill EOS!!
 * @param char*  Pointer to buffer to hold string
 * @param char End of string character.
 */
int  USARTgets(char * buffer, char eos)
{
    unsigned char i = 0;
    while(buffer[i] != eos)
    {
        buffer[i]=USARTReadChar();
        i++;
    }
    USARTputc('\0');
    UDR = 0;
    return i;
}



/**
 * For strings in flash memory we need a slightly diffferent routine
 * @param const char* Pointer to a string in EEPROM
 */
void USARTputs_P(const char * str)
{
    while (pgm_read_byte(str))
        USARTputc(pgm_read_byte(str++));
}


/**
 *  Print a number out as a string
 * @param uint32_t  A 32bit number, or not.
 */
void USARTputnum(uint32_t uint32)
{
    char * messageBuf;
    messageBuf = malloc(sizeof(char) * 10);
    sprintf(messageBuf,"%lu\r\n", uint32 );
    //messageBuf = int2string (uint32, 0, messageBuf);
    USARTputs(messageBuf);
}
