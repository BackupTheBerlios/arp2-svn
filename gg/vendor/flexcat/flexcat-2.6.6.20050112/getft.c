/* $Id: getft.c,v 1.4 2003/02/19 23:03:24 amiandrew Exp $
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

#include <proto/exec.h>
#include <proto/dos.h>

/// getft

/* Returns the time of change.
   Used for compatibility */
long getft ( char *filename )
{
    BPTR            p_flock;
    long            timestamp = 0;
    struct FileInfoBlock *p_fib;

    if ( ( p_fib = AllocDosObject ( DOS_FIB, NULL ) ) != NULL )
    {
        if ( ( p_flock = Lock ( filename, ACCESS_READ ) ) != NULL )
        {
            Examine ( p_flock, p_fib );
            timestamp = p_fib->fib_Date.ds_Days * 86400;        // days
            timestamp += p_fib->fib_Date.ds_Minute * 60;        // minutes
            timestamp += p_fib->fib_Date.ds_Tick / TICKS_PER_SECOND;    // seconds
            UnLock ( p_flock );
        }
        FreeDosObject ( DOS_FIB, p_fib );
    }

    return timestamp;
}

//|
