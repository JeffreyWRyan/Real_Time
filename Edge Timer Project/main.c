#include "stm32l476xx.h"
#include "SysClock.h"
#include "LED.h"
#include "UART.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void initialize_final_arrays(int lower_limit, int final_display_time[]);
void GPIO_Timer_init(void);
int POST(int timer_value[]);
char adjust_limits(int *lower_limit, int *upper_limit);
void system_wait(void);
void data_collection(int timer_value[]);
void fill_final_arrays(int timer_value[], int final_display_time[], int final_display_counts[], int *out_of_limits);
void display_final_arrays(int final_display_time[], int final_display_counts[], int out_of_limits);

char post_fail_str[] = "\nPOST has failed.  Would you like to run POST again?\nEnter 0 to retest, or enter 1 to continue";
char retest_str[] = "\nEnter 0 to retest.  Enter 1 to quit.";
char reuse_lims_str[] = "\nEnter 0 to use the same limits.  Enter 1 to change the limits.";
char low_limit_str[] = "\nEnter value for new lower limit.  Value must be between 50 and 9950. Press enter when finished";
char sys_ready_str[] = "\nSystem ready to measure input signal.  Press enter to continue.";
char curr_low_lim_str[] = "\nCurrent Lower Limit:\t";
char curr_high_lim_str[] = "\tCurrent Upper Limit:\t";
char tab_str[] = "\t";
char ret_str[] = "\n";
char int_buff[4];

int			timer_value[1000];
int			final_display_time[100];
int   	final_display_counts[100] = {0};

int main(void){
	int 		post_fail = 0;
	char 		rxByte, rxByte2;
	int 		lower_limit = 950;
	int			*p_lower_limit = &lower_limit;
	int 		upper_limit = 1050;
	int			*p_upper_limit = &upper_limit;
	int			out_of_limits = 0;
	int			*p_out_of_limits = &out_of_limits;
	int 		retest = 0;
	char 		limit_choice = 0;
	
	System_Clock_Init(); // Switch System Clock = 80 MHz
	LED_Init();
	UART2_Init();
	GPIO_Timer_init();
	
	do
	{
		post_fail = POST(timer_value);
		if(post_fail == 1)
		{
			USART_Write(USART2, (uint8_t *)post_fail_str, strlen(post_fail_str));
			rxByte = USART_Read(USART2);
			if (rxByte == 0)
				post_fail = 1;
			else if (rxByte == 1)
				post_fail = 0;
			else
				continue;
		}
	}
	while(post_fail == 1);
	
	while(limit_choice == 0)
	{
		limit_choice = adjust_limits(p_lower_limit, p_upper_limit);
	}
	initialize_final_arrays(lower_limit, final_display_time);
	
	system_wait();
	
	while (retest == 0){
		
		data_collection(timer_value);
		fill_final_arrays(timer_value, final_display_time, final_display_counts, p_out_of_limits);
		display_final_arrays(final_display_time, final_display_counts, out_of_limits);
		
		USART_Write(USART2, (uint8_t *)retest_str, strlen(retest_str));
		rxByte = USART_Read(USART2);
		if(rxByte == 0)
		{
			USART_Write(USART2,(uint8_t *)reuse_lims_str , strlen(reuse_lims_str));
			rxByte2 = USART_Read(USART2);
			if(rxByte2 == 0)
				continue;
			else if (rxByte == 1)
			{
				adjust_limits(p_lower_limit, p_upper_limit);
			}
		}
		else
			break;
	}
	
	while(1){}
}

void initialize_final_arrays(int lower_limit, int final_display_time[])
{
		//Initialize final display array
	for (int i = 0; i <= 100; i++) 
	{
		final_display_time[i]= lower_limit + i;
	}
	
	return;
}

