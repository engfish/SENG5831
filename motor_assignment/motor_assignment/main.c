/* motor_lab - an application for the Pololu Orangutan SVP
 *
 * Motor warm-up assignment
 *
 * Created: 3/7/2015 9:58:54 AM
 *  Author: Kyle English
 */

#include <pololu/orangutan.h>
#include <stdbool.h>

int main()
{
	lcd_init_printf();
	clear();
	
	// Initialize the encoders and specify the four input pins.
	encoders_init(IO_D2, IO_D3, IO_D2, IO_D3);
	int speed = 8;
	
	while(1)
	{
		// Read the counts for motor 1 and print to LCD.
		lcd_goto_xy(0,0);
		printf("Encode:");
		print_long(encoders_get_counts_m1());
		
		lcd_goto_xy(0,1);
		printf("Speed:%d", speed);
		
		unsigned char button = get_single_debounced_button_press(TOP_BUTTON | BOTTOM_BUTTON | MIDDLE_BUTTON);
		
		if (button == TOP_BUTTON)
		{
			speed = speed + 8 > 256 ? 256 : speed + 8;
			
			clear();
		}
		
		else if (button == BOTTOM_BUTTON)
		{
			speed = speed - 8 < -256 ? -256 : speed - 8;
			
			clear();
		}
		
		else if (button == MIDDLE_BUTTON)
		{
			//reverse direction
			int newSpeed = speed * -1;
			bool positiveSpeed = speed > 0;
			
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
				
				delay_ms(2);
			}
			
			clear();
		}
		
		set_motors(speed, speed);
	}
}