/*
 *      twi.c
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



/*
 * Copyright (c) 2004, Atmel Corporation
 *
 * The sourcecode was orginally from Atmel Application Note
 * -- AVR311: TWI Slave Implementation --
 *
 * Changes to make it compile with the GNU C Compiler with the avr-libc were made by Bernhard Walle
 *
 * -------------------------------------------------------------------------------------------------
 */
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/twi.h>

//#include <tfdebug.h>
#include "twi.h"

static unsigned char TWI_buf[TWI_BUFFER_SIZE];     // Transceiver buffer. Set the size in the header file
static unsigned char TWI_msgSize  = 0;             // Number of bytes to be transmitted.
static unsigned char TWI_state    = TWI_NO_STATE;  // State byte. Default set to TWI_NO_STATE.

union TWI_statusReg TWI_statusReg = {0};           // TWI_statusReg is defined in TWI_Slave.h


/*********************************************************************
 * Set up chip in master mode.
 *
 **********************************************************************/
unsigned char  TWI_Master_Initialise(unsigned char TWI_ownAddress)
{

  uint8_t   twst;

   TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

   while(!(TWCR & (1<<TWINT)));

  twst = TW_STATUS & 0xF8;
   if ( (twst != TW_START) && (twst != TW_REP_START))
      return 1;

   TWDR = TWI_ownAddress;
   TWCR = (1<<TWINT) | (1<<TWEN);

   while(!(TWCR & (1<<TWINT)));

   twst = TW_STATUS & 0xF8;
   if ( (twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK) )
      return 1;

}


/*********************************************************************
 * Call this function to set up the TWI slave to its initial standby state.
 * Remember to enable interrupts from the main application after initializing the TWI.
 * Pass both the slave address and the requrements for triggering on a general call in the
 * same byte. Use e.g. this notation when calling this function:
 * TWI_Slave_Initialise( (TWI_slaveAddress<<TWI_ADR_BITS) | (TRUE<<TWI_GEN_BIT) );
 * The TWI module is configured to NACK on any requests. Use a TWI_Start_Transceiver function to
 * start the TWI.
 * ---------------------------------------------------------------------------------------------- */
void TWI_Slave_Initialise( unsigned char TWI_ownAddress )
{
  TWAR = TWI_ownAddress;                            // Set own TWI slave address. Accept TWI General Calls.
  TWDR = 0xFF;                                      // Default content = SDA released.
  TWCR = (1<<TWEN)|                                 // Enable TWI-interface and release TWI pins.
         (0<<TWIE)|(0<<TWINT)|                      // Disable TWI Interupt.
         (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO);           // Do not ACK on any requests, yet.
                                //
}


/**
 * Call this function to test if the TWI_ISR is busy transmitting.
 * ---------------------------------------------------------------------------------------------- */
unsigned char TWI_Transceiver_Busy( void )
{
    // IF TWI interrupt is enabled then the Transceiver is busy
    return ( TWCR & (1<<TWIE) );
}


/**
 * Call this function to fetch the state information of the previous operation. The function will
 * hold execution (loop) until the TWI_ISR has completed with the previous operation. If there was
 * an error, then the function will return the TWI State code.
 * ---------------------------------------------------------------------------------------------- */
unsigned char TWI_Get_State_Info( void )
{
  while ( TWI_Transceiver_Busy() );             // Wait until TWI has completed the transmission.
  return ( TWI_state );                         // Return error state.
}


/**
 * Call this function to send a prepared message, or start the Transceiver for reception. Include
 * a pointer to the data to be sent if a SLA+W is received. The data will be copied to the TWI
 * buffer.  Also include how many bytes that should be sent. Note that unlike the similar Master
 * function, the Address byte is not included in the message buffers.
 * The function will hold execution (loop) until the TWI_ISR has completed with the previous operation,
 * then initialize the next operation and return.
 * ---------------------------------------------------------------------------------------------- */
