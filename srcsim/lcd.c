/*
 * Functions for using the RP2040/RP2350-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#include <stdio.h>
#include <string.h>
#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "hardware/adc.h"
#if PICO_RP2040
#include "hardware/divider.h"
#endif

#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simmem.h"

#include "draw.h"
#include "lcd.h"

#if COLOR_DEPTH == 12
#define STRIDE (((WAVESHARE_GEEK_LCD_WIDTH + 1) / 2) * 3)
#else
#define STRIDE (WAVESHARE_GEEK_LCD_WIDTH * 2)
#endif
static uint8_t pixmap_bits[WAVESHARE_GEEK_LCD_HEIGHT * STRIDE];

/*
 *	pixmap for drawing into.
 */
static draw_pixmap_t lcd_pixmap = {
	.bits = pixmap_bits,
	.depth = COLOR_DEPTH,
	.width = WAVESHARE_GEEK_LCD_WIDTH,
	.height = WAVESHARE_GEEK_LCD_HEIGHT,
	.stride = STRIDE
};

static mutex_t lcd_mutex;
static volatile lcd_func_t lcd_draw_func;
static volatile lcd_func_t lcd_status_func;
static volatile int lcd_shows_status;
static volatile int lcd_task_done;

static void lcd_task(void);
static void lcd_draw_empty(int first);
static void lcd_draw_cpu_reg(int first);
static void lcd_draw_memory(int first);
#ifdef SIMPLEPANEL
static void lcd_draw_panel(int first);
#endif

void lcd_init(void)
{
	mutex_init(&lcd_mutex);

	lcd_status_func = lcd_draw_cpu_reg;
	lcd_draw_func = lcd_draw_empty;
	lcd_task_done = 0;

	draw_set_pixmap(&lcd_pixmap);

	/* launch LCD task on other core */
	multicore_launch_core1(lcd_task);
}

void lcd_exit(void)
{
	/* tell LCD refresh task to finish */
	lcd_custom_disp(NULL);

	/* wait until it stopped */
	while (!lcd_task_done)
		sleep_ms(20);

	/* kill LCD refresh task and reset core 1 */
	multicore_reset_core1();
}

#define LCD_REFRESH_US (1000000 / LCD_REFRESH)

static void __not_in_flash_func(lcd_task)(void)
{
	absolute_time_t t;
	int first = 1;
	int64_t d;
	lcd_func_t curr_func = NULL;

	/* initialize the LCD controller */
	lcd_dev_init();

	while (1) {
		/* loops every LCD_REFRESH_US */

		t = get_absolute_time();

		if (lcd_draw_func == NULL)
			break;

		if (curr_func != lcd_draw_func) {
			curr_func = lcd_draw_func;
			first = 1;
		}
		(*curr_func)(first);
		first = 0;
		mutex_enter_blocking(&lcd_mutex);
		lcd_dev_display(draw_pixmap);
		mutex_exit(&lcd_mutex);

		d = absolute_time_diff_us(t, get_absolute_time());
		// printf("SLEEP %lld\n", LCD_REFRESH_US - d);
		if (d < LCD_REFRESH_US)
			sleep_us(LCD_REFRESH_US - d);
#if 0
		else
			puts("REFRESH!");
#endif
	}

	mutex_enter_blocking(&lcd_mutex);
	/* deinitialize the LCD controller */
	lcd_dev_exit();
	mutex_exit(&lcd_mutex);
	lcd_task_done = 1;

	while (1)
		__wfi();
}

void lcd_brightness(int brightness)
{
	lcd_dev_backlight((uint8_t) brightness);
}

void lcd_set_rotated(int rotated)
{
	mutex_enter_blocking(&lcd_mutex);
	lcd_dev_rotated(rotated);
	mutex_exit(&lcd_mutex);
}

void lcd_custom_disp(lcd_func_t draw_func)
{
	mutex_enter_blocking(&lcd_mutex);
	lcd_draw_func = draw_func;
	lcd_shows_status = 0;
	mutex_exit(&lcd_mutex);
}

void lcd_status_disp(int which)
{
	mutex_enter_blocking(&lcd_mutex);
	switch (which) {
	case LCD_STATUS_REGISTERS:
		lcd_status_func = lcd_draw_cpu_reg;
		break;
	case LCD_STATUS_MEMORY:
		lcd_status_func = lcd_draw_memory;
		break;
#ifdef SIMPLEPANEL
	case LCD_STATUS_PANEL:
		lcd_status_func = lcd_draw_panel;
		break;
#endif
	case LCD_STATUS_CURRENT:
	default:
		break;
	}
	lcd_draw_func = lcd_status_func;
	lcd_shows_status = 1;
	mutex_exit(&lcd_mutex);
}

