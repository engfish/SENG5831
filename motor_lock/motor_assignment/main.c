/* motor_lab - an application for the Pololu Orangutan SVP
 *
 * Motor warm-up assignment
 *
 * Created: 3/7/2015 9:58:54 AM
 *  Author: Kyle English
 */

#include <pololu/orangutan.h>
#include <stdbool.h>
#include "serial.h"

volatile int encode_lock = 2730;
static int maxSpeed = 96;

bool isLocked = false;
int speed = 0;
volatile long encoder_count_m2 = 0;

void slowDownMotor(bool positiveSpeed)
{
	int newSpeed = 0;
	while(speed != newSpeed)
	{
		if(positiveSpeed)
		{
			speed--;
		}
		
		else
		{
			speed++;
		}
		
		lcd_goto_xy(0,0);
		printf("Encode:");
		print_long(encoders_get_counts_m1());
		
		lcd_goto_xy(0,1);
		printf("Speed:%d", speed);
		
		set_motors(speed, speed);
		
		delay_ms(1);
	}
}

void speedUpMotor(bool positiveSpeed)
{
	int newSpeed = positiveSpeed ? maxSpeed : -maxSpeed;
	
	while(speed != newSpeed)
	{
		if(positiveSpeed)
		{
			speed++;
		}
		
		else
		{
			speed--;
		}
		
		lcd_goto_xy(0,0);
		printf("Encode:");
		print_long(encoders_get_counts_m1());
		
		lcd_goto_xy(0,1);
		printf("Speed:%d", speed);
		
		set_motors(speed, speed);
		
		delay_ms(1);
	}
}

void lockDoor()
{
	speedUpMotor(false);
	
	//logic to stop
	encoder_count_m2 = encoders_get_counts_m2();
	
	while(encoder_count_m2 + 160 < encode_lock)
	{
		delay_ms(1);
		encoder_count_m2 = encoders_get_counts_m2();
	}
	
	slowDownMotor(false);
	isLocked = true;
}

void unlockDoor()
{
	speedUpMotor(true);
	
	//logic to stop
	encoder_count_m2 = encoders_get_counts_m2();
	
	while(encoder_count_m2 - 135 > 0)
	{
		delay_ms(1);
		encoder_count_m2 = encoders_get_counts_m2();
	}
	
	slowDownMotor(true);
	isLocked = false;
}

int main()
{
	lcd_init_printf();
	uart_init();
    clear();
	
	// Initialize the encoders and specify the four input pins.
	encoders_init(IO_D0, IO_D1, IO_D0, IO_D1);
	
	while(1)
	{
		// Read the counts for motor 2 and print to LCD.
		encoder_count_m2 = encoders_get_counts_m2();
		lcd_goto_xy(0,0);
		printf("Encode:");
		print_long(encoder_count_m2);
		
		lcd_goto_xy(0,1);
		printf("Speed:%d", speed);
		
		unsigned char button = get_single_debounced_button_press(TOP_BUTTON | BOTTOM_BUTTON);
		
		if (button == TOP_BUTTON)
		{
			lockDoor();
		}
		
		else if (button == BOTTOM_BUTTON)
		{
			unlockDoor();
		}

		// USB_COMM is always in SERIAL_CHECK mode, so we need to call this
		// function often to make sure serial receptions and transmissions
		// occur.
		serial_check();

		// Deal with any new bytes received.
		serial_check_for_new_bytes_received();
		
		//set_motors(speed, speed);
	}
}


void uart_init() {
	
	//****************************************************************************
	// Enable Serial Ports
	//**********************************************************************************
	// Set the Baud Rate to match the default bluesmirf baud rate 115200.
	serial_set_baud_rate(UART1, 9600);
	
	// Set the mode of UART1 to serial Check
	serial_set_mode(UART1, SERIAL_CHECK);
	
	// Start receiving bytes in the ring buffer.
	serial_receive_ring(UART1, receive_buffer, sizeof(receive_buffer));	
}

void processByte(char byte)
{
    switch (byte) {
		case 'H':
			red_led(1);
		    break;
	    case 'U':
		if(isLocked)
		{
			unlockDoor();
		}
			break;
		case 'L':
		if(!isLocked)
		{
			lockDoor();
		}
			break;
		default:
		   break;
	}
}