void TWI_Start_Transceiver_With_Data(unsigned char *msg, unsigned char msgSize, unsigned char address)
{
    unsigned char temp;

    // Wait until TWI is ready for next transmission.
    while ( TWI_Transceiver_Busy() );
    TWDR = (address << TWI_ADR_BITS) | (0 << TWI_READ_BIT);
    // Number of data to transmit.
    TWI_msgSize = msgSize;

    // Copy data that may be transmitted if the TWI Master requests data.
    for ( temp = 0; temp < msgSize; temp++ )
    {
        TWI_buf[ temp ] = msg[ temp ];
    }

    TWI_statusReg.all = 0;
    TWI_state         = TWI_NO_STATE ;
    TWCR = (1<<TWEN)|                             // TWI Interface enabled.
           (1<<TWIE)|(1<<TWINT)|                  // Enable TWI Interupt and clear the flag.
           (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|       // Prepare to ACK next time the Slave is addressed.
           (0<<TWWC);                             //
}


/**
 * Call this function to start the Transceiver without specifing new transmission data.
 * Usefull for restarting a transmission, or just starting the transceiver for reception.
 * The driver will reuse the data previously put in the transceiver buffers. The function will
 * hold execution (loop) until the TWI_ISR has completed with the  previous operation, then
 * initialize the next operation and return.
* ----------------------------------------------------------------------------------------------- */
void TWI_Start_Slave_Transceiver( void )
{
    // Wait until TWI is ready for next transmission/reception.
    while ( TWI_Transceiver_Busy() );

    TWI_statusReg.all = 0;
    TWI_state         = TWI_NO_STATE ;
    TWCR = (1<<TWEN)|                             // TWI Interface enabled.
           (1<<TWIE)|(1<<TWINT)|                  // Enable TWI Interupt and clear the flag.
           (1<<TWEA)|(0<<TWSTO)|                  // Prepare to ACK next time the node is addressed.
           (0<<TWWC);                             //
}


/**
 * Call this function to read out the received data from the TWI transceiver buffer. I.e. first
 * call TWI_Start_Transceiver to get the TWI Transceiver to fetch data. Then Run this function to
 * collect the data when they have arrived. Include a pointer to where to place the data and
 * the number of bytes to fetch in the function call. The function will hold execution (loop)
 * until the TWI_ISR has completed with the previous operation, before reading out the data
 * and returning. If there was an error in the previous transmission the function will return
 * the TWI State code.
 * ---------------------------------------------------------------------------------------------- */
unsigned char TWI_Get_Data_From_Transceiver( unsigned char *msg, unsigned char msgSize )
{
    unsigned char i;

    // Wait until TWI is ready for next transmission.
    while ( TWI_Transceiver_Busy() );

    // Last transmission competed successfully.
    if( TWI_statusReg.lastTransOK )
    {
        // Copy data from Transceiver buffer.
        for ( i=0; i<msgSize; i++ )
        {
            msg[i] = TWI_buf[i];
            ////RINT_DEBUG_P("Data %d: %d\n", i, msg[i]);
        }

        // Slave Receive data has been read from buffer.
        TWI_statusReg.RxDataInBuf = false;
    }

    return TWI_statusReg.lastTransOK;
}


/**
 * This function is the Interrupt Service Routine (ISR), and called when the TWI interrupt is
 * triggered; that is whenever a TWI event has occurred. This function should not be called
 * directly from the main application.
 * ---------------------------------------------------------------------------------------------- */
SIGNAL(SIG_TWI)
{
    static unsigned char TWI_bufPtr;

    //RINT_DEBUG_P(PSTR("\nReceived data:\n"));
    switch (TWSR)
    {
        // Own SLA+R has been received; ACK has been returned
        case TWI_STX_ADR_ACK:
            //RINT_DEBUG_P(PSTR("TWI_STX_ADR_ACK\n"));
            // Set buffer pointer to first data location
            TWI_bufPtr   = 0;

        // Data byte in TWDR has been transmitted; ACK has been received
        case TWI_STX_DATA_ACK:
            //RINT_DEBUG_P(PSTR("TWI_STX_DATA_ACK\n"));
            TWDR = TWI_buf[TWI_bufPtr++];

            // TWI Interface enabled
            // Enable TWI Interupt and clear the flag to send byte
            TWCR = (1<<TWEN) | (1<<TWIE)|(1<<TWINT)| (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| (0<<TWWC);
            break;

        // Data byte in TWDR has been transmitted; NACK has been received.
        // I.e. this could be the end of the transmission.
        case TWI_STX_DATA_NACK:
            //RINT_DEBUG_P(PSTR("TWI_STX_DATA_NACK\n"));
            // Have we transceived all expected data?
            if (TWI_bufPtr == TWI_msgSize)
            {
                // Set status bits to completed successfully.
                TWI_statusReg.lastTransOK = true;
            }
            else
            {
                // Master has sent a NACK before all data where sent.
                // Store TWI State as errormessage.
                TWI_state = TWSR;
            }

            // Put TWI Transceiver in passive mode.
            // Enable TWI-interface and release TWI pins
            TWCR = (1<<TWEN)|
                   (0<<TWIE)|(0<<TWINT)|                // Disable Interupt
                   (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|     // Do not acknowledge on any new requests.
                   (0<<TWWC);
           break;

        // General call address has been received; ACK has been returned
        case TWI_SRX_GEN_ACK:
            //RINT_DEBUG_P(PSTR("TWI_SRX_GEN_ACK\n"));
            TWI_statusReg.genAddressCall = true;
        // Own SLA+W has been received ACK has been returned
        case TWI_SRX_ADR_ACK:
            //RINT_DEBUG_P(PSTR("TWI_SRX_ADR_ACK\n"));
            // Dont need to clear TWI_S_statusRegister.generalAddressCall due to that it is the default state.
            TWI_statusReg.RxDataInBuf = true;

            // Set buffer pointer to first data location
            TWI_bufPtr   = 0;

            // Reset the TWI Interupt to wait for a new event.

            // TWI Interface enabled
            // Enable TWI Interupt and clear the flag to send byte
            // Expect ACK on this transmission
            TWCR = (1<<TWEN)| (1<<TWIE)|(1<<TWINT)| (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)| (0<<TWWC);                                 //
        break;

    // Previously addressed with own SLA+W; data has been received; ACK has been returned
    // Previously addressed with general call; data has been received; ACK has been returned
    case TWI_SRX_ADR_DATA_ACK:
        //RINT_DEBUG_P(PSTR("TWI_SRX_ADR_DATA_ACK\n"));
    case TWI_SRX_GEN_DATA_ACK:
        //RINT_DEBUG_P(PSTR("TWI_SRX_ADR_DATA_ACK/TWI_SRX_GEN_DATA_ACK\n"));
        TWI_buf[TWI_bufPtr++] = TWDR;
        //RINT_DEBUG_P(PSTR("Data: %d\n"), TWI_buf[TWI_bufPtr-1]);
        // Set flag transmission successfull.
        TWI_statusReg.lastTransOK = true;

        // Reset the TWI Interupt to wait for a new event.
        TWCR = (1<<TWEN)|                          // TWI Interface enabled
               (1<<TWIE)|(1<<TWINT)|               // Enable TWI Interupt and clear the flag to send byte
               (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Send ACK after next reception
               (0<<TWWC);
        break;

    // A STOP condition or repeated START condition has been received while still addressed as Slave
    case TWI_SRX_STOP_RESTART:
        //RINT_DEBUG_P(PSTR("TWI_SRX_STOP_RESTART\n"));
        // Put TWI Transceiver in passive mode.
        TWCR = (1<<TWEN)|                          // Enable TWI-interface and release TWI pins
               (0<<TWIE)|(0<<TWINT)|               // Disable Interupt
               (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Do not acknowledge on any new requests.
               (0<<TWWC);
        break;

    // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
    case TWI_SRX_ADR_DATA_NACK:
        //RINT_DEBUG_P(PSTR("TWI_SRX_ADR_DATA_NACK\n"));
    // Previously addressed with general call; data has been received; NOT ACK has been returned
    case TWI_SRX_GEN_DATA_NACK:
        //RINT_DEBUG_P(PSTR("TWI_SRX_GEN_DATA_NACK\n"));
    // Last data byte in TWDR has been transmitted (TWEA = �0�); ACK has been received
    case TWI_STX_DATA_ACK_LAST_BYTE:
        //RINT_DEBUG_P(PSTR("TWI_STX_DATA_ACK_LAST_BYTE\n"));
    // Bus error due to an illegal START or STOP condition
    case TWI_BUS_ERROR:
        //RINT_DEBUG_P(PSTR("TWI_BUS_ERROR\n"));
    default:
        //RINT_DEBUG_P(PSTR("default\n"));
        // Store TWI State as errormessage, operation also clears the Success bit.
        //TWI_state = TWSR;

        // transmit a stop condition. With this, the master gets in a valid state again
        // (because he may not get an bus error)
        TWCR = (1<<TWEN)|                          // Enable TWI-interface and release TWI pins
               (1<<TWIE)|(1<<TWINT)|               // Disable Interupt
               (1<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|    // Do not acknowledge on any new requests.
               (0<<TWWC);

    }
    handle_twi(TWI_buf);
}