void lcd_status_next(void)
{
	if (lcd_status_func == lcd_draw_cpu_reg)
#ifdef SIMPLEPANEL
		lcd_status_func = lcd_draw_panel;
	else if (lcd_status_func == lcd_draw_panel)
#endif
		lcd_status_func = lcd_draw_memory;
	else
		lcd_status_func = lcd_draw_cpu_reg;
	if (lcd_shows_status) {
		mutex_enter_blocking(&lcd_mutex);
		lcd_draw_func = lcd_status_func;
		mutex_exit(&lcd_mutex);
	}
}

static void lcd_draw_empty(int first)
{
	if (first)
		draw_clear(C_BLACK);
}

/*
 *	CPU status displays:
 *
 *	Z80 CPU using font20 (10 x 20 pixels)
 *	-------------------------------------
 *
 *	  012345678901234567890123
 *	0 A  12   BC 1234 DE 1234
 *	1 HL 1234 SP 1234 PC 1234
 *	2 IX 1234 IY 1234 AF`1234
 *	3 BC'1234 DE'1234 HL`1234
 *	4 F  SZHPNC  IF12 IR 1234
 *	5 info             xx.xxC
 *
 *	8080 CPU using font28 (14 x 28 pixels)
 *	--------------------------------------
 *
 *	  01234567890123456
 *	0 A  12    BC 1234
 *	1 DE 1234  HL 1234
 *	2 SP 1234  PC 1234
 *	3 F  SZHPC    IF 1
 *	4 info      xx.xxC (font20)
 */

#define XOFF20	5	/* x pixel offset of text coordinate grid for font20 */
#define YOFF20	0	/* y pixel offset of text coordinate grid for font20 */
#define SPC20	3	/* vertical spacing for font20 */

#define XOFF28	8	/* x pixel offset of text coordinate grid for font28 */
#define YOFF28	0	/* y pixel offset of text coordinate grid for font28 */
#define SPC28	1	/* vertical spacing for font28 */

/*
 *	Attribute used to disable cloning of __no_inline_not_in_flash functions
 *	with constant arguments at high optimization levels.
 *	Reduces RAM usage dramatically.
 */
#ifndef __noclone
#define __noclone	__attribute__((__noclone__))
#endif

/*
 *	Draw character "c" at text coordinates "x, y" with color "col".
 */
static void __noclone __no_inline_not_in_flash_func(cpu_char)
	(uint16_t x, uint16_t y, const char c, uint16_t col,
	 const font_t *font, uint16_t offx, uint16_t offy, uint16_t spc)
{
	draw_char(x * font->width + offx, y * (font->height + spc) + offy, c,
		  font, col, C_DKBLUE);
}

static inline void cpu_char20(uint16_t x, uint16_t y, const char c,
			      uint16_t col)
{
	cpu_char(x, y, c, col, &font20, XOFF20, YOFF20, SPC20);
}

static inline void cpu_char28(uint16_t x, uint16_t y, const char c,
			      uint16_t col)
{
	cpu_char(x, y, c, col, &font28, XOFF28, YOFF28, SPC28);
}

/*
 *	Draw horizontal grid line in the middle of vertical spacing below
 *	text row "y" with color "col" and vertical adjustment "adj".
 */
static void __noclone __no_inline_not_in_flash_func(cpu_gridh)
	(uint16_t y, uint16_t adj, uint16_t col, const font_t *font,
	 uint16_t offy, uint16_t spc)
{
	draw_hline(0,
		   (y + 1) * (font->height + spc) - (spc + 1) / 2 + offy - adj,
		   draw_pixmap->width, col);
}

static inline void cpu_gridh20(uint16_t y, uint16_t adj, uint16_t col)
{
	cpu_gridh(y, adj, col, &font20, YOFF20, SPC20);
}

static inline void cpu_gridh28(uint16_t y, uint16_t adj, uint16_t col)
{
	cpu_gridh(y, adj, col, &font28, YOFF28, SPC28);
}

/*
 *	Draw vertical grid line in the middle of text column "x" with
 *	color "col" from the top of the screen to the middle of vertical
 *	spacing below text row "y" with vertical adjustment "adj".
 */
static void __noclone __no_inline_not_in_flash_func(cpu_gridv)
	(uint16_t x, uint16_t y, uint16_t adj, uint16_t col,
	 const font_t *font, uint16_t offx, uint16_t offy, uint16_t spc)
{
	draw_vline(x * font->width + font->width / 2 + offx, 0,
		   (y + 1) * (font->height + spc) - (spc + 1) / 2 + offy - adj,
		   col);
}

static inline void cpu_gridv20(uint16_t x, uint16_t y, uint16_t adj,
			       uint16_t col) {
	cpu_gridv(x, y, adj, col, &font20, XOFF20, YOFF20, SPC20);
}

static inline void cpu_gridv28(uint16_t x, uint16_t y, uint16_t adj,
			       uint16_t col) {
	cpu_gridv(x, y, adj, col, &font28, XOFF28, YOFF28, SPC28);
}

