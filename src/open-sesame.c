/***********************************************************************
 *      @file
 *
 *      Copyright 2014 Mike Evans <mikee@saxicola.co.uk>
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
 *
 * OUTPUTS:
 * PC3 - LED GREEN
 * PC0 - LED RED
 * PC5 - RELAY_ DOOR
 *
 * INPUTS:
 * RX - Bluetooth module direct
 * TX - Bluetooth via Diode and pullup to 3.3V
 *
 * PD2 - WIEGAND-0 RFID Reader
 * PD3 - WIEGAND-1 RFID Reader
 *
 *
 *
 **********************************************************************/



#include "open-sesame.h"
#include "serial.h"
#include "wiegand.h"
#include <util/delay.h>
#include "string.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <avr/power.h>
#include <avr/sleep.h>


#define MSG_LENGTH 20       // Max allowable output message length.
#define NODE_ID    0x10     // 0100110
#define TWI 0

#define RESET() (((void(*)(void))(char *)0x0000)())
int card_present = 0;
#define nids  10 // Number of cards to store

#define idlen 10 // Length of each card data string
char EEMEM EE_pin[5];
char pin[5];
void *stored_rfid[nids][idlen] EEMEM;
uint32_t used_rfids[10] EEMEM;

char * EOM = "###\r\n"; // End Of Message marker

unsigned char messageBuf[4];

#define read_eeprom_word(address) eeprom_read_word ((const uint32_t*)address)
#define update_eeprom_dword(address,value) eeprom_update_dword ((uint32_t*)address,(uint32_t)value)

#define IOPORT PORTC
#define DDR_IOPORT DDRC
#define LED_G PC3
#define LED_R PC0
#define RELAY PC5
#define SWITCH PC4
#define READER_LED PD5
#define READER_BEEP PD6

// Next two are defined in wiegand.c, here for reference.
#define W0 PD2
#define W1 PD3

// Static function definitions.
static void list_recent(void);
static void access_allowed(void);
static void access_denied(void);
static void card_added(void);
static void card_removed(void);
static void wait_new_card(void);
static int delete_card(char idx);
static void list_cards(void);
//static void check_button_status(void);
static void master_reset (void);


/***********************************************************************
 * Functions/methods starts here
 **********************************************************************/


/***********************************************************************
 * Respond to a char sent on the serial line
 **********************************************************************/
//SIGNAL (SIG_UART_RECV) POISONED!!! DO NOT USE

ISR (USART_RXC_vect)
{
    char *tmp;
    //static char RXBuf[MSG_LENGTH];
    //static int rxindex = 0;   // receive buffer index
    // store received character in receive buffer
    char key;
    key = USARTReadChar();
    switch (key)
    {
#if 1
    case 'h':
        tmp = malloc(75 * sizeof(char));
        sprintf(tmp, "Compiled at: %s " " %s.\r\n",__DATE__, __TIME__);
        USARTputs (tmp);
        USARTputs(EOM);
        free(tmp);
        break;
#endif
    case 'l': // List all the key card numbers to serial
        list_cards();
        break;
    case 'd':
        list_cards();
        mode = DELETE;
        USARTputs("Input card idx number.\r\n");
        USARTputs(EOM);
        break;
    case 'r': // List recently used cards
        list_recent();
        break;
    case '0' ... '9':
        if (mode == DELETE)delete_card(key);
        break;
    case 's':
        if (mode == DELETE)mode = READ;
        access_allowed();
        break;
    default:
        USARTputc (key);
        break;
    }
}


/***********************************************************************
 * List all the keycards to serial line in the form:
 * idx num, card num, timestamp added.
 * NB: Timestamp would be nice except there's no RTC in the Atmega
 **********************************************************************/
