/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#ifndef LCD_H
#define LCD_H

#include "LCD_1in14_V2.h"
#include "GUI_Paint.h"

#define DEFAULT_BRIGHTNESS 90

extern void lcd_init(void), lcd_finish(void);
extern void lcd_cpudisp_on(void), lcd_cpudisp_off(void);
extern void lcd_brightness(int);
extern void lcd_banner(void), lcd_info(void);

#endif
