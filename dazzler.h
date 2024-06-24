/*
 * Emulation of Cromemco Dazzler on the RP2040-GEEK LCD
 *
 * Copyright (C) 2015-2019 by Udo Munk
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#ifndef DAZZLER_H
#define DAZZLER_H

extern void dazzler_ctl_out(BYTE), dazzler_format_out(BYTE);
extern BYTE dazzler_flags_in(void);

#endif
