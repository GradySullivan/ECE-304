/*
 * CovidProject.c
 *
 * Created: 4/3/2021 10:42:38 AM
 * Author : Grady Sullivan, Ashton Gray Abhinav Sharaff
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>

// pin declarations
#define RED 4		//PB4
#define YELLOW 5	//PC4
#define GREEN 4		//PC5

#define DIG1 0		//PC0
#define DIG2 1		//PC1
#define DIG3 2		//PC2
#define DIG4 3		//PC3

#define MOTOR 5		//PB5

#define TRIGGER1 1 //PB1
#define TRIGGER2 3 //PB3

// function declarations
void ledDisplay(float occupancy);

void triggerMotor(void); // simulate door opening

void displayCapacity(unsigned int number, unsigned int* digits); // displays capacity on 7 segment LED
void getDigits(unsigned int number, unsigned int* digits); // helper function for displayCapacity

float ultra_sonic_sensor1(void);
float ultra_sonic_sensor2(void);

int main(void)
{
	//LED pins
	DDRC |= (1<<YELLOW)|(1<<GREEN);
	DDRB |= (1<<RED);
	
	// timer
	sei();
	TCCR1B = (1<<CS12) | (1<<CS10); // 1024
	TCNT1 = 0;
	TCCR1A = 0; // timer mode - normal
	DDRB |= (1<<TRIGGER1) | (1<<TRIGGER2); // trigger outputs

	//motor pins
	DDRB |= 1 << MOTOR;
	
	//7 segment pins
	DDRD |= 0xFF; // turns on pins for 7 segment display "segments"
	DDRC |= (1<<DIG1)|(1<<DIG2)|(1<<DIG3)|(1<<DIG4); // turns on pins for digits

	unsigned int occupancy = 0;	// current occupancy
	unsigned int digits[4];		// digits used for 7 segment display
	
	float usstime = 0;
	float usstime2 = 0;
	
	while (1)
	{
		ledDisplay(occupancy);
		displayCapacity(occupancy, digits);
		
		// check if entering from outside
		usstime = ultra_sonic_sensor1();
		if (usstime < 150){
			triggerMotor();
			occupancy++;
		}
		
		ledDisplay(occupancy);
		displayCapacity(occupancy, digits); // this total delay should be long enough
		
		// check if entering from inside
		usstime2 = ultra_sonic_sensor2();
		if (usstime2 < 110){
			triggerMotor();
			occupancy--;
		}
	}
}

void ledDisplay(float occupancy)
{
	float occPercent = 0;	//occupancy percentage
	const int maxOcc = 50;	//max occupancy
	
	occPercent = (occupancy / maxOcc);
	if (occPercent < .40) // <40% occupancy
	{
		PORTC |= (1 << GREEN);
		PORTC &= ~(1 << YELLOW);
		PORTB &= ~(1 << RED);
	}
	
	else if (occPercent >= .40 && occPercent <= .80 ) // 40-80% capacity
	{
		PORTC &= ~(1 << GREEN);
		PORTC |= (1 << YELLOW);
		PORTB &= ~(1 << RED);
		
	}
	
	else // >80
	{
		PORTC &= ~(1 << GREEN);
		PORTC &= ~(1 << YELLOW);
		PORTB |= (1 << RED);
	}
}

void triggerMotor(void)
{
	PORTB |= 1 << MOTOR; // turn motor on
	_delay_ms(10000);	 // on for 10 seconds
	PORTB &= 0 << MOTOR; // turn motor off
}

float ultra_sonic_sensor1(void)
{
	// Trigger Pulse
	PORTB |= 1<< TRIGGER1;
	_delay_us(10);
	PORTB &= ~(1<< TRIGGER1);

	// Grab time value
	uint16_t echorising = TCNT1;
	
	while ((PINB & (1<<PINB0))  ==  0); // Wait for echo high on pb0
	while ((PINB & (1<<PINB0))  >  0);  // Wait for echo low on pb0
	
	uint16_t echofalling = TCNT1; // Grab time value

	float usstime = (echofalling - echorising);
	
	return usstime;
}

float ultra_sonic_sensor2(void)
{
	// Trigger Pulse
	PORTB |= 1<< TRIGGER2;
	_delay_us(10);
	PORTB &= ~(1<< TRIGGER2);

	// Grab time value
	uint16_t echorising = TCNT1;
	
	while ((PINB & (1<<PINB2))  ==  0); // Wait for echo high on pb2
	while ((PINB & (1<<PINB2))  >  0);  // Wait for echo low on pb2
	
	uint16_t echofalling = TCNT1; // Grab time value

	float usstime = echofalling - echorising;

	return usstime;
}

void displayCapacity(unsigned int capacity, unsigned int* digits)
{
	unsigned ledDigits[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67}; // 0 thru 9
	unsigned persistence = 2; // delay before displaying next digit
	
	getDigits(capacity, digits); // set digits into array

	unsigned char i = 0;
	unsigned char repeats = 100;
	
	for (i = 0; i<repeats; i++){
		// thousands digit
		PORTD = ledDigits[digits[0]];
		PORTC &= ~(1<<DIG1);
		_delay_ms(persistence);
		PORTC |= (1<<DIG1);
		
		// hundred digit
		PORTD = ledDigits[digits[1]];
		PORTC &= ~(1<<DIG2);
		_delay_ms(persistence);
		PORTC |= (1<<DIG2);
		
		// tens digit
		PORTD = ledDigits[digits[2]];
		PORTC &= ~(1<<DIG3);
		_delay_ms(persistence);
		PORTC |= (1<<DIG3);
		
		//ones digit
		PORTD = ledDigits[digits[3]];
		PORTC &= ~(1<<DIG4);
		_delay_ms(persistence);
		PORTC |= (1<<DIG4);
	}
}

void getDigits(unsigned int number, unsigned int* digits)
{
	digits[0] = (number/1000)%10; // thousands place
	digits[1] = (number/100)%10;  // hundreds place
	digits[2] = (number/10)%10;   // tens place
	digits[3] = (number%10);      // ones place
}