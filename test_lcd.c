/*********************************************
* vim: set sw=8 ts=8 si :
* Author: Guido Socher, Copyright: GPL 
* This is a test program which will write "LCD works"
* on the LCD display. 
* This program is also used to test the keypad. It
* displays the button last pressed.
* 
* See http://www.tuxgraphics.org/electronics/
* 
* Chip type           : ATMEGA8
* Clock frequency     : Internal clock 8 Mhz 
*********************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#define F_CPU 8000000UL  // 8 MHz
#include <util/delay.h>
#include "lcd.h"
#include "kbd.h"
#include <stdlib.h>
#include <string.h> 


void delay_ms(uint16_t ms)
/* delay for a minimum of <ms> */
{
        // we use a calibrated macro. This is more
        // accurate and not so much compiler dependent
        // as self made code.
        while(ms){
                _delay_ms(0.96);
                ms--;
        }
}


int main(void)
{
	int16_t cnt;
	uint8_t i=0;
	lcd_init();
	lcd_clrscr();
	lcd_puts("LCD works");
	init_kbd();
	delay_ms(500);
	while (1) {
		i++;
		cnt=1;
		check_u_button(&cnt);
		if (cnt>1){
			lcd_clrscr();
			lcd_puts_p(PSTR("U+ pressed"));
			i=0;
		}
		if (cnt<1){
			lcd_clrscr();
			lcd_puts_p(PSTR("U- pressed"));
			i=0;
		}
		cnt=1;
		check_i_button(&cnt);
		if (cnt>1){
			lcd_clrscr();
			lcd_puts_p(PSTR("I+ pressed"));
			i=0;
		}
		if (cnt<1){
			lcd_clrscr();
			lcd_puts_p(PSTR("I- pressed"));
			i=0;
		}
		if (check_store_button()){
			lcd_clrscr();
			lcd_puts_p(PSTR("store"));
			lcd_gotoxy(0,1);
			lcd_puts_p(PSTR("pressed"));
			i=0;
		}
		delay_ms(10);
		if (i>150){
			lcd_clrscr();
			lcd_puts_p(PSTR("press"));
			lcd_gotoxy(0,1);
			lcd_puts_p(PSTR("a button"));
			i=0;
		}

	}
	return(0);
}

