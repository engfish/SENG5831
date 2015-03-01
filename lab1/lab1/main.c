/* lab1 - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 2/7/2015 10:39:14 AM
 *  Author: A2CNMZZ
 */

#include <pololu/orangutan.h>
#include <stdbool.h>

// receive_buffer: A ring buffer that we will use to receive bytes on USB_COMM.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[32];

char command_buffer[32];

// receive_buffer_position: This variable will keep track of which bytes in the receive buffer
// we have already processed.  It is the offset (0-31) of the next byte
// in the buffer to process.
unsigned char receive_buffer_position = 0;
unsigned char command_buffer_position = 0;

#define forIterator10ms 5565

bool redForLoop = false;

int redFrequency = 1000;
int redCount = 0;
int redToggles = 0;
bool toggleRed = false;

int yellowFrequency = 10;
int yellowCount = 0;
int yellowToggles = 0;

int greenFrequency = 0;
int greenToggles = 0;

//-------------------------------------------------------------------------------------------
// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on USB_COMM.  We must call this before modifying
// send_buffer or trying to send more bytes, because otherwise we could
// corrupt an existing transmission.
void wait_for_sending_to_finish()
{
	while(!serial_send_buffer_empty(USB_COMM))
	serial_check();		// USB_COMM port is always in SERIAL_CHECK mode
}

// A generic function for whenever you want to print to your serial comm window.
// Provide a string and the length of that string. My serial comm likes "\r\n" at
// the end of each string (be sure to include in length) for proper linefeed.
void print_usb( char *buffer, int n ) {
	serial_send( USB_COMM, buffer, n );
	wait_for_sending_to_finish();
}

void setRedFrequency(int value)
{
	redCount = 0;
	redFrequency = value;
}

void setYellowFrequency(int value)
{
	yellowCount = 0;
	yellowFrequency = value / 100;
}

void setGreenFrequency(int value)
{
	int countPerMS = 19;
	
	if(value == 0)
	{
		TCCR1A = 0x00; //ClearOCRA and OCRA at top
		TCCR1B = 0x00; //Prescalar 1024
	}
	
	else
	{
		TCCR1A = 0x82; //ClearOCRA and OCRA at top
		TCCR1B = 0x1D; //Prescalar 1024
		ICR1 = countPerMS * value;
		OCR1A = (countPerMS * value) / 2;
	}
}

// process_received_byte: Responds to a byte that has been received on
// USB_COMM.
void process_received_byte(char byte)
{
	//put byte in command buffer
	command_buffer[command_buffer_position] = byte;
	command_buffer_position++;
	
	//if \n then process menu
	if(byte == 0x0D)
	{
		char tempBuffer[32];
		char color;
		char op_char;
		int value;
		int parsed;
		parsed = sscanf(command_buffer, "%c %c %d", &op_char, &color, &value);
		
		int length = sprintf( tempBuffer, "Op:%c C:%c V:%d\r\n", op_char, color, value );
		print_usb( tempBuffer, length );
		
		switch(op_char)
		{
			//Zero counters
			case 'Z':
			case 'z':
				switch(color)
					{
					case 'R': 
					case 'r':
						redToggles = 0;
						lcd_goto_xy(0,0);
						printf("        ");
						break;
				
					case 'G':
					case 'g':
						greenToggles = 0;
						lcd_goto_xy(0,1);
						printf("        ");
						break;
					
					case 'Y':
					case 'y':
						yellowToggles = 0;
						lcd_goto_xy(8,0);
						printf("        ");
						break;
					
					case 'A':
					case 'a':
						redToggles = greenToggles = yellowToggles = 0;
						clear();
						break;
					
					default:
						print_usb("Bad Color\r\n", 11);
						break;
				}
				break;

			case 'P':
			case 'p':
				switch(color)
				{
					case 'R':
					case 'r':
						length = sprintf( tempBuffer, "Red:%d\r\n", redToggles);
						print_usb( tempBuffer, length );
						break;
					
					case 'G':
					case 'g':
						length = sprintf( tempBuffer, "Green:%d\r\n", greenToggles);
						print_usb( tempBuffer, length );
						break;
					
					case 'Y':
					case 'y':
						length = sprintf( tempBuffer, "Yellow:%d\r\n", yellowToggles);
						print_usb( tempBuffer, length );
						break;
					
					case 'A':
					case 'a':
						length = sprintf( tempBuffer, "Red:%d Green:%d Yellow:%d\r\n", redToggles, greenToggles, yellowToggles);
						print_usb( tempBuffer, length );
						break;
					
					default:
						print_usb("Bad Color\r\n", 11);
						break;
				}
				break;

			case 'T':
			case 't':
				switch(color)
				{
					case 'R':
					case 'r':
						setRedFrequency(value);
						break;
					
					case 'G':
					case 'g':
						setGreenFrequency(value);
						break;
					
					case 'Y':
					case 'y':
						setYellowFrequency(value);
						break;
					
					case 'A':
					case 'a':
						setRedFrequency(value);
						setYellowFrequency(value);
						setGreenFrequency(value);
						break;
					
					default:
						print_usb("Bad Color\r\n", 11);
						break;
				}
				break;

			default:
				print_usb("Bad Command\r\n", 13);
				break;
		}
		
		//reset buffer and position
		for(int i = 0; i < 32; i++)
		{
			command_buffer[i] = 0;
		}
		
		command_buffer_position = 0;
	}
}

