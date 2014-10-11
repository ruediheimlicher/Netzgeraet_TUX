/*********************************************
 * vim: set sw=8 ts=8 si et :
 * Author: Guido Socher, Copyright: GPL v3
 * This is the main program for the digital dc power supply
 *
 * See http://www.tuxgraphics.org/electronics/
 *
 * Chip type           : ATMEGA8
 * Clock frequency     : Internal clock 8 Mhz
 *********************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL  // 8 MHz
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include "lcd.c"
#include "dac.c"
#include "kbd.c"
#include "uart.c"
#include "analog.c"
#include "hardware_settings.h"

// change this version string when you compile:
#define SWVERSION "ver: ddcp-0.6.6"
//#define DEBUGDISP 1

//debug LED:
// set output to VCC, red LED off
#define LEDOFF PORTD|=(1<<PORTD0)
// set output to GND, red LED on
#define LEDON PORTD&=~(1<<PORTD0)
// to test the state of the LED
#define LEDISOFF PORTD&(1<<PORTD0)
//
// the units are display units and work as follows: 100mA=10 5V=50
// The function int_to_dispstr is used to convert the intenal values
// into strings for the display
static int16_t measured_val[2]={0,0};
static int16_t set_val[2];
// the set values but converted to ADC steps
static int16_t set_val_adcUnits[2];
static uint8_t bpress=0;
// comment this out to use a debug LED on PD0 (RXD):
#define USE_UART 1
//
#ifdef USE_UART
#define UARTSTRLEN 10
static char uartstr[UARTSTRLEN+1];
static uint8_t uartstrpos=0;
static uint8_t uart_has_one_line=0;
#endif

// convert voltage values to adc values, disp=10 is 1.0V
// ADC for voltage is 11bit:
static int16_t disp_u_to_adc(int16_t disp)
{
   return((int16_t)(((float)disp * 204.7) / (ADC_REF * U_DIVIDER )));
}

// calculate the needed adc offset for voltage drop on the
// current measurement shunt (the shunt has about 0.75 Ohm =1/1.33 Ohm)
// use 1/1.2 instead of 1/1.3 because cables and connectors have as well
// a loss.
static int16_t disp_i_to_u_adc_offset(int16_t disp)
{
   return(disp_u_to_adc(disp/12));
}

// convert adc values to voltage values, disp=10 is 1.0V
// disp_i_val is needed to calculate the offset for the voltage drop over
// the current measurement shunt, voltage measurement is 11bit
static int16_t adc_u_to_disp(int16_t adcunits,int16_t disp_i_val)
{
   int16_t adcdrop;
   adcdrop=disp_i_to_u_adc_offset(disp_i_val);
   if (adcunits < adcdrop)
   {
      return(0);
   }
   adcunits=adcunits-adcdrop;
   return((int16_t)((((float)adcunits /204.7)* ADC_REF * U_DIVIDER)+0.5));
}

// convert adc values to current values, disp=10 needed to be printed
// by the printing function as 0.10 A, current measurement is 10bit
static int16_t disp_i_to_adc(int16_t disp)
{
   return((int16_t) (((disp * 10.23)* I_RESISTOR) / ADC_REF));
}

// convert adc values to current values, disp=10 needed to be printed
// by the printing function as 0.10 A, current measurement is 10bit
static int16_t adc_i_to_disp(int16_t adcunits)
{
   return((int16_t) (((float)adcunits* ADC_REF)/(10.23 * I_RESISTOR)+0.5));
}

static void update_controlloop_targets(void)
{
   // current
   measured_val[0]=adc_i_to_disp(getanalogresult(0));
   set_val_adcUnits[0]=disp_i_to_adc(set_val[0]);
   set_target_adc_val(0,set_val_adcUnits[0]);
   // voltage
   measured_val[1]=adc_u_to_disp(getanalogresult(1),measured_val[0]);
   set_val_adcUnits[1]=disp_u_to_adc(set_val[1])+disp_i_to_u_adc_offset(measured_val[0]);
   set_target_adc_val(1,set_val_adcUnits[1]);
}

void delay_ms_uartcheck(uint8_t ms)
// delay for a minimum of <ms>
{
   uint8_t innerloop=1;
   char c;
   while(ms)
   {
#ifdef USE_UART
      if(uart_has_one_line==0 && uart_getchar_isr_noblock(&c))
      {
         if (c=='\n') c='\r'; // Make unix scripting easier. A terminal, even under unix, does not send \n
         // ignore any white space and characters we do not use:
         if (!(c=='\b'||(c>='0'&&c<='z')||c==0x7f||c=='\r'))
         {
            goto NEXTCHAR;
         }
         if (c=='\r')
         {
            uartstr[uartstrpos]='\0';
            uart_sendchar('\r'); // the echo line end
            uart_sendchar('\n'); // the echo line end
            uart_has_one_line=1;
            goto NEXTCHAR;
         }
         /*
          // debug
          itoa(c,buf,10);
          uart_sendchar('\r'); // the echo line end
          uart_sendchar('\n'); // the echo line end
          uart_sendchar('|');uart_sendstr(buf);uart_sendchar('|');
          uart_sendchar('\r'); // the echo line end
          uart_sendchar('\n'); // the echo line end
          */
         if (c=='\b')
         { // backspace
            if (uartstrpos>0)
            {
               uartstrpos--;
               uart_sendchar(c); // echo back
               uart_sendchar(' '); // clear char on screen
               uart_sendchar('\b');
            }
         }
         else if (c==0x7f)
         { // del
            if (uartstrpos>0)
            {
               uartstrpos--;
               uart_sendchar(c); // echo back
            }
         }
         else
         {
            uart_sendchar(c); // echo back
            uartstr[uartstrpos]=c;
            uartstrpos++;
         }
         if (uartstrpos>UARTSTRLEN)
         {
            uart_sendstr_p(PSTR("\r\nERROR\r\n"));
            uartstrpos=0; // empty buffer
            uartstr[0]='\0'; // just print prompt
            uart_has_one_line=1;
         }
      }
