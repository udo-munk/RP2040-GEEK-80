/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#ifndef LCD_INC
#define LCD_INC

#include "LCD_1in14_V2.h"
#include "GUI_Paint.h"

typedef void (*lcd_func_t)(int first_flag);

extern void lcd_init(void), lcd_exit(void);
extern void lcd_set_rotated(int rotated);
extern void lcd_custom_disp(lcd_func_t draw_func);
extern void lcd_status_disp(void);
extern void lcd_brightness(int brightness);

#endif /* !LCD_INC */
