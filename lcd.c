/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#include <stdio.h>
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/divider.h"
#include "lcd.h"
#include "sim.h"
#include "simglb.h"

/* memory image for drawing */
#if LCD_COLOR_DEPTH == 12
static uint8_t img[LCD_1IN14_V2_HEIGHT * ((LCD_1IN14_V2_WIDTH + 1) / 2) * 3];
#else
static uint16_t img[LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH];
#endif

static void (*volatile lcd_draw_func)(int);
static volatile int lcd_task_busy;

static char info_line[16];	/* last line in CPU display */

static void lcd_draw_cpu_reg(int);
static void lcd_task(void);

void lcd_init(void)
{
	/* initialize LCD device */
	/* set orientation, x = 240 width and y = 135 height */
	LCD_1IN14_V2_Init(HORIZONTAL);

	/* use this image memory */
	Paint_NewImage((uint8_t *) &img[0], LCD_1IN14_V2.WIDTH,
		       LCD_1IN14_V2.HEIGHT, 0, WHITE);

	/* with this depth */
	Paint_SetDepth(LCD_COLOR_DEPTH);

	/* 0,0 is upper left corner */
	Paint_SetRotate(ROTATE_0);

	/* initialize here to not waste time in draw function */
	snprintf(info_line, sizeof(info_line), "Z80pack %s", USR_REL);

	/* launch LCD draw & refresh task */
	multicore_launch_core1(lcd_task);
}

void lcd_exit(void)
{
	/* tell LCD refresh task to stop drawing */
	lcd_set_draw_func(NULL);
	/* wait until it stopped */
	while (lcd_task_busy)
		sleep_ms(20);

	/* kill LCD refresh task and reset core 1 */
	multicore_reset_core1();

	LCD_1IN14_V2_Exit();
}

void lcd_default_draw_func(void)
{
	lcd_set_draw_func(lcd_draw_cpu_reg);
}

void lcd_set_draw_func(void (*draw_func)(int))
{
	lcd_draw_func = draw_func;
}

void lcd_brightness(int brightness)
{
	LCD_1IN14_V2_SetBacklight((uint8_t) brightness);
}

/*
 *	CPU status displays:
 *
 *	Z80 CPU using Font20 (10 x 20 pixels)
 *	-------------------------------------
 *
 *	  012345678901234567890123
 *	0 A  12   BC 1234 DE 1234
 *	1 HL 1234 SP 1234 PC 1234
 *	2 IX 1234 IY 1234 AF`1234
 *	3 BC'1234 DE'1234 HL`1234
 *	4 F  SZHPNC  IF12 IR 1234
 *	5 info_line        xx.xxC
 *
 *	8080 CPU using Font28 (14 x 28 pixels)
 *	--------------------------------------
 *
 *	  01234567890123456
 *	0 A  12    BC 1234
 *	1 DE 1234  HL 1234
 *	2 SP 1234  PC 1234
 *	3 F  SZHPC    IF 1
 *	4 info_line (Font 20)
 */

#if LCD_COLOR_DEPTH == 12
#define DKBLUE		0x0007	/* color used for background */
#define DKYELLOW	0x0880	/* color used for grid lines */
#else
#define DKBLUE		0x000f	/* color used for background */
#define DKYELLOW	0x8400	/* color used for grid lines */
#endif

#define OFFX20	5	/* x pixel offset of text coordinate grid for Font20 */
#define OFFY20	0	/* y pixel offset of text coordinate grid for Font20 */
#define SPC20	3	/* vertical spacing for Font20 */

#define OFFX28	8	/* x pixel offset of text coordinate grid for Font28 */
#define OFFY28	0	/* y pixel offset of text coordinate grid for Font28 */
#define SPC28	1	/* vertical spacing for Font28 */

/*
 * Draw character "c" at text coordinates "x, y" with color "col"
 */
static inline void cpu_char(uint16_t x, uint16_t y, const char c,
			    uint16_t col, const sFONT *font,
			    uint16_t offx, uint16_t offy, uint16_t spc)
{
	Paint_FastChar(x * font->Width + offx,
		       y * (font->Height + spc) + offy,
		       c, font, col, DKBLUE);
}

static inline void cpu_char20(uint16_t x, uint16_t y, const char c,
			      uint16_t col)
{
	cpu_char(x, y, c, col, &Font20, OFFX20, OFFY20, SPC20);
}

