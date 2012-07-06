/*
* written by: Jan Sommer
* 05.07.2012
*
* ATtiny2313
* Dimms a 4 LEDs connected to OC0A/B and OC1A/B (Pins D5 and B2 - B4)
*
*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#define duty1	OCR0A
#define duty2	OCR0B
#define duty3	OCR1A
#define duty4	OCR1B


int main(void)
{

  //Setup the I/O for the LEDs

  //LED1
  DDRD  |= 1<<DDD5;		//Set PortD Pin5 (OC0B) as an output

  //LED2
  DDRB  |= 1<<DDB2;		//Set PortB Pin2 (OC0A) as an output
  DDRB  |= 1<<DDB3;		//Set PortB Pin3 (OC1A) as an output
  DDRB  |= 1<<DDB4;		//Set PortB Pin4 (O1BA) as an output
      
  //Setup the clocks
  cli();				//Disable global interrupts
  
  //Setup Timer0
//   TCCR0B |= 1<<CS01  | 1<<CS00;	//Divide by 64
  TCCR0A |= (1<<WGM00);			//Set phase correct PWM
  TCCR0A |= (1<<COM0A1) | (1<<COM0B1);	//Set both output pins to non-inverting PWM
  TCCR0B |=  1<<CS01;			//Divide by 8
  
  //Setup Timer1
//   TCCR1B |= 1<<CS11 | 1<<CS10;	//Divide by 64
  TCCR1B |= 1<<CS11;			//Divide by 8
  TCCR1A |= 1<<WGM10;			//Set phase correct PWM 8-Bit
  TCCR1A |= 1<<COM1A1 | 1<<COM1B1;	//Activate both output pins to non-inverting pwm
  
  //Initialize duty cycles at the beginning
  duty1 = 1;				
  duty2 = 255;
  duty3 = 1;
  duty4 = 255;
  
  sei();			//Enable global interrupts

  // Dimm the LEDs by changing the duty cycle of the PWMs
  int i;
  while(1) 
  {
      for(i=0; i<7; i++)
      {
	    duty1 *=2;
	    duty2 /=2;
	    duty3 *=2;
	    duty4 /=2;
	    _delay_ms(150);
      }
      _delay_ms(1000);
      
      for(i=0; i<7; i++)
      {
	    duty1 /=2;
	    duty2 *=2;
	    duty3 /=2;
	    duty4 *=2;
	    _delay_ms(150);
      }
      _delay_ms(1000);
  }
}