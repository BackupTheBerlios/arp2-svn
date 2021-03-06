/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  */

#ifdef HAVE_OSDEP_RPT
#include "osdep/rpt.h"
#else

typedef unsigned long frame_time_t;

STATIC_INLINE frame_time_t read_processor_time (void)
{
    return 0;
}

#endif
