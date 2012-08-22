/**
*
* @file main.c
* @brief Hackaday.com AVR Tutorial firmware
* @author Mike Szczys (@szczys)
* @date 10/24/2010
*
* ATtiny2313
* Blinks one LED conneced to PD0
*
* http://hackaday.com/2010/10/25/avr-programming-02-the-hardware/
*/

#include <avr/io.h>
#include <avr/interrupt.h>

/**
 * @brief main program to blink an LED with Attiny2313
 *
 * Based on the tutorial of Mike Szczys the program was edited for the
 * Attiny2313
 * @return int
 */
int main(void)
{

  // Initialize OC1A pin (Pin PB3) for LED
  DDRB  |= 1<<DDB3;             // Set PortB Pin3 as an output (OC1A)
  PORTB |= 1<<PORTB3;           // Set PortB Pin3 high to turn on LED
      
  // Setup the clock
  cli();                        // Disable global interrupts
  TCCR1B |= 1<<CS11 | 1<<CS10;	// Divide by 64
  OCR1A = 15624;                // Count 15624 cycles for 1 second interrupt
  TCCR1B |= 1<<WGM12;           // Put Timer/Counter1 in CTC mode
  TCCR1A |= 1<<COM1A0;          // Toggle OC1A pin on compare match

  /*
   * Originally the timer interrupt was handled by a ISR which toggled the
   * pin. Now the pin is automatically toggled on compare match
   */
  //   TIMSK |= 1<<OCIE1A;		//enable timer compare interrupt

  sei();                        // Enable global interrupts

// Not needed with auto toogle mode
/*   Setup the I/O for the LED
 *    DDRD |= (1<<0);		//Set PortD Pin0 as an output
 *    PORTD |= (1<<0);		//Set PortD Pin0 high to turn on LED
 */
  
  while(1) { }              // Loop forever, interrupts do the rest
}

// Not needed with auto toogle mode
/* ISR(TIMER1_COMPA_vect)		//Interrupt Service Routine
 * {
 *   PORTD ^= (1<<0);		//Use xor to toggle the LED
 * }
 */
