/**

    @file   src-avr/main.c
    @brief  I2C-slave to control 4 ESCs
    @author Jan Sommer
    This program uses the USI TWI Slave driver - I2C/TWI-EEPROM by Martin Junghans:

    Version	: 1.3  - Stable
    autor	: Martin Junghans	jtronics@gmx.de
    page	: www.jtronics.de
    License	: GNU General Public License

    Created from Atmel source files for Application Note AVR312:
    Using the USI Module as an I2C slave like a I2C-EEPROM.

    The program initializes the USI interface as an I2C-Slave. The I2C-buffer is constantly
    polled to update value for the PPM signal of the channels.

    The PPM signal of each channel is a pulse between 1 ms (motor off) and 2 ms (full speed).
    The update frequency of the signal is approx. 250 Hz using the following scheme:

    - a full update cycle lasts 4 ms

    - 0.0 ms: turn channel 0 on
    - 0.x ms: turn channel 3 off
    - 1.0 ms: turn channel 1 on
    - 1.x ms: turn channel 0 off
    - 2.0 ms: turn channel 2 on
    - 2.x ms: turn channel 1 off
    - 3.0 ms: turn channel 3 on
    - 3.x ms: turn channel 2 off
    - start at 0.0 ms

*/


#include 	<stdlib.h>
#include 	<avr/io.h>
#include 	<avr/interrupt.h>
#include 	<avr/pgmspace.h>
#include    <stdint.h>
#include    <util/atomic.h>

//################################################################## USI-TWI-I2C

#include 	"usiTwiSlave.h"     		

/** Note: The LSB is the I2C r/w flag and must not be used for addressing!
  I2C-address for the slave (only the first 7 bit)
  Note: The LSB is the I2C r/w flag and must not be used for addressing!
*/
#define 	SLAVE_ADDR_ATTINY       0b00110100

#ifndef 	F_CPU
#define 	F_CPU 8000000UL ///< CPU clock frequency
#endif

//####################################################################### Macros


#define uniq(LOW,HEIGHT)	((HEIGHT << 8)|LOW)           ///< Create 16 bit number from two bytes
#define LOW_BYTE(x)        	(x & 0xff)					  ///< Get low byte from 16 bit number
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)			  ///< Get high byte from 16 bit number

#define sbi(ADDRESS,BIT) 	((ADDRESS) |= (1<<(BIT)))	///< Set bit
#define cbi(ADDRESS,BIT) 	((ADDRESS) &= ~(1<<(BIT)))  ///< Clear bit
#define	toggle(ADDRESS,BIT)	((ADDRESS) ^= (1<<BIT))	    ///< Toggle bit

#define	bis(ADDRESS,BIT)	(ADDRESS & (1<<BIT))		///< Is bit set?
#define	bic(ADDRESS,BIT)	(!(ADDRESS & (1<<BIT)))		///< Is bit clear?

//#################################################################### Variables

    #define ch0 PORTD5  ///< channel 0 on pin D5
    #define ch1 PORTB2  ///< channel 1 on pin B2
    #define ch2 PORTB3  ///< channel 2 on pin B3
    #define ch3 PORTB4  ///< channel 3 on pin B4

    static uint8_t onCounter;  ///< Stores the next channel to turn on
    static uint8_t offCounter; ///< Stores the next channel to turn off

    static uint16_t onValues[4] = {0, 8191, 16383, 24575};       //OCRA1 values for every ms
    static uint16_t dutyCycles[4] = {12287, 20479, 28671, 4095}; //Stores the duty cycles for each channel


//################################################################# Main routine

static void ppmInit(void);

