#include "stm32l476xx.h"
#include "SysClock.h"
#include "LED.h"
#include "UART.h"

#include <string.h>
#include <stdio.h>

char RxComByte = 0;
uint8_t buffer[BufferSize];
char str[] = "Give Red LED control input (Y = On, N = off):\r\n";

int main(void){
	char rxByte;
	int 		lower_limit = 950;
	int 		upper_limit = 1050;
	int 		new_low_limit;
	int		timer_value[1000];
	int		final_display_time[100], final_display_counts[100];
	int		out_of_limits = 0';
	int		n ;
	int		i ;
	int retest = 0;
	float b;
	char limit_choice = 0;
	
	System_Clock_Init(); // Switch System Clock = 80 MHz
	LED_Init();
	UART2_Init();
	
	// Initialize Input Pin
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Enable the clock to GPIO Port A
	GPIOA->MODER &= ~0xCFF; // Set PA0, PA1, PA2, PA3, and PA5 as input
	
	// Set up timer 2 for Input Capture mode
	TIM2->TIM2_CR1 |= 0x0001; // Enable TIM1
	TIM2->TIM2_CCMR1 |= 0x00000021; //Enable Input Capture mode on CC1 channel with N=4 capture filter
	TIM2->TIM2_CCER |= 0x0001; // Enable capture of counter into capture register
	TIM2->TIM2_PSC |= 0x0000; // Presacler value
	
	//Initialize final display array
	for (int i = 0; i <= 100; i++) 
	{
		final_display_time[i]= lower_limit + i;
	}
	
	final_display_counts = {0};
	
	while(limit_choice == 0)
	{
		USART_Write("Current Lower Limit:\t", lower_limit, "\tCurrent Upper Limit:\t", upper_limit);
		USART_Write("\nEnter 0 to change limits.  Enter 1 to continue");
		limit_choice = USART_Read(USART2);
		if(limit_choice == 0)
		{
			do
			{
				USART_Write("\nEnter value for new lower limit.  Value must be between 50 and 9950.")
				new_low_limit = USART_Read(USART2);
				if((new_low_limit < 50) || (new_low_limit > 9950))
					USART_Write("\nLower limit must be between 50 and 9950");
			} while ((new_low_limit < 50) || (new_low_limit > 9950));
			lower_limit = new_low_limit;
			upper_limit = lower_limit + 100;
		}
		else 
			break;
	}
	
	
	USART_Write("\nSystem ready to measure input signal.  Press enter to continue.");
	do
	{
		rxByte = USART_Read(USART2);
	}
	while (rxByte != 0x0A);
	
	while (retest == 0){
		
		for (int i = 0; i <1000; i++)
		{
			if (TIM2->TIM2_SR & 0x0002)
			{
				timer_value[i]= TIM2->TIM2_CCR1;
				TIM2->TIM2_CNT &= 0x0000;
				//TIM2->TIM2_SR &= 0x0000;
			}
		}
		
		for (int i = 0; i < 1000; i++) 
		{
			if(timer_value[i] == 0)
				continue;
			for(int q = 0; q <= 100; q++)
			{
				if(timer_value[i] == final_display_time[q])
					final_display_counts[q] +=1;
					break;
				else
					continue;
				if(q==100)
					out_of_limits++;
			}
		}
		
		for(int i = 0; i <= 100; i++)
		{
			USART_Write(final_display_time[i], "\t", final_display_counts[i], "\n");
		}
		USART_Write(out_of_limits);
		
		USART_Write("Would you like to gather another data set?");
		
		
		//n = sprintf((char *)buffer, "a = %d\t", a);
		//n += sprintf((char *)buffer + n, "b = %f\r\n", b);
		//USART_Write(USART2, buffer, n);		
		//a = a + 1;
		//b = (float)a/100;
		// now spin for a while to slow it down
		//for (i = 0; i < 4000000; i++)
		//	;
		
//		USART_Write(USART2, (uint8_t *)str, strlen(str));	
//		rxByte = USART_Read(USART2);
//		if (rxByte == 'N' || rxByte == 'n'){
//			Red_LED_Off();
//			USART_Write(USART2, (uint8_t *)"LED is Off\r\n\r\n", 16);
//		}
//		else if (rxByte == 'Y' || rxByte == 'y'){
//			Red_LED_On();
//			USART_Write(USART2, (uint8_t *)"LED is on\r\n\r\n", 15);
//		}
	}
}