static inline void cpu_char28(uint16_t x, uint16_t y, const char c,
			      uint16_t col)
{
	cpu_char(x, y, c, col, &Font28, OFFX28, OFFY28, SPC28);
}

/*
 * Draw horizontal grid line in the middle of vertical spacing below
 * text row "y" with color "col" and vertical adjustment "adj"
 */
static inline void cpu_gridh(uint16_t y, uint16_t adj, uint16_t col,
			     const sFONT *font, uint16_t offy, uint16_t spc)
{
	Paint_FastHLine(0,
			(y + 1) * (font->Height + spc)
				- (spc + 1) / 2 + offy - adj,
			Paint.Width,
			col);
}

static inline void cpu_gridh20(uint16_t y, uint16_t adj, uint16_t col)
{
	cpu_gridh(y, adj, col, &Font20, OFFY20, SPC20);
}

static inline void cpu_gridh28(uint16_t y, uint16_t adj, uint16_t col)
{
	cpu_gridh(y, adj, col, &Font28, OFFY28, SPC28);
}

/*
 * Draw vertical grid line in the middle of text column "x" with
 * color "col" from the top of the screen to the middle of vertical
 * spacing below text row "y" with vertical adjustment "adj"
 */
static inline void cpu_gridv(uint16_t x, uint16_t y, uint16_t adj,
			     uint16_t col, const sFONT *font,
			     uint16_t offx, uint16_t offy, uint16_t spc)
{
	Paint_FastVLine(x * font->Width + font->Width / 2 + offx,
			0,
			(y + 1) * (font->Height + spc)
				- (spc + 1) / 2 + offy - adj,
			col);
}

static inline void cpu_gridv20(uint16_t x, uint16_t y, uint16_t adj,
			       uint16_t col) {
	cpu_gridv(x, y, adj, col, &Font20, OFFX20, OFFY20, SPC20);
}

static inline void cpu_gridv28(uint16_t x, uint16_t y, uint16_t adj,
			       uint16_t col) {
	cpu_gridv(x, y, adj, col, &Font28, OFFX28, OFFY28, SPC28);
}

/*
 * Draw short vertical grid line in the middle of text column "x" with
 * color "col" from the middle of vertical spacing above text row "y0"
 * to the middle of vertical spacing below text row "y1" with vertical
 * adjustment "adj"
 */
static inline void cpu_gridvs(uint16_t x, uint16_t y0, uint16_t y1,
			      uint16_t adj, uint16_t col, const sFONT *font,
			      uint16_t offx, uint16_t offy, uint16_t spc)
{
	Paint_FastVLine(x * font->Width + font->Width / 2 + offx,
			y0 * (font->Height + spc)
				- (spc + 1) / 2 + offy,
			(y1 - y0 + 1) * (font->Height + spc) - adj,
			col);
}

static inline void cpu_gridvs20(uint16_t x, uint16_t y0, uint16_t y1,
				uint16_t adj, uint16_t col)
{
	cpu_gridvs(x, y0, y1, adj, col, &Font20, OFFX20, OFFY20, SPC20);
}

static inline void cpu_gridvs28(uint16_t x, uint16_t y0, uint16_t y1,
				uint16_t adj, uint16_t col)
{
	cpu_gridvs(x, y0, y1, adj, col, &Font28, OFFX28, OFFY28, SPC28);
}

static const char *__not_in_flash("hex_table") hex = "0123456789ABCDEF";

static inline char hex3(uint16_t x) { return hex[(x >> 12) & 0xf]; }
static inline char hex2(uint16_t x) { return hex[(x >> 8) & 0xf]; }
static inline char hex1(uint16_t x) { return hex[(x >> 4) & 0xf]; }
static inline char hex0(uint16_t x) { return hex[x & 0xf]; }

