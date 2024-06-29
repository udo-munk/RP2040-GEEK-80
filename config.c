/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 *
 * This module configures the machine appropriate for the
 * Z80/8080 software we want to run on it.
 *
 * History:
 * 20-APR-2024 dummy, no configuration implemented yet
 * 12-MAY-2024 implemented configuration dialog
 * 27-MAY-2024 implemented load file
 * 28-MAY-2024 implemented mount/unmount of disk images
 * 03-JUN-2024 added directory list for code files and disk images
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "f_util.h"
#include "ff.h"
#include "sim.h"
#include "simglb.h"
#include "lcd.h"

#define DEFAULT_BRIGHTNESS 90

extern FIL sd_file;
extern FRESULT sd_res;
extern char disks[4][22];
extern int speed;
extern BYTE fp_value;
extern int msc_ejected;
extern FATFS fs;

extern int get_cmdline(char *, int);
extern void switch_cpu(int);
extern void load_file(char *);
extern void mount_disk(int, char *);
extern void my_ls(const char *, const char *);
extern unsigned char fp_value;

/*
 * prompt for a filename
 */
static void prompt_fn(char *s)
{
	printf("Filename: ");
	get_cmdline(s, 9);
	while (*s) {
		*s = toupper((unsigned char) *s);
		s++;
	}
}

/*
 * get an integer with range check
 */
static int get_int(char *prompt, int min_val, int max_val)
{
	int i;
	char s[7];

	for (;;) {
		printf("Enter %s: ", prompt);
		get_cmdline(s, 6);
		if (s[0] == '\0')
			return -1;
		i = atoi(s);
		if (i < min_val || i > max_val) {
			printf("Invalid %s: range %d - %d\n",
			       prompt, min_val, max_val);
		} else
			return i;
	}
}

/*
 * Configuration dialog for the machine
 */
