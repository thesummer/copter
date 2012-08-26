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
#include    <stdint.h>
#include    <util/atomic.h>

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

    #define ch0 PORTD5
    #define ch1 PORTB2
    #define ch2 PORTB3
    #define ch3 PORTB4

    static uint8_t onCounter;  //Stores the next channel to turn on
    static uint8_t offCounter; //Stores the next channel to turn off

    static uint16_t onValues[4] = {0, 8191, 16383, 24575};       //OCRA1 values for every ms
    static uint16_t dutyCycles[4] = {12287, 20479, 28671, 4095}; //Stores the duty cycles for each channel


//################################################################# Main routine
static void PwmInit(void);

int main(void)
{	 
    int i;

    cli();  // Disable interrupts
	
    usiTwiSlaveInit(SLAVE_ADDR_ATTINY);	// TWI slave init
    //Initialize rxbuffer
//    for(i=0; i<3; i++)
//    {
//        rxbuffer[i*2]   = HIGH_BYTE( (dutyCycles[i] - i*8192) );
//        rxbuffer[i*2+1] = LOW_BYTE( (dutyCycles[i]  - i*8192) );
//    }

    rxbuffer[0] = HIGH_BYTE( (dutyCycles[0] - 1*8192) );
    rxbuffer[1] = LOW_BYTE(  (dutyCycles[0] - 1*8192) );

    rxbuffer[2] = HIGH_BYTE( (dutyCycles[1] - 2*8192) );
    rxbuffer[3] = LOW_BYTE(  (dutyCycles[1] - 2*8192) );

    rxbuffer[4] = HIGH_BYTE( (dutyCycles[2] - 3*8192) );
    rxbuffer[5] = LOW_BYTE(  (dutyCycles[2] - 3*8192) );

    rxbuffer[6] = HIGH_BYTE( (dutyCycles[3] - 0*8192) );
    rxbuffer[7] = LOW_BYTE(  (dutyCycles[3] - 0*8192) );

    PwmInit();                          // PWM init
	
	sei();  // Re-enable interrupts

    while(1)
    {
//        for (i=0; i<3; i++)
//        {
//            ATOMIC_BLOCK(ATOMIC_FORCEON)
//            {
//                //Update duty cycles and txbuffer
//                dutyCycles[i] = uniq(rxbuffer[i*2+1], rxbuffer[i*2]) + ((i+1)%4) * 8192;
//                txbuffer[i*2]   = rxbuffer[i*2];
//                txbuffer[i*2+1] = rxbuffer[i*2+1];
//            }
        //        }
        uint16_t temp;
        switch (receivedNewValue)
        {
        case 1:
            receivedNewValue = 0;
            temp = uniq(rxbuffer[1], rxbuffer[0]) + 1 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                //Update duty cycles and txbuffer
                dutyCycles[0] = temp;
            }
            txbuffer[0]   = rxbuffer[0];
            txbuffer[1]   = rxbuffer[1];
            break;

        case  3:
            receivedNewValue = 0;
            temp = uniq(rxbuffer[3], rxbuffer[2]) + 2 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                //Update duty cycles and txbuffer
                dutyCycles[1] = temp;
            }
            txbuffer[2]   = rxbuffer[2];
            txbuffer[3]   = rxbuffer[3];
            break;

        case 5:
            receivedNewValue = 0;
            temp = uniq(rxbuffer[5], rxbuffer[4]) + 3 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                //Update duty cycles and txbuffer
                dutyCycles[2] = temp;
            }
            txbuffer[4]   = rxbuffer[4];
            txbuffer[5]   = rxbuffer[5];
            break;

        case 7:
            receivedNewValue = 0;
            temp = uniq(rxbuffer[7], rxbuffer[6]) + 0 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                //Update duty cycles and txbuffer
                dutyCycles[3] = temp;
            }
            txbuffer[6]   = rxbuffer[6];
            txbuffer[7]   = rxbuffer[7];
            break;

        }
    } //end.while
} //end.main

static void PwmInit()
{
      //Set output pins of all 4 channels to output
      sbi(DDRD, DDD5);		//pin PD5 (OC0B): ch0
      sbi(DDRB, DDB2);		//pin PB2 (OC0A): ch1
      sbi(DDRB, DDB3);		//pin PB3 (OC1A): ch2
      sbi(DDRB, DDB4);		//pin PB4 (OC1B): ch3
      
      //Setup timer1
      sbi(TCCR1B, WGM13);	//CTC mode with ICR1 as TOP register
      sbi(TCCR1B, WGM12);

      ICR1 = 0x7fff;     //Set Top value to 2^15-1 == 4ms at 8MHz
      sbi(TIMSK, ICIE1);   //Activate interrupt at TOP
      sbi(TIMSK, OCIE1A);  //Activate interrupt for OCR1A register
      sbi(TIMSK, OCIE1B);  //Activate interrupt for OCR1B register

      //Initialize registers and variables
      OCR1A = onValues[1];
      OCR1B = dutyCycles[3];
      onCounter  = 1;
      offCounter = 0;

      sbi(PORTD, ch0);      //Set ch0 high
      sbi(TCCR1B, CS10);    //Start clock with prescaler 1
}

/*
  ISR for the TOP value of Timer1
  Begin of pwm-cycle:
*/
ISR(TIMER1_CAPT_vect)
{
    sbi(PORTD, ch0);    //Set ch0 high
    OCR1A = 8191;       //Next OCR1A interrupt after 1ms
    onCounter = 1;
    offCounter = 0;
}

/*
  ISR for the compare match of OCR1A of Timer1
*/
ISR(TIMER1_COMPA_vect)
{
    //Switch on correct channel and increase onCounter
    switch (onCounter++)
    {
    case 0: sbi(PORTD, ch0); break;
    case 1: sbi(PORTB, ch1); break;
    case 2: sbi(PORTB, ch2); break;
    case 3: sbi(PORTB, ch3); break;
    }
    //Set next compare interrupt (turn on next channel) in 1 ms
    OCR1A = onValues[onCounter];
}

/*
  ISR for the compare match of OCR1B of Timer1
*/
ISR(TIMER1_COMPB_vect)
{
    switch (offCounter)
    {
    case 0: cbi(PORTB, ch3); break;
    case 1: cbi(PORTD, ch0); break;
    case 2: cbi(PORTB, ch1); break;
    case 3: cbi(PORTB, ch2); break;
    }
    OCR1B = dutyCycles[offCounter++];
}
