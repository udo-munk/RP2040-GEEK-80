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
		lcd_dev_send_pixmap(draw_pixmap);
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

void lcd_set_rotation(int rotated)
{
	mutex_enter_blocking(&lcd_mutex);
	lcd_dev_rotation(rotated);
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
 *	2 IX 1234 IY 1234 AF'1234
 *	3 BC'1234 DE'1234 HL'1234
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
		} b;
		struct {
			const WORD *p;
		} w;
		struct {
			char c;
			uint8_t m;
		} f;
	};
} reg_t;

#ifndef EXCLUDE_Z80
static const reg_t __not_in_flash("lcd_tables") regs_z80[] = {
	{  4, 0, RB, .b.p = &A },
	{ 12, 0, RB, .b.p = &B },
	{ 14, 0, RB, .b.p = &C },
	{ 20, 0, RB, .b.p = &D },
	{ 22, 0, RB, .b.p = &E },
	{  4, 1, RB, .b.p = &H },
	{  6, 1, RB, .b.p = &L },
	{ 14, 1, RW, .w.p = &SP },
	{ 22, 1, RW, .w.p = &PC },
	{  6, 2, RW, .w.p = &IX },
	{ 14, 2, RW, .w.p = &IY },
	{ 20, 2, RB, .b.p = &A_ },
	{ 22, 2, RFA, .b.p = NULL },
	{  4, 3, RB, .b.p = &B_ },
	{  6, 3, RB, .b.p = &C_ },
	{ 12, 3, RB, .b.p = &D_ },
	{ 14, 3, RB, .b.p = &E_ },
	{ 20, 3, RB, .b.p = &H_ },
	{ 22, 3, RB, .b.p = &L_ },
	{  3, 4, RF, .f.c = 'S', .f.m = S_FLAG },
	{  4, 4, RF, .f.c = 'Z', .f.m = Z_FLAG },
	{  5, 4, RF, .f.c = 'H', .f.m = H_FLAG },
	{  6, 4, RF, .f.c = 'P', .f.m = P_FLAG },
	{  7, 4, RF, .f.c = 'N', .f.m = N_FLAG },
	{  8, 4, RF, .f.c = 'C', .f.m = C_FLAG },
	{ 13, 4, RI, .f.c = '1', .f.m = 1 },
	{ 14, 4, RI, .f.c = '2', .f.m = 2 },
	{ 20, 4, RB, .b.p = &I },
	{ 22, 4, RR, .b.p = NULL }
};
static const int num_regs_z80 = sizeof(regs_z80) / sizeof(reg_t);
#endif