float __not_in_flash_func(read_onboard_temp)(void)
{
	/* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
	const float conversionFactor = 3.3f / (1 << 12);

	float adc = (float) adc_read() * conversionFactor;
	float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

	return tempC;
}

static void __not_in_flash_func(lcd_draw_cpu_reg)(int first_flag)
{
	BYTE r;
	char *p;
	int temp;
	divmod_result_t res;
	static int cpu_type, count;

	if (first_flag || (cpu_type != cpu)) {
		/* if first call or new CPU type, draw static background */
		Paint_Clear(DKBLUE);

		cpu_type = cpu;
		if (cpu_type == Z80) {
			/* draw vertical grid lines */
			cpu_gridv20(7, 3, 0, DKYELLOW);
			cpu_gridvs20(10, 4, 4, 0, DKYELLOW);
			cpu_gridv20(15, 4, 0, DKYELLOW);

			/* 012345678901234567890123 */
			/* A       BC      DE       */
			cpu_char20( 0, 0, 'A', WHITE);
			cpu_char20( 8, 0, 'B', WHITE);
			cpu_char20( 9, 0, 'C', WHITE);
			cpu_char20(16, 0, 'D', WHITE);
			cpu_char20(17, 0, 'E', WHITE);

			cpu_gridh20(0, 0, DKYELLOW);

			/* HL      SP      PC       */
			cpu_char20( 0, 1, 'H', WHITE);
			cpu_char20( 1, 1, 'L', WHITE);
			cpu_char20( 8, 1, 'S', WHITE);
			cpu_char20( 9, 1, 'P', WHITE);
			cpu_char20(16, 1, 'P', WHITE);
			cpu_char20(17, 1, 'C', WHITE);

			cpu_gridh20(1, 0, DKYELLOW);

			/* IX      IY      AF'      */
			cpu_char20( 0, 2, 'I', WHITE);
			cpu_char20( 1, 2, 'X', WHITE);
			cpu_char20( 8, 2, 'I', WHITE);
			cpu_char20( 9, 2, 'Y', WHITE);
			cpu_char20(16, 2, 'A', WHITE);
			cpu_char20(17, 2, 'F', WHITE);
			cpu_char20(18, 2, '\'', WHITE);

			cpu_gridh20(2, 0, DKYELLOW);

			/* BC'     DE'     HL'      */
			cpu_char20( 0, 3, 'B', WHITE);
			cpu_char20( 1, 3, 'C', WHITE);
			cpu_char20( 2, 3, '\'', WHITE);
			cpu_char20( 8, 3, 'D', WHITE);
			cpu_char20( 9, 3, 'E', WHITE);
			cpu_char20(10, 3, '\'', WHITE);
			cpu_char20(16, 3, 'H', WHITE);
			cpu_char20(17, 3, 'L', WHITE);
			cpu_char20(18, 3, '\'', WHITE);

			cpu_gridh20(3, 0, DKYELLOW);

			/* F          IF   IR       */
			cpu_char20( 0, 4, 'F', WHITE);
			cpu_char20(11, 4, 'I', WHITE);
			cpu_char20(12, 4, 'F', WHITE);
			cpu_char20(16, 4, 'I', WHITE);
			cpu_char20(17, 4, 'R', WHITE);

			cpu_gridh20(4, 0, DKYELLOW);
		} else {
			/* adjustment keeps vertical grid line outside
			   "info_line" */
			cpu_gridv28(8, 3, 2, DKYELLOW);

			/* 01234567890123456 */
			/* A        BC       */
			cpu_char28( 0, 0, 'A', WHITE);
			cpu_char28( 9, 0, 'B', WHITE);
			cpu_char28(10, 0, 'C', WHITE);

			cpu_gridh28(0, 0, DKYELLOW);

			/* DE       HL       */
			cpu_char28( 0, 1, 'D', WHITE);
			cpu_char28( 1, 1, 'E', WHITE);
			cpu_char28( 9, 1, 'H', WHITE);
			cpu_char28(10, 1, 'L', WHITE);

			cpu_gridh28(1, 0, DKYELLOW);

			/* SP       PC       */
			cpu_char28( 0, 2, 'S', WHITE);
			cpu_char28( 1, 2, 'P', WHITE);
			cpu_char28( 9, 2, 'P', WHITE);
			cpu_char28(10, 2, 'C', WHITE);

			cpu_gridh28(2, 0, DKYELLOW);

			/* F           IF    */
			cpu_char28( 0, 3, 'F', WHITE);
			cpu_char28(12, 3, 'I', WHITE);
			cpu_char28(13, 3, 'F', WHITE);
		}
		/* info-line          .  C  */
		for (p = info_line; *p; p++)
			cpu_char20(p - info_line, 5, *p, BRRED);
		cpu_char20(19, 5, '.', BRRED);
		cpu_char20(22, 5, 'C', BRRED);
	} else {
		if (cpu_type == Z80) {
			/* 012345678901234567890123 */
			/*    aa      bbcc    ddee  */
			cpu_char20( 3, 0, hex1(A), GREEN);
			cpu_char20( 4, 0, hex0(A), GREEN);
			cpu_char20(11, 0, hex1(B), GREEN);
			cpu_char20(12, 0, hex0(B), GREEN);
			cpu_char20(13, 0, hex1(C), GREEN);
			cpu_char20(14, 0, hex0(C), GREEN);
			cpu_char20(19, 0, hex1(D), GREEN);
			cpu_char20(20, 0, hex0(D), GREEN);
			cpu_char20(21, 0, hex1(E), GREEN);
			cpu_char20(22, 0, hex0(E), GREEN);

			/*    hhll    shsl    phpl  */
			cpu_char20( 3, 1, hex1(H), GREEN);
			cpu_char20( 4, 1, hex0(H), GREEN);
			cpu_char20( 5, 1, hex1(L), GREEN);
			cpu_char20( 6, 1, hex0(L), GREEN);
			cpu_char20(11, 1, hex3(SP), GREEN);
			cpu_char20(12, 1, hex2(SP), GREEN);
			cpu_char20(13, 1, hex1(SP), GREEN);
			cpu_char20(14, 1, hex0(SP), GREEN);
			cpu_char20(19, 1, hex3(PC), GREEN);
			cpu_char20(20, 1, hex2(PC), GREEN);
			cpu_char20(21, 1, hex1(PC), GREEN);
			cpu_char20(22, 1, hex0(PC), GREEN);

			/*    xhxl    yhyl    a'f'  */
			cpu_char20( 3, 2, hex3(IX), GREEN);
			cpu_char20( 4, 2, hex2(IX), GREEN);
			cpu_char20( 5, 2, hex1(IX), GREEN);
			cpu_char20( 6, 2, hex0(IX), GREEN);
			cpu_char20(11, 2, hex3(IY), GREEN);
			cpu_char20(12, 2, hex2(IY), GREEN);
			cpu_char20(13, 2, hex1(IY), GREEN);
			cpu_char20(14, 2, hex0(IY), GREEN);
			cpu_char20(19, 2, hex1(A_), GREEN);
			cpu_char20(20, 2, hex0(A_), GREEN);
			cpu_char20(21, 2, hex1(F_), GREEN);
			cpu_char20(22, 2, hex0(F_), GREEN);

			/*    b'c'    d'e'    h'l'  */
			cpu_char20( 3, 3, hex1(B_), GREEN);
			cpu_char20( 4, 3, hex0(B_), GREEN);
			cpu_char20( 5, 3, hex1(C_), GREEN);
			cpu_char20( 6, 3, hex0(C_), GREEN);
			cpu_char20(11, 3, hex1(D_), GREEN);
			cpu_char20(12, 3, hex0(D_), GREEN);
			cpu_char20(13, 3, hex1(E_), GREEN);
			cpu_char20(14, 3, hex0(E_), GREEN);
			cpu_char20(19, 3, hex1(H_), GREEN);
			cpu_char20(20, 3, hex0(H_), GREEN);
			cpu_char20(21, 3, hex1(L_), GREEN);
			cpu_char20(22, 3, hex0(L_), GREEN);

			/*    szhpnc    if    iirr  */
			cpu_char20( 3, 4, 'S', F & S_FLAG ? GREEN : RED);
			cpu_char20( 4, 4, 'Z', F & Z_FLAG ? GREEN : RED);
			cpu_char20( 5, 4, 'H', F & H_FLAG ? GREEN : RED);
			cpu_char20( 6, 4, 'P', F & P_FLAG ? GREEN : RED);
			cpu_char20( 7, 4, 'N', F & N_FLAG ? GREEN : RED);
			cpu_char20( 8, 4, 'C', F & C_FLAG ? GREEN : RED);
			cpu_char20(13, 4, '1', IFF & 1 ? GREEN : RED);
			cpu_char20(14, 4, '2', IFF & 2 ? GREEN : RED);
			cpu_char20(19, 4, hex1(I), GREEN);
			cpu_char20(20, 4, hex0(I), GREEN);
			r = (R_ & 0x80) | (R & 0x7f);
			cpu_char20(21, 4, hex1(r), GREEN);
			cpu_char20(22, 4, hex0(r), GREEN);
		} else {
			/* 01234567890123456 */
			/*    aa       bbcc  */
			cpu_char28( 3, 0, hex1(A), GREEN);
			cpu_char28( 4, 0, hex0(A), GREEN);
			cpu_char28(12, 0, hex1(B), GREEN);
			cpu_char28(13, 0, hex0(B), GREEN);
			cpu_char28(14, 0, hex1(C), GREEN);
			cpu_char28(15, 0, hex0(C), GREEN);

			/*    ddee     hhll  */
			cpu_char28( 3, 1, hex1(D), GREEN);
			cpu_char28( 4, 1, hex0(D), GREEN);
			cpu_char28( 5, 1, hex1(E), GREEN);
			cpu_char28( 6, 1, hex0(E), GREEN);
			cpu_char28(12, 1, hex1(H), GREEN);
			cpu_char28(13, 1, hex0(H), GREEN);
			cpu_char28(14, 1, hex1(L), GREEN);
			cpu_char28(15, 1, hex0(L), GREEN);

			/*    shsl     phpl  */
			cpu_char28( 3, 2, hex3(SP), GREEN);
			cpu_char28( 4, 2, hex2(SP), GREEN);
			cpu_char28( 5, 2, hex1(SP), GREEN);
			cpu_char28( 6, 2, hex0(SP), GREEN);
			cpu_char28(12, 2, hex3(PC), GREEN);
			cpu_char28(13, 2, hex2(PC), GREEN);
			cpu_char28(14, 2, hex1(PC), GREEN);
			cpu_char28(15, 2, hex0(PC), GREEN);

			/*    szhpc       i  */
			cpu_char28( 3, 3, 'S', F & S_FLAG ? GREEN : RED);
			cpu_char28( 4, 3, 'Z', F & Z_FLAG ? GREEN : RED);
			cpu_char28( 5, 3, 'H', F & H_FLAG ? GREEN : RED);
			cpu_char28( 6, 3, 'P', F & P_FLAG ? GREEN : RED);
			cpu_char28( 7, 3, 'C', F & C_FLAG ? GREEN : RED);
			cpu_char28(15, 3, '1', IFF == 3 ? GREEN : RED);

			/*
			 * The adjustment moves the grid line from inside the
			 * "info_line" into the descenders space of the
			 * "F IF" line, therefore the need to draw it inside
			 * the refresh code.
			 */
			cpu_gridh28(3, 2, DKYELLOW);
		}
		/* update temperature every second */
		if (++count == 30) {
			/*                  xx xx   */
			count = 0;
			temp = (int) (read_onboard_temp() * 100.0f + 0.5f);
			res = hw_divider_divmod_u32(temp, 10);
			cpu_char20(21, 5, '0' + to_remainder_u32(res), BRRED);
			res = hw_divider_divmod_u32(to_quotient_u32(res), 10);
			cpu_char20(20, 5, '0' + to_remainder_u32(res), BRRED);
			res = hw_divider_divmod_u32(to_quotient_u32(res), 10);
			cpu_char20(18, 5, '0' + to_remainder_u32(res), BRRED);
			res = hw_divider_divmod_u32(to_quotient_u32(res), 10);
			cpu_char20(17, 5, '0' + to_remainder_u32(res), BRRED);
		}
	}
}

static inline void lcd_refresh(void)
{
#if LCD_COLOR_DEPTH == 12
	LCD_1IN14_V2_Display12(&img[0]);
#else
	LCD_1IN14_V2_Display(&img[0]);
#endif
}

static void __not_in_flash_func(lcd_task)(void)
{
	absolute_time_t t;
	int64_t d;
	static void (*curr_func)(int);

	while (1) {
		/* loops every LCD_REFRESH_US */

		t = get_absolute_time();

		if (curr_func != lcd_draw_func) {
			curr_func = lcd_draw_func;
			/* new draw function */
			if (curr_func == NULL) {
				/* new function is NULL, clear screen,
				   and refresh one last time */
				Paint_Clear(BLACK);
				lcd_refresh();
				lcd_task_busy = 0;
			} else {
				/* new function, call with parameter 1 to
				   mark the first call */
				lcd_task_busy = 1;
				(*curr_func)(1);
				lcd_refresh();
			}
		} else if (curr_func != NULL) {
			(*curr_func)(0);
			lcd_refresh();
		}

		d = absolute_time_diff_us(t, get_absolute_time());
		if (d < LCD_REFRESH_US)
			sleep_us(LCD_REFRESH_US - d);
		else
			puts("REFRESH!");
	}
}
