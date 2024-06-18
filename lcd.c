/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (c) 2024 Udo Munk
 */

#include "lcd.h"

/* memory image for drawing */
static UWORD img[LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH];

void lcd_init(void)
{
	/* initialize LCD device */
	DEV_Module_Init();

	/* set orientation, x = 240 width and y = 135 height */
	LCD_1IN14_V2_Init(HORIZONTAL);

	/* use this image memory */
	Paint_NewImage((UBYTE *) &img[0], LCD_1IN14_V2.WIDTH,
			LCD_1IN14_V2.HEIGHT, 0, WHITE);
	/* with this scale */
	Paint_SetScale(65);
	/* 0,0 is upper left corner */
	Paint_SetRotate(ROTATE_0);
}

void lcd_refresh(void)
{
	/* refresh the picture in memory to LCD */
	LCD_1IN14_V2_Display(&img[0]);
}

void lcd_info(void)
{
	Paint_DrawString_EN(25, 40, "Waiting for", &Font24, RED, BLACK);
	Paint_DrawString_EN(25, 64, "  terminal ", &Font24, RED, BLACK);

	lcd_refresh();
}

void lcd_banner(void)
{
	Paint_DrawString_EN(25, 40, "# Z80pack #", &Font24, BLACK, WHITE);
	Paint_DrawString_EN(25, 64, "RP2040-GEEK", &Font24, BLACK, WHITE);

	lcd_refresh();
}

void lcd_running(void)
{
	Paint_DrawString_EN(25, 40, "   CPU is  ", &Font24, GREEN, BLACK);
	Paint_DrawString_EN(25, 64, "   running ", &Font24, GREEN, BLACK);

	lcd_refresh();
}
