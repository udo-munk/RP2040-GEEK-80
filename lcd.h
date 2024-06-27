/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#ifndef LCD_H
#define LCD_H

#include "LCD_1in14_V2.h"
#include "GUI_Paint.h"

extern void lcd_init(void), lcd_exit(void);
extern void lcd_set_rotated(int);
extern void lcd_set_draw_func(void (*)(int));
extern void lcd_default_draw_func(void);
extern void lcd_brightness(int);

#endif
