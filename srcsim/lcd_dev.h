/*
 * Functions for communicating with the RP2040/RP2350-GEEK LCD
 * (based on Waveshare example code)
 *
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#ifndef LCD_DEV_INC
#define LCD_DEV_INC

#include <stdint.h>

#include "draw.h"

extern void lcd_dev_init(void);
extern void lcd_dev_exit(void);
extern void lcd_dev_backlight(uint8_t value);
extern void lcd_dev_rotation(uint8_t rotated);
extern void lcd_dev_send_pixmap(draw_pixmap_t *pixmap);

#endif /* !LCD_DEV_INC */
