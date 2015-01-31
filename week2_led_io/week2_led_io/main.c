/* week2_led_io - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  The top button toggles the LED
 * located in A7 blinking while the bottom toggles the LED located in A0 blinking.  With
 * putty/serial communication, a '+' will increase the frequency of blinking by 50 ms (decreasing the
 * time between blinking) while a '-' will do the opposite.  Only if the LED is currently blinking
 * will it change the frequency, this way the user can toggle one frequency without the other.
 * The min and max frequencies are 100ms
 * and 2000ms.The display shows the current frequencies of the two ports.
 *
 * Created: 1/30/2015 10:44:16 AM
 *  Author: Kyle English
 */

#include <pololu/orangutan.h>
#include <stdbool.h> 

// receive_buffer: A ring buffer that we will use to receive bytes on USB_COMM.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[32];

// receive_buffer_position: This variable will keep track of which bytes in the receive buffer
// we have already processed.  It is the offset (0-31) of the next byte
// in the buffer to process.
unsigned char receive_buffer_position = 0;

int a7Freuquency = 500;
int a1Freuquency = 1000;
bool a7Blinking = false;
bool a1Blinking = false;

// process_received_byte: Responds to a byte that has been received on
// USB_COMM.
void process_received_byte(char byte)
{
	switch(byte)
	{
		// If the character '+' is received, increase frequency of blinks
		case '+':
			if(a7Blinking)
			{
				a7Freuquency = a7Freuquency - 50 < 100 ? a7Freuquency : a7Freuquency - 50;
			}
			
			if(a1Blinking)
			{
				a1Freuquency = a1Freuquency - 50 < 100 ? a1Freuquency : a1Freuquency - 50;
			}
			break;

		// If the character '-' is received, decrease the frequency of blinks
		case '-':
			if(a7Blinking)
			{
				a7Freuquency = a7Freuquency + 50 > 2000 ? a7Freuquency : a7Freuquency + 50;
			}
			
			if(a1Blinking)
			{
				a1Freuquency = a1Freuquency + 50 > 2000 ? a1Freuquency : a1Freuquency + 50;
			}
			break;

		default:
			break;
	}
	
	clear();
	printf("A7:%i ms", a7Freuquency);
	printf("\nA0:%i ms", a1Freuquency);
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

int main()
{
	lcd_init_printf();
	clear();
	printf("A7:%i ms", a7Freuquency);
	printf("\nA0:%i ms", a1Freuquency);
	
	int a7Counter = 0;
	int a1Counter = 0;
		
	DDRA = 0x81;
	PORTA = 0x00;
		
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));
		
	while(1)
	{
		serial_check();
		check_for_new_bytes_received();
		
		unsigned char button = get_single_debounced_button_press(TOP_BUTTON | BOTTOM_BUTTON);
		
		if (button == TOP_BUTTON)     // display the button that was pressed
		{
			a7Blinking = !a7Blinking;
		}
		
		else if (button == BOTTOM_BUTTON)
		{
			a1Blinking = !a1Blinking;
		}
		
		int currentTime = get_ms();
		
		if (a7Blinking)
		{
			if(a7Counter <= currentTime - a7Freuquency)
			{
				PORTA ^= 0x80;
				a7Counter = currentTime;
			}
		}
			
		else
		{
			PORTA &= 0x7F;
		}
			
		if(a1Blinking)
		{
			if(a1Counter <= currentTime - a1Freuquency)
			{
				PORTA ^= 0x01;
				a1Counter = currentTime;
			}
		}
		
		else
		{
			PORTA &= 0xFE;
		}
	}
}
