/*
 * functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (c) 2024 Udo Munk
 */

#include "lcd.h"

void lcd_init(void)
{
	if (DEV_Module_Init() != 0)
	{
		return;
	}

	DEV_SET_PWM(0);
	LCD_1IN14_V2_Init(HORIZONTAL);
	LCD_1IN14_V2_Clear(BLACK);
	DEV_SET_PWM(75);
}
