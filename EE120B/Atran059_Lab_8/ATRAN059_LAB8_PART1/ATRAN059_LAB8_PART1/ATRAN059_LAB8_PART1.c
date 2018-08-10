/*
 * ATRAN059_LAB5_PART1.c
 *
 * Created: 2/1/2018 3:57:38 PM
 *  Author: student
 */ 


#include <avr/io.h>
volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks	
enum States {Start, Init, Wait, SwitchOne, SwitchTwo, SwitchThree} state;
enum States2 {Start2, Init2, Wait2, SwitchOne2, SwitchTwo2, SwitchThree2} state2;
//enum States3 {Start3, Init3, Wait3, SwitchOne3, SwitchTwo3, SwitchThree3} state3;
//enum SPKStates {SPKStart, SPKInit, SPKOn, SPKOff} SPKState;

unsigned char threeLEDs;
unsigned char blinkingLED;
unsigned char combineLED;
	
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}

void TimerISR() {
	TimerFlag = 1;
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

	
/*void Tick() {
		switch (state) {
		case Start:
			state = Init;
			break;
		case Init:
			state = Wait;
			break;
		case Wait:
			state = threeLEDs = 0x00 ? SwitchOne : state; 
			state = threeLEDs == 0x01 ? SwitchTwo : state;
			state = threeLEDs == 0x02 ? SwitchThree: state;
			state = threeLEDs == 0x04 ? SwitchOne : state; 
			break;
		case SwitchOne:
			break;
		case SwitchTwo:
			break;
		case SwitchThree:
			break;
		default:
			break;
	}
	
	switch (state) {
		case Start:
			break;
		case Init:
			threeLEDs = 0x00;
			break;
		case Wait:
			break;
		case SwitchOne:
			threeLEDs = 0x01;
			break;
		case SwitchTwo:
			threeLEDs = 0x02;
			break;
		case SwitchThree:
			threeLEDs = 0x04;
			break;
		default:
			break;
	}
	

	
	
}

void BL_Tick()
{
			switch (state2) {
				case Start:
				state2 = Init;
				break;
				case Init:
				state2 = Wait;
				break;
				case Wait:
				state2 = blinkingLED = 0x00 ? SwitchOne : state2;
				state2 = blinkingLED == 0x08 ? SwitchTwo : state2;
				state2 = blinkingLED == 0x01 ? SwitchOne: state2;
				break;
				case SwitchOne:
				break;
				case SwitchTwo:
				break;
				case SwitchThree:
				break;
				default:
				break;
			}
			
			switch (state2) {
				case Start:
				break;
				case Init:
				blinkingLED = 0x00;
				break;
				case Wait:
				break;
				case SwitchOne:
				blinkingLED = 0x08;
				break;
				case SwitchTwo:
				threeLEDs = 0x01;
				break;
				case SwitchThree:
				break;
				default:
				break;
			}
}

void CB_Tick()
{
					switch (state3) {
						case Start:
						state3 = Init;
						break;
						case Init:
						break;
						default:
						break;
					}
					switch (state3) {
						case Start:
						combineLED = 0x00;
						break;
						case Init:
						combineLED = threeLEDs | blinkingLED;
						PORTB = combineLED;
						break;
						default:
						break;
					}
}

void SP_Tick()
{
	switch (SPKStates)
	{
		case Start:
			SPKState = Init;
			break;
		case Init:
			SPKState = GetBit(PINA, 2) ? SPKOn : SPKOff;
			break;
		case SPKOn:
			SPKState = Init;
			break;
		case SPKOff:
			SPKState = Init;
			break;
		default:
			break;
		
			
		case Start:
			break;
		case Init:
			break;
		case SPKOn:
			PORTB = SetBit(PORTB, 4, 1);
			break;
		case SPKOff:
			PORTB = SetBit(PORTB, 4, 0);
			break;
			default:
			break;
	}
}*/

int main(void)
{
	ADC_init();
	unsigned short result = PINA;
	
    while(1)
    {
		PORTB = (char)result;
		PORTD = (char)result;
		PORTD = (char)(PORTD >> 6);
        /*Tick();
		while (!TimerFlag) {}
		TimerFlag = 0;*/
    }
}