#endif
   NEXTCHAR:
      innerloop--;
      if (innerloop==0)
      {
         innerloop=45;
         ms--;
      }
   }
}

// Convert an integer which is representing a float into a string.
// Our display is always 4 digits long (including one
// decimal point position). decimalpoint_pos defines
// after how many positions from the right we set the decimal point.
// The resulting string is fixed width and padded with leading space.
//
// decimalpoint_pos=2 sets the decimal point after 2 pos from the right:
// e.g 74 becomes "0.74"
// The integer should not be larger than 999.
// The integer must be a positive number.
// decimalpoint_pos can be 0, 1 or 2
static void int_to_dispstr(uint16_t inum,char *outbuf,int8_t decimalpoint_pos){
   int8_t i,j;
   char chbuf[8];
   itoa(inum,chbuf,10); // convert integer to string
   i=strlen(chbuf);
   if (i>3) i=3; //overflow protection
   strcpy(outbuf,"   0"); //decimalpoint_pos==0
   if (decimalpoint_pos==1) strcpy(outbuf," 0.0");
   if (decimalpoint_pos==2) strcpy(outbuf,"0.00");
   j=4;
   while(i){
      outbuf[j-1]=chbuf[i-1];
      i--;
      j--;
      if (j==4-decimalpoint_pos){
         // jump over the pre-set dot
         j--;
      }
   }
}

