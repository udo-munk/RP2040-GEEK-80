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
	LCD_1IN14_V2_Clear(WHITE);
	DEV_SET_PWM(100);
	Paint_SetScale(65);
	Paint_SetRotate(ROTATE_0);

    Paint_Clear(WHITE);
    Paint_DrawPoint(2, 1, BLACK, DOT_PIXEL_1X1, DOT_FILL_RIGHTUP); // 240 240
    Paint_DrawPoint(2, 6, BLACK, DOT_PIXEL_2X2, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2, 11, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2, 16, BLACK, DOT_PIXEL_4X4, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2, 21, BLACK, DOT_PIXEL_5X5, DOT_FILL_RIGHTUP);
    Paint_DrawLine(10, 5, 40, 35, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(10, 35, 40, 5, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawLine(80, 20, 110, 20, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(95, 5, 95, 35, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawRectangle(10, 5, 40, 35, RED, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(45, 5, 75, 35, BLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

    Paint_DrawCircle(95, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(130, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawNum(50, 40, 9.87654321, &Font20, 3, WHITE, BLACK);
    Paint_DrawString_EN(1, 40, "ABC", &Font20, 0x000f, 0xfff0);
    Paint_DrawString_CN(1, 60, "»¶Ó­Ê¹ÓÃ", &Font24CN, WHITE, BLUE);
    Paint_DrawString_EN(1, 100, "Pico-GEEK", &Font16, RED, WHITE);
}
