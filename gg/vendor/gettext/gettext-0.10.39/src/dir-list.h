/* GNU gettext - internationalization aids
   Copyright (C) 1996, 1998, 2000 Free Software Foundation, Inc.

   This file was written by Peter Miller <millerp@canb.auug.org.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef SRC_DIR_LIST_H
#define SRC_DIR_LIST_H

/* Management of the list of directories where PO files are searched.
   It is an ordered list, without duplicates.  The default value of the
   list consists of the single directory ".".  */

/* Append a directory to the end of the list of directories.  */
extern void dir_list_append PARAMS ((const char *__directory));

/* Return the nth directory, or NULL of n is out of range.  */
extern const char *dir_list_nth PARAMS ((int __n));

#endif /* SRC_DIR_LIST_H */
