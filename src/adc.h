

#ifndef ADC_H
#define ADC_H

#include <avr/io.h>

#define ADC_ENABLE ADCSRA |= (1<<ADEN)
#define ADC_DISABLE ADCSRA &= 0x7F
#define ADC_START_CONVERSION ADCSRA |= (1<<ADSC)

void ADC_init(char);
int ADC_read(char);


#endif