static void list_cards()
{
    char *buffer;
    uint8_t n;
    buffer = malloc (sizeof (char) * idlen);
    USARTputs ("\r\n");
    USARTputs ("idx, number\r\n");
    for (n = 0; n < nids; n++) // 0 is the master card
        {
            sprintf(buffer, "%d, ",n);
            USARTputs (buffer);
            eeprom_read_block (buffer, stored_rfid[n], idlen);
            USARTputs (buffer);
            USARTputs ("\r\n");
        }
    USARTputs(EOM);
    free(buffer);
    return;

}


/* List recently used card. */
static void list_recent()
{

}


/***********************************************************************
 * Delete a card at idx from the database
 * @param idx is the char code NOT the index so we hace to convert.
 * @return On success the idx of the deleted card else -1.
 **********************************************************************/
static int delete_card(char idx)
{
    char *buffer;
    uint8_t n;
    buffer = "Card %d deleted.\r\n  ";
    list_cards();
    n = idx - '0';
    mode = READ;
    eeprom_update_block("",stored_rfid[n], idlen);
    sprintf(buffer, "Card %d deleted.\r\n",n);
    USARTputs(buffer);
    USARTputs ("####");
    list_cards();
    return idx;
}


/***********************************************************************
 * Read the RFID from chip.
 * If this is first run, store the first card as the master card.
 * Else, if it's the master card, add the next card as a user card,
 * unless the user card already exists in which case we remove the card.
 * If the card is a user card, unlock the door.
 * TODO Add reset button to clear master card, or all cards.
 **********************************************************************/
void actionRFID (char *rfid)
{
    int n;
    enum
    { LOCKED, OPEN, DENIED, GRANTED } condition;
    char *buffer;
    condition = LOCKED;
    buffer = malloc (1 + (sizeof (char) * idlen));
    USARTputs ("\r\n");
    USARTputs (rfid);
    USARTputs(EOM);
    if (mode == READ)
    {
        // First see if this is a master RFID
        eeprom_read_block (buffer, stored_rfid[0], idlen);
        // Is this the first card ever presented, AKA a new master?
        if (strncmp ("", buffer, idlen) == 0)
        {
            eeprom_write_block (rfid, stored_rfid[0], idlen);
            USARTputs ("New master ID stored.\r\n");
            USARTputs (rfid);
            //USARTputs (EOM);
            card_added ();
            mode = READ;

            // As it's the master card we also use it's number as the bluetooth PIN
            // Get the last 4 digits of the card serial number.
            n = 0;
            for( int i = strlen(rfid)-4; i !=strlen(rfid) ; i++)
            {
                pin[n] = rfid[i];
                n++;
            }
            pin[n] = 0;
            // Store this EEPROM, although there's no need as it gets set in the HC-06
            eeprom_write_block (pin, EE_pin, 5);
            strcpy(buffer,"AT+PIN");
            strcat(buffer, pin);
            // Send the command
            USARTputs(buffer);
            free (buffer);
            rfid = 0;
            return;
        }
        if (strncmp (rfid, buffer, idlen) == 0)
        {
            USARTputs ("You are the master.  Present a new card.\r\n");
            USARTputs(EOM);
            mode = MASTER;
            wait_new_card ();   // TODO need to use timer for this
            free (buffer);
            return;
        }
        // In normal run mode
        for (n = 1; n < nids; n++)
        {
            eeprom_read_block (buffer, stored_rfid[n], idlen);
            if (strncmp (rfid, buffer, idlen) == 0)
            {
                condition = OPEN;
            }
        }
        if (condition == OPEN)
        {
            access_allowed ();
            condition = LOCKED;
            free (buffer);
            return;
        }
        else if (condition == LOCKED)
        {
            access_denied ();
            condition = LOCKED;
            free (buffer);
            return;
        }

    }
    if (mode == MASTER)
    {
        // If it's the master card again just exit, after sw off LEDS
        eeprom_read_block (buffer, stored_rfid[0], idlen);
        if (strncmp (rfid, buffer, idlen) == 0)
        {
            USARTputs ("Action cancelled.\r\n");
            USARTputs(EOM);
            card_removed ();
            mode = READ;
            free (buffer);
            return;
        }
        // Find first empty slot
        for (n = 1; n < nids; n++)
        {
            eeprom_read_block (buffer, stored_rfid[n], idlen);
            // If a match is found we remove it, or do nothing!
            if (strncmp (rfid, buffer, idlen) == 0)
            {
                _EEPUT (stored_rfid[n], 0);
                //eeprom_write_block ("", stored_rfid[n], idlen);
                USARTputs ("Removed this card.\r\n");
                USARTputs(EOM);
                card_removed ();
                mode = READ;
                free (buffer);
                return;
            }
            // if not there then add it
            else if (strncmp ("", buffer, idlen) == 0)
            {
                USARTputs ("Found space, saving new ID.\r\n");
                eeprom_write_block (rfid, stored_rfid[n], idlen);
                USARTputs ("New tag ID stored.\r\n");
                USARTputs(EOM);
                card_added ();
                mode = READ;
                free (buffer);
                return;
            }

        }


        USARTputs ("No more space for new IDs!\r\n");
        USARTputs(EOM);
        mode = READ;
    }
    free (buffer);
}


