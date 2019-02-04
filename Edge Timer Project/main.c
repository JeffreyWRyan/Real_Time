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
	//char rxByte;
	int		timer_value;
	int		n ;
	int		i ;
	float b;
	bool capture_control = 0;
	
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
	
	//IF CC1F FLAG IS SET AND capture_control TRUE THEN 
	//GET VALUE FROM TIMx_CCR1
	//RESET	TIM2_CNT TO 0
	//TOGGLE capture_control
	//CLEAR FLAG
	//ELSE IF CC1F FLAG IS SET AND capture_control FALSE THEN
	//TOGGLE capture_control
	//CLEAR FLAG
	//END IF
	
	while (1){
		if ((TIM2->TIM2_SR & 0x0002) && (capture_control == 1))
		{
			timer_value = TIM2->TIM2_CCR1;
			TIM2->TIM2_CNT &= 0x0000;
			capture_control = 0;
			TIM2->TIM2_SR &= 0x0000;
		}
		else if ((TIM2->TIM2_SR & 0x0002) && (capture_control == 0))
		{
			capture_control = 1;
			TIM2->TIM2_SR &= 0x0000;
		}
		
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

