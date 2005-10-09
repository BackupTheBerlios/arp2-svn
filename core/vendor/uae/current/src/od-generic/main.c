/*
 * UAE - The Un*x Amiga Emulator
 *
 * Copyright 2004 Richard Drummond
 * 
 * Start-up and support functions used by Linux/Unix targets
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "uae.h"
#include "debug.h"

/*
 * Handle break signal
 */
#include <signal.h>

#ifdef __cplusplus
static RETSIGTYPE sigbrkhandler(...)
#else
static RETSIGTYPE sigbrkhandler (int foo)
#endif
{
    activate_debugger();

#if !defined(__unix) || defined(__NeXT__)
    signal (SIGINT, sigbrkhandler);
#endif
}

void setup_brkhandler (void)
{
#if defined(__unix) && !defined(__NeXT__)
    struct sigaction sa;
    sa.sa_handler = sigbrkhandler;
    sa.sa_flags = 0;
#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#endif
    sigemptyset (&sa.sa_mask);
    sigaction (SIGINT, &sa, NULL);
#else
    signal (SIGINT, sigbrkhandler);
#endif
}

/*
 * Handle target-specific cfgfile options
 */
void target_save_options (FILE *f, struct uae_prefs *p)
{
}

int target_parse_option (struct uae_prefs *p, char *option, char *value)
{
    return 0;
}

void target_default_options (struct uae_prefs *p)
{
}
