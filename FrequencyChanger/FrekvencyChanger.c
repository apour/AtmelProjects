/*
 * FrequencyChanger.c
 *
 * Created: 27.8.2022 18:13:48
 * Author: Ales
 *
 * MEASURE FREQUENCY ON PD3 (INT1) AND SEND INFO BY UART 19200 B
 * Input delta is set by voltage frequency on PIN PC1
 * Output Frequency is on PINT PC0
 * Output duty is on PC2
 * Input temp sensor by voltage on PIN PC4
 *
 * Must be connected
 * -----------------
 * AGND - Ground
 * AREF - +5V
 * AVCC - +5V
 * 
 *
 */ 

//#define __AVR_ATmega8__
#define F_CPU 12000000UL // Clock Speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

#define ADJUST_VOLTAGE_FREQUENCY_INPUT_PIN				1
#define TEMPERATURE_SENSOR_VOLTAGE_INPUT_PIN			3
//#define DUMP_TO_UART									1
#define OUTPUT_DUTY_MIN									20
#define OUTPUT_DUTY_MAX									100
#define TEMPERATURE_SENSOR_MIN_VALUE					0x298

// counters
volatile unsigned int repeat_cnt1=0;
volatile unsigned int repeat_cnt2=0;

// measure input signal
volatile unsigned int measureFrc=0;
volatile unsigned int signalDetectorCounter=0;
volatile unsigned int lastSignalDetectorCounter=0;

// output signal
unsigned short outPinHi;
volatile unsigned int outSignalLimit=0;
volatile unsigned int signalOutCounter=0;
unsigned long deltaOutSignalLimit=0;

volatile unsigned short needChange = 0;
volatile unsigned short needCheck = 0;
	
void timer0Init()
   {
   // enable interrupt from timer 0 
   TIMSK = TIMSK | _BV(TOIE0);
   // set CLOCK / 64
   TCCR0 = _BV(CS01) | _BV(CS00);
   // set counter
   TCNT0 = 241;
   sei();
   }

void OutFrequenceChangeLogicLevel()
	{
	if (outPinHi==1)
		{
		PORTC&= ~_BV(PC0);	
		outPinHi=0;
		}		 
	else
		{
		PORTC|= _BV(PC0);
		outPinHi=1;
		}	   
	}			
      
SIGNAL(SIG_OVERFLOW0)
   {
   cli();
   // set counter
   TCNT0 = 241;
   signalDetectorCounter++;
   signalOutCounter++;
   
   if (signalOutCounter>outSignalLimit)
		{
		needChange = 1;
		signalOutCounter = 0;
	    } 
   sei();  
   }


SIGNAL(SIG_INTERRUPT1)
	{
	cli();
	needCheck = 1;
	sei();
	}	

static void hardwareInit(void)
{
    PORTD = 0xfa;   /* 1111 1010 bin: activate pull-ups except on USB lines */
    DDRD = 0x02;    /* 0000 0010 bin: remove USB reset condition */
	
    // set PC0 and PC2 as output
	DDRC = _BV(PC0) | _BV(PC2);
	PORTC = 0;
	
	// SET INT1 for input frequency
	PORTD|= _BV(PD3);
	// SET INT0 for input frequency temperature
	//PORTD|= _BV(PD2);
	
	// set PC0 and PC2 as output
	DDRC = _BV(PC0) | _BV(PC2);
	PORTC = 0;
	
	GICR |= _BV(INT1);	/* enable INT1 */
	//GICR |= _BV(INT0);	/* enable INT0 */
	MCUCR|= _BV(ISC11);
	MCUCR|= _BV(ISC01);

	sei();
}
	
int main(void)  
{
	hardwareInit();
	timer0Init();
	
	// test 
	outSignalLimit = 1000;
	lastSignalDetectorCounter = 1000;
	
	outPinHi = 0;
    sei();
	
	while (1)
		{
		if (needChange)
			{
			OutFrequenceChangeLogicLevel();
			needChange = 0;
			}
		
		if (needCheck)
			{
			needCheck = 0;
			outSignalLimit = signalDetectorCounter<<1;
			signalDetectorCounter=0;		
			if (signalOutCounter>outSignalLimit)
				{
				signalOutCounter = 0;
				needChange = 1;
				}
			}		
		}			
		
	return 1;
}	