/*
 *	Draw short vertical grid line in the middle of text column "x" with
 *	color "col" from the middle of vertical spacing above text row "y0"
 *	to the middle of vertical spacing below text row "y1" with vertical
 *	adjustment "adj".
 */
static void __noclone __no_inline_not_in_flash_func(cpu_gridvs)
	(uint16_t x, uint16_t y0, uint16_t y1, uint16_t adj, uint16_t col,
	 const font_t *font, uint16_t offx, uint16_t offy, uint16_t spc)
{
	draw_vline(x * font->width + font->width / 2 + offx,
		   y0 * (font->height + spc) - (spc + 1) / 2 + offy,
		   (y1 - y0 + 1) * (font->height + spc) - adj, col);
}

static inline void cpu_gridvs20(uint16_t x, uint16_t y0, uint16_t y1,
				uint16_t adj, uint16_t col)
{
	cpu_gridvs(x, y0, y1, adj, col, &font20, XOFF20, YOFF20, SPC20);
}

typedef struct lbl {
	uint8_t x;
	uint8_t y;
	char c;
} lbl_t;

#ifndef EXCLUDE_Z80
static const lbl_t __not_in_flash("lcd_tables") lbls_z80[] = {
	{  0, 0, 'A'},
	{  8, 0, 'B'}, {  9, 0, 'C'},
	{ 16, 0, 'D'}, { 17, 0, 'E'},
	{  0, 1, 'H'}, {  1, 1, 'L'},
	{  8, 1, 'S'}, {  9, 1, 'P'},
	{ 16, 1, 'P'}, { 17, 1, 'C'},
	{  0, 2, 'I'}, {  1, 2, 'X'},
	{  8, 2, 'I'}, {  9, 2, 'Y'},
	{ 16, 2, 'A'}, { 17, 2, 'F'}, { 18, 2, '\''},
	{  0, 3, 'B'}, {  1, 3, 'C'}, {  2, 3, '\''},
	{  8, 3, 'D'}, {  9, 3, 'E'}, { 10, 3, '\''},
	{ 16, 3, 'H'}, { 17, 3, 'L'}, { 18, 3, '\''},
	{  0, 4, 'F'},
	{ 11, 4, 'I'}, { 12, 4, 'F'},
	{ 16, 4, 'I'}, { 17, 4, 'R'}
};
static const int num_lbls_z80 = sizeof(lbls_z80) / sizeof(lbl_t);
#endif

#ifndef EXCLUDE_I8080
static const lbl_t __not_in_flash("lcd_tables") lbls_8080[] = {
	{  0, 0, 'A'},
	{  9, 0, 'B'}, { 10, 0, 'C'},
	{  0, 1, 'D'}, {  1, 1, 'E'},
	{  9, 1, 'H'}, { 10, 1, 'L'},
	{  0, 2, 'S'}, {  1, 2, 'P'},
	{  9, 2, 'P'}, { 10, 2, 'C'},
	{  0, 3, 'F'},
	{ 12, 3, 'I'}, { 13, 3, 'F'}
};
static const int num_lbls_8080 = sizeof(lbls_8080) / sizeof(lbl_t);
#endif

typedef struct reg {
	uint8_t x;
	uint8_t y;
	enum { RB, RW, RF, RI, RFA, RR } type;
	union {
		struct {
			const BYTE *p;
			uint8_t s;
		} b;
		struct {
			const WORD *p;
			uint8_t s;
		} w;
		struct {
			char c;
			uint8_t m;
		} f;
	};
} reg_t;

