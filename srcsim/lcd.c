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
#include "hardware/divider.h"

#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simmem.h"

#include "lcd.h"

/* memory image for drawing */
#if LCD_COLOR_DEPTH == 12
static uint8_t img[LCD_1IN14_V2_HEIGHT * ((LCD_1IN14_V2_WIDTH + 1) / 2) * 3];
#else
static uint16_t img[LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH];
#endif

static mutex_t lcd_mutex;
static volatile lcd_func_t lcd_draw_func;
static volatile lcd_func_t lcd_status_func;
static volatile int lcd_shows_status;
static volatile int lcd_task_busy;

static char info_line[16];	/* last line in CPU display */

static void lcd_draw_cpu_reg(int first_flag);
static void lcd_draw_memory(int first_flag);
static void lcd_draw_panel(int first_flag);
static void lcd_task(void);

void lcd_init(void)
{
	/* initialize LCD device */
	/* set orientation, x = 240 width and y = 135 height */
	LCD_1IN14_V2_Init(LCD_HORIZONTAL);

	/* use this image memory */
	Paint_NewImage((uint8_t *) &img[0], LCD_1IN14_V2.WIDTH,
		       LCD_1IN14_V2.HEIGHT, 0, WHITE);

	/* with this depth */
	Paint_SetDepth(LCD_COLOR_DEPTH);

	/* 0,0 is upper left corner */
	Paint_SetRotate(ROTATE_0);

	/* initialize here to not waste time in draw function */
	snprintf(info_line, sizeof(info_line), "Z80pack %s", USR_REL);

	mutex_init(&lcd_mutex);

	lcd_status_func = lcd_draw_cpu_reg;

	/* launch LCD draw & refresh task */
	multicore_launch_core1(lcd_task);
}

void lcd_exit(void)
{
	/* tell LCD refresh task to stop drawing */
	lcd_custom_disp(NULL);
	/* wait until it stopped */
	while (lcd_task_busy)
		sleep_ms(20);

	/* kill LCD refresh task and reset core 1 */
	multicore_reset_core1();

	LCD_1IN14_V2_Exit();
}

void lcd_brightness(int brightness)
{
	mutex_enter_blocking(&lcd_mutex);
	LCD_1IN14_V2_SetBacklight((uint8_t) brightness);
	mutex_exit(&lcd_mutex);
}

