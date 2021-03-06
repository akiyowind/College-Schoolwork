#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.c"


//States
enum SM1_States {sm1_start, sm1_init, sm1_setup, sm1_display, sm1_pressed, sm1_reset, sm1_reset_level, sm1_death} sm1_state;
enum lvl2_States {lvl2_start, lvl2_init, lvl2_setup, lvl2_display_1, lvl2_display_2, lvl2_display_3, lvl2_display_4, lvl2_pressed, lvl2_reset, lvl2_reset_level, sm2_death} lvl2_state;
enum level_SM_States {lvl_start, lvl_init, lvl_wait, lvl_up, lvl_down, lvl_select} lvl_state;
	
//Global variables
unsigned sm1_tempLED_column_sel = 0x00;
unsigned sm1_tempLED_column_val = 0x00;

int column_sel_index_lvl2 = 7;
int column_val_index_lvl2 = 7;
	
unsigned char levelTemp = 0x00;

unsigned int ledTimeEarly = 0;
unsigned int ledTimeLate = 0;

unsigned char lvl1_passedLED = 0x00;
unsigned char lvl2_passedLED = 0x00;

unsigned int lvl2_col_index = 0;
unsigned int lvl2_val_index = 0;
//unsigned char buttonTemp = 0x00;
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

//unsigned char column_sel_array[8] = {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE};
//unsigned char column_val_array[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

//LED Matrix arrays


void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;    // Timer interrupt will be generated when TCNT1==OCR1A
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

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

int hexadecimalToDecimal(char hexVal[])
{
	int len = strlen(hexVal);
	
	// Initializing base value to 1, i.e 16^0
	int base = 1;
	
	int dec_val = 0;
	
	// Extracting characters as digits from last character
	for (int i=len-1; i>=0; i--)
	{
		// if character lies in '0'-'9', converting
		// it to integral 0-9 by subtracting 48 from
		// ASCII value.
		if (hexVal[i]>='0' && hexVal[i]<='9')
		{
			dec_val += (hexVal[i] - 48)*base;
			
			// incrementing base by power
			base = base * 16;
		}
		
		// if character lies in 'A'-'F' , converting
		// it to integral 10 - 15 by subtracting 55
		// from ASCII value
		else if (hexVal[i]>='A' && hexVal[i]<='F')
		{
			dec_val += (hexVal[i] - 55)*base;
			
			// incrementing base by power
			base = base*16;
		}
	}
	
	return dec_val;
}