#ifndef EXCLUDE_Z80
static const reg_t __not_in_flash("lcd_tables") regs_z80[] = {
	{  3, 0, RB, .b.p = &A,  .b.s =  4 },
	{  4, 0, RB, .b.p = &A,  .b.s =  0 },
	{ 11, 0, RB, .b.p = &B,  .b.s =  4 },
	{ 12, 0, RB, .b.p = &B,  .b.s =  0 },
	{ 13, 0, RB, .b.p = &C,  .b.s =  4 },
	{ 14, 0, RB, .b.p = &C,  .b.s =  0 },
	{ 19, 0, RB, .b.p = &D,  .b.s =  4 },
	{ 20, 0, RB, .b.p = &D,  .b.s =  0 },
	{ 21, 0, RB, .b.p = &E,  .b.s =  4 },
	{ 22, 0, RB, .b.p = &E,  .b.s =  0 },
	{  3, 1, RB, .b.p = &H,  .b.s =  4 },
	{  4, 1, RB, .b.p = &H,  .b.s =  0 },
	{  5, 1, RB, .b.p = &L,  .b.s =  4 },
	{  6, 1, RB, .b.p = &L,  .b.s =  0 },
	{ 11, 1, RW, .w.p = &SP, .w.s = 12 },
	{ 12, 1, RW, .w.p = &SP, .w.s =  8 },
	{ 13, 1, RW, .w.p = &SP, .w.s =  4 },
	{ 14, 1, RW, .w.p = &SP, .w.s =  0 },
	{ 19, 1, RW, .w.p = &PC, .w.s = 12 },
	{ 20, 1, RW, .w.p = &PC, .w.s =  8 },
	{ 21, 1, RW, .w.p = &PC, .w.s =  4 },
	{ 22, 1, RW, .w.p = &PC, .w.s =  0 },
	{  3, 2, RW, .w.p = &IX, .w.s = 12 },
	{  4, 2, RW, .w.p = &IX, .w.s =  8 },
	{  5, 2, RW, .w.p = &IX, .w.s =  4 },
	{  6, 2, RW, .w.p = &IX, .w.s =  0 },
	{ 11, 2, RW, .w.p = &IY, .w.s = 12 },
	{ 12, 2, RW, .w.p = &IY, .w.s =  8 },
	{ 13, 2, RW, .w.p = &IY, .w.s =  4 },
	{ 14, 2, RW, .w.p = &IY, .w.s =  0 },
	{ 19, 2, RB, .b.p = &A_, .b.s =  4 },
	{ 20, 2, RB, .b.p = &A_, .b.s =  0 },
	{ 21, 2, RFA, .b.p = NULL, .b.s = 4 },
	{ 22, 2, RFA, .b.p = NULL, .b.s = 0 },
	{  3, 3, RB, .b.p = &B_, .b.s =  4 },
	{  4, 3, RB, .b.p = &B_, .b.s =  0 },
	{  5, 3, RB, .b.p = &C_, .b.s =  4 },
	{  6, 3, RB, .b.p = &C_, .b.s =  0 },
	{ 11, 3, RB, .b.p = &D_, .b.s =  4 },
	{ 12, 3, RB, .b.p = &D_, .b.s =  0 },
	{ 13, 3, RB, .b.p = &E_, .b.s =  4 },
	{ 14, 3, RB, .b.p = &E_, .b.s =  0 },
	{ 19, 3, RB, .b.p = &H_, .b.s =  4 },
	{ 20, 3, RB, .b.p = &H_, .b.s =  0 },
	{ 21, 3, RB, .b.p = &L_, .b.s =  4 },
	{ 22, 3, RB, .b.p = &L_, .b.s =  0 },
	{  3, 4, RF, .f.c = 'S', .f.m = S_FLAG },
	{  4, 4, RF, .f.c = 'Z', .f.m = Z_FLAG },
	{  5, 4, RF, .f.c = 'H', .f.m = H_FLAG },
	{  6, 4, RF, .f.c = 'P', .f.m = P_FLAG },
	{  7, 4, RF, .f.c = 'N', .f.m = N_FLAG },
	{  8, 4, RF, .f.c = 'C', .f.m = C_FLAG },
	{ 13, 4, RI, .f.c = '1', .f.m =  1 },
	{ 14, 4, RI, .f.c = '2', .f.m =  2 },
	{ 19, 4, RB, .b.p = &I,  .b.s =  4 },
	{ 20, 4, RB, .b.p = &I,  .b.s =  0 },
	{ 21, 4, RR, .b.p = NULL, .b.s =  4 },
	{ 22, 4, RR, .b.p = NULL, .b.s =  0 }
};
static const int num_regs_z80 = sizeof(regs_z80) / sizeof(reg_t);
#endif

#ifndef EXCLUDE_I8080
static const reg_t __not_in_flash("lcd_tables") regs_8080[] = {
	{  3, 0, RB, .b.p = &A,  .b.s =  4 },
	{  4, 0, RB, .b.p = &A,  .b.s =  0 },
	{ 12, 0, RB, .b.p = &B,  .b.s =  4 },
	{ 13, 0, RB, .b.p = &B,  .b.s =  0 },
	{ 14, 0, RB, .b.p = &C,  .b.s =  4 },
	{ 15, 0, RB, .b.p = &C,  .b.s =  0 },
	{  3, 1, RB, .b.p = &D,  .b.s =  4 },
	{  4, 1, RB, .b.p = &D,  .b.s =  0 },
	{  5, 1, RB, .b.p = &E,  .b.s =  4 },
	{  6, 1, RB, .b.p = &E,  .b.s =  0 },
	{ 12, 1, RB, .b.p = &H,  .b.s =  4 },
	{ 13, 1, RB, .b.p = &H,  .b.s =  0 },
	{ 14, 1, RB, .b.p = &L,  .b.s =  4 },
	{ 15, 1, RB, .b.p = &L,  .b.s =  0 },
	{  3, 2, RW, .w.p = &SP, .w.s = 12 },
	{  4, 2, RW, .w.p = &SP, .w.s =  8 },
	{  5, 2, RW, .w.p = &SP, .w.s =  4 },
	{  6, 2, RW, .w.p = &SP, .w.s =  0 },
	{ 12, 2, RW, .w.p = &PC, .w.s = 12 },
	{ 13, 2, RW, .w.p = &PC, .w.s =  8 },
	{ 14, 2, RW, .w.p = &PC, .w.s =  4 },
	{ 15, 2, RW, .w.p = &PC, .w.s =  0 },
	{  3, 3, RF, .f.c = 'S', .f.m = S_FLAG },
	{  4, 3, RF, .f.c = 'Z', .f.m = Z_FLAG },
	{  5, 3, RF, .f.c = 'H', .f.m = H_FLAG },
	{  6, 3, RF, .f.c = 'P', .f.m = P_FLAG },
	{  7, 3, RF, .f.c = 'C', .f.m = C_FLAG },
	{ 15, 3, RI, .f.c = '1', .f.m =  3 }
};
static const int num_regs_8080 = sizeof(regs_8080) / sizeof(reg_t);
#endif

