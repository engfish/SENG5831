/* motor_lab - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 3/7/2015 11:19:01 AM
 *  Author: Kyle English
 * 
 * Doesn't work for maintaining speed when encounter rolls over briefly
 * Max speed value is about 6600 counts/sec
 * 'T' stops execution, doesn't clear Pid so set to 0 and run before starting again
 * p/d command add or subtract value/10 to gain
 * r/s command set gain values to default speed/position values
 * some ocsillation around the position because the small values of torque aren't enough to turn the motor
 * New i/I command to init interpolator for questions 4 and 5 in the lab
 */

//6600 counts/sec Max = speed 255

#include <pololu/orangutan.h>
#include <stdbool.h>

typedef struct
{
	double dState; // Last position input
	double iState; // Integrator state
	double iMax, iMin; // Maximum and minimum allowable integrator state
	double iGain; // integral gain
	double pGain; // proportional gain
	double dGain; // derivative gain
	int desiredValue;
	bool desireSpeed;
	int lastValue;
} Pid;

typedef struct
{
	int interType; // 0 PID, 1 Sleep
	int interValue;
} Interpolator;

Pid globalPID;

Interpolator *myInterpolator[5];
int currentInter = 0;
bool runInter = false;
bool isSleeping = false;
int lastInterValue = 0;
int lastInterCount = 0;

int interSleepCount = 0;

static int encode360 = 2249; // encodes per 360 degrees
static double encodeTorquePerSecond = 25.9; // encode counts per second per torque

static int PidFrequency = 1; //in 10ms increments
int PidFrequencyCounter = 0;
bool runPid = false;

long encounterBuffer[10] = {0,0,0,0,0,0,0,0,0,0};
int encounterBufferPosition = 0;
double avgSpeed = 0;

bool execute = false;
bool logPid = false;
bool isInter = false;

static char global_m2a;
static char global_m2b;

static long global_counts_m2;

static char global_last_m2a_val;
static char global_last_m2b_val;

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

//Return speed value for motor given the new value (speed/position)
int UpdatePID(double newValue)
{
	double pTerm, dTerm, iTerm;
	
	pTerm = globalPID.pGain * (globalPID.desiredValue - newValue); // calculate the proportional term
	
	globalPID.iState += (globalPID.desiredValue - newValue);
	if (globalPID.iState > globalPID.iMax) globalPID.iState = globalPID.iMax;
	else if (globalPID.iState < globalPID.iMin) globalPID.iState = globalPID.iMin;
	iTerm = globalPID.iGain * globalPID.iState; // calculate the integral term
	
	dTerm = globalPID.dGain * (globalPID.dState - newValue);  //calculate the d term
	globalPID.dState = newValue;
	
	int torque = (int)(pTerm + dTerm + iTerm);
	//torque = -torque;
	
	if(globalPID.desireSpeed)
	{
		torque = (int)((double)torque / encodeTorquePerSecond); //(counts / second / torque)
		torque += globalPID.lastValue;
	}
	
	torque = torque > 255 ? 255 : (torque < -255 ? -255 : torque);
	
	globalPID.lastValue = torque;
	
	return torque;
}

//Set Motor 2 to given speed/direction
void setMotor(int motorSpeed)
{
	unsigned char reverse = 0;

	if (motorSpeed < 0)
	{
		motorSpeed = -motorSpeed;	// make speed a positive quantity
		reverse = 1;	// preserve the direction
	}
	
	OCR2B = motorSpeed;
	
	if (motorSpeed > 0xFF)	// 0xFF = 255
	{
		motorSpeed = 0xFF;
	}

	if (motorSpeed == 0)
	{
		// Achieve a 0% duty cycle on the PWM pin by driving it low,
		// disconnecting it from Timer2
		TCCR2A &= ~(1<<COM2B1);
	}
	
	else
	{
		// Achieve a variable duty cycle on the PWM pin using Timer2.
		TCCR2A |= 1<<COM2B1;

		if (reverse)
		{
			PORTC |= 0x40;
		}
		else
		{
			PORTC &= 0xBF;
		}
	}
}