void GPIO_Timer_init(void)
{
	//Enable GPIOB clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	//set PB.6 as AF2
	GPIOB->MODER |= 0x02<<(2*6);
	
	//Enable the clock of timer4
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;
	
	//Define better prescaler
	TIM4->PSC = 799;
	
	//Set direction as input and select the active input
	TIM4->CCMR1 &= ~TIM_CCMR1_CC1S;
	TIM4->CCMR1 |= 0x1;
	//Disbale digital filtering
	TIM4->CCMR1 &= ~TIM_CCMR1_IC1F;
	//Select rising edge
	TIM4->CCER &= (0<<1 | 0<<3);
	//Program the input prescaler
	TIM4->CCMR1 &= ~(TIM_CCMR1_IC1PSC);
	//Enable capture of the counter
	TIM4->CCER |= TIM_CCER_CC1E;
	//Enable realted interrupts
	TIM4->DIER |= TIM_DIER_CC1DE;
	//Enable the counter
	TIM4->CR1 |= TIM_CR1_CEN;
	//Set priority to 1
	//NVIC_SetPriority(TIM4_IRQn, 1);
	//Enable TIM4 interrupt in NVIC
	//NVIC_EnableIRQ(TIM4_IRQn);
	
	return;
}

char adjust_limits(int *lower_limit, int *upper_limit)
{
	int 		new_low_limit = 0;
	char		i = 0;
	char		rxByte = 0;
	char		new_low_limit_str[5];
	char		limit_choice;
		
	  USART_Write(USART2, (uint8_t *)curr_low_lim_str, strlen(curr_low_lim_str));
	  sprintf(int_buff, "%d", (int)*lower_limit);
	  USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
	  USART_Write(USART2, (uint8_t *)curr_high_lim_str, strlen(curr_high_lim_str));
	  sprintf(int_buff, "%d", (int)*upper_limit);
	  USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
		USART_Write(USART2, (uint8_t *)reuse_lims_str, strlen(reuse_lims_str));
	
		limit_choice = USART_Read(USART2);

		if(limit_choice == 0x31)
		{
			do
			{
				USART_Write(USART2, (uint8_t *)low_limit_str, strlen(low_limit_str));
				
				while(rxByte != 0x0D)
				{
					rxByte = USART_Read(USART2);
					new_low_limit_str[i] = rxByte;
					i++;
				}
				i = 0;
				new_low_limit = atoi(new_low_limit_str);
				
				sprintf(int_buff, "%d", (int)new_low_limit);
				USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
				rxByte = 0;
			} while ((new_low_limit < 50) || (new_low_limit > 9950));
			
			*lower_limit = new_low_limit;
			*upper_limit = new_low_limit + 100;
		}
		
		return limit_choice;
}

void system_wait(void)
{
	char rxByte;
	
	USART_Write(USART2, (uint8_t *)sys_ready_str, strlen(sys_ready_str));
	do
	{
		rxByte = USART_Read(USART2);
	}
	while (rxByte != 0x0D);
	
	return;
}

void data_collection(int timer_value[])
{
		for (int i = 0; i <1000; i++)
		{
			while(TIM4->SR != 0x2){}
			timer_value[i]= TIM4->CCR1;
			TIM4->CNT = 0;
			//TIM4->SR &= 0x0000;
		}
		
		return;
}

void fill_final_arrays(int timer_value[], int final_display_time[], int final_display_counts[], int *out_of_limits)
{
		for (int i = 0; i < 1000; i++) 
		{
			if(timer_value[i] == 0)
				continue;
			for(int q = 0; q <= 100; q++)
			{
				if(timer_value[i] == final_display_time[q])
					final_display_counts[q] +=1;
				if(q==100)
					*out_of_limits+=1;
			}
		}
		
		return;
}

void display_final_arrays(int final_display_time[], int final_display_counts[], int out_of_limits)
{
			for(int i = 0; i <= 100; i++)
		{
			sprintf(int_buff, "%d", (int)final_display_time[i]);
			USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
			USART_Write(USART2, (uint8_t *)tab_str, strlen(tab_str));
			sprintf(int_buff, "%d", (int)final_display_counts[i]);
			USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
			USART_Write(USART2, (uint8_t *)ret_str, strlen(ret_str));
		}
		USART_Write(USART2, (uint8_t *)out_of_limits, 4);
		
		return;
}

int POST(int timer_value[])
{
	int post_fail = 0;
	
	data_collection(timer_value);
	
	for(int i = 0; i < 1000; i++)
	{
		if(timer_value[i] > 100)
			post_fail = 1;
		else
			continue;
	}
	
	return post_fail;
}
