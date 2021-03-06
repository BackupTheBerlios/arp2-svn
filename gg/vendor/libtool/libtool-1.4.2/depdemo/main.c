/* main.c -- inter-library dependency test program
   Copyright (C) 1998, 1999, 2000 Free Software Foundation
   by Thomas Tanner <tanner@ffii.org>
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

#include "l1/l1.h"
#include "l2/l2.h"
#include "l4/l4.h"
#include <stdio.h>
#include <string.h>

int
main (argc,argv)
    int argc;
    char **argv;
{
  printf("dependencies:\n");
  func_l1(0);
  func_l2(0);
  func_l4(0);
  if (argc == 2 && strcmp (argv[1], "-alt") == 0
      && var_l1 + var_l2 + var_l4 == 8)
	return 0;
  if (var_l1 + var_l2 + var_l4 != 20)
  	return 1;
  return 0;
}