/*********************************************************************
 * Various LED flash functions for user feedback.
 *
 ******************************************************************* */
/* Red 3 seconds. */
static void access_denied (void)
{
    USARTputs ("ACCESS DENIED\r\n");
    USARTputs (EOM);
    IOPORT |= (1 << LED_R); //&= ~0x0F;//led ON and relay ON
    //PORTD &= ~_BV(READER_BEEP); // ON
    _delay_ms (1000);
    //PORTD |= _BV(READER_BEEP); // OFF
    _delay_ms (1000);
    //PORTD &= ~_BV(READER_BEEP); // ON
    _delay_ms (1000);//TODO Uncomment for real thing
    _delay_ms (1000);
    //PORTD |= _BV(READER_BEEP); // OFF
    IOPORT &= ~(1 << LED_R);    //0x40;//led OFF
    USARTputs ("TRY AGAIN?\r\n");
    USARTputs(EOM);
}


/* Light green LED for 3 seconds and activate relay. */
static void access_allowed (void)
{

    USARTputs ("DOOR OPEN\r\n");
    USARTputs(EOM);
    // Light GREEN LED
    IOPORT |= (1 << LED_G); //&= ~0x0F;//led ON
    IOPORT |= (1 << RELAY); //and relay ON
    //PORTD &= ~_BV(READER_LED); // ON
    _delay_ms (1000); //TODO Uncomment for real thing
    _delay_ms (1000);
    _delay_ms (1000);
    IOPORT &= ~(1 << LED_G);    //0x40;//led OFF
    IOPORT &= ~(1 << RELAY);    //RElay OFF
    //PORTD |= _BV(READER_LED); // OFF
    USARTputs ("DOOR LOCKED\r\n");
    USARTputs(EOM);
}


/* Flash red for 3 seconds. */
static void card_removed (void)
{
    int n;
    IOPORT &= ~(1 << LED_G);    //Green led OFF
    for (n = 0; n < 10; n++)
    {
        IOPORT |= (1 << LED_R); //0x00;//Red led ON
        _delay_ms (100);
        IOPORT &= ~(1 << LED_R);    //0xE0;//Red led OFF
        _delay_ms (100);
    }
    PORTD &= ~(1 << LED_R); //led OFF
}


/* Flash green for 3 seconds. */
static void card_added (void)
{
    int n;
    IOPORT &= ~(1 << LED_R);    //Red led OFF
    for (n = 0; n < 10; n++)
    {
        IOPORT |= (1 << LED_G); //0x00;//Green led ON
        _delay_ms (100);
        IOPORT &= ~(1 << LED_G);    //0xE0;//Green led OFF
        _delay_ms (100);
    }
    IOPORT &= ~(1 << LED_G);    //0xE0;//Green led OFF
    IOPORT &= ~(1 << LED_R);    //0xE0;//Red led OFF
}


/* Wait for a new card to be scanned.
 * TODO: This needs to be a 5 second timeout that can scan AND flash at
 * the same time
 */