void config(void)
{
	const char *cfg = "/CONF80/CFG.DAT";
	const char *cpath = "/CODE80";
	const char *cext = "*.BIN";
	const char *dpath = "/DISKS80";
	const char *dext = "*.DSK";
	char s[10];
	unsigned int br;
	int go_flag = 0, brightness = DEFAULT_BRIGHTNESS, rotated = 0, i;
	datetime_t t;
	static const char *dotw[7] = { "Sun", "Mon", "Tue", "Wed",
				       "Thu", "Fri", "Sat" };

	/* try to read config file */
	sd_res = f_open(&sd_file, cfg, FA_READ);
	if (sd_res == FR_OK) {
		f_read(&sd_file, &cpu, sizeof(cpu), &br);
		f_read(&sd_file, &speed, sizeof(speed), &br);
		f_read(&sd_file, &fp_value, sizeof(fp_value), &br);
		f_read(&sd_file, &brightness, sizeof(brightness), &br);
		f_read(&sd_file, &rotated, sizeof(rotated), &br);
		f_read(&sd_file, &t, sizeof(datetime_t), &br);
		f_read(&sd_file, &disks[0], 22, &br);
		f_read(&sd_file, &disks[1], 22, &br);
		f_read(&sd_file, &disks[2], 22, &br);
		f_read(&sd_file, &disks[3], 22, &br);
		f_close(&sd_file);
	}
	lcd_brightness(brightness);
	lcd_set_rotated(rotated);
	rtc_set_datetime(&t);
	sleep_us(64);

	while (!go_flag) {
		if (rtc_get_datetime(&t)) {
			printf("Current time: %s %04d-%02d-%02d "
			       "%02d:%02d:%02d\n", dotw[t.dotw],
			       t.year, t.month, t.day, t.hour, t.min, t.sec);
		}
		printf("b - LCD brightness: %d\n", brightness);
		printf("m - rotate LCD\n");
		printf("a - set date\n");
		printf("t - set time\n");
		printf("u - enable USB mass storage access\n");
		printf("c - switch CPU, currently %s\n",
		       (cpu == Z80) ? "Z80" : "8080");
		printf("s - CPU speed: %d MHz\n", speed);
		printf("p - Port 255 value: %02XH\n", fp_value);
		printf("f - list files\n");
		printf("r - load file\n");
		printf("d - list disks\n");
		printf("0 - Disk 0: %s\n", disks[0]);
		printf("1 - Disk 1: %s\n", disks[1]);
		printf("2 - Disk 2: %s\n", disks[2]);
		printf("3 - Disk 3: %s\n", disks[3]);
		printf("g - run machine\n\n");
		printf("Command: ");
		get_cmdline(s, 2);
		putchar('\n');

		switch (tolower((unsigned char) *s)) {
		case 'b':
			printf("Value (0-100): ");
			get_cmdline(s, 4);
			putchar('\n');
			brightness = atoi((const char *) &s);
			if (brightness < 0 || brightness > 100) {
				printf("invalid brightness value: %d\n\n",
				       brightness);
				brightness = DEFAULT_BRIGHTNESS;
			}
			lcd_brightness((uint8_t) brightness);
			break;

		case 'm':
			rotated = !rotated;
			lcd_set_rotated(rotated);
			break;

		case 'a':
			if ((i = get_int("weekday", 0, 6)) >= 0)
				t.dotw = i;
			if ((i = get_int("year", 0, 4095)) >= 0)
				t.year = i;
			if ((i = get_int("month", 1, 12)) >= 0)
				t.month = i;
			if ((i = get_int("day", 1, 31)) >= 0)
				t.day = i;
			rtc_set_datetime(&t);
			sleep_us(64);
			putchar('\n');
			break;

		case 't':
			if ((i = get_int("hour", 0, 23)) >= 0)
				t.hour = i;
			if ((i = get_int("minute", 0, 59)) >= 0)
				t.min = i;
			if ((i = get_int("second", 0, 59)) >= 0)
				t.sec = i;
			rtc_set_datetime(&t);
			sleep_us(64);
			putchar('\n');
			break;

		case 'u':
			/* unmount SD card */
			f_unmount("");
			puts("Waiting for disk to be ejected");
			msc_ejected = false;
			while (!msc_ejected)
				sleep_ms(500);
			puts("Disk ejected");
			/* try to mount SD card */
			sd_res = f_mount(&fs, "", 1);
			if (sd_res != FR_OK)
				panic("f_mount error: %s (%d)\n",
				      FRESULT_str(sd_res), sd_res);
			break;

		case 'c':
			if (cpu == Z80)
				switch_cpu(I8080);
			else
				switch_cpu(Z80);
			break;

		case 's':
			printf("Value in MHz, 0=unlimited: ");
			get_cmdline(s, 2);
			putchar('\n');
			speed = atoi((const char *) &s);
			break;

		case 'p':
again:
			printf("Value in Hex: ");
			get_cmdline(s, 3);
			putchar('\n');
			if (!isxdigit((unsigned char) *s) ||
			    !isxdigit((unsigned char) *(s + 1))) {
				printf("What?\n");
				goto again;
			}
			fp_value = (*s <= '9' ? *s - '0' : *s - 'A' + 10) << 4;
			fp_value += (*(s + 1) <= '9' ? *(s + 1) - '0' :
				     *(s + 1) - 'A' + 10);
			break;

		case 'f':
			my_ls(cpath, cext);
			printf("\n\n");
			break;

		case 'r':
			prompt_fn(s);
			load_file(s);
			putchar('\n');
			break;

		case 'd':
			my_ls(dpath, dext);
			printf("\n\n");
			break;

		case '0':
			prompt_fn(s);
			if (strlen(s) == 0) {
				disks[0][0] = 0x0;
				putchar('\n');
			} else {
				mount_disk(0, s);
			}
			break;

		case '1':
			prompt_fn(s);
			if (strlen(s) == 0) {
				disks[1][0] = 0x0;
				putchar('\n');
			} else {
				mount_disk(1, s);
			}
			break;

		case '2':
			prompt_fn(s);
			if (strlen(s) == 0) {
				disks[2][0] = 0x0;
				putchar('\n');
			} else {
				mount_disk(2, s);
			}
			break;

		case '3':
			prompt_fn(s);
			if (strlen(s) == 0) {
				disks[3][0] = 0x0;
				putchar('\n');
			} else {
				mount_disk(3, s);
			}
			break;

		case 'g':
			go_flag = 1;
			break;

		default:
			break;
		}
	}

	/* try to save config file */
	sd_res = f_open(&sd_file, cfg, FA_WRITE | FA_CREATE_ALWAYS);
	if (sd_res == FR_OK) {
		f_write(&sd_file, &cpu, sizeof(cpu), &br);
		f_write(&sd_file, &speed, sizeof(speed), &br);
		f_write(&sd_file, &fp_value, sizeof(fp_value), &br);
		f_write(&sd_file, &brightness, sizeof(brightness), &br);
		f_write(&sd_file, &rotated, sizeof(rotated), &br);
		f_write(&sd_file, &t, sizeof(datetime_t), &br);
		f_write(&sd_file, &disks[0], 22, &br);
		f_write(&sd_file, &disks[1], 22, &br);
		f_write(&sd_file, &disks[2], 22, &br);
		f_write(&sd_file, &disks[3], 22, &br);
		f_close(&sd_file);
	}
}
