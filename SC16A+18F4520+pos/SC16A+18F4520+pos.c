//==========================================================================
//	Author				: Ng Hock Yuan	
//	Project				: Sample code for SC16A using 16F877A
//	Project description	: This source code is used to control 16 servos.
//						  The 16 servos should continues rotate together from one end to another end.
//						  Delay subroutine is used to wait for servo reach the other end.    	
//
//==========================================================================
// This is the sample code for controlling 16 channel of servo using SC16A
/**************************************************************
*                  Communication Protocol                    *
**************************************************************
*
* UART is chosen as the interface to this module. In order to
* change the position of a servo, the host (master) needs to
* write 4 bytes of data to this module. The data format is:
* 
*        Byte 1            Byte 2             Byte 3		    Byte 4
*    --------------    ---------------    --------------    -------------
*  /  Servo channel \/    Position     \/    Position    \/   Speed value \
*  \  (0x41 - 0x60) /\  (Higher Byte)  /\  (Lower Byte)  /\      (0-63)   /
*    --------------    ---------------    --------------    -------------
*
*	Servo channel: 0b01XX XXXX
*	Higher Byte: 0b00XX XXXX
*	Lower Byte:	 0b00XX XXXX
*	Speed Value: 0b00XX XXXX	
*
* The position for the servo is in 12-bit and the valid range
* is from 0 (0.5mS) to the resolution defined below (2.5mS).
* It is the host responsibility to make sure the position will
* not stall the servo motor.
*
**************************************************************/
//	include
//==========================================================================
#include <p18F4520.h>   					// this sample code is using 16F877A !!
#include "delays.h"
#include "usart.h"

//	configuration
//==========================================================================
#pragma	config OSC = HS				// HS oscillator
#pragma	config FCMEN = OFF			// Fail-Safe Clock Monitor disabled
#pragma	config IESO = OFF			// Oscillator Switchover mode disabled
#pragma	config PWRT = OFF			// PWRT disabled
#pragma	config BOREN = OFF			// Brown-out Reset disabled in hardware and software
#pragma	config WDT = OFF			// WDT disabled (control is placed on the SWDTEN bit)
#pragma	config MCLRE = ON			// MCLR pin enabled; RE3 input pin disabled 
#pragma	config PBADEN = OFF			// PORTB<4:0> pins are configured as digital I/O on Reset 
#pragma	config CCP2MX = PORTC		// CCP2 input/output is multiplexed with RC1 
#pragma	config LVP = OFF			// Single-Supply ICSP disabled 
#pragma	config XINST = OFF			// Extended Instruction Set



	
//	define
//==========================================================================
#define sw1		PORTBbits.RB0					
#define sw2		PORTBbits.RB1					
#define	LED1	LATBbits.LATB6
#define LED2	LATBbits.LATB7


//	global variable
//=========================================================================
static volatile unsigned int received_servo_position[0x11];   // Array declared to store the feedback position of servo 


//	function prototype				(every function must have a function prototype)
//==========================================================================
void send_cmd(unsigned int num, unsigned int data, unsigned int ramp);
void request_feedback(unsigned char num);	//UART transmit 2 bytes: start byte '@' and servo number 0x41-0x60 to request position
void get_position(void);					//UART receive 3 bytes: servo number 0x41-0x60, higher byte and lower byte for position value to update the current position of servo


//	main function					(main fucntion of the program)
//==========================================================================
void main(void)
{
	unsigned int i,j,temp;
	//set IO port for led and switch
	TRISC = 0b10000000;					//set input or output
	TRISB = 0b00000011;

	//-------------------------------------------------------------------------
	// Configure UART
  	//-------------------------------------------------------------------------

	OpenUSART( USART_TX_INT_OFF &	// Transmit interrupt off	
		USART_RX_INT_OFF &			// Receive interrupt off
		USART_ASYNCH_MODE &			// Asynchronous mode
		USART_EIGHT_BIT &			// 8-bit data
		USART_CONT_RX &				// Continuous reception
		USART_BRGH_HIGH,			// High baud rate			
		129);					// 20 Mhz

	for(i=0x41;i<0x51;i+=1)				//set initial position of servos
	{
	send_cmd(i,1300,0);
	}			
	while(1)
	{
			if(sw1==0)
			{
				while(sw1==0);
			
				while(1)
				{
					for(i=0x01;i<0x11;i+=1)								//servo number start from 0x01 to 0x10, total 16servos
					{													//servo number will convert to 0x41 to 0x50 in send cmd routine
						send_cmd(i,200,20);								//send position and speed command to SC16A
					}
						request_feedback(1);							//request the position to SC16A
						get_position();									//receive 3 bytes on position reporting from SC16A to get the current position
						while(received_servo_position[1]!=200)			//test if servo position reach 200 value 
						{
							request_feedback(1);						//if no yet reach, keep request
							get_position();								//receive 3 bytes on position reporting from SC16A to keep update the current position
						}
					
									
					for(i=0x01;i<0x11;i+=1)
					{
						send_cmd(i,1300,20);
					}
						request_feedback(1);
						get_position();	
						while(received_servo_position[1]!=1300)			//test if servo position reach 1300 value
						{
							request_feedback(1);						//if no yet reach, keep request
							get_position();								//receive 3 bytes on position reporting from SC16A to keep update the current position
						}
												
				
				}//while loop
			}//if loop
			
	}		
}
	
// UART subroutine
//============================================================================	
void send_cmd(unsigned int num, unsigned int data, unsigned int ramp)
{
	unsigned char higher_byte=0, lower_byte=0;
	
	if(0x01<=num<=0x16)
	{
		num=num|0x40;
	}
	
	higher_byte=(data>>6)&0x003f;
	lower_byte=data&0x003f;


			while (BusyUSART());
			putcUSART(num);
			//delay(2000);
			while (BusyUSART());
			putcUSART(higher_byte);
			//delay(2000);
			while (BusyUSART());
			putcUSART(lower_byte);
			//delay(2000);
			while (BusyUSART());
			putcUSART(ramp);
			//delay(2000);
	
}

void request_feedback(unsigned char num)			
{	

			num=num|0b01000000;
			
			while (BusyUSART());
			putcUSART('@');			
			while (BusyUSART());
			putcUSART(num);								
}

void get_position(void)								
{unsigned int i;
static unsigned int received_servo_num=0, higher_byte=0,lower_byte=0,received_position=0;
	
	while(PIR1bits.RCIF==0);		
	received_servo_num=getcUSART();
	while(PIR1bits.RCIF==0);														
	higher_byte=getcUSART();
	while(PIR1bits.RCIF==0);															
	lower_byte=getcUSART();																	
						
	received_servo_num=received_servo_num&0b00011111;										//Change back to 0x01-0x20
	received_servo_position[received_servo_num]=((higher_byte<<6)|(lower_byte&0x3F)); 		//Update the servo position value in corresponding array


}