/* l2.c -- trivial test library
   Copyright (C) 1998-1999 Thomas Tanner <tanner@ffii.org>
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

#include "l2/l2.h"

#include "l1/l1.h"
#include <stdio.h>

int	var_l2 = 0;

int
func_l2(indent)
    int indent;
{
  int i;

  for (i = 0; i < indent; i++)
    putchar(' ');
  printf("l2 (%i)\n", var_l2);
  func_l1(indent+1);
  var_l2 += var_l1;
  return 0; 
}
