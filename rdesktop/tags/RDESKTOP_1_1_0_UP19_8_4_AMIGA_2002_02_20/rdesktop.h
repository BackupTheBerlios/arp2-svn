/*
   rdesktop: A Remote Desktop Protocol client.
   Master include file
   Copyright (C) Matthew Chapman 1999-2001

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
	interesting variables/macros etc.
	WASHERE(); -- define XDEBUG to use it.
	X_SYNCED -- define if you wish to run X synced.
 */

#include <stdio.h>
#include <string.h>

#include "localendian.h"

#define VERSION "1.1.0-pl19-8-4 "

/*
 * check endianess
 */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define L_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define B_ENDIAN
#else
#error Unknown endianness. Edit rdesktop.h.
#endif

/*
 * check if we need to align data
 */

#if defined(__sparc__) || defined(__alpha__) || defined(__hppa__) || \
    defined(__AIX__) || defined(__PPC__) || defined(__mips__) || \
    defined(__ia64__)
#define NEED_ALIGN
#endif

#ifdef XDEBUG
#define WASHERE() {fprintf(stderr,"file %s line %d\n",__FILE__,__LINE__);}
#else
#define WASHERE()
#endif

#define STATUS(args...) fprintf(stderr, args);
#define WARN(args...)   fprintf(stderr, "WARNING: "args);

#if defined(DEBUG) || defined(WITH_DEBUG_RDP)
#undef DEBUG
#define DEBUG(args...)  fprintf(stderr, args);
#else
#define DEBUG(args...)
#endif

#include "constants.h"
#include "types.h"
#include "parse.h"

#ifndef MAKE_PROTO
#include "proto.h"
#endif

#ifdef WITH_DEBUG_LEAKS
#define xmalloc(size) xmalloc2(__LINE__, ##__FILE__, size)
#define xrealloc(mem, size) xrealloc2(__LINE__, ##__FILE__, mem, size)
#define xfree(mem) xfree2(__LINE__, ##__FILE__, mem)
#endif
