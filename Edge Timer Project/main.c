#include "stm32l476xx.h"
#include "SysClock.h"
#include "UART.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void initialize_final_arrays(int lower_limit);
void GPIO_Timer_init(void);
void system_wait(void);
void data_collection(void);
void fill_final_arrays(int lower_limit);
void display_final_arrays(void);

void limit_process(int *lower_limit, int *upper_limit);
void post_process(void);

char begin_post_str[] = "\r\nPOST is running.\r\n";
char post_fail_str[] = "\r\nPOST has failed.  Would you like to run POST again?\r\nEnter 0 to retest, or enter 1 to continue\r\n";
char retest_str[] = "\r\nEnter 0 to retest.  Enter 1 to quit.\r\n";
char reuse_lims_str[] = "\r\nEnter 0 to use these limits.  Enter 1 to change the limits.\r\n";
char low_limit_str[] = "\r\nEnter value for new lower limit; must be between 50 and 9950. Press enter when finished\r\n";
char sys_ready_str[] = "\r\nSystem ready to measure input signal.  Press enter to continue.\r\n";
char curr_low_lim_str[] = "\r\nCurrent Lower Limit:\t";
char curr_high_lim_str[] = "\tCurrent Upper Limit:\t";
char data_collect_str[] = "\r\nCollecting Data...\r\n";
char tab_str[] = "\t";
char ret_str[] = "\r\n";
char int_buff[4];

int			timer_value[1000];
int			final_display_time[100];
int   	final_display_counts[100];

int main(void){
	char 		rxByte;
	int 		lower_limit = 950;
	int			*p_lower_limit = &lower_limit;
	int 		upper_limit = 1050;
	int			*p_upper_limit = &upper_limit;
	int 		retest = 0;
	
	System_Clock_Init();	// Switch System Clock = 80 MHz
	
	UART2_Init();
	
	GPIO_Timer_init();
	
	post_process();
	
	while (retest == 0)
	{	
		limit_process(p_lower_limit, p_upper_limit);
	
		initialize_final_arrays(lower_limit);
	
		system_wait();
	
		data_collection();
		fill_final_arrays(lower_limit);
		display_final_arrays();
		
		USART_Write(USART2, (uint8_t *)retest_str, strlen(retest_str));
		rxByte = USART_Read(USART2);
		retest = rxByte - 48;
	}
	
	while(1){}
}
//FUNCTION FILLS THE TIME ARRAY WITH INTEGER VALUES FROM LOWER LIMIT TO UPPER LIMIT
void initialize_final_arrays(int lower_limit)
{
	//Initialize final display array
	for (int i = 0; i < 100; i++) 
	{
		final_display_time[i]= lower_limit + i;
		final_display_counts[i] = 0;
	}
	return;
}
//FUNCTION THAT INITIALIZES THE GPIO PORT AND CONFIGURES THE TIMER AS INPUT CAPTURE
void GPIO_Timer_init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER &= ~3;
	GPIOA->MODER |= 2;
	GPIOA->AFR[0] |= 0x1;
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	TIM2->PSC = 80;
	TIM2->EGR |= TIM_EGR_UG;
	TIM2->CCER &= ~(0xFFFFFFFF);
	TIM2->CCMR1 |= 0x1;
	TIM2->CCER |= 0x1;
	
	return;
}
//FUNCTION THAT DISPLAYS THE LIMITS OF THE PROGRAM AND ALLOWS THE USER TO CHANGE THE LIMITS
void limit_process(int *lower_limit, int *upper_limit)
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
		
		while(limit_choice == 0x31)
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
				rxByte = 0;
			} while ((new_low_limit < 50) || (new_low_limit > 9950));
			
			*lower_limit = new_low_limit;
			*upper_limit = new_low_limit + 100;
			
			USART_Write(USART2, (uint8_t *)curr_low_lim_str, strlen(curr_low_lim_str));
			sprintf(int_buff, "%d", (int)*lower_limit);
			USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
			USART_Write(USART2, (uint8_t *)curr_high_lim_str, strlen(curr_high_lim_str));
			sprintf(int_buff, "%d", (int)*upper_limit);
			USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
			USART_Write(USART2, (uint8_t *)reuse_lims_str, strlen(reuse_lims_str));
	
			limit_choice = USART_Read(USART2);
		}
		return;
}
//FUNCTION THAT WAITS FOR THE USER TO PRESS ENTER
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
//FUNCTION THAT COLLECTS THE DATA
void data_collection(void)
{
	unsigned int begin_sample_time;
	
	USART_Write(USART2, (uint8_t *)data_collect_str, strlen(data_collect_str));
	for(int i = 0; i <= 1000; i++)
	{
		TIM2->CR1 = 0x1;
		begin_sample_time = (unsigned int) TIM2->CCR1;
		while(1)
		{
			if(TIM2->SR & 0x2)
			{
				timer_value[i] = (((unsigned int) TIM2->CCR1) - begin_sample_time);
				TIM2->CR1 = 0x0;
				break;
			}
		}
	}
	TIM2->CR1 = 0x0;
		
	return;
}
//FUNCTION THAT FILLS THE FINAL ARRAYS WITH THE FREQUENCY DATA FROM DATA COLLECTION
void fill_final_arrays(int lower_limit)
{
	unsigned int timing_bin;
	for (int i = 0; i <= 1000; i++) 
	{
		timing_bin = (unsigned int) (timer_value[i] - lower_limit);
		if(timing_bin > 100)
			continue;
		final_display_counts[timing_bin]++;
	}
		
	return;
}
//FUNCTION THAT DISPLAYS THE FINAL ARRAYS AS TWO COLUMNS
void display_final_arrays(void)
{
		for(int i = 0; i <= 100; i++)
		{
			if(final_display_counts[i] == 0)
				continue;
			else
			{	
				sprintf(int_buff, "%d", (int)final_display_time[i]);
				USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
				USART_Write(USART2, (uint8_t *)tab_str, strlen(tab_str));
				sprintf(int_buff, "%d", (int)final_display_counts[i]);
				USART_Write(USART2, (uint8_t *)int_buff, strlen(int_buff));
				USART_Write(USART2, (uint8_t *)ret_str, strlen(ret_str));
			}
		}
		
		return;
}
// FUNCTION THAT PERFORMS THE POST
void post_process(void)
{
	int post_value;
	char re_post = 0;
	char fail_post = 0;
	unsigned int begin_sample_time;
	
	USART_Write(USART2, (uint8_t *)begin_post_str, strlen(begin_post_str));
	while(re_post == 0)
	{
		for(int i = 0; i < 500; i++)
		{
			TIM2->CR1 = 0x1;
			begin_sample_time = (unsigned int) TIM2->CCR1;
			while(1)
			{
				if(TIM2->SR & 0x2)
				{
					post_value = (((unsigned int) TIM2->CCR1) - begin_sample_time);
					TIM2->CR1 = 0x0;
					break;
				}
			}
			if(post_value > 100000)
			{
				fail_post = 1;
				break;
			}
		}
		
		//Disable the counter
		TIM2->CR1 = 0x0;
		
		if(fail_post == 1)
		{
			USART_Write(USART2, (uint8_t *)post_fail_str, strlen(post_fail_str));
			re_post = USART_Read(USART2);
			re_post = re_post - 48;
			if(re_post == 0)
			{
				fail_post = 0;
			}
		}
		else
			re_post = 1;
	}
	return;
}