static void wait_new_card (void)
{
    IOPORT |= (1 << LED_G); //led ON
    IOPORT |= (1 << LED_R); //led ON
}


/* If the master reset button is pressed remove all the programmed cards.
 Connect switch to PORTD6, use 10K pull-up and switch to GND.
 * Press while boot LEDS are flashing .
 * May not use this bu leaving it in anyway.
 */
static void master_reset (void)
{
    return;
    int mr_sw, n;
    //char *null;
    //null = "";
    mr_sw = (PIND & (1 << PD5));
    if (mr_sw == 0)
    {
        for (n = 0; n < nids; n++)
        {
            _EEPUT (stored_rfid[n], 0);
            //eeprom_write_block (null, stored_rfid[n], idlen);
        }
        // Now flash alternate LEDS
        for (n = 0; n < 10; n++)
        {
            IOPORT &= ~(1 << LED_G);    //led ON
            _delay_ms (100);
            IOPORT |= (1 << LED_G); //0xE0;//led OFF
            IOPORT &= ~(1 << LED_R);    //led ON
            _delay_ms (100);
            IOPORT |= (1 << LED_R); //0xE0;//led OFF
            _delay_ms (100);
        }
        USARTputs ("ALL CARDS RESET!\r\n");
        USARTputs(EOM);
        //Set up Bluetooth with default PIN
        USARTputs ("AT+PIN0000");
        RESET ();
    }
}


/***********************************************************************
* Check the status of THE BUTTON and do something useful .
* The button(s) lives on PD6
* Button function depends on the machine state at the time of button press
*
static void check_button_status()
{
    uint8_t button_status, n;
    button_status = PIND & 0xff;  //FIX ME
    switch(button_status)
    {
        case 1:
            if (mode == STARTUP){ // Do the special startup things
                // Like a full system reset and card clear
                // Write empty strings into EEPROM
                for (n = 1; n < nids; n++)
                    eeprom_write_block ("", stored_rfid[0], idlen);
                break;
            }else{ // Do the ordinary things
            // Like open the door.
            access_allowed();
            break;
            }
        default:
            break;
    }
    return;
}
*/

/***********************************************************************
 * MAIN YAAYYYY
 *
 **********************************************************************/
int
main (void)
{
    short temper;
    char *tmp;
    // Set up PORTD
    //DDRD = _BV(READER_BEEP) | _BV(READER_LED);
    //PORTD = _BV(READER_BEEP) | _BV(READER_LED);
    // Set PD6 pullup
    PORTD |= _BV(PD6);
    tmp = malloc(50 * sizeof(char));
    /* Initiatl UART */
    USARTInit ();
    strcat(tmp, "Compiled at: ");
    strcat(tmp,__DATE__);
    strcat(tmp," ");
    strcat(tmp,__TIME__);
    strcat(tmp,"\r\n");
    USARTputs (tmp);
    USARTputs(EOM);
    free(tmp);
    eeprom_read_block(&pin, &EE_pin, sizeof(char)*4);
    //USARTputs ("Welcome.  From your front door lock.\n\r");

// Main loop
    sei ();         // enable interrupts

// Flash all LEDs on startup
    mode = STARTUP; // Special startup mode
    for (temper = 0; temper < 10; temper++)
    {
        // Check if the factory reset button in pressed
        if (PIND & (1 << PD6) == 0) // Then switch is pressed
        {
            master_reset();
        }
        IOPORT |= ((1 << LED_G) | (1 << LED_R));        //led ON
        _delay_ms (100);
        //check_button_status();
        IOPORT &= ~(1 << LED_G) & ~(1 << LED_R);        //led OFF
        _delay_ms (100);
        //check_button_status();
    }
    mode = READ; // Into ordinary run mode
    //master_reset(); // Only resets if button is pressed during start LEDs.

    wiegand_enable ();      //

    // Main loop
    for (;;)            // Forever
    {
        // Check status of manual entry switch.
        //check_button_status();
//sleepNow();
    }



}