void SM1_Tick() {
	
	unsigned D0 = ~PIND & 0x01;
	unsigned D1 = ~PIND & 0x02;
	unsigned D2 = ~PIND & 0x04;
	
	// === Local Variables ===
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x7F; // grounds column to display pattern
	
	
	// === Transitions ===
	switch (sm1_state) {
		case sm1_start:
			sm1_state = sm1_init;
			break;
		case sm1_init:
			sm1_state = sm1_setup;
			break;
		case sm1_setup:
			sm1_state = sm1_display;
			break;
		case sm1_display:    
			sm1_state = D0 ? sm1_pressed : sm1_state;
			sm1_state = D2 ? sm1_reset : sm1_state;
			break;
		case sm1_pressed:
			sm1_state = D2 ? sm1_reset : sm1_state;
			sm1_state = D1 ? sm1_reset_level : sm1_state;
			break;
		case sm1_reset:
			sm1_state = sm1_start;
			break;
		case sm1_reset_level:
			sm1_state = sm1_start;
			break;
		case sm1_death:
			//sm1_state = D2 ? sm1_reset : sm1_state;
			//sm1_state = D1 ? sm1_reset_level : sm1_state;
			break;
		default:               
			sm1_state = sm1_display;
			break;
	}
	
	// === Actions ===
	switch (sm1_state) {
		case sm1_start:
			break;
		case sm1_init:
			TimerSet(2000);
			TimerOn();
			column_sel = 0xF7;
			column_val = 0x08;
			ledTimeLate = 0;
			ledTimeEarly = 2700;
			lvl1_passedLED = 0;
			LCD_ClearScreen();
			LCD_Cursor(1);
			break;
		case sm1_setup:
			TimerSet(100);
			TimerOn();
			column_sel = 0x7F;
			column_val = 0x01;
			break;
		case sm1_display:   // If illuminated LED in bottom right corner
			LCD_ClearScreen();
			LCD_DisplayString(1, "Timer: ");
			LCD_WriteData(1 + '0');
			LCD_WriteData(0 + '0');
			LCD_WriteData(0 + '0');
			if (column_sel == 0xFE && column_val == 0x80) {
				column_sel = 0x7F; // display far left column
				column_val = 0x01; // pattern illuminates top row
			}
			// else if far right column was last to display (grounded)
			else if (column_sel == 0xFE) {
				column_sel = 0x7F; // resets display column to far left column
				column_val = column_val << 1; // shift down illuminated LED one row
			}
			// else Shift displayed column one to the right
			else {
				column_sel = (column_sel >> 1) | 0x80;
			}
			if (lvl1_passedLED)
			{
				ledTimeLate += 100;
			}
			else if (PORTA == 0x08 && PORTB == 0xF7)
			{
				lvl1_passedLED = 0x01;
			}
			else
			{
				ledTimeEarly -= 100;
			}
			break;
		case sm1_pressed:
			if (PORTA == 0x08 && PORTB == 0xF7 || D0)
			{
				LCD_DisplayString(1, "Congratulations!");
			}
			else
			{
				//LCD_ClearScreen();
				//LCD_Cursor(1);
				
				//LCD_DisplayString(1, "STINKY");
				if (ledTimeLate > 0)
				{
					unsigned int tempLEDTime = ledTimeLate;
					unsigned int tempLEDTimeLate = ledTimeLate;
					if (ledTimeLate >= 1000)
					{
						LCD_DisplayString(1, "You were late by 1000 ms or more!");
					}
					else
					{
						if (ledTimeLate >= 100)
						{		
							LCD_DisplayString(1, "You were late by ");
							
							tempLEDTime = (int)(tempLEDTimeLate/100);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeLate = ledTimeLate - (tempLEDTime*100);
							
							tempLEDTime = (int)(tempLEDTimeLate/10);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeLate = tempLEDTimeLate - (tempLEDTime*10);
							
							LCD_WriteData(tempLEDTimeLate+'0');
						}
						/*else
						{
							LCD_DisplayString(1, "You were late by ");
							
							tempLEDTime = (int)(tempLEDTimeLate/100);
							tempLEDTimeLate = ledTimeLate - (tempLEDTime*100);
							
							tempLEDTime = (int)(tempLEDTimeLate/10);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeLate = tempLEDTimeLate - (tempLEDTime*10);
							
							LCD_WriteData(tempLEDTimeLate+'0');
						}*/
					}
				}
				else 
				{
					unsigned int tempLEDTime = ledTimeEarly;
					unsigned int tempLEDTimeEarly = ledTimeEarly;
					
					if (ledTimeEarly >= 1000)
					{
						LCD_DisplayString(1, "You were early 1000 ms or more!");
					}
					else if (ledTimeEarly >= 100)
					{
						LCD_DisplayString(1, "You were early by ");
						
						tempLEDTime = (int)(tempLEDTimeEarly/100);
						LCD_WriteData(tempLEDTime+'0');
						tempLEDTimeEarly = ledTimeEarly - (tempLEDTime*100);
										
						tempLEDTime = (int)(tempLEDTimeEarly/10);
						LCD_WriteData(tempLEDTime+'0');
						tempLEDTimeEarly = tempLEDTimeEarly - (tempLEDTime*10);
						
						LCD_WriteData(tempLEDTimeEarly+'0');
					}
					/*else
					{
							LCD_DisplayString(1, "You were early by ");
						
							tempLEDTime = (int)(tempLEDTimeEarly/100);
							tempLEDTimeEarly = ledTimeEarly - (tempLEDTime*100);
							
							tempLEDTime = (int)(tempLEDTimeEarly/10);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeEarly = tempLEDTimeEarly - (tempLEDTime*10);
							
							LCD_WriteData(tempLEDTimeEarly+'0');
					}*/
				}
			}
			break;
		case sm1_reset:
			column_sel = 0x7F;
			column_val = 0x01;
			break;
		case sm1_reset_level:
			lvl_state = lvl_init;
			levelTemp = 0x00;
			break;
		case sm1_death:
			break;
		default:
			break;
	}
	
	PORTA = column_val; // PORTA displays column pattern
	PORTB = column_sel; // PORTB selects column to display pattern

};

