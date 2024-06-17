/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (c) 2024 Udo Munk
 */

#include "lcd.h"

/* memory image for drawing */
static UWORD *Image;
static UDOUBLE Imagesize = LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH * 2;

int lcd_init(void)
{
	/* initialize LCD device */
	if (DEV_Module_Init() != 0)
	{
		return 1;
	}

	/* set orientation, x = 240 width and y = 135 height */
	LCD_1IN14_V2_Init(HORIZONTAL);
	LCD_1IN14_V2_Clear(BLACK);

	/* allocate memory for an image */
	if ((Image = (UWORD *) malloc(Imagesize)) == NULL)
	{
		return 2;
	}

	/* use this image memory */
	Paint_NewImage((UBYTE *)Image, LCD_1IN14_V2.WIDTH,
			LCD_1IN14_V2.HEIGHT, 0, WHITE);
	/* with this scale */
	Paint_SetScale(65);
	/* 0,0 is upper left corner */
	Paint_SetRotate(ROTATE_0);

	return 0;
}

void lcd_refresh(void)
{
	/* refresh the picture in memory to LCD */
	LCD_1IN14_V2_Display(Image);
}

void lcd_banner(void)
{
	Paint_Clear(BLUE);
	Paint_DrawString_EN(25, 40, "# Z80pack #", &Font24, BLACK, WHITE);
	Paint_DrawString_EN(25, 64, "RP2040-GEEK", &Font24, BLACK, WHITE);

	lcd_refresh();
}

void lcd_info(void)
{
	Paint_Clear(BLACK);
	Paint_DrawString_EN(25, 40, "Waiting for", &Font24, RED, BLACK);
	Paint_DrawString_EN(25, 64, "  terminal ", &Font24, RED, BLACK);

	lcd_refresh();
}
