/* foo.h -- interface to the libfoo* libraries
   Copyright (C) 1998-1999 Free Software Foundation, Inc.
   Originally by Thomas Tanner <tanner@ffii.org>
   This file is part of GNU Libtool.

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
USA. */

/* Only include this header file once. */
#ifndef _FOO_H_
#define _FOO_H_ 1

/* Silly constants that the functions return. */
#define HELLO_RET 0xe110
#define FOO_RET 0xf00

extern void sub();

#endif /* !_FOO_H_ */