void lcd_set_rotated(int rotated)
{
	mutex_enter_blocking(&lcd_mutex);
	LCD_1IN14_V2_SetRotated(rotated);
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
 *	4 info_line xx.xxC (Font 20)
 */

#if LCD_COLOR_DEPTH == 12
#define DKBLUE		0x0007	/* color used for background */
#define DKYELLOW	0x0880	/* color used for grid lines */
#else
#define DKBLUE		0x000f	/* color used for background */
#define DKYELLOW	0x8400	/* color used for grid lines */
#endif

#define XOFF20	5	/* x pixel offset of text coordinate grid for Font20 */
#define YOFF20	0	/* y pixel offset of text coordinate grid for Font20 */
#define SPC20	3	/* vertical spacing for Font20 */

#define XOFF28	8	/* x pixel offset of text coordinate grid for Font28 */
#define YOFF28	0	/* y pixel offset of text coordinate grid for Font28 */
#define SPC28	1	/* vertical spacing for Font28 */

#ifndef __noclone
#define __noclone	__attribute__((__noclone__))
#endif

/*
 * Draw character "c" at text coordinates "x, y" with color "col"
 */
static void __noclone __no_inline_not_in_flash_func(cpu_char)
	(uint16_t x, uint16_t y, const char c, uint16_t col, const sFONT *font,
	 uint16_t offx, uint16_t offy, uint16_t spc)
{
	Paint_FastChar(x * font->Width + offx,
		       y * (font->Height + spc) + offy,
		       c, font, col, DKBLUE);
}

static inline void cpu_char20(uint16_t x, uint16_t y, const char c,
			      uint16_t col)
{
	cpu_char(x, y, c, col, &Font20, XOFF20, YOFF20, SPC20);
}

static inline void cpu_char28(uint16_t x, uint16_t y, const char c,
			      uint16_t col)
{
	cpu_char(x, y, c, col, &Font28, XOFF28, YOFF28, SPC28);
}

/*
 * Draw horizontal grid line in the middle of vertical spacing below
 * text row "y" with color "col" and vertical adjustment "adj"
 */
static void __noclone __no_inline_not_in_flash_func(cpu_gridh)
	(uint16_t y, uint16_t adj, uint16_t col, const sFONT *font,
	 uint16_t offy, uint16_t spc)
{
	Paint_FastHLine(0, (y + 1) * (font->Height + spc) -
			(spc + 1) / 2 + offy - adj,
			Paint.Width, col);
}

static inline void cpu_gridh20(uint16_t y, uint16_t adj, uint16_t col)
{
	cpu_gridh(y, adj, col, &Font20, YOFF20, SPC20);
}

static inline void cpu_gridh28(uint16_t y, uint16_t adj, uint16_t col)
{
	cpu_gridh(y, adj, col, &Font28, YOFF28, SPC28);
}

/*
 * Draw vertical grid line in the middle of text column "x" with
 * color "col" from the top of the screen to the middle of vertical
 * spacing below text row "y" with vertical adjustment "adj"
 */
static void __noclone __no_inline_not_in_flash_func(cpu_gridv)
	(uint16_t x, uint16_t y, uint16_t adj, uint16_t col, const sFONT *font,
	 uint16_t offx, uint16_t offy, uint16_t spc)
{
	Paint_FastVLine(x * font->Width + font->Width / 2 + offx,
			0, (y + 1) * (font->Height + spc) -
			(spc + 1) / 2 + offy - adj, col);
}

static inline void cpu_gridv20(uint16_t x, uint16_t y, uint16_t adj,
			       uint16_t col) {
	cpu_gridv(x, y, adj, col, &Font20, XOFF20, YOFF20, SPC20);
}

static inline void cpu_gridv28(uint16_t x, uint16_t y, uint16_t adj,
			       uint16_t col) {
	cpu_gridv(x, y, adj, col, &Font28, XOFF28, YOFF28, SPC28);
}

/*
 * Draw short vertical grid line in the middle of text column "x" with
 * color "col" from the middle of vertical spacing above text row "y0"
 * to the middle of vertical spacing below text row "y1" with vertical
 * adjustment "adj"
 */
static void __noclone __no_inline_not_in_flash_func(cpu_gridvs)
	(uint16_t x, uint16_t y0, uint16_t y1, uint16_t adj, uint16_t col,
	 const sFONT *font, uint16_t offx, uint16_t offy, uint16_t spc)
{
	Paint_FastVLine(x * font->Width + font->Width / 2 + offx,
			y0 * (font->Height + spc) - (spc + 1) / 2 + offy,
			(y1 - y0 + 1) * (font->Height + spc) - adj, col);
}

static inline void cpu_gridvs20(uint16_t x, uint16_t y0, uint16_t y1,
				uint16_t adj, uint16_t col)
{
	cpu_gridvs(x, y0, y1, adj, col, &Font20, XOFF20, YOFF20, SPC20);
}

static inline void cpu_gridvs28(uint16_t x, uint16_t y0, uint16_t y1,
				uint16_t adj, uint16_t col)
{
	cpu_gridvs(x, y0, y1, adj, col, &Font28, XOFF28, YOFF28, SPC28);
}

static const char *__not_in_flash("hex_table") hex = "0123456789ABCDEF";

static inline char hex3(uint16_t x) { return hex[(x >> 12) & 0xf]; }
static inline char hex2(uint16_t x) { return hex[(x >> 8) & 0xf]; }
static inline char hex1(uint16_t x) { return hex[(x >> 4) & 0xf]; }
static inline char hex0(uint16_t x) { return hex[x & 0xf]; }

static inline float read_onboard_temp(void)
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
	static int cpu_type, counter;

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
		if (++counter == LCD_REFRESH) {
			/*                  xx xx   */
			counter = 0;
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

#define MEM_XOFF 3
#define MEM_YOFF 0
#define MEM_BRDR 3

static void __not_in_flash_func(lcd_draw_memory)(int first_flag)
{
	int x, y;
	uint32_t *p;

	if (first_flag) {
		Paint_Clear(DKBLUE);
		Paint_FastHLine(MEM_XOFF, MEM_YOFF,
				128 + 96 + 4 * MEM_BRDR - 1, GREEN);
		Paint_FastHLine(MEM_XOFF, MEM_YOFF + 128 + 2 * MEM_BRDR - 1,
				128 + 96 + 4 * MEM_BRDR - 1, GREEN);
		Paint_FastVLine(MEM_XOFF, MEM_YOFF,
				128 + 2 * MEM_BRDR, GREEN);
		Paint_FastVLine(MEM_XOFF + 128 + 2 * MEM_BRDR - 1, 0,
				128 + 2 * MEM_BRDR, GREEN);
		Paint_FastVLine(MEM_XOFF + 128 + 96 + 4 * MEM_BRDR - 2, 0,
				128 + 2 * MEM_BRDR, GREEN);
		return;
	} else {
		p = (uint32_t *) bnk0;
		for (x = MEM_XOFF + MEM_BRDR;
		     x < MEM_XOFF + MEM_BRDR + 128; x++) {
			for (y = MEM_YOFF + MEM_BRDR;
			     y < MEM_YOFF + MEM_BRDR + 128; y++) {
				/* constant = 2^32 / ((1 + sqrt(5)) / 2) */
				Paint_FastPixel(x, y,
						(*p++ * 2654435769U) >> 20);
			}
		}
		p = (uint32_t *) bnk1;
		for (x = MEM_XOFF + 3 * MEM_BRDR - 1 + 128;
		     x < MEM_XOFF + 3 * MEM_BRDR - 1 + 128 + 96; x++) {
			for (y = MEM_YOFF + MEM_BRDR;
			     y < MEM_YOFF + MEM_BRDR + 128; y++) {
				Paint_FastPixel(x, y,
						(*p++ * 2654435769U) >> 20);
			}
		}
	}
}

#ifdef SIMPLEPANEL

#define PXOFF	6				/* panel x offset */
#define PYOFF	6				/* panel y offset */

#define PFNTH	12				/* Font12 height */
#define PFNTW	6				/* Font12 width */
#define PFNTS	1				/* Font12 letter spacing */

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

static const struct led {
	uint16_t x;
	uint16_t y;
	char c1;
	char c2;
	enum { byte, word } type;
	union {
		struct {
			BYTE inv;
			BYTE mask;
			const BYTE *data;
		} b;
		struct {
			WORD mask;
			const WORD *data;
		} w;
	};
} leds[] = {
	{ .x = PLEDX( 0), .y = PLEDY(0), .c1 = 'P', .c2 = '7', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x80, .b.data = &fp_led_output },
	{ .x = PLEDX( 1), .y = PLEDY(0), .c1 = 'P', .c2 = '6', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x40, .b.data = &fp_led_output },
	{ .x = PLEDX( 2), .y = PLEDY(0), .c1 = 'P', .c2 = '5', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x20, .b.data = &fp_led_output },
	{ .x = PLEDX( 3), .y = PLEDY(0), .c1 = 'P', .c2 = '4', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x10, .b.data = &fp_led_output },
	{ .x = PLEDX( 4), .y = PLEDY(0), .c1 = 'P', .c2 = '3', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x08, .b.data = &fp_led_output },
	{ .x = PLEDX( 5), .y = PLEDY(0), .c1 = 'P', .c2 = '2', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x04, .b.data = &fp_led_output },
	{ .x = PLEDX( 6), .y = PLEDY(0), .c1 = 'P', .c2 = '1', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x02, .b.data = &fp_led_output },
	{ .x = PLEDX( 7), .y = PLEDY(0), .c1 = 'P', .c2 = '0', .type = byte,
	  .b.inv = 0xff, .b.mask = 0x01, .b.data = &fp_led_output },
	{ .x = PLEDX(12), .y = PLEDY(0), .c1 = 'I', .c2 = 'E', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x01, .b.data = &IFF },
	{ .x = PLEDX(13), .y = PLEDY(0), .c1 = 'R', .c2 = 'U', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x01, .b.data = &cpu_state },
	{ .x = PLEDX(14), .y = PLEDY(0), .c1 = 'W', .c2 = 'A', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x01, .b.data = &fp_led_wait },
	{ .x = PLEDX(15), .y = PLEDY(0), .c1 = 'H', .c2 = 'O', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x01, .b.data = &bus_request },
	{ .x = PLEDX( 0), .y = PLEDY(1), .c1 = 'M', .c2 = 'R', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x80, .b.data = &cpu_bus },
	{ .x = PLEDX( 1), .y = PLEDY(1), .c1 = 'I', .c2 = 'P', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x40, .b.data = &cpu_bus },
	{ .x = PLEDX( 2), .y = PLEDY(1), .c1 = 'M', .c2 = '1', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x20, .b.data = &cpu_bus },
	{ .x = PLEDX( 3), .y = PLEDY(1), .c1 = 'O', .c2 = 'P', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x10, .b.data = &cpu_bus },
	{ .x = PLEDX( 4), .y = PLEDY(1), .c1 = 'H', .c2 = 'A', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x08, .b.data = &cpu_bus },
	{ .x = PLEDX( 5), .y = PLEDY(1), .c1 = 'S', .c2 = 'T', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x04, .b.data = &cpu_bus },
	{ .x = PLEDX( 6), .y = PLEDY(1), .c1 = 'W', .c2 = 'O', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x02, .b.data = &cpu_bus },
	{ .x = PLEDX( 7), .y = PLEDY(1), .c1 = 'I', .c2 = 'A', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x01, .b.data = &cpu_bus },
	{ .x = PLEDX( 8), .y = PLEDY(1), .c1 = 'D', .c2 = '7', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x80, .b.data = &fp_led_data },
	{ .x = PLEDX( 9), .y = PLEDY(1), .c1 = 'D', .c2 = '6', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x40, .b.data = &fp_led_data },
	{ .x = PLEDX(10), .y = PLEDY(1), .c1 = 'D', .c2 = '5', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x20, .b.data = &fp_led_data },
	{ .x = PLEDX(11), .y = PLEDY(1), .c1 = 'D', .c2 = '4', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x10, .b.data = &fp_led_data },
	{ .x = PLEDX(12), .y = PLEDY(1), .c1 = 'D', .c2 = '3', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x08, .b.data = &fp_led_data },
	{ .x = PLEDX(13), .y = PLEDY(1), .c1 = 'D', .c2 = '2', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x04, .b.data = &fp_led_data },
	{ .x = PLEDX(14), .y = PLEDY(1), .c1 = 'D', .c2 = '1', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x02, .b.data = &fp_led_data },
	{ .x = PLEDX(15), .y = PLEDY(1), .c1 = 'D', .c2 = '0', .type = byte,
	  .b.inv = 0x00, .b.mask = 0x01, .b.data = &fp_led_data },
	{ .x = PLEDX( 0), .y = PLEDY(2), .c1 = '1', .c2 = '5', .type = word,
	  .w.mask = 0x8000, .w.data = &fp_led_address },
	{ .x = PLEDX( 1), .y = PLEDY(2), .c1 = '1', .c2 = '4', .type = word,
	  .w.mask = 0x4000, .w.data = &fp_led_address },
	{ .x = PLEDX( 2), .y = PLEDY(2), .c1 = '1', .c2 = '3', .type = word,
	  .w.mask = 0x2000, .w.data = &fp_led_address },
	{ .x = PLEDX( 3), .y = PLEDY(2), .c1 = '1', .c2 = '2', .type = word,
	  .w.mask = 0x1000, .w.data = &fp_led_address },
	{ .x = PLEDX( 4), .y = PLEDY(2), .c1 = '1', .c2 = '1', .type = word,
	  .w.mask = 0x0800, .w.data = &fp_led_address },
	{ .x = PLEDX( 5), .y = PLEDY(2), .c1 = '1', .c2 = '0', .type = word,
	  .w.mask = 0x0400, .w.data = &fp_led_address },
	{ .x = PLEDX( 6), .y = PLEDY(2), .c1 = 'A', .c2 = '9', .type = word,
	  .w.mask = 0x0200, .w.data = &fp_led_address },
	{ .x = PLEDX( 7), .y = PLEDY(2), .c1 = 'A', .c2 = '8', .type = word,
	  .w.mask = 0x0100, .w.data = &fp_led_address },
	{ .x = PLEDX( 8), .y = PLEDY(2), .c1 = 'A', .c2 = '7', .type = word,
	  .w.mask = 0x0080, .w.data = &fp_led_address },
	{ .x = PLEDX( 9), .y = PLEDY(2), .c1 = 'A', .c2 = '6', .type = word,
	  .w.mask = 0x0040, .w.data = &fp_led_address },
	{ .x = PLEDX(10), .y = PLEDY(2), .c1 = 'A', .c2 = '5', .type = word,
	  .w.mask = 0x0020, .w.data = &fp_led_address },
	{ .x = PLEDX(11), .y = PLEDY(2), .c1 = 'A', .c2 = '4', .type = word,
	  .w.mask = 0x0010, .w.data = &fp_led_address },
	{ .x = PLEDX(12), .y = PLEDY(2), .c1 = 'A', .c2 = '3', .type = word,
	  .w.mask = 0x0008, .w.data = &fp_led_address },
	{ .x = PLEDX(13), .y = PLEDY(2), .c1 = 'A', .c2 = '2', .type = word,
	  .w.mask = 0x0004, .w.data = &fp_led_address },
	{ .x = PLEDX(14), .y = PLEDY(2), .c1 = 'A', .c2 = '1', .type = word,
	  .w.mask = 0x0002, .w.data = &fp_led_address },
	{ .x = PLEDX(15), .y = PLEDY(2), .c1 = 'A', .c2 = '0', .type = word,
	  .w.mask = 0x0001, .w.data = &fp_led_address }
};
static const int num_leds = sizeof(leds) / sizeof(struct led);

/*
 *	draw a 10x10 LED circular socket
 */
static inline void __not_in_flash_func(lcd_draw_socket)(uint16_t x, uint16_t y)
{
	Paint_FastHLine(x + 2, y, 6, GRAY);
	Paint_FastPixel(x + 1, y + 1, GRAY);
	Paint_FastPixel(x + 8, y + 1, GRAY);
	Paint_FastVLine(x, y + 2, 6, GRAY);
	Paint_FastVLine(x + 9, y + 2, 6, GRAY);
	Paint_FastPixel(x + 1, y + 8, GRAY);
	Paint_FastPixel(x + 8, y + 8, GRAY);
	Paint_FastHLine(x + 2, y + 9, 6, GRAY);
}

#if LCD_COLOR_DEPTH == 12
#define DKRED 0x0500
#else
#define DKRED 0x5000
#endif

/*
 *	draw a LED inside a 10x10 circular socket
 */
static inline void __not_in_flash_func(lcd_draw_led)(uint16_t x, uint16_t y,
						     int on_off)
{
	uint16_t col = on_off ? RED : DKRED;

	Paint_FastHLine(x + 2, y + 1, 6, col);
	Paint_FastHLine(x + 1, y + 2, 8, col);
	Paint_FastHLine(x + 1, y + 3, 8, col);
	Paint_FastHLine(x + 1, y + 4, 8, col);
	Paint_FastHLine(x + 1, y + 5, 8, col);
	Paint_FastHLine(x + 1, y + 6, 8, col);
	Paint_FastHLine(x + 1, y + 7, 8, col);
	Paint_FastHLine(x + 2, y + 8, 6, col);
}

static void __not_in_flash_func(lcd_draw_panel)(int first_flag)
{
	const struct led *p;
	const char *model = "Z80pack RP2040-GEEK " USR_REL, *s;
	int i, bit;

	p = leds;
	if (first_flag) {
		Paint_Clear(DKBLUE);
		for (i = 0; i < num_leds; i++) {
			Paint_FastChar(p->x - PLEDXO, p->y - PLEDYO,
				       p->c1, &Font12, WHITE, DKBLUE);
			Paint_FastChar(p->x - PLEDXO + PFNTW, p->y - PLEDYO,
				       p->c2, &Font12, WHITE, DKBLUE);
			if (p->c1 == 'W' && p->c2 == 'O')
				Paint_FastHLine(p->x - PLEDXO,
						p->y - PLEDYO - 2,
						PLBLW, WHITE);
			lcd_draw_socket(p->x, p->y);
			p++;
		}
		i = (Paint.Width - strlen(model) * Font20.Width) / 2;
		for (s = model; *s; s++) {
			Paint_FastChar(i, Paint.Height - Font20.Height - 1, *s,
				       &Font20, BROWN, DKBLUE);
			i += Font20.Width;
		}
	} else {
		for (i = 0; i < num_leds; i++) {
			if (p->type == byte)
				bit = (*(p->b.data) ^ p->b.inv) & p->b.mask;
			else
				bit = *(p->w.data) & p->w.mask;
			lcd_draw_led(p->x, p->y, bit);
			p++;
		}
	}
}

#endif /* SIMPLEPANEL */

static void __not_in_flash_func(lcd_refresh)(void)
{
	mutex_enter_blocking(&lcd_mutex);
#if LCD_COLOR_DEPTH == 12
	LCD_1IN14_V2_Display12(&img[0]);
#else
	LCD_1IN14_V2_Display(&img[0]);
#endif
	mutex_exit(&lcd_mutex);
}

#define LCD_REFRESH_US (1000000 / LCD_REFRESH)

static void __not_in_flash_func(lcd_task)(void)
{
	absolute_time_t t;
	int64_t d;
	static lcd_func_t curr_func;

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
		// printf("SLEEP %lld\n", LCD_REFRESH_US - d);
		if (d < LCD_REFRESH_US)
			sleep_us(LCD_REFRESH_US - d);
#if 0
		else
			puts("REFRESH!");
#endif
	}
}
