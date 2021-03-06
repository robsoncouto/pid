#define F_CPU 16000000UL
#define UART_BAUD_RATE 38400


#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "pid.h"
#include "serial/uart.h"
#include <util/delay.h>
#include <stdio.h>



/*! \brief P, I and D parameter values
 *
 * The K_P, K_I and K_D values (P, I and D gains)
 * need to be modified to adapt to the application at hand
 */
//! \xref item todo "Todo" "Todo list"
#define K_P 1.00
//! \xref item todo "Todo" "Todo list"
#define K_I 0.00
//! \xref item todo "Todo" "Todo list"
#define K_D 0.00

/*! \brief Flags for status information
 */
struct GLOBAL_FLAGS {
	//! True when PID control loop should run one time
	uint8_t pidTimer : 1;
	uint8_t dummy : 7;
} gFlags = {0, 0};

//! Parameters for regulator
struct PID_DATA pidData;

/*! \brief Sampling Time Interval
 *
 * Specify the desired PID sample time interval
 * With a 8-bit counter (255 cylces to overflow), the time interval value is calculated as follows:
 * TIME_INTERVAL = ( desired interval [sec] ) * ( frequency [Hz] ) / 255
 */
//! \xref item todo "Todo" "Todo list"
#define TIME_INTERVAL 157

ISR(TIMER0_OVF_vect)
{
	static uint16_t i = 0;

	if (i < TIME_INTERVAL) {
		i++;
	} else {
		gFlags.pidTimer = 1;
		i               = 0;
	}
}

/*! \brief Init of PID controller demo
 */
void Init(void)
{
	pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR, K_D * SCALING_FACTOR, &pidData);

	// Set up timer, enable timer/counter 0 overflow interrupt
	TCCR0B = (1 << CS00); // clock source to be used by the Timer/Counter clkI/O
	TIMSK0 = (1 << TOIE0);
	TCNT0  = 0;

	ADMUX = (1<<REFS0)|(1<<MUX1)|(1<<MUX0);//PIN ADC7 used
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS0);

	DDRB=(1<<PB2)|(1<<PB3);
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
}

uint16_t get_adc(uint8_t channel){
  ADMUX&=0xF0;
  ADMUX|=channel;
  ADCSRA |= (1<<ADSC);
  while(ADCSRA & (1<<ADSC));
  return (ADC);
}

/*! \brief Read reference value.
 *
 * This function must return the reference value.
 * May be constant or varying
 */
int16_t Get_Reference(void)
{
	return 512;
}

/*! \brief Read system process value
 *
 * This function must return the measured data
 */
int16_t Get_Measurement(void)
{
	return get_adc(0);
}

/*! \brief Set control input to system
 *
 * Set the output from the controller as input
 * to system.
 */
void Set_Input(int16_t inputValue)
{

	if(inputValue>0){
		OCR1A=inputValue;
		PORTB|=(1<<PB2);
		PORTB&=~(1<<PB3);
	}else{
		OCR1A=-inputValue;
		PORTB|=(1<<PB3);
		PORTB&=~(1<<PB2);
	}
}
void pwm_test(uint8_t value){
	DDRB=(1<<PB1);
	TCCR1A=(1<<COM1A1)|(1<<WGM10);
	TCCR1B=(1<<WGM12)|(CS11<<1);
	OCR1A=value;
}


/*! \brief Demo of PID controller
 */
int main(void){
	static FILE mystdout = FDEV_SETUP_STREAM(uart0_putc, NULL,_FDEV_SETUP_WRITE);
	stdout = &mystdout;
	int16_t referenceValue, measurementValue, inputValue;
	//system_init();
	// Configure Power reduction register to enable the Timer0 module
	// Atmel START code by default configures PRR to reduce the power consumption.
	//PRR &= ~(1 << PRTIM0);
	Init();
	pwm_test(0);
	//DDRD=(1<<1)|(0<<0);
	sei();
	while (1) {
		printf("\nAnalog read: %d\n",Get_Measurement());


		// Run PID calculations once every PID timer timeout
		if (gFlags.pidTimer == 1) {
			uart_puts("\nPID test\n");

			referenceValue   = Get_Reference();
			measurementValue = Get_Measurement();

			inputValue = pid_Controller(referenceValue, measurementValue, &pidData);

			Set_Input(inputValue);

			gFlags.pidTimer = FALSE;

			printf("\nOutput: %d\n",inputValue);
		}
	}
}

/*! \mainpage
 * \section Intro Introduction
 * This documents data structures, functions, variables, defines, enums, and
 * typedefs in the software for application note AVR221.
 *
 *
 * \section DI Device Info
 *
 *
 * \section TDL ToDo List
 * \todo Write own code in:
 * \ref Get_Reference(void), \ref Get_Measurement(void) and \ref Set_Input(int16_t inputValue)
 *
 * \todo Modify the \ref K_P (P), \ref K_I (I) and \ref K_D (D) gain to adapt to your application
 * \todo Specify the sampling interval time \ref TIME_INTERVAL
 */