static inline float read_onboard_temp(void)
{
	/* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
	const float conversionFactor = 3.3f / (1 << 12);

	float adc = (float) adc_read() * conversionFactor;
	float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

	return tempC;
}

static void __not_in_flash_func(lcd_draw_cpu_reg)(int first)
{
	char *p, c;
	int i, n = 0, temp, rem;
	uint16_t col;
	const lbl_t *lp;
	const reg_t *rp = NULL;
#if PICO_RP2040
	divmod_result_t res;
#endif
	static int cpu_type, counter;

	if (first || (cpu_type != cpu)) {
		/* if first call or new CPU type, draw static background */
		draw_clear(C_DKBLUE);

		cpu_type = cpu;
		/* use cpu_type in the following code, since cpu can change */
#ifndef EXCLUDE_Z80
		if (cpu_type == Z80) {
			/* draw vertical grid lines */
			cpu_gridv20(7, 3, 0, C_DKYELLOW);
			cpu_gridvs20(10, 4, 4, 0, C_DKYELLOW);
			cpu_gridv20(15, 4, 0, C_DKYELLOW);
			/* draw horizontal grid lines */
			for (i = 0; i < 5; i++)
				cpu_gridh20(i, 0, C_DKYELLOW);
			/* draw labels */
			lp = lbls_z80;
			for (i = 0; i < num_lbls_z80; i++) {
				cpu_char20(lp->x, lp->y, lp->c, C_WHITE);
				lp++;
			}
		}
#endif
#ifndef EXCLUDE_I8080
		if (cpu_type == I8080) {
			/* adjustment keeps vertical grid line outside
			   "info" */
			cpu_gridv28(8, 3, 2, C_DKYELLOW);
			/* draw horizontal grid lines */
			for (i = 0; i < 3; i++)
				cpu_gridh28(i, 0, C_DKYELLOW);
			/* draw labels */
			lp = lbls_8080;
			for (i = 0; i < num_lbls_8080; i++) {
				cpu_char28(lp->x, lp->y, lp->c, C_WHITE);
				lp++;
			}
		}
#endif
		/* draw info line */
		p = "Z80pack " USR_REL;
		for (i = 0; *p && i < 16; i++)
			cpu_char20(i, 5, *p++, C_ORANGE);
		cpu_char20(19, 5, '.', C_ORANGE);
		cpu_char20(22, 5, 'C', C_ORANGE);
	} else {
#ifndef EXCLUDE_Z80
		if (cpu_type == Z80) {
			rp = regs_z80;
			n = num_regs_z80;
		}
#endif
#ifndef EXCLUDE_I8080
		if (cpu_type == I8080) {
			rp = regs_8080;
			n = num_regs_8080;
		}
#endif
		/* draw register contents */
		for (i = 0; i < n; i++) {
			col = C_GREEN;
			switch (rp->type) {
			case RB: /* byte sized register */
				c = (*(rp->b.p) >> rp->b.s) & 0xf;
				c += c < 10 ? '0' : 'A' - 10;
				break;
			case RW: /* word sized register */
				c = (*(rp->w.p) >> rp->w.s) & 0xf;
				c += c < 10 ? '0' : 'A' - 10;
				break;
			case RF: /* flags */
				c = rp->f.c;
				if (!(F & rp->f.m))
					col = C_RED;
				break;
			case RI: /* interrupt register */
				c = rp->f.c;
				if ((IFF & rp->f.m) != rp->f.m)
					col = C_RED;
				break;
#ifndef EXCLUDE_Z80
			case RFA: /* alternate flags (int) */
				c = (F_ >> rp->b.s) & 0xf;
				c += c < 10 ? '0' : 'A' - 10;
				break;
			case RR: /* refresh register */
				c = (((R_ & 0x80) | (R & 0x7f)) >> rp->b.s)
					& 0xf;
				c += c < 10 ? '0' : 'A' - 10;
				break;
#endif
			default: /* shouldn't happen, silence compiler */
				c = ' ';
				break;
			}
#ifndef EXCLUDE_Z80
			if (cpu_type == Z80)
				cpu_char20(rp->x, rp->y, c, col);
#endif
#ifndef EXCLUDE_I8080
			if (cpu_type == I8080)
				cpu_char28(rp->x, rp->y, c, col);
#endif
			rp++;
		}
#ifndef EXCLUDE_I8080
		if (cpu_type == I8080) {
			/*
			 *	The adjustment moves the grid line from inside
			 *	the "info" into the descenders space of the
			 *	"F IF" line, therefore the need to draw it
			 *	inside the refresh code.
			 */
			cpu_gridh28(3, 2, C_DKYELLOW);
		}
#endif
		/* update temperature every second */
		if (++counter == LCD_REFRESH) {
			counter = 0;
			temp = (int) (read_onboard_temp() * 100.0f + 0.5f);
#if PICO_RP2040
			res = (divmod_result_t) temp;
#endif
			for (i = 0; i < 5; i++) {
#if PICO_RP2040
				res = hw_divider_divmod_u32
					(to_quotient_u32(res), 10);
				rem = to_remainder_u32(res);
#else
				rem = temp % 10;
				if (i < 4)
					temp /= 10;
#endif
				cpu_char20(21 - i, 5, '0' + rem, C_ORANGE);
				if (i == 1)
					i++; /* skip decimal point */
			}
		}
	}
}

