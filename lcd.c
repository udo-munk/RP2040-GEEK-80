/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#include "pico/multicore.h"
#include "lcd.h"
#include "sim.h"
#include "simglb.h"

/* memory image for drawing */
#if LCD_COLOR_DEPTH == 12
static UBYTE img[LCD_1IN14_V2_HEIGHT * ((LCD_1IN14_V2_WIDTH + 1) / 2) * 3];
#else
static UWORD img[LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH];
#endif

static int refresh_flag;	/* task will refresh LCD display when > 0 */
static int cpudisp_flag;	/* task will draw CPU status when > 0 */
static int curr_cpu;		/* currently shown CPU type */
static char info_line[17];	/* last line in display */

static void lcd_task(void);

void lcd_init(void)
{
	/* initialize LCD device */
	/* set orientation, x = 240 width and y = 135 height */
	LCD_1IN14_V2_Init(HORIZONTAL);

	/* use this image memory */
	Paint_NewImage((UBYTE *) &img[0], LCD_1IN14_V2.WIDTH,
			LCD_1IN14_V2.HEIGHT, 0, WHITE);

	/* with this depth */
	Paint_SetDepth(LCD_COLOR_DEPTH);

	/* 0,0 is upper left corner */
	Paint_SetRotate(ROTATE_0);

	cpudisp_flag = 0;
	refresh_flag = 1;
	curr_cpu = cpu;
	snprintf(info_line, sizeof(info_line), "Z80pack pico %s", USR_REL);
	multicore_launch_core1(lcd_task);
}

void lcd_cpudisp_on(void)
{
	if (cpudisp_flag == 0)
		cpudisp_flag = 1;
}

void lcd_cpudisp_off(void)
{
	if (cpudisp_flag > 0) {
		cpudisp_flag = -1;
		while (cpudisp_flag == -1)
			sleep_ms(20);

		Paint_Clear(BLACK);
	}
}

void lcd_finish(void)
{
	lcd_cpudisp_off();

	if (refresh_flag > 0) {
		refresh_flag = -1;
		while (refresh_flag == -1)
			sleep_ms(20);
	}

	multicore_reset_core1();
}

static void lcd_refresh(void)
{
#if LCD_COLOR_DEPTH == 12
	LCD_1IN14_V2_Display12(&img[0]);
#else
	LCD_1IN14_V2_Display(&img[0]);
#endif
}

void lcd_brightness(int brightness)
{
	LCD_1IN14_V2_SetBacklight((uint8_t) brightness);
}

void lcd_info(void)
{
	Paint_Clear(BLACK);
	Paint_DrawString(25, 40, "Waiting for", &Font24, RED, BLACK);
	Paint_DrawString(25, 64, "  terminal ", &Font24, RED, BLACK);
}

void lcd_banner(void)
{
	Paint_Clear(BLACK);
	Paint_DrawString(25, 40, "# Z80pack #", &Font24, BLACK, WHITE);
	Paint_DrawString(25, 64, "RP2040-GEEK", &Font24, BLACK, WHITE);
}

static void lcd_cpubg(void)
{
	Paint_Clear(BLACK);
	Paint_DrawChar  (8+ 0*14,  0+0*20, 'A', &Font20, WHITE, BLACK);
	Paint_DrawString(8+ 9*14,  0+0*20, "BC", &Font20, WHITE, BLACK);
	Paint_DrawString(8+ 0*14,  3+1*20, "DE", &Font20, WHITE, BLACK);
	Paint_DrawString(8+ 9*14,  3+1*20, "HL", &Font20, WHITE, BLACK);
	Paint_DrawString(8+ 0*14,  6+2*20, "SP", &Font20, WHITE, BLACK);
	Paint_DrawString(8+ 9*14,  6+2*20, "PC", &Font20, WHITE, BLACK);
	if (curr_cpu == Z80) {
	Paint_DrawString(8+ 0*14,  9+3*20, "IX", &Font20, WHITE, BLACK);
	Paint_DrawString(8+ 9*14,  9+3*20, "IY", &Font20, WHITE, BLACK);
	Paint_DrawChar  (8+ 0*14, 12+4*20, 'F', &Font20, WHITE, BLACK);
	}
	else {
	Paint_DrawChar  (8+ 0*14,  9+3*20, 'F', &Font20, WHITE, BLACK);
	}
	Paint_DrawString(8+ 0*14, 15+5*20, info_line, &Font20, BRRED, BLACK);
}

