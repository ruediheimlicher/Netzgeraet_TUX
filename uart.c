// vim: set sw=8 ts=8 si et: 
/********************************************
* UART interface without interrupt
* Author: Guido Socher
* Copyright: GPL
**********************************************/
#include <avr/interrupt.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"
#define F_CPU 8000000UL  // 8 MHz

// a receiver stack:
#define SENDSTACKSIZE 12
static volatile char ustack[SENDSTACKSIZE];
static volatile uint8_t stackpointer_end=0;
static uint8_t stackpointer_start=0;

void uart_init(void) 
{
        unsigned int baud=51;   // 9600 baud at 8MHz
#ifdef VAR_88CHIP
        UBRR0H=(unsigned char) (baud >>8);
        UBRR0L=(unsigned char) (baud & 0xFF);
        // enable tx/rx and no interrupt on tx/rx 
        UCSR0B =  (1<<RXEN0) | (1<<TXEN0);
        // format: asynchronous, 8data, no parity, 1stop bit 
        UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
#else
        UBRRH=(unsigned char) (baud >>8);
        UBRRL=(unsigned char) (baud & 0xFF);
        // enable tx/rx and no interrupt on tx/rx 
        UCSRB =  (1<<RXEN) | (1<<TXEN);
        // format: asynchronous, 8data, no parity, 1stop bit 
        UCSRC = (1<<URSEL)|(3<<UCSZ0);
#endif
}

// send one character to the rs232 
void uart_sendchar(char c) 
{
#ifdef VAR_88CHIP
        // wait for empty transmit buffer 
        while (!(UCSR0A & (1<<UDRE0)));
        UDR0=c;
#else
        // wait for empty transmit buffer 
        while (!(UCSRA & (1<<UDRE)));
        UDR=c;
#endif
}
// send string to the rs232 
void uart_sendstr(char *s) 
{
        while (*s){
                uart_sendchar(*s);
                s++;
        }
}

void uart_sendstr_p(const char *progmem_s )
// print string from program memory on rs232 
{
        char c;
        while ((c = pgm_read_byte(progmem_s++))) {
                uart_sendchar(c);
        }

}

// call this function from interrupt to fill the
// ustack reveiver buffer. We need this big buffer
// to handle fast copy/paste of strings comming
// into the UART
void uart_poll_getchar_isr(void)
{
#ifdef VAR_88CHIP
        if(!(UCSR0A & (1<<RXC0))) return;
        ustack[stackpointer_end]=UDR0;
#else
        if(!(UCSRA & (1<<RXC))) return;
        ustack[stackpointer_end]=UDR;
#endif
        stackpointer_end=(stackpointer_end+1) % SENDSTACKSIZE;
}

// get the characters out of the buffer which is filled by
// the above interrupt function
unsigned char uart_getchar_isr_noblock(char *returnval)  
{
        if (stackpointer_start!=stackpointer_end){
                *returnval=ustack[stackpointer_start];
                stackpointer_start=(stackpointer_start+1) % SENDSTACKSIZE;
                return(1);
        }
        return(0);
}

/*
// get a byte from rs232
// this function does a blocking read 
char uart_getchar(void)  
{
#ifdef VAR_88CHIP
        while(!(UCSR0A & (1<<RXC0)));
        return(UDR0);
#else
        while(!(UCSRA & (1<<RXC)));
        return(UDR);
#endif
}

// get a byte from rs232
// this function does a non blocking read 
// returns 1 if a character was read
unsigned char uart_getchar_noblock(char *returnval)  
{
#ifdef VAR_88CHIP
        if(UCSR0A & (1<<RXC0)){
                *returnval=UDR0;
                return(1);
        }
#else
        if(UCSRA & (1<<RXC)){
                *returnval=UDR;
                return(1);
        }
#endif
        return(0);
}

// read and discard any data in the receive buffer 
void uart_flushRXbuf(void)  
{
        unsigned char tmp;
#ifdef VAR_88CHIP
        while(UCSR0A & (1<<RXC0)){
                tmp=UDR0;
        }
#else
        while(UCSRA & (1<<RXC)){
                tmp=UDR;
        }
#endif
}

*/
