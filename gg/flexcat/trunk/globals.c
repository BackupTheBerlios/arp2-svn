
/* $Id: globals.c,v 1.5 2003/02/12 15:00:37 amiandrew Exp $
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

/// Globals
char           *BaseName = NULL;        /* Basename of catalog description */
char           *Language = "english";   /* Language of catalog description */
int             CatVersion = 0; /* Version of catalog to be opened */
int             NumStrings = 0; /* Number of catalog strings */
char           *ScanFile;       /*  File currently scanned */
int             ScanLine;       /*  Line currently scanned */
int             GlobalReturnCode = 0;   /*  Will be 5, if warnings appear */
int             NumberOfWarnings = 0;   /* We count warnings to be smart
                                           and not to do Beep bombing, but
                                           call DisplayBeep() only once */
int             buffer_size = 2048;     /* Size of the IO buffer */
char            VersTag[] = VERSTAG;

//|
