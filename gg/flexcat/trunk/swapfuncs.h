
/* $Id: swapfuncs.h,v 1.4 2003/02/19 23:03:28 amiandrew Exp $
 * 
 * Copyright (C) 2002 Ondrej Zima <amiandrew@volny.cz>
 * Copyright (C) 2002 Stefan Kost <ensonic@sonicpulse.de>
 * Copyright (C) 1993 Jochen Wiedmann and Marcin Orlowski <carlos@wfmh.org.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef FLEXCAT_SWAPFUNCS_H
#define FLEXCAT_SWAPFUNCS_H

// Functions
extern unsigned long   ( *SwapLong ) ( unsigned long r );
extern unsigned short  ( *SwapWord ) ( unsigned short r );
unsigned short  SwapWord21 ( unsigned short r );
unsigned short  SwapWord12 ( unsigned short r );
unsigned long   SwapLong4321 ( unsigned long r );
unsigned long   SwapLong1234 ( unsigned long r );
int             SwapChoose ( void );

#endif  /* FLEXCAT_SWAPFUNCS_H */
