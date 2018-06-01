/*
 * wiegand.c
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

/*
This is for ATMega8 only.

The external interrupts are triggered by the INT0, and INT1 pins. Observe that, if enabled, the
interrupts will trigger even if the INT0..1 pins are configured as outputs. This feature provides a
way of generating a software interrupt. The external interrupts can be triggered by a falling or ris-
ing edge or a low level. This is set up as indicated in the specification for the MCU Control
Register – MCUCR. When the external interrupt is enabled and is configured as level triggered,
the interrupt will trigger as long as the pin is held low. Note that recognition of falling or rising
edge interrupts on INT0 and INT1 requires the presence of an I/O clock, described in “Clock
Systems and their Distribution” on page 25. Low level interrupts on INT0/INT1 are detected
asynchronously. This implies that these interrupts can be used for waking the part also from
sleep modes other than Idle mode. The I/O clock is halted in all sleep modes except Idle mode.
Note that if a level triggered interrupt is used for wake-up from Power-down mode, the changed
level must be held for some time to wake up the MCU. This makes the MCU less sensitive to
noise. The changed level is sampled twice by the Watchdog Oscillator clock. The period of the
Watchdog Oscillator is 1 μs (nominal) at 5.0V and 25°C. The frequency of the Watchdog Oscilla-
tor is voltage dependent as shown in “Electrical Characteristics” on page 242. The MCU will
wake up if the input has the required level during this sampling or if it is held until the end of the
start-up time. The start-up time is defined by the SUT Fuses as described in “System Clock and
Clock Options” on page 25. If the level is sampled twice by the Watchdog Oscillator clock but
disappears before the end of the start-up time, the MCU will still wake up, but no interrupt will be
generated. The required level must be held long enough for the MCU to complete the wake up to
trigger the level interrupt.

INT0 = PD2 = pin4 = DATA0
INT1 = PD3 = pin5 = DATA1

* Wiegand uses two data lines, one for the zeros, the other for ones.  The first and last bits
* of the data are parity bits for half the data.  First half is even parity, second
* half is odd parity.
*
*/
#include "wiegand.h"
#include <stdlib.h>
#include <stddef.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#if  MCU != atmega8
#error Wrong MCU.
#endif

uint8_t data_length = 0 ;
uint32_t card_num = 0;
uint8_t active = 1;

// Define static functions here.
static void wiegand_on(void);
static void wiegand_off(void);

/**
 * Set up interupts and stuff.
 */
void
wiegand_enable ()
{
    SFIOR = 0;
    // Make sure we're set to inputs
    DDRD = 0b00000000;
    // Enable internal pullups by writing to the pins
    PORTD = 0b00001100;
    //Set PCINT0 bit in pin change mask register PCMSK0
    MCUCR |= (1 << ISC11) | (1 << ISC01);
    GICR |= (1 << INT0) | (1 << INT1);
    SREG |= 0b10000000;
    sei ();
}


/*
 * Re enable the reader
 * We need to clear the interrupt flags as this may be set while the interrupts 
 * were disabled and this means the ISR wil run as soon as we re-enable the interrupts.
 */
void wiegand_on ()
{
    GIFR = (1 << INTF0) | (1 << INTF1); // Clear flags
    GICR |= (1 << INT0) | (1 << INT1);
    active = 1;
    data_length = 0 ;
}

void wiegand_off()
{
    GICR = 0;
    active = 0;
}
// The reader has sent a 1
ISR (INT1_vect)
{
    if(!active)return;
    USARTputs("1");
    card_num = card_num << 1;
    card_num |= 1;
    data_length++;
    if (data_length >= 26)send_data(card_num);

}
// The reader has sent a zero
ISR (INT0_vect)
{
    if(!active)return;
    USARTputs("0");
    card_num = card_num << 1;
    data_length++;
    if (data_length >= 26)send_data(card_num);
}

// Send data back to the function that stores the data. 
void send_data(uint32_t card_num)
{
    char * messageBuf;
    //uint32_t site_code,serial_number;
    //USARTputs("\r\n");
    // First and last bits are parity get rid of 'em
    //site_code = (card_num >> 17) & 0xff;
    //serial_number = (card_num >> 1) & 0x7fff;
    card_num = (card_num >> 1) & 0xffffff;
    messageBuf = malloc(sizeof(char) * 20);
    sprintf(messageBuf,"%lu", card_num );
    wiegand_off();
    actionRFID (messageBuf);
    wiegand_on();
    card_num = '\0';
    data_length = 0;
    free(messageBuf);
}