static void store_permanent(void)
{
   int16_t tmp;
   uint8_t changeflag=1;
   lcd_clrscr();
   if (eeprom_read_byte((uint8_t *)0x0) == 19){
      changeflag=0;
      // ok magic number matches accept values
      tmp=eeprom_read_word((uint16_t *)0x04);
      if (tmp != set_val[1]){
         changeflag=1;
      }
      tmp=eeprom_read_word((uint16_t *)0x02);
      if (tmp != set_val[0]){
         changeflag=1;
      }
   }
   delay_ms_uartcheck(1); // check for uart without delay
   if (changeflag){
      lcd_puts_p(PSTR("setting stored"));
      eeprom_write_byte((uint8_t *)0x0,19); // magic number
      eeprom_write_word((uint16_t *)0x02,set_val[0]);
      eeprom_write_word((uint16_t *)0x04,set_val[1]);
   }else{
      if (bpress> 2){
         // display software version after long press
         lcd_puts_p(PSTR(SWVERSION));
         lcd_gotoxy(0,1);
         lcd_puts_p(PSTR("tuxgraphics.org"));
      }else{
         lcd_puts_p(PSTR("already stored"));
      }
   }
   delay_ms_uartcheck(200);
   delay_ms_uartcheck(200);
   delay_ms_uartcheck(200);
}

// check the keyboard
static uint8_t check_buttons(void)
{
   uint8_t uartprint_ok=0;
   uint8_t cmdok=0;
#ifdef USE_UART
   char buf[21];
#endif
   //
#ifdef USE_UART
   if (uart_has_one_line)
   {
      if (uartstr[0]=='i' && uartstr[1]=='=' && uartstr[2]!='\0')
      {
         set_val[0]=atoi(&uartstr[2]);
         if(set_val[0]>I_MAX)
         {
            set_val[0]=I_MAX;
         }
         if(set_val[0]<0)
         {
            set_val[0]=0;
         }
         uartprint_ok=1;
      }
      // version
      if (uartstr[0]=='v' && uartstr[1]=='e')
      {
         uart_sendstr_p(PSTR("  "));
         uart_sendstr_p(PSTR(SWVERSION));
         uart_sendstr_p(PSTR("\r\n"));
         cmdok=1;
      }
      // store
      if (uartstr[0]=='s' && uartstr[1]=='t')
      {
         store_permanent();
         uartprint_ok=1;
      }
      if (uartstr[0]=='u' && uartstr[1]=='=' && uartstr[2]!='\0')
      {
         set_val[1]=atoi(&uartstr[2]);
         if(set_val[1]>U_MAX)
         {
            set_val[1]=U_MAX;
         }
         if(set_val[1]<0)
         {
            set_val[1]=0;
         }
         uartprint_ok=1;
      }
      // help
      if (uartstr[0]=='h' || uartstr[0]=='H')
      {
         uart_sendstr_p(PSTR("  Usage: u=V*10|i=mA/10|store|help|version\r\n"));
         uart_sendstr_p(PSTR("  Examples:\r\n"));
         uart_sendstr_p(PSTR("  set 6V: u=60\r\n"));
         uart_sendstr_p(PSTR("  max 200mA: i=20\r\n"));
         cmdok=1;
      }
      if (uartprint_ok)
      {
         cmdok=1;
         uart_sendstr_p(PSTR("  ok\r\n"));
      }
      if (uartstr[0]!='\0' && cmdok==0)
      {
         uart_sendstr_p(PSTR("  command unknown\r\n"));
      }
      uart_sendchar('#'); // marking char for script interface
      int_to_dispstr(measured_val[1],buf,1);
      uart_sendstr(buf);
      uart_sendchar('V');
      uart_sendchar(' ');
      uart_sendchar('[');
      int_to_dispstr(set_val[1],buf,1);
      uart_sendstr(buf);
      uart_sendchar(']');
      uart_sendchar(',');
      int_to_dispstr(measured_val[0],buf,2);
      uart_sendstr(buf);
      uart_sendchar('A');
      uart_sendchar(' ');
      uart_sendchar('[');
      int_to_dispstr(set_val[0],buf,2);
      uart_sendstr(buf);
      uart_sendchar(']');
      if (is_current_limit())
      {
         uart_sendchar('I');
      }else{
         uart_sendchar('U');
      }
      uart_sendchar('>');
      uartstrpos=0;
      uart_has_one_line=0;
   }
#endif
   if (check_u_button(&(set_val[1])))
   {
      if(set_val[1]>U_MAX)
      {
         set_val[1]=U_MAX;
      }
      return(1);
   }
   if (check_i_button(&(set_val[0])))
   {
      if(set_val[0]>I_MAX)
      {
         set_val[0]=I_MAX;
      }
      return(1);
   }
   if (check_store_button())
   {
      store_permanent();
      return(2);
   };
   return(0);
}

