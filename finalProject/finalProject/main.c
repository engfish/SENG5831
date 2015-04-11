/* finalProject - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 4/11/2015 11:23:30 AM
 *  Author: A2CNMZZ
 */

#include <pololu/orangutan.h>

int main()
{
	while(1)
	{
		// Get battery voltage (in mV) from the auxiliary processor
		// and print it on the the LCD.
		clear();
		print_long(read_battery_millivolts_svp());

		red_led(1);     // Turn on the red LED.
		delay_ms(200);  // Wait for 200 ms.

		red_led(0);     // Turn off the red LED.
		delay_ms(200);  // Wait for 200 ms.
	}
}
