/*##############################################################################

	Name	: USI TWI Slave driver - I2C/TWI-EEPROM
	Version	: 1.3  - Stable
	autor	: Martin Junghans	jtronics@gmx.de
	page	: www.jtronics.de
	License	: GNU General Public License 

	Created from Atmel source files for Application Note AVR312: 
  Using the USI Module as an I2C slave like a I2C-EEPROM.
  
//############################################################################*/


#include 	<stdlib.h>
#include 	<avr/io.h>
#include 	<avr/interrupt.h>
#include 	<avr/pgmspace.h>

//################################################################## USI-TWI-I2C

#include 	"usiTwiSlave.h"     		

// Note: The LSB is the I2C r/w flag and must not be used for addressing!
#define 	SLAVE_ADDR_ATTINY       0b00110100

#ifndef 	F_CPU
#define 	F_CPU 8000000UL
#endif

//####################################################################### Macros

#define uniq(LOW,HEIGHT)	((HEIGHT << 8)|LOW)			  // Create 16 bit number from two bytes
#define LOW_BYTE(x)        	(x & 0xff)					    // Get low byte from 16 bit number  
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)			  // Get high byte from 16 bit number

#define sbi(ADDRESS,BIT) 	((ADDRESS) |= (1<<(BIT)))	// Set bit
#define cbi(ADDRESS,BIT) 	((ADDRESS) &= ~(1<<(BIT)))// Clear bit
#define	toggle(ADDRESS,BIT)	((ADDRESS) ^= (1<<BIT))	// Toggle bit

#define	bis(ADDRESS,BIT)	(ADDRESS & (1<<BIT))		  // Is bit set?
#define	bic(ADDRESS,BIT)	(!(ADDRESS & (1<<BIT)))		// Is bit clear?

//#################################################################### Variables

	#define duty1	OCR0A
	#define duty2	OCR0B
        #define duty3	OCR1A
        #define duty4	OCR1B


//################################################################# Main routine
void PwmInit(void);

int main(void)
{	 
  
	cli();  // Disable interrupts
	
	usiTwiSlaveInit(SLAVE_ADDR_ATTINY);	// TWI slave init
	PwmInit();				// PWM init
	
	sei();  // Re-enable interrupts


while(1)
    {

	//Set new duty cycles
	duty1 = rxbuffer[0];
	duty2 = rxbuffer[1];
	duty3 = rxbuffer[2];
	duty4 = rxbuffer[3];
	
	//Update status register for read from master
	txbuffer[0] = duty1;
	txbuffer[1] = duty2;
	txbuffer[2] = duty3;
	txbuffer[3] = duty4;
	
	} //end.while
} //end.main

void PwmInit()
{
      //Set output pins of all 4 channels to output
      sbi(DDRD, DDD5);		//pin PD2 (OC0B)
      sbi(DDRB, DDB2);		//pin PB2 (OC0A)
      sbi(DDRB, DDB3);		//pin PB2 (OC1A)
      sbi(DDRB, DDB4);		//pin PB2 (OC1B)
      
      //Setup timers
      
      //Setup timer0
      sbi(TCCR0A, WGM00);	//Set to phase correct PWM
      sbi(TCCR0A, COM0A1);	//Activate channel1 at pin OC0A
      sbi(TCCR0A, COM0B1);	//Activate channel2 at pin OC0B
      sbi(TCCR0B, CS01);	//Set prescaler to 8
      
      //Setup timer1
      sbi(TCCR1A, WGM10);	//Set to 8-bit phase correct pwm
      sbi(TCCR1A, COM1A1);	//Activate channel1 at pin OC1A
      sbi(TCCR1A, COM1B1);	//Activate channel2 at pin OC1B
      sbi(TCCR1B, CS11);	//Set prescaler to 8
}