static void lcd_cpudisp(void)
{
	static const char *hex = "0123456789ABCDEF";

	/*
	 *  A  12    BC 1234
	 *  DE 1234  HL 1234
	 *  SP 1234  PC 1234
	 *  IX 1234  IY 1234
	 *  F  S Z H P N C
	 *  Z80pack pico 1.2
	 */

	Paint_DrawChar(8+ 3*14,  0+0*20, hex[A >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 4*14,  0+0*20, hex[A & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+12*14,  0+0*20, hex[B >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+13*14,  0+0*20, hex[B & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+14*14,  0+0*20, hex[C >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+15*14,  0+0*20, hex[C & 15], &Font20, GREEN, BLACK);

	Paint_DrawChar(8+ 3*14,  3+1*20, hex[D >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 4*14,  3+1*20, hex[D & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 5*14,  3+1*20, hex[E >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 6*14,  3+1*20, hex[E & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+12*14,  3+1*20, hex[H >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+13*14,  3+1*20, hex[H & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+14*14,  3+1*20, hex[L >> 4], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+15*14,  3+1*20, hex[L & 15], &Font20, GREEN, BLACK);

	Paint_DrawChar(8+ 3*14,  6+2*20, hex[SP >> 12], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 4*14,  6+2*20, hex[(SP >> 8) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 5*14,  6+2*20, hex[(SP >> 4) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 6*14,  6+2*20, hex[SP & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+12*14,  6+2*20, hex[PC >> 12], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+13*14,  6+2*20, hex[(PC >> 8) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+14*14,  6+2*20, hex[(PC >> 4) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+15*14,  6+2*20, hex[PC & 15], &Font20, GREEN, BLACK);

	if (curr_cpu == Z80) {
	Paint_DrawChar(8+ 3*14,  9+3*20, hex[IX >> 12], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 4*14,  9+3*20, hex[(IX >> 8) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 5*14,  9+3*20, hex[(IX >> 4) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+ 6*14,  9+3*20, hex[IX & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+12*14,  9+3*20, hex[IY >> 12], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+13*14,  9+3*20, hex[(IY >> 8) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+14*14,  9+3*20, hex[(IY >> 4) & 15], &Font20, GREEN, BLACK);
	Paint_DrawChar(8+15*14,  9+3*20, hex[IY & 15], &Font20, GREEN, BLACK);

	Paint_DrawChar(8+ 3*14, 12+4*20, 'S', &Font20, F & S_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+ 5*14, 12+4*20, 'Z', &Font20, F & Z_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+ 7*14, 12+4*20, 'H', &Font20, F & H_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+ 9*14, 12+4*20, 'P', &Font20, F & P_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+11*14, 12+4*20, 'N', &Font20, F & N_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+13*14, 12+4*20, 'C', &Font20, F & C_FLAG ? GREEN : RED, BLACK);
	} else {
	Paint_DrawChar(8+ 3*14,  9+3*20, 'S', &Font20, F & S_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+ 5*14,  9+3*20, 'Z', &Font20, F & Z_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+ 7*14,  9+3*20, 'H', &Font20, F & H_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+ 9*14,  9+3*20, 'P', &Font20, F & P_FLAG ? GREEN : RED, BLACK);
	Paint_DrawChar(8+11*14,  9+3*20, 'C', &Font20, F & C_FLAG ? GREEN : RED, BLACK);
	}
}

static void lcd_task(void)
{
	absolute_time_t t;
	int64_t d;

	while (1) {
		/* loops every LCD_REFRESH_US */

		t = get_absolute_time();

		if (cpudisp_flag) {
			if (cpudisp_flag < 0) {
				/* request to finish */
				cpudisp_flag = 0;
			} else {
				if (curr_cpu != cpu) {
					curr_cpu = cpu;
					cpudisp_flag = 1;
				}
				if (cpudisp_flag == 1) {
					/* first time here, draw background */
					lcd_cpubg();
					cpudisp_flag++;
				} else
					lcd_cpudisp();
			}
		}

		if (refresh_flag) {
			if (refresh_flag < 0) {
				/* request to finish */
				refresh_flag = 0;
			} else
				lcd_refresh();
		}

		d = absolute_time_diff_us(t, get_absolute_time());
		if (d < LCD_REFRESH_US)
			sleep_us(LCD_REFRESH_US - d);
		else
			puts("REFRESH!");
	}
}
