/* vim: set sw=8 ts=8 si et: */
/*************************************************************************
 Title:   C include file for uart
 Target:    atmega8
 Copyright: GPL, Guido Socher
***************************************************************************/
#ifndef UART_H
#define UART_H
#include <avr/io.h>
#include <avr/pgmspace.h>

extern void uart_init(void);
extern void uart_poll_getchar_isr(void); // call this periodically from interrupt, has a buffer bigger than one char
extern unsigned char uart_getchar_isr_noblock(char *returnval); // get a char from buffer if available
extern void uart_sendchar(char c); // blocking, no buffer
extern void uart_sendstr(char *s); // blocking, no buffer
extern void uart_sendstr_p(const char *progmem_s); // blocking, no buffer
// uart_sendstr_p can be used like this: uart_sendstr_p(PSTR("my string"));
/*
// you can either use the above _isr functions or one of the
// following two but you can not mix them.
extern char uart_getchar(void);
extern unsigned char uart_getchar_noblock(char *returnval);
extern void uart_flushRXbuf(void);
*/


#endif /* UART_H */