/*!
 @brief main program of the I2C-slave

 Initializes the timer/counters and the I2C.
 Constantly updates the duty cycles of the PPM

 @return int
*/
int main(void)
{	 
    int i;

    cli();  // Disable interrupts
	
    usiTwiSlaveInit(SLAVE_ADDR_ATTINY);	// TWI slave init

    //Initialize rxbuffer
    rxbuffer[0] = HIGH_BYTE( (dutyCycles[0] - 1*8192) );
    rxbuffer[1] = LOW_BYTE(  (dutyCycles[0] - 1*8192) );

    rxbuffer[2] = HIGH_BYTE( (dutyCycles[1] - 2*8192) );
    rxbuffer[3] = LOW_BYTE(  (dutyCycles[1] - 2*8192) );

    rxbuffer[4] = HIGH_BYTE( (dutyCycles[2] - 3*8192) );
    rxbuffer[5] = LOW_BYTE(  (dutyCycles[2] - 3*8192) );

    rxbuffer[6] = HIGH_BYTE( (dutyCycles[3] - 0*8192) );
    rxbuffer[7] = LOW_BYTE(  (dutyCycles[3] - 0*8192) );

    ppmInit();
	
	sei();  // Re-enable interrupts

    while(1)
    {
        /*
            receivedNewValue is updated whenever a new value is written
            to the rxbuffer with the corresponding index
            As always 2 bytes are used for one dutyCycle the interesting values are 1, 3, 5, 7
            Then the corresponding value in the dutyCycle array is updated

            The new value for a dutyCycle is calculated first in a temp-variable.
            In a previous version it was directly assigned within the atomic block which caused severs problems
            that the ucontroller stopped responding to the I2C-master.
            Therefore now only the assignment to dutyCycles[x] is done in atomic block
        */
        uint16_t temp;
        switch (receivedNewValue)
        {
        case 1:  //update dutyCycle for channel 0
            receivedNewValue = 0;
            temp = uniq(rxbuffer[1], rxbuffer[0]) + 1 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                dutyCycles[0] = temp;
            }
            txbuffer[0]   = rxbuffer[0];
            txbuffer[1]   = rxbuffer[1];
            break;

        case  3:  //update dutyCycle for channel 1
            receivedNewValue = 0;
            temp = uniq(rxbuffer[3], rxbuffer[2]) + 2 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                dutyCycles[1] = temp;
            }
            txbuffer[2]   = rxbuffer[2];
            txbuffer[3]   = rxbuffer[3];
            break;

        case 5:  //update dutyCycle for channel 2
            receivedNewValue = 0;
            temp = uniq(rxbuffer[5], rxbuffer[4]) + 3 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                dutyCycles[2] = temp;
            }
            txbuffer[4]   = rxbuffer[4];
            txbuffer[5]   = rxbuffer[5];
            break;

        case 7:   //update dutyCycle for channel 3
            receivedNewValue = 0;
            temp = uniq(rxbuffer[7], rxbuffer[6]) + 0 * 8192;
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                dutyCycles[3] = temp;
            }
            txbuffer[6]   = rxbuffer[6];
            txbuffer[7]   = rxbuffer[7];
            break;
        } //end.switch
    } //end.while
} //end.main

/*!
 @brief Initialize the timercounter and interrupts for the ppm signals

  The following pins are used:
    pin PD5 (OC0B): ch0
    pin PB2 (OC0A): ch1
    pin PB3 (OC1A): ch2
    pin PB4 (OC1B): ch3

*/
static void ppmInit()
{

      //Set output pins of all 4 channels to output
      sbi(DDRD, DDD5);		//pin PD5 (OC0B): ch0
      sbi(DDRB, DDB2);		//pin PB2 (OC0A): ch1
      sbi(DDRB, DDB3);		//pin PB3 (OC1A): ch2
      sbi(DDRB, DDB4);		//pin PB4 (OC1B): ch3
      
      //Setup timer1
      sbi(TCCR1B, WGM13);	//CTC mode with ICR1 as TOP register
      sbi(TCCR1B, WGM12);

      ICR1 = 0x7fff;       //Set Top value to 2^15-1 == 4ms at 8MHz
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

/**
  @brief ISR for the TOP value of Timer1 --> Begin of ppm-cycle
*/
ISR(TIMER1_CAPT_vect)
{
    sbi(PORTD, ch0);    //Set ch0 high
    OCR1A = 8191;       //Next OCR1A interrupt after 1ms
    onCounter = 1;
    offCounter = 0;
}

/**
  @brief ISR for the compare match of OCR1A of Timer1 --> switch on channels

  The channel in onCounter is switched on and the time value for the
  switch on interrupt of the next channel is loaded into the OCR1A-register
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

/**
  @brief ISR for the compare match of OCR1B of Timer1 --> switch off channels

  The channel corresponding to offCounter is set low. And the time value for the
  next switch off interrupt is loaded into the OCR1B-register.

*/
ISR(TIMER1_COMPB_vect)
{
    //Switch off correct channel
    switch (offCounter)
    {
    case 0: cbi(PORTB, ch3); break;
    case 1: cbi(PORTD, ch0); break;
    case 2: cbi(PORTB, ch1); break;
    case 3: cbi(PORTB, ch2); break;
    }
    //Set next compare interrupt (turn off next channel) and increase counter
    OCR1B = dutyCycles[offCounter++];
}
