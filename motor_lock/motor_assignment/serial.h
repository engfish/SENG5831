/*
 * IncFile1.h
 *
 * Created: 2/25/2015 12:55:07 AM
 *  Author: jwfregie
 */ 

#ifndef serial_H_
#define serial_H_

char receive_buffer[32];

unsigned char receive_buffer_position = 0;

char serial_send_buffer[32];

void serial_wait_for_sending_to_finish()
{
	while(!serial_send_buffer_empty(UART1))
	serial_check();		// USB_COMM port is always in SERIAL_CHECK mode
}

// process_received_byte: Responds to a byte that has been received on
// USB_COMM.  
void processBytes(char byte);

void serial_check_for_new_bytes_received()
{
	while(serial_get_received_bytes(UART1) != receive_buffer_position)
	{
		// Process the new byte that has just been received.
		processByte(receive_buffer[receive_buffer_position]);

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


#endif /* serial_H_ */