void lvl2_Tick() {
	
	unsigned D0 = ~PIND & 0x01;
	unsigned D1 = ~PIND & 0x02;
	unsigned D2 = ~PIND & 0x04;
	
	// === Local Variables ===
	//int column_sel_index = 7;
	//int column_val_index = 7;
	
	unsigned char column_sel_array[8] = {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE};
	unsigned char column_val_array[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
	
	// === Transitions ===
	switch (lvl2_state) {
		case lvl2_start:
			lvl2_state = lvl2_init;
			break;
		case lvl2_init:
			lvl2_state = lvl2_setup;
			break;
		case lvl2_setup:
			lvl2_state = lvl2_display_1;
			break;
		case lvl2_display_1:
			lvl2_state = D0 ? lvl2_pressed : lvl2_state;
			lvl2_state = D2 ? lvl2_reset : lvl2_state;
			lvl2_state = column_sel_index_lvl2 == 0 && column_val_index_lvl2 == 0 ? lvl2_display_2 : lvl2_state;
			//lvl2_state = column_sel_index == 0 && column_val_index == 0 ? lvl2_init : lvl2_state; 
			break;
		case lvl2_display_2:
			lvl2_state = D0 ? lvl2_pressed : lvl2_state;
			lvl2_state = D2 ? lvl2_reset : lvl2_state;
			lvl2_state = column_sel_index_lvl2 == 7 && column_val_index_lvl2 == 7 ? lvl2_display_3 : lvl2_state;
			break;
		case lvl2_display_3:
			lvl2_state = D0 ? lvl2_pressed : lvl2_state;
			lvl2_state = D2 ? lvl2_reset : lvl2_state;
			lvl2_state = column_sel_index_lvl2 == 0 && column_val_index_lvl2 == 0 ? lvl2_display_4 : lvl2_state;
			break;
		case lvl2_display_4:
			lvl2_state = D0 ? lvl2_pressed : lvl2_state;
			lvl2_state = D2 ? lvl2_reset : lvl2_state;
			break;
		case lvl2_pressed:
			lvl2_state = D2 ? lvl2_reset : lvl2_state;
			lvl2_state = D1 ? lvl2_reset_level : lvl2_state;
			break;
		case lvl2_reset:
			lvl2_state = lvl2_start;
			break;
		case lvl2_reset_level:
			lvl2_state = lvl2_start;
			break;
		default:
			lvl2_state = lvl2_display_1;
		break;
	}


	// === Actions ===
	switch (lvl2_state) {
		case lvl2_start:
			break;
		case lvl2_init:
			TimerSet(2000);
			TimerOn();
			column_sel_index_lvl2 = (rand() % 8);
			column_val_index_lvl2 = (rand() % 8);
			lvl2_col_index = column_sel_index_lvl2;
			lvl2_val_index = column_val_index_lvl2;
			ledTimeLate = 0;
			ledTimeEarly = 0;
			
			ledTimeEarly = (7 - lvl2_col_index)*75*8;
			ledTimeEarly += (7 - lvl2_val_index)*75;
			
			lvl2_passedLED = 0;
			LCD_ClearScreen();
			LCD_Cursor(1);
			break;
		case lvl2_setup:
			TimerSet(75);
			TimerOn();
			column_sel_index_lvl2 = 7;
			column_val_index_lvl2 = 7;
			break;
		case lvl2_display_1:   // If illuminated LED in bottom left corner
			/*if (column_sel_index_lvl2 == 0 && column_val_index_lvl2 == 0)
			{
				column_sel_index_lvl2 = 7;
				column_val_index_lvl2 = 7;
			}*/
			LCD_ClearScreen();
			LCD_DisplayString(1, "Timer: ");
			LCD_WriteData(7 + '0');
			LCD_WriteData(5+ '0');
			
			if (column_val_index_lvl2 == 0)
			{
				column_sel_index_lvl2 = column_sel_index_lvl2 - 1;
				column_val_index_lvl2 = 7;
			}
			else
			{
				column_val_index_lvl2 = column_val_index_lvl2 - 1;
			}
			if (lvl2_passedLED)
			{
				ledTimeLate += 75;
			}
			else if (PORTA == column_val_array[lvl2_val_index] && PORTB == column_sel_array[lvl2_col_index])
			{
				lvl2_passedLED = 0x01;
			}
			else
			{
				ledTimeEarly -= 75;
			}
			break;
		case lvl2_display_2:
			TimerSet(55);
			TimerOn();
			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Timer: ");
			LCD_WriteData(5 + '0');
			LCD_WriteData(5+ '0');
			/*if (column_sel_index_lvl2 == 0 && column_val_index_lvl2 == 0)
			{
				column_sel_index_lvl2 = 7;
				column_val_index_lvl2 = 7;
			}*/
			if (column_val_index_lvl2 == 7)
			{
				column_sel_index_lvl2 = column_sel_index_lvl2 + 1;
				column_val_index_lvl2 = 0;
			}
			else
			{
				column_val_index_lvl2 = column_val_index_lvl2 + 1;
			}
			ledTimeLate += 55;
			break;
		case lvl2_display_3:
			TimerSet(45);
			TimerOn();
			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Timer: ");
			LCD_WriteData(4 + '0');
			LCD_WriteData(5+ '0');
			/*if (column_sel_index_lvl2 == 0 && column_val_index_lvl2 == 0)
			{
				column_sel_index_lvl2 = 7;
				column_val_index_lvl2 = 7;
			}
			else*/ if (column_sel_index_lvl2 == 0)
			{
				column_val_index_lvl2 = column_val_index_lvl2 - 1;
				column_sel_index_lvl2 = 7;
			}
			else
			{
				column_sel_index_lvl2 = column_sel_index_lvl2 - 1;
			}
			ledTimeLate += 45;
			break;
		case lvl2_display_4:
			TimerSet(25);
			TimerOn();
			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Timer: ");
			LCD_WriteData(2 + '0');
			LCD_WriteData(5+ '0');
			if (column_val_index_lvl2 == 7 && column_sel_index_lvl2 == 7)
			{
				column_sel_index_lvl2 = 0;
				column_val_index_lvl2 = 0;
			}
			else if (column_sel_index_lvl2 == 7)
			{
				column_val_index_lvl2 = column_val_index_lvl2 + 1;
				column_sel_index_lvl2 = 0;
			}
			else
			{
				column_sel_index_lvl2 = column_sel_index_lvl2 + 1;
			}
			
			ledTimeLate += 25;
			break;
		case lvl2_pressed:
			//buttonTemp = 0x01;
			if (PORTA == column_val_array[lvl2_val_index] && PORTB == column_sel_array[lvl2_val_index] || D0)
			{
				LCD_DisplayString(1, "Congratulations!");
			}
			else
			{
				//LCD_ClearScreen();
				//LCD_Cursor(1);
				
				//LCD_DisplayString(1, "STINKY");
				if (ledTimeLate > 0)
				{
					unsigned int tempLEDTime = ledTimeLate;
					unsigned int tempLEDTimeLate = ledTimeLate;
					if (ledTimeLate >= 1000)
					{
						LCD_DisplayString(1, "You were late by 1000 ms or more!");
					}
					else
					{
						if (ledTimeLate >= 100)
						{		
							LCD_DisplayString(1, "You were late by ");
							
							tempLEDTime = (int)(tempLEDTimeLate/100);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeLate = ledTimeLate - (tempLEDTime*100);
							
							tempLEDTime = (int)(tempLEDTimeLate/10);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeLate = tempLEDTimeLate - (tempLEDTime*10);
							
							LCD_WriteData(tempLEDTimeLate+'0');
						}
						else
						{
							LCD_DisplayString(1, "You were late by ");
							
							tempLEDTime = (int)(tempLEDTimeLate/100);
							tempLEDTimeLate = ledTimeLate - (tempLEDTime*100);
							
							tempLEDTime = (int)(tempLEDTimeLate/10);
							LCD_WriteData(tempLEDTime+'0');
							tempLEDTimeLate = tempLEDTimeLate - (tempLEDTime*10);
							
							LCD_WriteData(tempLEDTimeLate+'0');
						}
					}
				}
				else 
				{
					unsigned int tempLEDTime = ledTimeEarly;
					unsigned int tempLEDTimeEarly = ledTimeEarly;
					
					if (ledTimeEarly >= 1000)
					{
						LCD_DisplayString(1, "You were early 1000 ms or more!");
					}
					else if (ledTimeEarly >= 100)
					{
						LCD_DisplayString(1, "You were early by ");
						
						tempLEDTime = (int)(tempLEDTimeEarly/100);
						LCD_WriteData(tempLEDTime+'0');
						tempLEDTimeEarly = ledTimeEarly - (tempLEDTime*100);
										
						tempLEDTime = (int)(tempLEDTimeEarly/10);
						LCD_WriteData(tempLEDTime+'0');
						tempLEDTimeEarly = tempLEDTimeEarly - (tempLEDTime*10);
						
						LCD_WriteData(tempLEDTimeEarly+'0');
					}
					else
					{
						LCD_DisplayString(1, "You were early by ");
						
						tempLEDTime = (int)(tempLEDTimeEarly/100);
						tempLEDTimeEarly = ledTimeEarly - (tempLEDTime*100);
						
						tempLEDTime = (int)(tempLEDTimeEarly/10);
						LCD_WriteData(tempLEDTime+'0');
						tempLEDTimeEarly = tempLEDTimeEarly - (tempLEDTime*10);
						
						LCD_WriteData(tempLEDTimeEarly+'0');
					}
				}
			}
			break;
		case lvl2_reset:
			TimerSet(75);
			TimerOn();
			column_sel_index_lvl2 = 7;
			column_val_index_lvl2 = 7;
			break;
		case lvl2_reset_level:
			lvl_state = lvl_init;
			levelTemp = 0x00;
			break;
		default:
		break;
	}
	
	PORTA = column_val_array[column_val_index_lvl2]; // PORTA displays column pattern
	PORTB = column_sel_array[column_sel_index_lvl2]; // PORTB selects column to display pattern

};


unsigned char level_SM_Tick()
{
	unsigned D0 = ~PIND & 0x01;
	unsigned D1 = ~PIND & 0x02;
	unsigned D2 = ~PIND & 0x04;
	
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x7F; // grounds column to display pattern
	
	switch(lvl_state)
	{
		case lvl_start:
			lvl_state = lvl_init;
			break;
		case lvl_init:
			lvl_state = lvl_wait;
			break;
		case lvl_wait:
			lvl_state = D0 ? lvl_up : lvl_state;
			lvl_state = D1 ? lvl_down : lvl_state;
			lvl_state = D2 ? lvl_select : lvl_state;
			break;
		case lvl_up:
			lvl_state = lvl_wait;
			break;
		case lvl_down:
			lvl_state = lvl_wait;
			break;
		case lvl_select:
			break;
		default:
			break;
	}
	
	switch(lvl_state)
	{
		case lvl_start:
			break;
		case lvl_init:
			column_val = 0x01;
			column_sel = 0x7F;			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Level: ");
			LCD_WriteData(1 + '0');
			break;
		case lvl_wait:
			LCD_ClearScreen();
			LCD_DisplayString(1, "Level: ");
			LCD_WriteData(column_val + '0');
			break;
		case lvl_up:
			if (column_val < 0x02)
			{
				column_val += 0x01;
			}
			break;
		case lvl_down:
			if (column_val > 0x01)
			{
				column_val -= 0x01;
			}
			break;
		case lvl_select:
			return column_val;
			break;
		default:
			break;
	}
	
	PORTA = column_val; // PORTA displays column pattern
	PORTB = column_sel; // PORTB selects column to display pattern
	
	return 0x00;
};

int main(void)
{
	//Timer
	TimerSet(100);
	TimerOn();
	
	//Ports
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xF9; PORTD = 0x07; // LCD control lines
	
	
	//Local variables
	unsigned char levelSelected = 0x00;
	unsigned char buttonPressed = 0x00;
	
	//sm states
	sm1_state = sm1_start;
	lvl_state = lvl_start;
	lvl2_state = lvl2_start;
	
	//LCD
	LCD_init();
	
	while (1)
	{
		levelSelected = levelTemp;
		
		if (levelSelected == 0x00)
		{
			levelTemp = level_SM_Tick();	
		}
		else if (levelSelected == 0x01)
		{	
			SM1_Tick();
		}
		else if (levelSelected == 0x02)
		{
			lvl2_Tick();
		}
		while (!TimerFlag)
		{
			
		}
		TimerFlag = 0;
			
	}
	
	return 1;
};