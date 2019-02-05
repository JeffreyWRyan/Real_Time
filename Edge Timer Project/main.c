#include "stm32l476xx.h"
#include "SysClock.h"
#include "LED.h"
#include "UART.h"

#include <string.h>
#include <stdio.h>

void initialize_final_arrays(int final_display_time[], int final_display_counts[]);
void GPIO_Timer_init(void);
int POST(int timer_value[]);
void adjust_limits(int *lower_limit, int *upper_limit, int limit_choice);
void system_wait(void);
void data_collection(int timer_value[]);
void fill_final_arrays(int timer_value[], int final_display_time[], int final_display_counts[]);
void display_final_arrays(int final_display_time[], int final_display_counts[]);

int main(void){
	int 		post_fail = 0;
	char 		rxByte, rxByte2;
	int 		lower_limit = 950;
	int		*p_lower_limit = &lower_limit;
	int 		upper_limit = 1050;
	int		*p_upper_limit = &upper_limit;
	int		timer_value[1000];
	int		final_display_time[100], final_display_counts[100];
	int		out_of_limits = 0';
	int 		retest = 0;
	char 		limit_choice = 0;
	
	System_Clock_Init(); // Switch System Clock = 80 MHz
	LED_Init();
	UART2_Init();
	GPIO_Timer_init();
	initialize_final_arrays(final_display_time, final_display_counts);
	
	do
	{
		post_fail = POST(timer_value);
		if(post_fail == 1)
		{
			USART_Write("POST has failed.  Would you like to run POST again?\nEnter 0 to retest, or enter 1 to continue\n");
			rxByte = USART_Read(USART2);
			if (rxByte == 0)
				post_fail = 1;
			else if (rxByte = 1)
				post_fail = 0;
			else
				continue;
		}
	}
	while(post_fail == 1);
	
	while(limit_choice == 0)
	{
		adjust_limits(p_lower_limit, p_upper_limit, limit_choice);
	}
	
	system_wait();
	
	while (retest == 0){
		
		data_collection(timer_value);
		fill_final_arrays(timer_value, final_display_time, final_display_counts);
		display_final_arrays(final_display_time, final_display_counts);
		
		USART_Write("Enter 0 to retest.  Enter 1 to quit.\n");
		rxByte = USART_Read(USART2);
		if(rxByte == 0)
		{
			USART_Write("Enter 0 to reuse the same limits.  Enter 1 to change the limits.\n);
			rxByte2 = USART_Read(USART2);
			if(rxByte2 == 0)
				continue;
			else if (rxByte == 1)
			{
				adjust_limits(p_lower_limit, p_upper_limit, limit_choice);
			}
		}
		else
			break;
	}
	
	while(1){}
}

void initialize_final_arrays(int final_display_time[], int final_display_counts[])
{
		//Initialize final display array
	for (int i = 0; i <= 100; i++) 
	{
		final_display_time[i]= lower_limit + i;
	}
	final_display_counts = {0};
	
	return;
}

void GPIO_Timer_init(void)
{
	// Initialize Input Pin
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Enable the clock to GPIO Port A
	GPIOA->MODER &= ~0xCFF; // Set PA0, PA1, PA2, PA3, and PA5 as input
	
	// Set up timer 2 for Input Capture mode
	TIM2->TIM2_CR1 |= 0x0001; // Enable TIM1
	TIM2->TIM2_CCMR1 |= 0x00000021; //Enable Input Capture mode on CC1 channel with N=4 capture filter
	TIM2->TIM2_CCER |= 0x0001; // Enable capture of counter into capture register
	TIM2->TIM2_PSC |= 0x0000; // Presacler value
	
	return;
}

void adjust_limits(int *lower_limit, int *upper_limit, int limit_choice)
{
		int 		new_low_limit;
		
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
			*lower_limit = new_low_limit;
			*upper_limit = lower_limit + 100;
		}
		else 
			break;
		
		return;
}

void system_wait(void)
{
	USART_Write("\nSystem ready to measure input signal.  Press enter to continue.");
	do
	{
		rxByte = USART_Read(USART2);
	}
	while (rxByte != 0x0A);
	
	return;
}

void data_collection(int timer_value[])
{
			for (int i = 0; i <1000; i++)
		{
			if (TIM2->TIM2_SR & 0x0002)
			{
				timer_value[i]= TIM2->TIM2_CCR1;
				TIM2->TIM2_CNT &= 0x0000;
				//TIM2->TIM2_SR &= 0x0000;
			}
		}
		
		return;
}

void fill_final_arrays(int timer_value[], int final_display_time[], int final_display_counts[])
{
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
		
		return;
}

void display_final_arrays(int final_display_time[], int final_display_counts[])
{
			for(int i = 0; i <= 100; i++)
		{
			USART_Write(final_display_time[i], "\t", final_display_counts[i], "\n");
		}
		USART_Write(out_of_limits);
		
		return;
}

int POST(int timer_value[])
{
	int post_fail = 0;
	
	data_collection(timer_value);
	
	for(int i = 0; i < 1000; i++)
	{
		if(timer_value[i] <= 100)
			post_fail = 1;
		else
			continue;
	}
	
	return post_fail;
}
			
	