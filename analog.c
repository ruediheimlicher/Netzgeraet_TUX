/* vim: set sw=8 ts=8 si : */
/*********************************************
* Author: Guido Socher, Copyright: GPL 
*
* Digital analog conversion of channel ADC0 and ADC1 in
* free running mode. 
**********************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include "dac.h"
#include "uart.h"
#include "hardware_settings.h"


//debug LED:
// set output to VCC, red LED off
#define LEDOFF PORTD|=(1<<PORTD0)
// set output to GND, red LED on
#define LEDON PORTD&=~(1<<PORTD0)
// to test the state of the LED
#define LEDISOFF PORTD&(1<<PORTD0)
static volatile uint8_t currentcontrol=1; // 0=voltage control, otherwise current control
// adc measurement results (11bit ADC):
static volatile int16_t analog_result[2];  


// target_val is the value that is requested (control loop calibrates to this).
// We use the same units a the ADC produces.
static volatile int16_t target_val[2];  // datatype int is 16 bit

static volatile int16_t dac_val=800; // the current dac setting

// You must enable interrupt with sei() in the main program 
// before you call init_analog 
void init_analog(void) 
{
	// initialize the adc result to very high values
	// to keep the control-loop down until proper measurements
	// are done:
	analog_result[0]=0; // I
	analog_result[1]=20;  // U
	target_val[0]=0; // initialize to zero, I
	target_val[1]=0; // initialize to zero, U
        /* enable analog to digital conversion in free run mode
        *  without noise canceler function. See datasheet of atmega8a page 210
        * We set ADPS2=1,ADPS1=1,ADPS0=0 to have a clock division factor of 64.
        * This is needed to stay in the recommended range of 50-200kHz 
        * ADEN: Analog Digital Converter Enable
        * ADIE: ADC Interrupt Enable
        * ADIF: ADC Interrupt Flag
        * ADFR: ADC Free Running Mode
        * ADCSR: ADC Control and Status Register
        * ADPS2..ADPS0: ADC Prescaler Select Bits
         * REFS: Reference Selection Bits (page 203)
        */

	// int-ref with external capacitor at AREF pin: 
	// 2.56V int ref=REFS1=1,REFS0=1
	// write only the lower 3 bit for channel selection
	
	// 2.56V ref, start with channel 0
	ADMUX=(1<<REFS1)|(1<<REFS0);

        ADCSR=(1<<ADEN)|(1<<ADIE)|(1<<ADFR)|(1<<ADIF)|(1<<ADPS2)|(1<<ADPS1)|(0<<ADPS0);

	//  start conversion 
	ADCSR|=(1<<ADSC);
}

int16_t get_dacval(void) 
{
	return(dac_val);
}

uint8_t is_current_limit(void) 
{
	// return 1 if current control loop active
	if (currentcontrol)
   {
		return(1);
	}
	return(0);
}

/* set the target adc value for the control loop
 * values for item: 1 = u, 0 = i, units must be of the same as the values 
 * from the dac.
 */
void set_target_adc_val(uint8_t item,int16_t val) 
{
	// here we can directly write to target_val 
	target_val[item]=val;
}

int16_t getanalogresult(uint8_t channel) 
{
	return(analog_result[channel]);
}

// the control loop changes the dac:
static void control_loop(void){
	int16_t tmp;
	tmp=target_val[0] - analog_result[0]; // current diff
	if (tmp <0)
   {
		// ** current control:
		//
		// stay in current control if we are
		// close to the target. We never regulate
		// the difference down to zero otherweise
		// we would suddenly hop to voltage control
		// and then back to current control. Permanent
		// hopping would lead to oscillation and current
		// spikes.
		if (tmp>-2) tmp=0;
		currentcontrol=10; // I control
		if (analog_result[1]>target_val[1]){
			// oh, voltage too high, get out of current control:
			tmp=-20;
			currentcontrol=0; // U control
		}
	}
   else
   {
		// ** voltage control:
		//
		// if we are in current control then we can only go
		// down (tmp is negative). To increase the current
		// we come here to voltage control. We must slowly
		// count up.
		tmp=1+ target_val[1]  - analog_result[1]; // voltage diff
		if (currentcontrol)
      {
			currentcontrol--;
			// do not go up immediately after we came out of current control:
			if (tmp>0) tmp=0;
		}
	}
	if (tmp> -3 && tmp<4){ // avoid LSB bouncing if we are close
		tmp=0;
	}
	if (tmp==0) return; // nothing to change
	// put a cap on increase
	if (tmp>1)
   {
		tmp=1;
	}
	// put a cap on decrease
	if (tmp<-200)
   {
		tmp=-20;
	}
   else if (tmp<-1)
   {
		tmp=-1;
	}
	dac_val+=tmp;
	if (dac_val>0xFFF)
   {
		dac_val=0xFFF; //max, 12bit
	}
	if (dac_val<400){  // the output is zero below 400 due to transistor threshold
		dac_val=400;
	}
	dac(dac_val);
}
/* the following function will be called when analog conversion is done.
 * It will be called every 13th cycle of the converson clock. At 8Mhz
 * and a ADPS factor of 64 this is: ((8000 kHz/ 64) / 13)= 9.6KHz intervalls
 *
 * We do 4 fold oversampling as explained in atmel doc AVR121.
 */
//SIGNAL(SIG_ADC) {
ISR(ADC_vect)
{
	uint8_t i=0;
	uint8_t adlow;
	int16_t currentadc;
	static uint8_t channel=0; 
	static uint8_t chpos=0;
	// raw 10bit values:
        static int16_t raw_analog_u_result[8];  
        int16_t new_analog_u_result=0;
	adlow=ADCL; // read low first !! 
	currentadc=(ADCH<<8)|adlow;
	// toggel the channel between 0 an 1. This will however
	// not effect the next conversion as that one is already
	// ongoing.
	channel^=1;
	// 2.56V ref
	ADMUX=(1<<REFS1)|(1<<REFS0)|channel;
	// channel=1 = U, channel=0 = I
	if (channel==1) // Spannung messen
   {
		raw_analog_u_result[chpos]=currentadc;
		//
		// we do 4 bit oversampling to get 11bit ADC resolution
		chpos=(chpos+1)%4; // rotate over 4 
		//analog_result[1]=0;
		while(i<4)
      {
			new_analog_u_result+=raw_analog_u_result[i];
			i++;
		}
		new_analog_u_result=new_analog_u_result>>1; // 11bit
		// mean value:
		analog_result[1]=(new_analog_u_result+analog_result[1])/2; 
	}
   else // channel==0, Strom messen
   {
		analog_result[0]=currentadc; // 10bit
	}
   
	// short circuit protection does not use the over sampling results
	// for speed reasons.
	// short circuit protection, current is 10bit ADC
	if (channel==0 && currentadc > SH_CIR_PROT)
   {
		dac_val=400;
		dac(dac_val);
		currentcontrol=20;
		return;
	}
        //
	if (channel==1)
   {
		// only after full measurement cycle
		control_loop();
	}
   else
   {
		uart_poll_getchar_isr();
	}
	// end of interrupt handler
}

