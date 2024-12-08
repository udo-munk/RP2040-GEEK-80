/*
 * Emulation of Cromemco Dazzler on the RP2040/RP2350-GEEK LCD
 *
 * Copyright (C) 2015-2019 by Udo Munk
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#ifndef DAZZLER_INC
#define DAZZLER_INC

#include "sim.h"
#include "simdefs.h"

extern void dazzler_ctl_out(BYTE data), dazzler_format_out(BYTE data);
extern BYTE dazzler_flags_in(void);

#endif /* !DAZZLER_INC */
