/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 *
 * This is the main program for a RP2040-GEEK board,
 * substitutes z80core/simmain.c
 *
 * History:
 * 28-APR-2024 implemented first release of Z80 emulation
 * 09-MAY-2024 test 8080 emulation
 * 27-MAY-2024 add access to files on MicroSD
 * 28-MAY-2024 implemented boot from disk images with some OS
 * 31-MAY-2024 use USB UART
 * 09-JUN-2024 implemented boot ROM
 * 13-JUN-2024 ported to RP2040-GEEK
 * 15-JUN-2024 added access to RP2040-GEEK LCD display
 * 24-JUN-2024 added emulation of Cromemco Dazzler
 */

/* Raspberry SDK and FatFS includes */
#include <stdio.h>
#include <string.h>
#if LIB_PICO_STDIO_USB || LIB_STDIO_MSC_USB
#include <tusb.h>
#endif
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/adc.h"

#include "ff.h"
#include "hw_config.h"
#include "rtc.h"

/* Project includes */
#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simcfg.h"
#include "simmem.h"
#include "simcore.h"
#include "simport.h"
#include "simio.h"
#ifdef WANT_ICE
#include "simice.h"
#endif

#include "disks.h"
#include "lcd.h"

#define BS  0x08 /* backspace */
#define DEL 0x7f /* delete */

/* CPU speed */
int speed = CPU_SPEED;

#if LIB_PICO_STDIO_USB || (LIB_STDIO_MSC_USB && !STDIO_MSC_USB_DISABLE_STDIO)
void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
	UNUSED(itf);
	UNUSED(duration_ms);

	cpu_error = USERINT;
	cpu_state = STOPPED;
}
#endif

#if LIB_PICO_STDIO_USB || (LIB_STDIO_MSC_USB && !STDIO_MSC_USB_DISABLE_STDIO)
static void draw_wait_term(int first_flag)
{
	if (!first_flag)
		return;
	Paint_Clear(BLACK);
	Paint_DrawString(43, 39, "Waiting for", &Font28, RED, BLACK);
	Paint_DrawString(43, 67, "  terminal ", &Font28, RED, BLACK);
}
#endif

static void draw_banner(int first_flag)
{
	if (!first_flag)
		return;
	Paint_Clear(BLACK);
	Paint_DrawString(43, 39, "# Z80pack #", &Font28, BLACK, WHITE);
	Paint_DrawString(43, 67, "RP2040-GEEK", &Font28, BLACK, WHITE);
}

int main(void)
{
	char s[2];

	stdio_init_all();	/* initialize stdio */
#if LIB_STDIO_MSC_USB
	sd_init_driver();	/* initialize SD card driver */
	tusb_init();		/* initialize TinyUSB */
	stdio_msc_usb_init();	/* initialize MSC USB stdio */
#endif
	time_init();		/* initialize FatFS RTC */
	lcd_init();		/* initialize LCD */

	/*
	 * initialize hardware AD converter, enable onboard
	 * temperature sensor and select its channel
	 */
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

	/* when using USB UART wait until it is connected */
#if LIB_PICO_STDIO_USB || (LIB_STDIO_MSC_USB && !STDIO_MSC_USB_DISABLE_STDIO)
	lcd_set_draw_func(draw_wait_term);
	while (!tud_cdc_connected())
		sleep_ms(100);
#endif

	/* print banner */
	lcd_set_draw_func(draw_banner);
	printf("\fZ80pack release %s, %s\n", RELEASE, COPYR);
	printf("%s release %s\n", USR_COM, USR_REL);
	printf("%s\n\n", USR_CPR);

	init_cpu();		/* initialize CPU */
	init_disks();		/* initialize disk drives */
	init_memory();		/* initialize memory configuration */
	init_io();		/* initialize I/O devices */
	config();		/* configure the machine */

	f_flag = speed;		/* setup speed of the CPU */
	tmax = speed * 10000;	/* theoretically */

	PC = 0xff00;		/* power on jump into the boot ROM */

	lcd_default_draw_func(); /* tell LCD task to display default infos */

#ifdef WANT_ICE
	ice_cmd_loop(0);
#else
	run_cpu();		/* run the CPU with whatever is in memory */
#endif

	exit_disks();		/* stop disk drives */
	lcd_exit();		/* shutdown LCD */

#ifndef WANT_ICE
	putchar('\n');
	report_cpu_error();	/* check for CPU emulation errors and report */
	report_cpu_stats();	/* print some execution statistics */
#endif
	puts("\nPress any key to restart CPU");
	get_cmdline(s, 2);

	stdio_flush();
	return 0;
}

/*
 * Read an ICE or config command line of maximum length len - 1
 * from the terminal. For single character requests (len == 2),
 * returns immediately after input is received.
 */
int get_cmdline(char *buf, int len)
{
	int i = 0;
	char c;

	for (;;) {
		c = getchar();
		if ((c == BS) || (c == DEL)) {
			if (i >= 1) {
				putchar(BS);
				putchar(' ');
				putchar(BS);
				i--;
			}
		} else if (c != '\r') {
			if (i < len - 1) {
				buf[i++] = c;
				putchar(c);
				if (len == 2)
					break;
			}
		} else {
			break;
		}
	}
	buf[i] = '\0';
	putchar('\n');
	return 0;
}