int main(void)
{
   char out_buf[21];
   uint8_t i=0;
   uint8_t ilimit=0;
   
#ifndef USE_UART
   // debug led, you can not have an LED if you use the uart
   DDRD|= (1<<DDD0); // LED, enable PD0, LED as output
   LEDOFF;
#endif
   
   init_dac();
   lcd_init();
   init_kbd();
   set_val[0]=15;set_val[1]=50; // 150mA and 5V
   if (eeprom_read_byte((uint8_t *)0x0) == 19)
   {
      // ok magic number matches accept values
      set_val[1]=eeprom_read_word((uint16_t *)0x04);
      set_val[0]=eeprom_read_word((uint16_t *)0x02);
      // sanity check:
      if (set_val[0]<0) set_val[0]=0;
      if (set_val[1]<0) set_val[1]=0;
   }
#ifdef USE_UART
   uart_init();
#endif
   sei();
   init_analog();
   while (1)
   {
      i++;
      // due to electrical interference we can get some
      // garbage onto the display especially if the power supply
      // source is not stable enough. We can remedy it a bit in
      // software with an ocasional reset:
      if (i==50)
      { // not every round to avoid flicker
         lcd_reset();
         i=0;
      }
      lcd_home();
      update_controlloop_targets();
      ilimit=is_current_limit();
      
      // voltage
#ifdef DEBUGDISP
      itoa(getanalogresult(1),out_buf,10);
#else
      int_to_dispstr(measured_val[1],out_buf,1);
#endif
      lcd_puts(out_buf);
      lcd_puts("V [");
#ifdef DEBUGDISP
      itoa(set_val_adcUnits[1],out_buf,10);
#else
      int_to_dispstr(set_val[1],out_buf,1);
#endif
      lcd_puts(out_buf);
      lcd_putc(']');
      delay_ms_uartcheck(1); // check for uart without delay
      if (!ilimit)
      {
         // put a marker to show which value is currenlty limiting
         lcd_puts("<- ");
      }
      else
      {
         lcd_puts("   ");
      }
      
      // current
      lcd_gotoxy(0,1);
#ifdef DEBUGDISP
      itoa(getanalogresult(0),out_buf,10);
#else
      int_to_dispstr(measured_val[0],out_buf,2);
#endif
      lcd_puts(out_buf);
      lcd_puts("A [");
#ifdef DEBUGDISP
      itoa(set_val_adcUnits[0],out_buf,10);
#else
      int_to_dispstr(set_val[0],out_buf,2);
#endif
      lcd_puts(out_buf);
      lcd_putc(']');
      if (ilimit)
      {
         // put a marker to show which value is currenlty limiting
         lcd_puts("<- ");
      }
      else
      {
         lcd_puts("   ");
      }
      
      update_controlloop_targets();
      
      // the buttons must be responsive but they must not
      // scroll too fast if pressed permanently
      if (check_buttons()==0)
      {
         // no buttons pressed
         delay_ms_uartcheck(80);
         bpress=0;
         if (check_buttons()==0)
         {
            // no buttons pressed
            delay_ms_uartcheck(80);
         }
         else
         {
            bpress++;
            delay_ms_uartcheck(160);
            delay_ms_uartcheck(160);
            delay_ms_uartcheck(160);
            delay_ms_uartcheck(160);
         }
      }
      else
      {
         // button press
         if (bpress > 10)
         {
            // somebody pressed permanetly the button=>scroll fast
            delay_ms_uartcheck(120);
         }
         else
         {
            bpress++;
            delay_ms_uartcheck(180);
            delay_ms_uartcheck(180);
            delay_ms_uartcheck(180);
            delay_ms_uartcheck(180);
         }
      }
   }
   return(0);
}

