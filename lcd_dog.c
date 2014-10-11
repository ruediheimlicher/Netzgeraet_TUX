
/* ---------------------------------------------------------------------
 
 Sample code for driving ST7036 on ELECTRONIC ASSEMBLY's DOG-Series
 (tested on ATmega8, EA DOGM163, AVR-GCC)
 *** NO FREE SUPPORT ON THIS PIECE OF CODE ***
 if you need an offer: mailto: programmer@demmel-m.de
 
 --------------------------------------------------------------------- */

//Serieller Betrieb von EA DOGModulen mit dem ST7036

#define DOG_PORT        PORTD

#define DOG_SI		5
#define DOG_CLK	4
#define DOG_CSB	3
#define DOG_RS		2
#define DOG_RESET	1

void delay_ms(unsigned int ms)
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


/* ein Byte in das ???-Register des ST7036 senden */
void ST7036_write_byte( char data )
{
   signed char	u8_zahl = 8;
   char c_data;
	// Chip-Select auf log.0
	//Clear_Bit( ST7036_CSB );
   DOG_PORT &= ~(1<<DOG_CSB);
	c_data = data;
   
	do
	{
		_delay_loop_2(6);
		if ( c_data & 0x80 )
      {// oberstes Bit von c_data betrachten
			//Set_Bit(ST7036_SI);		// und Datenleitung entsprechend setzen
         DOG_PORT |= (1<<DOG_SI);
      }
      else
      {
			//Clear_Bit(ST7036_SI);
         DOG_PORT &= ~(1<<DOG_SI);
      }
      
		_delay_loop_2(5);			// einen Clockpuls erzeugen
		//Clear_Bit(ST7036_CLK);
      DOG_PORT &= ~(1<<DOG_CLK);
		_delay_loop_2(6);
		//Set_Bit(ST7036_CLK);
      DOG_PORT |= (1<<DOG_CLK);
      
      
		c_data = c_data << 1;
		u8_zahl --;
      
	} while (u8_zahl > 0);
   
	// Chip-Select wieder auf log.1
	delay_ms( 2 );
	//Set_Bit( ST7036_CSB );
   DOG_PORT |= (1<<DOG_CSB);
}

/* ein Byte in das Control-Register des KS0073 senden */
void ST7036_write_command_byte( char data )
{
	//Clear_Bit( ST7036_RS );
   DOG_PORT &= ~(1<<DOG_RS);
	_delay_loop_2( 1 );
	ST7036_write_byte( data );
}

/* ein Byte in das Daten-Register des KS0073 senden */
void ST7036_write_data_byte( char data )
{
	//Set_Bit( ST7036_RS );
   DOG_PORT |= (1<<DOG_RS);
	_delay_loop_2( 7 );
	ST7036_write_byte( data );
}

/* Reset durchf�hren */
void ST7036_reset(void)
{
#ifdef ST7036_RESET
	//Clear_Bit(lcdReset);	// Hardware-Reset log.0 an den ST7036 anlegen
	DOG_PORT &= ~(1<<DOG_RESET);
   delay_ms( 100 );
	Set_Bit(lcdReset);
   DOG_PORT |= (1<<DOG_SI);
#endif
}


/* ein Byte in das Daten-Register des KS0073 senden */
void ST7036_init(void)
{
   
   //Set_Bit(ST7036_CLK);
   DOG_PORT |= (1<<DOG_CLK);
   //Set_Bit(ST7036_CSB);
   DOG_PORT |= (1<<DOG_CSB);
   
   //ST7036_reset();
   
   
	delay_ms(50);		// mehr als 40ms warten
   
   //	ST7036_write_command_byte( 0x38 );	// Function set; 8 bit Datenlänge, 2 Zeilen
   
	_delay_us(50);		// mehr als 26,3µs warten
   
	ST7036_write_command_byte( 0x39 );	// Function set; 8 bit Datenlänge, 2 Zeilen, Instruction table 1
	_delay_us(50);		// mehr als 26,3µs warten
   
	ST7036_write_command_byte( 0x1d );	// Bias Set; BS 1/5; 3 zeiliges Display /1d
	_delay_us(50);		// mehr als 26,3µs warten
   
	ST7036_write_command_byte( 0x7c );	// Kontrast C3, C2, C1 setzen /7c
	_delay_us(50);		// mehr als 26,3µs warten
   
	ST7036_write_command_byte( 0x50 );	// Booster aus; Kontrast C5, C4 setzen /50
	_delay_us(50);		// mehr als 26,3µs warten
   
	ST7036_write_command_byte( 0x6c );	// Spannungsfolger und Verstärkung setzen /6c
	_delay_us( 500 );	// mehr als 200ms warten !!!
   
	ST7036_write_command_byte( 0x0f );	// Display EIN, Cursor EIN, Cursor BLINKEN /0f
	_delay_us(50);		// mehr als 26,3µs warten
   
	ST7036_write_command_byte( 0x01 );	// Display löschen, Cursor Home
	delay_ms(400);		//
   
	ST7036_write_command_byte( 0x06 );	// Cursor Auto-Increment
	_delay_us(50);		// mehr als 26,3µs warten
   
}


void ST7036_putsf( char* string )
{
   unsigned char zahl;
   zahl = 0;
   while (string[zahl] != 0x00)
	{
      ST7036_write_data_byte( string[zahl]);
      zahl++;
	}
}

void ST7036_puts( char * string )
{
   unsigned char zahl;
   zahl = 0;
   while (string[zahl] != 0x00)
	{
      _delay_loop_2(50000);
      ST7036_write_data_byte( string[zahl] );
      string ++;
	}
}

void ST7036_putc( char zeichen )
{
	ST7036_write_data_byte( zeichen );
}

// positioniert den Cursor auf x/y.
// 0/0 ist links oben, 2/15 ist rechts unten
void ST7036_goto_xy(unsigned char xwert, unsigned char ywert)
{
   //_delay_loop_2(50000);
   ST7036_write_command_byte(0x80 + ywert*0x20 + xwert);
   //_delay_loop_2(50000);
   //Set_Bit(ST7036_RS);
   DOG_PORT |= (1<<DOG_RS);
}


void ST7036_clearline( unsigned char zeile )
{
   unsigned char zahl;
   ST7036_goto_xy( 0, zeile );
   for (zahl=1; zahl<20; zahl++) ST7036_write_data_byte( ' ' );
}

void ST7036_clear( void )
{
	ST7036_clearline( 0 );
	ST7036_clearline( 1 );
	ST7036_clearline( 2 );
}
