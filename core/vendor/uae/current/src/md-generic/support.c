 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Miscellaneous machine dependent support functions and definitions
  *
  * Copyright 1996 Bernd Schmidt
  * Copyright 2004-2005 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "machdep/m68k.h"

void machdep_init (void)
{
}

/*
 * Handle processor-specific cfgfile options
 */
void machdep_save_options (FILE *f, struct uae_prefs *p)
{
}

int machdep_parse_option (struct uae_prefs *p, char *option, char *value)
{
    return 0;
}

void machdep_default_options (struct uae_prefs *p)
{
}
