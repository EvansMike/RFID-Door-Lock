/**
 * ADC routines for the atmega8
 * 
 * */


#include "adc.h"


/*******************************************************************
 * 
 * @param char Channel number
 * @return void
 * 
 *******************************************************************/
void ADC_init(char channel)
{
ADCSRA = 0x00; //disable adc
ADMUX |= (unsigned char)channel;// 0x40; //select adc input 0
ADMUX |= 0x40; // External reference mode
ADCSRA = 0x86;
}

/*******************************************************************
 * @todo Make this take a param for channel number
 * @param char Channel number
 * @return int ADC value 0-1024
 * 
 *******************************************************************/
int ADC_read(char channel)
{
char i;
int ADC_temp, ADCH_temp;
int ADC_var = 0;
//ADC_init(channel);
ADC_ENABLE;
ADC_START_CONVERSION; //do a dummy readout first
while(!(ADCSRA & 0x10)); // wait for conversion done, ADIF flag active
ADCSRA|=(1<<ADIF);

for(i=0;i<8;i++) // do the ADC conversion 8 times for better accuracy
{
ADC_START_CONVERSION;
while(!(ADCSRA & 0x10)); // wait for conversion done, ADIF flag set
ADCSRA|=(1<<ADIF);

ADC_temp = ADCL; // read out ADCL register
ADCH_temp = ADCH; // read out ADCH register
ADC_temp +=(ADCH_temp << 8);
ADC_var += ADC_temp; // accumulate result (8 samples) for later averaging
}

ADC_var = ADC_var >> 3; // average the 8 samples
//ADC_DISABLE;
return ADC_var;
}

