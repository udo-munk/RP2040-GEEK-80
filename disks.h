/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 *
 * This module implements the disks drives and low level
 * access functions for MicroSD, needed by the FDC.
 *
 * History:
 * 29-JUN-2024 split of from memsim.c and picosim.c
 */

#define NUMDISK	4	/* number of disk drives */
#define DISKLEN	22	/* path length for disk drives /DISKS80/filename.DSK */

extern FIL sd_file;
extern FRESULT sd_res;
extern char disks[NUMDISK][DISKLEN];

extern void init_disks(void), exit_disks(void);
extern void list_files(const char *, const char *);
extern void load_file(const char *);
extern void check_disks(void);
extern void mount_disk(int, const char *name);

extern BYTE read_sec(int, int, int, WORD);
extern BYTE write_sec(int, int, int, WORD);
extern void get_fdccmd(BYTE *, WORD);
