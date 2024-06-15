/*
 * functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (c) 2024 Udo Munk
 */

#include "lcd.h"

int lcd_init(void)
{
	/* initialize LCD device */
	if (DEV_Module_Init() != 0)
	{
		return 1;
	}

	/* set orientation */
	LCD_1IN14_V2_Init(HORIZONTAL);
	LCD_1IN14_V2_Clear(BLACK);

	/* allocate memory for an image */
	UWORD *BlackImage;
	UDOUBLE Imagesize = LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH * 2;
	if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL)
	{
		return 2;
	}

	/* use this image memory */
	Paint_NewImage((UBYTE *)BlackImage, LCD_1IN14_V2.WIDTH,
			LCD_1IN14_V2.HEIGHT, 0, WHITE);
	Paint_SetScale(65);
	Paint_SetRotate(ROTATE_0);

	/* draw our stuff */
	Paint_Clear(BLUE);
	Paint_DrawString_EN(25, 40, "# Z80pack #", &Font24, BLACK, WHITE);
	Paint_DrawString_EN(25, 64, "RP2040-GEEK", &Font24, BLACK, WHITE);

	/* refresh the picture in memory to LCD */
	LCD_1IN14_V2_Display(BlackImage);

	return 0;
}
