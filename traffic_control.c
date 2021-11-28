/*
 * LabExamTrafficLightController.c
 * This simulates a traffic light controller where the red light is on for 4 seconds, then the green light is on for 2 seconds, and the yellow light is on for 2 seconds.
 * When the red light is on, the green and yellow lights MUST be off.
 * When the yellow light is on, the red and green lights MUST be off.
 * When the green light is on, the yellow and red lights MUST be off.
 * Pushing a button will trigger the pedestrian light.
 * The pedestrian light will be on while the green light is on.
 * The pedestrian light will blink on and off every 0.5 seconds while the yellow light is on.
 * The pedestrian light will be off when the red light is on
 * If the pedestrian crossing button has not been pushed, the pedestrian light will always remain off.
 * A user should only have to push the pedestrian button once to activate the pedestrian light for a full light cycle (from red, to green, to yellow, back to red)
 * Created: 10/22/2020 4:06:43 PM
 * Author : Cole
 */ 
#define F_CPU									16000000	//speed of ATmega328p is 16MHz

#include <avr/io.h>
#include <avr/interrupt.h>									//needed to make interrupts work
#include <util/delay.h>										//needed for debouncing

#define	TURN_ON(n)								PORTB |= 1<<n
#define TURN_OFF(n)								PORTB &= ~(1<<n)
#define  TOGGLE(n)								PORTB ^= 1<<n

#define F_COUNT 1000;										//frequency of count (period is 1ms)
#define V_OCR0									F_CPU/64/F_COUNT; //OCR0 value

volatile unsigned int count; //our counting variable, every count is 1 ms
unsigned char buttonpushed; //this is a global variable because it's the state of our button and is set and cleared by more than one function

enum {
RED												= PORTB0,
GREEN											= PORTB1,
YELLOW											= PORTB2,
PEDESTRIAN										= PORTB3
}; //traffic light signal


#define RED_ON									count < 4000
#define GREEN_ON								(count >= 4000 && count < 6000)
#define YELLOW_ON								!(RED_ON || GREEN_ON)

void button_press()
{
		//if PINC3 is LOW,  the button will be considered pushed
		 //need to implement a delay for debouncing our button input
		 if(!buttonpushed){
			 unsigned char t_buttonpushed = PINC & 1<<PINC3;
			 _delay_ms(20);
			 buttonpushed = !((PINC & 1<<PINC3) || t_buttonpushed);
		 }
}

void pedestrian_light()
{
	if(RED_ON) //duration of red light
	{
		//turn off pedestrian light
		TURN_OFF(PEDESTRIAN);
	}
	else//duration of green and yellow light
	{
		if(buttonpushed) //pedestrian light will only turn on if a button was pushed
		{
			if(GREEN_ON) //duration of green light
			{
				//turn on pedestrian light
				TURN_ON(PEDESTRIAN);
			}
			else if(YELLOW_ON)
			{
				if(count < 6500) //blink off 1st time during yellow
				{
					TURN_OFF(PEDESTRIAN);
				}
				else if(count < 7000) //blink on 1st time during yellow
				{
					//turn on pedestrian light
					TURN_ON(PEDESTRIAN);
				}
				else if(count < 7500) //blink off 2nd time during yellow
				{
					//turn off pedestrian light
					TURN_OFF(PEDESTRIAN);
				}
				else if(count < 8000) //blink on 2nd time during yellow, then reset button status
				{
					//turn on pedestrian light
					TURN_ON(PEDESTRIAN);
					//reset button status
					buttonpushed = 0;
				}
			}
		}	
	}
}

ISR(TIMER0_COMPA_vect)
{
	TCNT0 = 0;
	//this ISR counts in milliseconds, counts to 8 seconds and then resets to 0 and starts again
	if (count >= 8/1e-3) {
		count = 0;
		return;
	}
	count++;
}

void red_light()
{
	//turn red on
	TURN_ON(RED);
	//turn yellow off
	TURN_OFF(YELLOW);
	//turn green off
	TURN_OFF(GREEN);
}

void yellow_on()
{
	//red off
	TURN_OFF(RED);
	//yellow on
	TURN_ON(YELLOW);
	//green off
	TURN_OFF(GREEN);
}

void green_on()
{
	//red off
	TURN_OFF(RED);
	//yellow off
	TURN_OFF(YELLOW);
	//green on
	TURN_ON(GREEN);
}

void manage_traffic_lights()
{
	//lights start red
	if(RED_ON){
		red_light();
	}
	//green light after 4 seconds
	else if (GREEN_ON) {
		green_on();	
	}
	//yellow light after 2 seconds of green light
	else {
		yellow_on();
	}
}

int main(void)
{	
	//PORTB 0-3 is outputs
	DDRB = 1<<PORTB0
		   | 1<<PORTB1
		   | 1<<PORTB2
		   | 1<<PORTB3;
		   
	//PORTC is button input
	PORTC &= ~(1<<PORTC3);
	
	//turn on pull up resistor on PORTC3 so that we can use the pushbutton to force it low (wire pushbutton from C3 to GND)
	PORTC |= 1<<PORTC3; 
	
	PORTB = 0x00; //initialize all PORTB outputs as 0 at the start just in case
	TCNT0 = 0; //start Timer/Counter0 register at 0
	TCCR0A = 1<<COM0A0; //toggle OC0A on compare match
	TCCR0B = 1<<CS01 | 1<<CS00; //pre-scale of 64, so our clock slows down to 62500Hz
		   
	OCR0A =  V_OCR0; //ensures we hit output compare roughly every 1 ms
	buttonpushed = 0; //initially our button was not pushed
	TIMSK0 |= 1<<OCIE0A; //toggle output compare 0A interrupt enable so we can use the output compare interrupt
	sei(); //set interrupt enable so we can use whichever interrupts we want to use
    while (1) 
    {
		manage_traffic_lights(); //start red/green/yellow traffic light loop
		button_press();			//check to see if pedestrian button was pressed
		pedestrian_light();		//check to see if we need to turn on pedestrian light. If YES, activate pedestrian light loop
    }
}