#ifndef EXCLUDE_I8080
static const reg_t __not_in_flash("lcd_tables") regs_8080[] = {
	{  4, 0, RB, .b.p = &A },
	{ 13, 0, RB, .b.p = &B },
	{ 15, 0, RB, .b.p = &C },
	{  4, 1, RB, .b.p = &D },
	{  6, 1, RB, .b.p = &E },
	{ 13, 1, RB, .b.p = &H },
	{ 15, 1, RB, .b.p = &L },
	{  6, 2, RW, .w.p = &SP },
	{ 15, 2, RW, .w.p = &PC },
	{  3, 3, RF, .f.c = 'S', .f.m = S_FLAG },
	{  4, 3, RF, .f.c = 'Z', .f.m = Z_FLAG },
	{  5, 3, RF, .f.c = 'H', .f.m = H_FLAG },
	{  6, 3, RF, .f.c = 'P', .f.m = P_FLAG },
	{  7, 3, RF, .f.c = 'C', .f.m = C_FLAG },
	{ 15, 3, RI, .f.c = '1', .f.m = 3 }
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
	int i, j, n = 0, temp;
	uint16_t col, x;
	WORD w;
	const lbl_t *lp;
	const reg_t *rp = NULL;
	const draw_grid_t *gridp = NULL;
	static int cpu_type, counter;
	static draw_grid_t grid20, grid28;

	if (first || (cpu_type != cpu)) {
		/* if first call or new CPU type, draw static background */

		draw_setup_grid(&grid20, XOFF20, YOFF20, &font20, SPC20);
		draw_setup_grid(&grid28, XOFF28, YOFF28, &font28, SPC28);
		counter = LCD_REFRESH - 1;

		draw_clear(C_DKBLUE);

		cpu_type = cpu;
		/* use cpu_type in the following code, since cpu can change */
#ifndef EXCLUDE_Z80
		if (cpu_type == Z80) {
			gridp = &grid20;
			lp = lbls_z80;
			n = num_lbls_z80;

			/* draw vertical grid lines */
			draw_grid_vline(7, 0, 4, gridp, C_DKYELLOW);
			draw_grid_vline(10, 4, 1, gridp, C_DKYELLOW);
			draw_grid_vline(15, 0, 5, gridp, C_DKYELLOW);
			/* draw horizontal grid lines */
			for (i = 1; i <= 5; i++)
				draw_grid_hline(0, i, gridp->cols, gridp,
						C_DKYELLOW);
		}
#endif
#ifndef EXCLUDE_I8080
		if (cpu_type == I8080) {
			gridp = &grid28;
			lp = lbls_8080;
			n = num_lbls_8080;

			/* draw vertical grid line */
			draw_grid_vline(8, 0, 4, gridp, C_DKYELLOW);
			/* draw horizontal grid lines */
			for (i = 1; i <= 3; i++)
				draw_grid_hline(0, i, gridp->cols, gridp,
						C_DKYELLOW);
		}
#endif
		/* draw labels */
		for (i = 0; i < n; lp++, i++)
			draw_grid_char(lp->x, lp->y, lp->c, gridp, C_WHITE,
				       C_DKBLUE);
		/* draw info line (font20) */
		p = "Z80pack " USR_REL;
		for (i = 0; *p && i < 16; i++)
			draw_grid_char(i, 5, *p++, &grid20, C_ORANGE,
				       C_DKBLUE);
		draw_grid_char(19, 5, '.', &grid20, C_ORANGE, C_DKBLUE);
		draw_grid_char(22, 5, 'C', &grid20, C_ORANGE, C_DKBLUE);
	} else {
#ifndef EXCLUDE_Z80
		if (cpu_type == Z80) {
			gridp = &grid20;
			rp = regs_z80;
			n = num_regs_z80;
		}
#endif
#ifndef EXCLUDE_I8080
		if (cpu_type == I8080) {
			gridp = &grid28;
			rp = regs_8080;
			n = num_regs_8080;
		}
#endif
		/* draw register contents */
		for (i = 0; i < n; rp++, i++) {
			col = C_GREEN;
			switch (rp->type) {
			case RB: /* byte sized register */
				w = *(rp->b.p);
				j = 2;
				break;
			case RW: /* word sized register */
				w = *(rp->w.p);
				j = 4;
				break;
			case RF: /* flags */
				if (!(F & rp->f.m))
					col = C_RED;
				draw_grid_char(rp->x, rp->y, rp->f.c, gridp,
					       col, C_DKBLUE);
				continue;
			case RI: /* interrupt register */
				if ((IFF & rp->f.m) != rp->f.m)
					col = C_RED;
				draw_grid_char(rp->x, rp->y, rp->f.c, gridp,
					       col, C_DKBLUE);
				continue;
#ifndef EXCLUDE_Z80
			case RFA: /* alternate flags (int) */
				w = F_;
				j = 2;
				break;
			case RR: /* refresh register */
				w = (R_ & 0x80) | (R & 0x7f);
				j = 2;
				break;
#endif
			default:
				continue;
			}
			x = rp->x;
			while (j--) {
				c = w & 0xf;
				c += c < 10 ? '0' : 'A' - 10;
				draw_grid_char(x--, rp->y, c, gridp, col,
					       C_DKBLUE);
				w >>= 4;
			}
		}
		/* update temperature every second */
		if (++counter >= LCD_REFRESH) {
			counter = 0;
			temp = (int) (read_onboard_temp() * 100.0f + 0.5f);
			for (i = 0; i < 5; i++) {
				draw_grid_char(21 - i, 5, '0' + temp % 10,
					       &grid20, C_ORANGE, C_DKBLUE);
				if (i < 4)
					temp /= 10;
				if (i == 1)
					i++; /* skip decimal point */
			}
		}
#ifndef EXCLUDE_I8080
		if (cpu_type == I8080) {
			/* info occupies the same space, so it draw here */
			draw_grid_hline(0, 4, gridp->cols, gridp, C_DKYELLOW);
		}
#endif
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

#define LX(x)	(PXOFF + PLEDXO + PLEDBS * ((x) / 8) + PLEDHO * (x))
#define LY(y)	(PYOFF + PLEDYO + PLEDVO * (y))

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
	{ LX( 0), LY(0), 'P', '7', LB, .b.i = 0xff, .b.m = 0x80, .b.p = &fp_led_output },
	{ LX( 1), LY(0), 'P', '6', LB, .b.i = 0xff, .b.m = 0x40, .b.p = &fp_led_output },
	{ LX( 2), LY(0), 'P', '5', LB, .b.i = 0xff, .b.m = 0x20, .b.p = &fp_led_output },
	{ LX( 3), LY(0), 'P', '4', LB, .b.i = 0xff, .b.m = 0x10, .b.p = &fp_led_output },
	{ LX( 4), LY(0), 'P', '3', LB, .b.i = 0xff, .b.m = 0x08, .b.p = &fp_led_output },
	{ LX( 5), LY(0), 'P', '2', LB, .b.i = 0xff, .b.m = 0x04, .b.p = &fp_led_output },
	{ LX( 6), LY(0), 'P', '1', LB, .b.i = 0xff, .b.m = 0x02, .b.p = &fp_led_output },
	{ LX( 7), LY(0), 'P', '0', LB, .b.i = 0xff, .b.m = 0x01, .b.p = &fp_led_output },
	{ LX(12), LY(0), 'I', 'E', LB, .b.i = 0x00, .b.m = 0x01, .b.p = &IFF },
	{ LX(13), LY(0), 'R', 'U', LB, .b.i = 0x00, .b.m = 0x01, .b.p = &cpu_state },
	{ LX(14), LY(0), 'W', 'A', LB, .b.i = 0x00, .b.m = 0x01, .b.p = &fp_led_wait },
	{ LX(15), LY(0), 'H', 'O', LB, .b.i = 0x00, .b.m = 0x01, .b.p = &bus_request },
	{ LX( 0), LY(1), 'M', 'R', LB, .b.i = 0x00, .b.m = 0x80, .b.p = &cpu_bus },
	{ LX( 1), LY(1), 'I', 'P', LB, .b.i = 0x00, .b.m = 0x40, .b.p = &cpu_bus },
	{ LX( 2), LY(1), 'M', '1', LB, .b.i = 0x00, .b.m = 0x20, .b.p = &cpu_bus },
	{ LX( 3), LY(1), 'O', 'P', LB, .b.i = 0x00, .b.m = 0x10, .b.p = &cpu_bus },
	{ LX( 4), LY(1), 'H', 'A', LB, .b.i = 0x00, .b.m = 0x08, .b.p = &cpu_bus },
	{ LX( 5), LY(1), 'S', 'T', LB, .b.i = 0x00, .b.m = 0x04, .b.p = &cpu_bus },
	{ LX( 6), LY(1), 'W', 'O', LB, .b.i = 0x00, .b.m = 0x02, .b.p = &cpu_bus },
	{ LX( 7), LY(1), 'I', 'A', LB, .b.i = 0x00, .b.m = 0x01, .b.p = &cpu_bus },
	{ LX( 8), LY(1), 'D', '7', LB, .b.i = 0x00, .b.m = 0x80, .b.p = &fp_led_data },
	{ LX( 9), LY(1), 'D', '6', LB, .b.i = 0x00, .b.m = 0x40, .b.p = &fp_led_data },
	{ LX(10), LY(1), 'D', '5', LB, .b.i = 0x00, .b.m = 0x20, .b.p = &fp_led_data },
	{ LX(11), LY(1), 'D', '4', LB, .b.i = 0x00, .b.m = 0x10, .b.p = &fp_led_data },
	{ LX(12), LY(1), 'D', '3', LB, .b.i = 0x00, .b.m = 0x08, .b.p = &fp_led_data },
	{ LX(13), LY(1), 'D', '2', LB, .b.i = 0x00, .b.m = 0x04, .b.p = &fp_led_data },
	{ LX(14), LY(1), 'D', '1', LB, .b.i = 0x00, .b.m = 0x02, .b.p = &fp_led_data },
	{ LX(15), LY(1), 'D', '0', LB, .b.i = 0x00, .b.m = 0x01, .b.p = &fp_led_data },
	{ LX( 0), LY(2), '1', '5', LW, .w.m = 0x8000, .w.p = &fp_led_address },
	{ LX( 1), LY(2), '1', '4', LW, .w.m = 0x4000, .w.p = &fp_led_address },
	{ LX( 2), LY(2), '1', '3', LW, .w.m = 0x2000, .w.p = &fp_led_address },
	{ LX( 3), LY(2), '1', '2', LW, .w.m = 0x1000, .w.p = &fp_led_address },
	{ LX( 4), LY(2), '1', '1', LW, .w.m = 0x0800, .w.p = &fp_led_address },
	{ LX( 5), LY(2), '1', '0', LW, .w.m = 0x0400, .w.p = &fp_led_address },
	{ LX( 6), LY(2), 'A', '9', LW, .w.m = 0x0200, .w.p = &fp_led_address },
	{ LX( 7), LY(2), 'A', '8', LW, .w.m = 0x0100, .w.p = &fp_led_address },
	{ LX( 8), LY(2), 'A', '7', LW, .w.m = 0x0080, .w.p = &fp_led_address },
	{ LX( 9), LY(2), 'A', '6', LW, .w.m = 0x0040, .w.p = &fp_led_address },
	{ LX(10), LY(2), 'A', '5', LW, .w.m = 0x0020, .w.p = &fp_led_address },
	{ LX(11), LY(2), 'A', '4', LW, .w.m = 0x0010, .w.p = &fp_led_address },
	{ LX(12), LY(2), 'A', '3', LW, .w.m = 0x0008, .w.p = &fp_led_address },
	{ LX(13), LY(2), 'A', '2', LW, .w.m = 0x0004, .w.p = &fp_led_address },
	{ LX(14), LY(2), 'A', '1', LW, .w.m = 0x0002, .w.p = &fp_led_address },
	{ LX(15), LY(2), 'A', '0', LW, .w.m = 0x0001, .w.p = &fp_led_address }
};
static const int num_leds = sizeof(leds) / sizeof(led_t);

/*
 *	Draw a 10x10 LED circular bracket.
 */
static inline void __not_in_flash_func(draw_led_bracket)(uint16_t x,
							 uint16_t y)
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
static inline void __not_in_flash_func(draw_led)(uint16_t x, uint16_t y,
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
	const led_t *p;
	const char *model = "Z80pack " MODEL " " USR_REL;
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
			draw_led_bracket(p->x, p->y);
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
			draw_led(p->x, p->y, bit);
			p++;
		}
	}
}

#endif /* SIMPLEPANEL */