#define MEM_XOFF 3
#define MEM_YOFF 0
#define MEM_BRDR 3

static void __not_in_flash_func(lcd_draw_memory)(int first)
{
	int x, y;
	uint32_t *p;

	if (first) {
		draw_clear(C_DKBLUE);
		draw_hline(MEM_XOFF, MEM_YOFF, 128 + 96 + 4 * MEM_BRDR - 1,
			   C_GREEN);
		draw_hline(MEM_XOFF, MEM_YOFF + 128 + 2 * MEM_BRDR - 1,
			   128 + 96 + 4 * MEM_BRDR - 1, C_GREEN);
		draw_vline(MEM_XOFF, MEM_YOFF, 128 + 2 * MEM_BRDR, C_GREEN);
		draw_vline(MEM_XOFF + 128 + 2 * MEM_BRDR - 1, 0,
			   128 + 2 * MEM_BRDR, C_GREEN);
		draw_vline(MEM_XOFF + 128 + 96 + 4 * MEM_BRDR - 2, 0,
			   128 + 2 * MEM_BRDR, C_GREEN);
	} else {
		p = (uint32_t *) bnk0;
		for (x = MEM_XOFF + MEM_BRDR;
		     x < MEM_XOFF + MEM_BRDR + 128; x++) {
			for (y = MEM_YOFF + MEM_BRDR;
			     y < MEM_YOFF + MEM_BRDR + 128; y++) {
				/* constant = 2^32 / ((1 + sqrt(5)) / 2) */
				draw_pixel(x, y, (*p++ * 2654435769U) >> 20);
			}
		}
		p = (uint32_t *) bnk1;
		for (x = MEM_XOFF + 3 * MEM_BRDR - 1 + 128;
		     x < MEM_XOFF + 3 * MEM_BRDR - 1 + 128 + 96; x++) {
			for (y = MEM_YOFF + MEM_BRDR;
			     y < MEM_YOFF + MEM_BRDR + 128; y++) {
				draw_pixel(x, y, (*p++ * 2654435769U) >> 20);
			}
		}
	}
}

#ifdef SIMPLEPANEL

#define PXOFF	6				/* panel x offset */
#define PYOFF	6				/* panel y offset */

#define PFNTH	12				/* font12 height */
#define PFNTW	6				/* font12 width */
#define PFNTS	1				/* font12 letter spacing */

#define PLBLW	(2 * PFNTW - PFNTS)		/* Label width */
#define PLBLS	2				/* Label vertical spacing */
#define PLEDS	3 				/* LED spacing */
#define PLEDBS	6				/* LED bank of 8 spacing */

#define PLEDD	10				/* LED diameter */
#define PLEDXO	((PLBLW - PLEDD + 1) / 2)	/* LED x off from label left */
#define PLEDYO	(PFNTH + PLBLS)			/* LED y off from label top */
#define PLEDHO	(PLBLW + PLEDS)			/* horiz. offset to next LED */
#define PLEDVO	(3 * PFNTH)			/* vert. offset to next row */

#define PLEDX(x) (PXOFF + PLEDXO + PLEDBS * ((x) / 8) + PLEDHO * (x))
#define PLEDY(y) (PYOFF + PLEDYO + PLEDVO * (y))

static BYTE fp_led_wait;			/* dummy */

typedef struct led {
	uint16_t x;
	uint16_t y;
	char c1;
	char c2;
	enum { LB, LW } type;
	union {
		struct {
			BYTE i;
			BYTE m;
			const BYTE *p;
		} b;
		struct {
			WORD m;
			const WORD *p;
		} w;
	};
} led_t;

