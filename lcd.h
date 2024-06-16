/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (c) 2024 Udo Munk
 */

#ifndef LCD_H
#define LCD_H

#include "LCD_1in14_V2.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"

extern int lcd_init(void);
extern void lcd_refresh(void);
extern void lcd_banner(void), lcd_info(void);

#endif