void check_for_new_bytes_received()
{
	while(serial_get_received_bytes(USB_COMM) != receive_buffer_position)
	{
		// Process the new byte that has just been received.
		process_received_byte(receive_buffer[receive_buffer_position]);

		// Increment receive_buffer_position, but wrap around when it gets to
		// the end of the buffer.
		if (receive_buffer_position == sizeof(receive_buffer)-1)
		{
			receive_buffer_position = 0;
		}
		else
		{
			receive_buffer_position++;
		}
	}
}

void determine10msIterator()
{
	long interations = 0;
	
	volatile long i = 0;
	long currentTime = get_ms();
	for(i = 0; i < 4000000; i++)
	{
		;
	}
	
	long loopTime = get_ms();

	interations = (10 * 4000000) / (loopTime - currentTime);
	
	lcd_goto_xy(0,0);
	printf("%lu", interations);
	//5565 iterations for 10ms execution time
}

//For loop wait 10ms
void Iterator10ms()
{
	volatile long i = 0;
	//long currentTime = get_ms();
	for(i = 0; i < forIterator10ms; i++)
	{
		;
	}
	
	//long loopTime = get_ms();
	//printf("%lu", loopTime - currentTime);
}

ISR(TIMER0_COMPA_vect)
{
	toggleRed = true;
}

ISR(TIMER1_COMPA_vect)
{
	//SREG |= 0x80;
	
	greenToggles++;
	lcd_goto_xy(0,1);
	printf("G:%d", greenToggles);
	
	/*for(int i = 0; i < 51; i ++)
	{
		Iterator10ms();
	}*/
}

ISR(TIMER1_OVF_vect)
{
	greenToggles++;
	lcd_goto_xy(0,1);
	printf("G:%d", greenToggles);
}

ISR(TIMER3_COMPA_vect)
{
	//SREG |= 0x80;
	
	yellowCount++;
	if(yellowFrequency != 0 && yellowCount == yellowFrequency / 2)
	{
		PORTA ^= 0x01;
		yellowToggles++;
		yellowCount = 0;
		
		lcd_goto_xy(8,0);
		printf("Y:%d", yellowToggles);
	}
	
	/*for(int i = 0; i < 51; i ++)
	{
		Iterator10ms();
	}*/
}

void setupInterrupt()
{
	SREG |= 0x80;
}

void setupRedInterrupt()
{
	DDRA |= 0x04;
	
	//OCR0A 1ms interrupt
	//20M/sec * 1sec/1000ms * 1count/256ticks * 1INT/78Counts = 1.0016ms
	//Need 256 prescalar and count of 78
	
	TCCR0A = 0x82; //ClearOCRA and OCRA at top
	TCCR0B = 0x04; //Prescalar 256
	OCR0A = 0x4E; // Set OCR0A Top Rate to 78
	
	//Enable interrupt
	TIMSK0 |= 0x02;
}

void setupRedForLoop()
{
	DDRA |= 0x04;
	
	redForLoop = true;
}

void setupYellowInterrupt()
{
	DDRA |= 0x01;
	
	//OCR3A 100ms 16-bit interrupt
	//20M/sec * 1sec/1000ms * 1count/64ticks * 1INT/31250 = 100ms
	//Need 64 prescalar and count of 31250
	
	TCCR3A = 0x80;
	TCCR3B = 0x0B;
	OCR3AH = 0x7A;
	OCR3AL = 0x12;
	
	//Enable interrupt
	TIMSK3 |= 0x02;
}

void setupGreenInterrupt()
{
	DDRD |= 0x20;
	
	//OCR1A 16-bit PWM
	//Mode 14, ICR whatever for top...ocr1a to half of top, clear on match
	
	TCCR1A = 0x82; //ClearOCRA and OCRA at top
	TCCR1B = 0x1D; //Prescalar 1024
	ICR1 = 0x4A38;
	OCR1A = 0x251C;
	
	//Enable interrupt
	TIMSK1 |= 0x03;
}

int main()
{
	lcd_init_printf();
	clear();
	
	//Determine for-loop iterations for 10ms (5565 iterations)
	//determine10msIterator();
	//Iterator10ms();
	
	//setup output
	PORTA = 0x00;
	
	//Experiment 1
	/*setupRedForLoop();
	setupYellowInterrupt();
	setupGreenInterrupt();
	setupInterrupt();*/
	
	//Experiment 2-6 (minus the busy-wait loops for 3-6)
	setupRedInterrupt();
	setupYellowInterrupt();
	setupGreenInterrupt();
	setupInterrupt();
	
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

	while(1)
	{
		serial_check();
		check_for_new_bytes_received();
		
		if(!redForLoop)
		{
			if(toggleRed)
			{
				redCount++;
				if(redFrequency != 0 && redCount == redFrequency / 2)
				{
					PORTA ^= 0x04;
					redToggles++;
					redCount = 0;
					
					lcd_goto_xy(0,0);
					printf("R:%d", redToggles);
				}
			
				toggleRed = false;
			}
		}
		
		else
		{
			Iterator10ms();
			
			redCount++;
			if(redFrequency != 0 && redCount == (redFrequency / 20))
			{
				PORTA ^= 0x04;
				redToggles++;
				redCount = 0;
				
				lcd_goto_xy(0,0);
				printf("R:%d", redToggles);
			}
		}
	}
}