static const led_t __not_in_flash("lcd_tables") leds[] = {
	{ PLEDX( 0), PLEDY(0), 'P', '7', LB, .b.i = 0xff, .b.m = 0x80,
	  .b.p = &fp_led_output },
	{ PLEDX( 1), PLEDY(0), 'P', '6', LB, .b.i = 0xff, .b.m = 0x40,
	  .b.p = &fp_led_output },
	{ PLEDX( 2), PLEDY(0), 'P', '5', LB, .b.i = 0xff, .b.m = 0x20,
	  .b.p = &fp_led_output },
	{ PLEDX( 3), PLEDY(0), 'P', '4', LB, .b.i = 0xff, .b.m = 0x10,
	  .b.p = &fp_led_output },
	{ PLEDX( 4), PLEDY(0), 'P', '3', LB, .b.i = 0xff, .b.m = 0x08,
	  .b.p = &fp_led_output },
	{ PLEDX( 5), PLEDY(0), 'P', '2', LB, .b.i = 0xff, .b.m = 0x04,
	  .b.p = &fp_led_output },
	{ PLEDX( 6), PLEDY(0), 'P', '1', LB, .b.i = 0xff, .b.m = 0x02,
	  .b.p = &fp_led_output },
	{ PLEDX( 7), PLEDY(0), 'P', '0', LB, .b.i = 0xff, .b.m = 0x01,
	  .b.p = &fp_led_output },
	{ PLEDX(12), PLEDY(0), 'I', 'E', LB, .b.i = 0x00, .b.m = 0x01,
	  .b.p = &IFF },
	{ PLEDX(13), PLEDY(0), 'R', 'U', LB, .b.i = 0x00, .b.m = 0x01,
	  .b.p = &cpu_state },
	{ PLEDX(14), PLEDY(0), 'W', 'A', LB, .b.i = 0x00, .b.m = 0x01,
	  .b.p = &fp_led_wait },
	{ PLEDX(15), PLEDY(0), 'H', 'O', LB, .b.i = 0x00, .b.m = 0x01,
	  .b.p = &bus_request },
	{ PLEDX( 0), PLEDY(1), 'M', 'R', LB, .b.i = 0x00, .b.m = 0x80,
	  .b.p = &cpu_bus },
	{ PLEDX( 1), PLEDY(1), 'I', 'P', LB, .b.i = 0x00, .b.m = 0x40,
	  .b.p = &cpu_bus },
	{ PLEDX( 2), PLEDY(1), 'M', '1', LB, .b.i = 0x00, .b.m = 0x20,
	  .b.p = &cpu_bus },
	{ PLEDX( 3), PLEDY(1), 'O', 'P', LB, .b.i = 0x00, .b.m = 0x10,
	  .b.p = &cpu_bus },
	{ PLEDX( 4), PLEDY(1), 'H', 'A', LB, .b.i = 0x00, .b.m = 0x08,
	  .b.p = &cpu_bus },
	{ PLEDX( 5), PLEDY(1), 'S', 'T', LB, .b.i = 0x00, .b.m = 0x04,
	  .b.p = &cpu_bus },
	{ PLEDX( 6), PLEDY(1), 'W', 'O', LB, .b.i = 0x00, .b.m = 0x02,
	  .b.p = &cpu_bus },
	{ PLEDX( 7), PLEDY(1), 'I', 'A', LB, .b.i = 0x00, .b.m = 0x01,
	  .b.p = &cpu_bus },
	{ PLEDX( 8), PLEDY(1), 'D', '7', LB, .b.i = 0x00, .b.m = 0x80,
	  .b.p = &fp_led_data },
	{ PLEDX( 9), PLEDY(1), 'D', '6', LB, .b.i = 0x00, .b.m = 0x40,
	  .b.p = &fp_led_data },
	{ PLEDX(10), PLEDY(1), 'D', '5', LB, .b.i = 0x00, .b.m = 0x20,
	  .b.p = &fp_led_data },
	{ PLEDX(11), PLEDY(1), 'D', '4', LB, .b.i = 0x00, .b.m = 0x10,
	  .b.p = &fp_led_data },
	{ PLEDX(12), PLEDY(1), 'D', '3', LB, .b.i = 0x00, .b.m = 0x08,
	  .b.p = &fp_led_data },
	{ PLEDX(13), PLEDY(1), 'D', '2', LB, .b.i = 0x00, .b.m = 0x04,
	  .b.p = &fp_led_data },
	{ PLEDX(14), PLEDY(1), 'D', '1', LB, .b.i = 0x00, .b.m = 0x02,
	  .b.p = &fp_led_data },
	{ PLEDX(15), PLEDY(1), 'D', '0', LB, .b.i = 0x00, .b.m = 0x01,
	  .b.p = &fp_led_data },
	{ PLEDX( 0), PLEDY(2), '1', '5', LW, .w.m = 0x8000,
	  .w.p = &fp_led_address },
	{ PLEDX( 1), PLEDY(2), '1', '4', LW, .w.m = 0x4000,
	  .w.p = &fp_led_address },
	{ PLEDX( 2), PLEDY(2), '1', '3', LW, .w.m = 0x2000,
	  .w.p = &fp_led_address },
	{ PLEDX( 3), PLEDY(2), '1', '2', LW, .w.m = 0x1000,
	  .w.p = &fp_led_address },
	{ PLEDX( 4), PLEDY(2), '1', '1', LW, .w.m = 0x0800,
	  .w.p = &fp_led_address },
	{ PLEDX( 5), PLEDY(2), '1', '0', LW, .w.m = 0x0400,
	  .w.p = &fp_led_address },
	{ PLEDX( 6), PLEDY(2), 'A', '9', LW, .w.m = 0x0200,
	  .w.p = &fp_led_address },
	{ PLEDX( 7), PLEDY(2), 'A', '8', LW, .w.m = 0x0100,
	  .w.p = &fp_led_address },
	{ PLEDX( 8), PLEDY(2), 'A', '7', LW, .w.m = 0x0080,
	  .w.p = &fp_led_address },
	{ PLEDX( 9), PLEDY(2), 'A', '6', LW, .w.m = 0x0040,
	  .w.p = &fp_led_address },
	{ PLEDX(10), PLEDY(2), 'A', '5', LW, .w.m = 0x0020,
	  .w.p = &fp_led_address },
	{ PLEDX(11), PLEDY(2), 'A', '4', LW, .w.m = 0x0010,
	  .w.p = &fp_led_address },
	{ PLEDX(12), PLEDY(2), 'A', '3', LW, .w.m = 0x0008,
	  .w.p = &fp_led_address },
	{ PLEDX(13), PLEDY(2), 'A', '2', LW, .w.m = 0x0004,
	  .w.p = &fp_led_address },
	{ PLEDX(14), PLEDY(2), 'A', '1', LW, .w.m = 0x0002,
	  .w.p = &fp_led_address },
	{ PLEDX(15), PLEDY(2), 'A', '0', LW, .w.m = 0x0001,
	  .w.p = &fp_led_address }
};
static const int num_leds = sizeof(leds) / sizeof(struct led);

