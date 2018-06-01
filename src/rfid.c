/***********************************************************************
 *      main.c
 *
 * 		$Id: main.c,v 1.6 2010/02/09 15:34:41 mikee Exp $
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
 **********************************************************************/
// RFID reader ID-12 for Arduino
// Based on code by BARRAGAN
// and code from HC Gilje - http://hcgilje.wordpress.com/resources/rfid_id12_tagreader/
// Modified for Arudino by djmatic
// Modified for ID-12 and checksum by Martijn The - http://www.martijnthe.nl/
//
// Use the drawings from HC Gilje to wire up the ID-12.
// Remark: disconnect the rx serial wire to the ID-12 when uploading the sketch

#define ledPin 13
byte pinState = 0;

void setup() {
Serial.begin(9600);                                 // connect to the serial port
pinMode(ledPin, OUTPUT);
}

void loop () {
byte i = 0;
byte val = 0;
byte code[6];
byte checksum = 0;
byte bytesread = 0;
byte tempbyte = 0;

if(Serial.available() > 0) {
if((val = Serial.read()) == 2) {                  // check for header
bytesread = 0;
while (bytesread < 12) {                        // read 10 digit code + 2 digit checksum
if( Serial.available() > 0) {
val = Serial.read();
if((val == 0×0D)||(val == 0×0A)||(val == 0×03)||(val == 0×02)) { // if header or stop bytes before the 10 digit reading
break;                                    // stop reading
}

// Do Ascii/Hex conversion:
if ((val >= ‘0′) && (val <= '9')) {
val = val - '0';
} else if ((val >= ‘A’) && (val <= 'F')) {
val = 10 + val - 'A';
}

// Every two hex-digits, add byte to code:
if (bytesread & 1 == 1) {
// make some space for this hex-digit by
// shifting the previous hex-digit with 4 bits to the left:
code[bytesread >> 1] = (val | (tempbyte << 4));

if (bytesread >> 1 != 5) {                // If we’re at the checksum byte,
checksum ^= code[bytesread >> 1];       // Calculate the checksum… (XOR)
};
} else {
tempbyte = val;                           // Store the first hex digit first…
};

bytesread++;                                // ready to read next digit
}
}

// Output to Serial:

if (bytesread == 12) {                          // if 12 digit read is complete
Serial.print(”5-byte code: “);
for (i=0; i<5; i++) {
if (code[i] < 16) Serial.print(”0″);
Serial.print(code[i], HEX);
Serial.print(” “);
}
toggle(13);
Serial.println();

Serial.print(”Checksum: “);
Serial.print(code[5], HEX);
Serial.println(code[5] == checksum ? ” — passed.” : ” — error.”);
Serial.println();
}

bytesread = 0;
}
}
}

void toggle(int pinNum) {
// set the LED pin using the pinState variable:
digitalWrite(pinNum, pinState);
// if pinState = 0, set it to 1, and vice versa:
pinState = !pinState;
}
