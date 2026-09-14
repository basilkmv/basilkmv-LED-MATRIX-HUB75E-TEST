
/* Includes ------------------------------------------------------------------*/
#include "board_led.h"

#define LED_ON(port,pin)			port->scr = pin;										//hi
#define LED_OFF(port,pin)			port->clr = pin;		                //low
#define LED_TOGGL(port,pin)   if(port->odt & pin)             \
                                LED_OFF(port,pin)             \
                              else                            \
                                LED_ON(port,pin);   

uint8_t BSP_LED_Init(void)
{
  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

	crm_periph_clock_enable(LED_1_GPIO_CLK, TRUE);
  
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

	gpio_init_struct.gpio_pins = LED_1_GPIO_PIN;
  gpio_init(LED_1_GPIO_PORT, &gpio_init_struct);
	LED_ON(LED_1_GPIO_PORT, LED_1_GPIO_PIN);

	gpio_init_struct.gpio_pins = LED_2_GPIO_PIN;
  gpio_init(LED_2_GPIO_PORT, &gpio_init_struct);
	LED_ON(LED_2_GPIO_PORT, LED_2_GPIO_PIN);

  gpio_init_struct.gpio_pins = LED_3_GPIO_PIN;
  gpio_init(LED_3_GPIO_PORT, &gpio_init_struct);
	LED_ON(LED_3_GPIO_PORT, LED_3_GPIO_PIN);

  gpio_init_struct.gpio_pins = LED_4_GPIO_PIN;
  gpio_init(LED_4_GPIO_PORT, &gpio_init_struct);
	LED_ON(LED_4_GPIO_PORT, LED_4_GPIO_PIN);

  return 0;
}

uint8_t BSP_LED_On(Led_TypeDef Led)
{
switch(Led)
	{
		case LED1:
			LED_ON(LED_1_GPIO_PORT, LED_1_GPIO_PIN);
		break;
		case LED2:
			LED_ON(LED_2_GPIO_PORT, LED_2_GPIO_PIN);
		break;
		case LED3:
			LED_ON(LED_3_GPIO_PORT, LED_3_GPIO_PIN);	
		break;
		case LED4:
			LED_ON(LED_4_GPIO_PORT, LED_4_GPIO_PIN);	
		break;

		default:
			return 1;
	}
	return 0;
}

uint8_t BSP_LED_Off(Led_TypeDef Led)
{
	switch(Led)
	{
		case LED1:
			LED_OFF(LED_1_GPIO_PORT, LED_1_GPIO_PIN);
		break;
		case LED2:
			LED_OFF(LED_2_GPIO_PORT, LED_2_GPIO_PIN);
		break;
		case LED3:
			LED_OFF(LED_3_GPIO_PORT, LED_3_GPIO_PIN);	
		break;
		case LED4:
			LED_OFF(LED_4_GPIO_PORT, LED_4_GPIO_PIN);	
		break;

		default:
			return 1;
	}
	return 0;
}

uint8_t BSP_LED_Toggle(Led_TypeDef Led)
{
 	switch(Led)
	{
		case LED1:
			LED_TOGGL(LED_1_GPIO_PORT, LED_1_GPIO_PIN);
		break;
		case LED2:
			LED_TOGGL(LED_2_GPIO_PORT, LED_2_GPIO_PIN);
		break;
		case LED3:
			LED_TOGGL(LED_3_GPIO_PORT, LED_3_GPIO_PIN);	
		break;
		case LED4:
			LED_TOGGL(LED_4_GPIO_PORT, LED_4_GPIO_PIN);	
		break;
		default:
			return 1;
	}
	return 0;
}

/*****************************END OF FILE****/
