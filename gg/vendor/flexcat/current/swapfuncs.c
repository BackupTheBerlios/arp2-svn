/* $Id: swapfuncs.c,v 1.3 2003/01/07 22:09:54 amiandrew Exp $
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

#include "flexcat.h"

/// FUNC: Swappers...
unsigned short  ( *SwapWord ) ( unsigned short r ) = NULL;
unsigned long   ( *SwapLong ) ( unsigned long r ) = NULL;
unsigned short SwapWord21 ( unsigned short r )
{
    return ( unsigned short )( ( r >> 8 ) + ( r << 8 ) );
}
unsigned short SwapWord12 ( unsigned short r )
{
    return r;
}
unsigned long SwapLong4321 ( unsigned long r )
{
    return ( ( r >> 24 ) & 0xFF ) + ( r << 24 ) + ( ( r >> 8 ) & 0xFF00 ) +
        ( ( r << 8 ) & 0xFF0000 );
}
unsigned long SwapLong1234 ( unsigned long r )
{
    return r;
}
//|

/// FUNC: SwapChoose
int SwapChoose ( void )
{
    unsigned short  w;
    unsigned long   d;

    strncpy ( ( char * )&w, "\1\2", 2 );
    strncpy ( ( char * )&d, "\1\2\3\4", 4 );

    if ( w == 0x0201 )
        SwapWord = SwapWord21;
    else if ( w == 0x0102 )
        SwapWord = SwapWord12;
    else
        return 0;

    if ( d == 0x04030201 )
        SwapLong = SwapLong4321;
    else if ( d == 0x01020304 )
        SwapLong = SwapLong1234;
    else
        return 0;

    return 1;
}
//|
