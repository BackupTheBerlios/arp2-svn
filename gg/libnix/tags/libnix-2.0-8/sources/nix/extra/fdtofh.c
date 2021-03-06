/*
 * Get the AmigaDOS FileHandle from the filedescriptor.
 * Be careful when mixing both kinds of I/O (flush() between changing).
 *
 * DICE has such a function SAS has not.
 * It's not ANSI but it's encapsulated and may be useful - so why not :-).
 * 
 */

#include "stdio.h"

long fdtofh(int filedescriptor)
{
  StdFileDes *fp = _lx_fhfromfd(filedescriptor);

  return fp != NULL ? fp->lx_fh : 0;
}
