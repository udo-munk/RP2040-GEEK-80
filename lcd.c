/*
 * functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (c) 2024 Udo Munk
 */

#include "LCD_1in14_V2.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"

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

void lcd_exit(void)
{
	DEV_Module_Exit();
}