/*
 *	Draw a 10x10 LED circular bracket.
 */
static inline void __not_in_flash_func(panel_bracket)(uint16_t x, uint16_t y)
{
	draw_hline(x + 2, y, 6, C_GRAY);
	draw_pixel(x + 1, y + 1, C_GRAY);
	draw_pixel(x + 8, y + 1, C_GRAY);
	draw_vline(x, y + 2, 6, C_GRAY);
	draw_vline(x + 9, y + 2, 6, C_GRAY);
	draw_pixel(x + 1, y + 8, C_GRAY);
	draw_pixel(x + 8, y + 8, C_GRAY);
	draw_hline(x + 2, y + 9, 6, C_GRAY);
}

/*
 *	Draw a LED inside a 10x10 circular bracket.
 */
static inline void __not_in_flash_func(panel_led)(uint16_t x, uint16_t y,
						  int on_off)
{
	uint16_t col = on_off ? C_RED : C_DKRED;
	int i;

	for (i = 1; i < 9; i++) {
		if (i == 1 || i == 8)
			draw_hline(x + 2, y + i, 6, col);
		else
			draw_hline(x + 1, y + i, 8, col);
	}
}

static void __not_in_flash_func(lcd_draw_panel)(int first)
{
	const struct led *p;
#if PICO_RP2040
	const char *model = "Z80pack RP2040-GEEK " USR_REL;
#else
	const char *model = "Z80pack RP2350-GEEK " USR_REL;
#endif
	const char *s;
	int i, bit;

	p = leds;
	if (first) {
		draw_clear(C_DKBLUE);
		for (i = 0; i < num_leds; i++) {
			draw_char(p->x - PLEDXO, p->y - PLEDYO,
				  p->c1, &font12, C_WHITE, C_DKBLUE);
			draw_char(p->x - PLEDXO + PFNTW, p->y - PLEDYO,
				  p->c2, &font12, C_WHITE, C_DKBLUE);
			if (p->c1 == 'W' && p->c2 == 'O')
				draw_hline(p->x - PLEDXO, p->y - PLEDYO - 2,
					   PLBLW, C_WHITE);
			panel_bracket(p->x, p->y);
			p++;
		}
		i = (draw_pixmap->width - strlen(model) * font20.width) / 2;
		for (s = model; *s; s++) {
			draw_char(i, draw_pixmap->height - font20.height - 1,
				  *s, &font20, C_ORANGE, C_DKBLUE);
			i += font20.width;
		}
	} else {
		for (i = 0; i < num_leds; i++) {
			if (p->type == LB)
				bit = (*(p->b.p) ^ p->b.i) & p->b.m;
			else
				bit = *(p->w.p) & p->w.m;
			panel_led(p->x, p->y, bit);
			p++;
		}
	}
}

#endif /* SIMPLEPANEL */