//Init speed Pid gain values
void initSpeedPID()
{
	globalPID.dGain = 0.3;
	globalPID.pGain = 0.3;
	globalPID.iGain = 0.01;
	globalPID.desireSpeed = true;
}

//Init position Pid gain values
void initPositionPID()
{
	globalPID.dGain = 0.3;
	globalPID.pGain = 0.2;
	globalPID.iGain = 0.01;
	globalPID.desireSpeed = false;
}

//initInterpolator
void initInterpolator()
{
	initPositionPID();
	
	Interpolator a;
	a.interType = 0;
	a.interValue = (encode360 * ( 0.25 ));
	myInterpolator[0] = calloc( 1, sizeof(Interpolator));
	memcpy( myInterpolator[0], &a, sizeof(Interpolator));

	a.interType = 1;
	a.interValue = 50000;
	myInterpolator[1] = calloc( 1, sizeof(Interpolator));
	memcpy( myInterpolator[1], &a, sizeof(Interpolator));
	
	a.interType = 0;
	a.interValue = (encode360 * ( -1 ));
	myInterpolator[2] = calloc( 1, sizeof(Interpolator));
	memcpy(myInterpolator[2], &a, sizeof(Interpolator));
	
	a.interType = 1;
	a.interValue = 500;
	myInterpolator[3] = calloc( 1, sizeof(Interpolator));
	memcpy(myInterpolator[3], &a, sizeof(Interpolator));
	
	a.interType = 0;
	a.interValue = (encode360 * ( 0.0139 ));
	myInterpolator[4] = calloc( 1, sizeof(Interpolator));
	memcpy(myInterpolator[4], &a, sizeof(Interpolator));
}

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
		char tempBuffer[255];
		char sendBuffer[255];
		char op_char;
		int value;
		int parsed;
		parsed = sscanf(command_buffer, "%c %d", &op_char, &value);
		
		int length = sprintf( tempBuffer, "Op:%c V:%d\r\n", op_char, value );
		print_usb( tempBuffer, length );
		
		switch(op_char)
		{
			//Set desired reference position
			case 'R':
			case 'r':
			initPositionPID();			
			globalPID.desireSpeed = false;
			globalPID.desiredValue = value;
			isInter = false;
			break;
			
			//Set desired reference speed
			case 'S':
			case 's':
			initSpeedPID();
			globalPID.desireSpeed = true;
			globalPID.desiredValue = value;
			isInter = false;
			break;
			
			case 'I':
			case 'i':
			initInterpolator();
			isInter = true;
			currentInter = 0;
			globalPID.desiredValue = myInterpolator[currentInter]->interValue;
			break;

			//Increase/Decrease pGain
			case 'P':
			globalPID.pGain +=((double) value)/(double)10;
			break;
			
			case 'p':
			globalPID.pGain -= ((double) value)/(double)10;;
			break;

			//Increase/Decrease dGain
			case 'D':
			globalPID.dGain += ((double) value)/10;;
			break;
			
			case 'd':
			globalPID.dGain -= ((double) value)/10;;
			break;
			
			case 't':
			UpdatePID(0.0);
			execute = true;
			break;
			
			case 'T':
			execute = false;
			setMotor(0);
			break;
			
			case 'v':
			case'V':
			if(globalPID.desireSpeed)
			{
				length = sprintf( sendBuffer, "PGain:%d.%d DGain:%d.%d PDesired:%d PMeasured:%d Torque:%d\r\n", (int)(globalPID.pGain) % 10, (int)(globalPID.pGain * 10) % 10, 
					(int)(globalPID.dGain) % 10, (int)(globalPID.dGain * 10) % 10, globalPID.desiredValue, (int)avgSpeed, globalPID.lastValue );
			}
			
			else
			{
				length = sprintf( sendBuffer, "PGain:%d.%d DGain:%d.%d PDesired:%d PMeasured:%d Torque:%d\r\n", (int)(globalPID.pGain) % 10, (int)(globalPID.pGain * 10) % 10,
					(int)(globalPID.dGain) % 10, (int)(globalPID.dGain * 10) % 10, globalPID.desiredValue, global_counts_m2, globalPID.lastValue );
			}
			
			print_usb(sendBuffer, length);
			break;
			
			case 'l':
			case 'L':
			logPid = !logPid;
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

//Speed/Position Calc Interrupt
ISR(TIMER0_COMPA_vect)
{
	if(execute)
	{
		encounterBufferPosition++;
	
		if(encounterBufferPosition == 10)
		{
			encounterBufferPosition = 0;
		}
	
		encounterBuffer[encounterBufferPosition] = global_counts_m2;
	
		//Calc Avg Speed if speed
		if((globalPID.desireSpeed))
		{
			avgSpeed = encounterBuffer[encounterBufferPosition] - encounterBuffer[(encounterBufferPosition == 9 ? 0 : encounterBufferPosition + 1)];
		
			avgSpeed = avgSpeed * 10; // gets to counts per second
		}
	
		PidFrequencyCounter++;
		if(PidFrequencyCounter == PidFrequency)
		{
			PidFrequencyCounter = 0;
			runPid = true;
		}
	}
	
	runInter = true;
}

//Encoder interrupt
ISR(PCINT3_vect)
{
	unsigned char m2a_val = is_digital_input_high(global_m2a);
	unsigned char m2b_val = is_digital_input_high(global_m2b);
	
	if(m2a_val > 0)
	{
		m2a_val = 1;
	}
	
	if(m2b_val > 0)
	{
		m2b_val = 1;
	}

	char plus_m1 = m2a_val ^ global_last_m2b_val;
	char minus_m1 = m2b_val ^ global_last_m2a_val;

	if(plus_m1)
	global_counts_m2 += 1;
	if(minus_m1)
	global_counts_m2 -= 1;

	global_last_m2a_val = m2a_val;
	global_last_m2b_val = m2b_val;
}

//Init Motor 2
void initMotor()
{
	TCCR2A = 0x03;
	TCCR2B = 0x02;
	PORTC &= 0xBF;
	PORTD &= 0xBF;
	DDRD |= 0x40;
	DDRC |= 0x40;
}

//Init Encoder
void initEncoder(unsigned char m2a, unsigned char m2b)
{
	//Encoder
	PCICR |= 0x08;
	PCMSK3 |= 0x0C;
	SREG |= 0x80;
	
	global_m2a = m2a;
	global_m2b = m2b;

	// initialize the global state
	global_counts_m2 = 0;

	global_last_m2a_val = is_digital_input_high(global_m2a);
	global_last_m2b_val = is_digital_input_high(global_m2b);
}

//init global PID
void initPID()
{
	globalPID.dGain = 1.0;
	globalPID.pGain = 1.0;
	globalPID.iGain = 1.0;
	globalPID.dState = 0;
	globalPID.iState = 0;
	globalPID.iMax = 1000;
	globalPID.iMin = -1000;
	globalPID.desiredValue = 0;
	globalPID.desireSpeed = true;
	globalPID.lastValue = 0;
}

void initSpeedInterrupt()
{
	//OCR0A 10ms interrupt
	//20M/sec * 1sec/1000ms * 1count/1024ticks * 10INT/195Counts = 10.016ms
	//Need 1024 prescalar and count of 195
	
	TCCR0A = 0x82; //ClearOCRA and OCRA at top
	TCCR0B = 0x05; //Prescalar 1024
	OCR0A = 0xC3; // Set OCR0A Top Rate to 195
	
	//Enable interrupt
	TIMSK0 |= 0x02;
}

int main()
{
	lcd_init_printf();
	clear();
	
	initMotor();
	
	initEncoder(IO_D2, IO_D3);
	
	initPID();
	initSpeedPID();
	//initPositionPID();
	
	initSpeedInterrupt();
	
	setMotor(0);
	
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

	while(1)
	{	
		serial_check();
		check_for_new_bytes_received();
		
		lcd_goto_xy(0,0);
		printf("Encode:");
		print_long(global_counts_m2);
		
		lcd_goto_xy(0,1);
		printf("T:");
		print_long(globalPID.lastValue);
		
		if(runPid)
		{
			//Update Pid
			if(globalPID.desireSpeed)
			{
				setMotor(UpdatePID(avgSpeed));
			}
			
			else
			{
				setMotor(UpdatePID((double)global_counts_m2));
			}
			
			if(logPid)
			{
				char tempBuffer[255];
				int length = 0;

				if(globalPID.desireSpeed)
				{
					length = sprintf( tempBuffer, "%d.%d %d.%d %d %d %d\r\n", (int)(globalPID.pGain) % 10, (int)(globalPID.pGain * 10) % 10, (int)(globalPID.dGain) % 10, (int)(globalPID.dGain * 10) % 10, 
						globalPID.desiredValue, (int)avgSpeed, globalPID.lastValue );
				}
				
				else
				{
					length = sprintf( tempBuffer, "%d.%d %d.%d %d %ld %d\r\n", (int)(globalPID.pGain) % 10, (int)(globalPID.pGain * 10) % 10, (int)(globalPID.dGain) % 10, (int)(globalPID.dGain * 10) % 10, 
						globalPID.desiredValue, global_counts_m2, globalPID.lastValue );
				}
				
				print_usb( tempBuffer, length );
				
				clear();
			}
			
			runPid = false;
		}
		
		/*if(logPid && isSleeping && !execute && runLog)
		{
			
		}*/
		
		//Determine if action needed for interpolator
		if(isInter && runInter && (execute || isSleeping))
		{
			if(myInterpolator[currentInter]->interType == 1) // Sleep
			{
				interSleepCount += 10; //ms since last called
				
				if(logPid && interSleepCount % 20 == 0)
				{
					char tempBuffer[255];
					int length = 0;

					if(globalPID.desireSpeed)
					{
						length = sprintf( tempBuffer, "%d.%d %d.%d %d %d %d\r\n", (int)(globalPID.pGain) % 10, (int)(globalPID.pGain * 10) % 10, (int)(globalPID.dGain) % 10, (int)(globalPID.dGain * 10) % 10,
						globalPID.desiredValue, (int)avgSpeed, globalPID.lastValue );
					}
					
					else
					{
						length = sprintf( tempBuffer, "%d.%d %d.%d %d %ld %d\r\n", (int)(globalPID.pGain) % 10, (int)(globalPID.pGain * 10) % 10, (int)(globalPID.dGain) % 10, (int)(globalPID.dGain * 10) % 10,
						globalPID.desiredValue, global_counts_m2, globalPID.lastValue );
					}
					
					print_usb( tempBuffer, length );
				}
				
				if(interSleepCount >= myInterpolator[currentInter]->interValue)
				{
					if(currentInter == 4)
					{
						isSleeping = false;
					}
					
					else
					{
						interSleepCount = 0;
						currentInter++;
						
						if(myInterpolator[currentInter]->interType == 0)
						{
							//Init new position
							int lastInterValue = 0;
							int lastInterCount = 0;
							isSleeping = false;
							
							initPID();
							initPositionPID();
							globalPID.desiredValue = myInterpolator[currentInter]->interValue + global_counts_m2;
							execute = true;
						}
					}
				}
			}
			
			else if(myInterpolator[currentInter]->interType == 0) // PidUpdate
			{
				//detect if in position
				if(lastInterValue == globalPID.lastValue)
				{
					lastInterCount++;
				}
				
				else
				{
					lastInterValue = globalPID.lastValue;
					lastInterCount = 0;
				}
				
				if(abs(lastInterValue) < 8 && lastInterCount >= 10)
				{
					if(currentInter == 4)
					{
						execute = false;
					}
					
					else
					{
						currentInter++;
						
						if(myInterpolator[currentInter]->interType == 0)
						{
							//Init new position
							int lastInterValue = 0;
							int lastInterCount = 0;
							
							initPID();
							initPositionPID();
							globalPID.desiredValue = myInterpolator[currentInter]->interValue + global_counts_m2;
						}
						
						else if(myInterpolator[currentInter]->interType == 1)
						{
							isSleeping = true;
							interSleepCount = 0;
							execute = false;
							globalPID.lastValue = 0;
							setMotor(0);
						}
					}
				}
			}
			
			runInter = false;
		}
	}
}
