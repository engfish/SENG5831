/* week1_led_push - Application blinks Red and Green LEDs at 100ms frequency.
 * Top input button toggles blinking of red LED
 * Bottom input button toggles blinking of green LED
 *
 * Created: 1/26/2015 7:41:10 PM
 *  Author: Kyle English
 */

#include <pololu/orangutan.h>
#include <stdbool.h> 

int main()
{
	clear();
	red_led(0);
	green_led(0);
	bool redBlinking = false;
	bool greenBlinking = false;
	
	while(1)
	{
		unsigned char button = get_single_debounced_button_press(TOP_BUTTON | BOTTOM_BUTTON);
		
		if (button == TOP_BUTTON)     // display the button that was pressed
		{
			redBlinking = !redBlinking;
		}
		
		else if (button == BOTTOM_BUTTON)
		{
			greenBlinking = !greenBlinking;
		}
		
		if (redBlinking && greenBlinking)
		{
			red_led(1);
			green_led(1);
			delay_ms(50);
			red_led(0);
			green_led(0);
			delay_ms(50);
		}
		
		else if(redBlinking)
		{
			red_led(1);
			delay_ms(50);
			red_led(0);
			delay_ms(50);
		}
		
		else if(greenBlinking)
		{
			green_led(1);
			delay_ms(50);
			green_led(0);
			delay_ms(50);
		}
		
		else
		{
			red_led(0);
			green_led(0);
		}
	}
}
