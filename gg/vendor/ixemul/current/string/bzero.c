/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef mc68000
#include <string.h>

#ifdef __PPC__

/* Relatively fast bzero for PPC. - Piru */

#define USE_INDEX	1
#define USE_WRITE64	0

void bzero(void *b, size_t len)
{
  typedef unsigned char u8;
#if USE_WRITE64
  typedef double        LTYPE;
#else
  typedef unsigned int  LTYPE;
#endif
  const size_t LSIZE = sizeof(LTYPE);
  const size_t LMASK = sizeof(LTYPE) - 1;

  if (len >= LSIZE * 4)
  {
    size_t n, o;

    n = (size_t)b & LMASK;
    if (n)
    {
      len -= n;
      do
        *((u8 *)b)++ = 0;
      while (--n);
    }
    n = len / LSIZE;
    len &= LMASK;

    if (n >= 8)
    {
      o = n / 8;
      n &= 7;

#if USE_INDEX

      do
      {
        ((LTYPE *)b)[0] = 0;
        ((LTYPE *)b)[1] = 0;
        ((LTYPE *)b)[2] = 0;
        ((LTYPE *)b)[3] = 0;
        ((LTYPE *)b)[4] = 0;
        ((LTYPE *)b)[5] = 0;
        ((LTYPE *)b)[6] = 0;
        ((LTYPE *)b) += 8;
        ((LTYPE *)b)[-1] = 0;

      } while (--o);

#else

      ((LTYPE *)b)--;
      do
      {
        *++((LTYPE *)b) = 0;
        *++((LTYPE *)b) = 0;
        *++((LTYPE *)b) = 0;
        *++((LTYPE *)b) = 0;

        *++((LTYPE *)b) = 0;
        *++((LTYPE *)b) = 0;
        *++((LTYPE *)b) = 0;
        *++((LTYPE *)b) = 0;

      } while (--o);
      ((LTYPE *)b)++;

#endif
    }

    while (n--)
      *((LTYPE *)b)++ = 0;
  }

  while (len--)
    *((u8 *)b)++ = 0;
}

#else

void bzero(void *b,size_t len)
{ size_t n;
  if (!len)
    return;
  if((unsigned long)b&1)
  { *((char *)b)++=0;
    len--; }
  n=len/sizeof(long);
  len-=n*sizeof(long);
  while(n--)
    *((long *)b)++=0;
  while(len--)
    *((char *)b)++=0;
}

#endif
#else
#include "defs.h"

/*
 * This is probably not the best we can do, but it is still much
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *      - unroll the longword loop (might not be good for a 68020)
 *      - longword, as opposed to word, align when possible (only on the 68020)
 *      - use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(bzero)
asm("
	movl    sp@(4),a0       /* destination */
	movl    sp@(8),d0       /* count */
	jeq     bzdone          /* nothing to do */
	movl    a0,d1
	btst    #0,d1           /* address odd? */
	jeq     bzeven          /* no, skip alignment */
	clrb    a0@+            /* yes, clear a byte */
	subql   #1,d0           /* adjust count */
	jeq     bzdone          /* if zero, all done */
bzeven:
	movl    d0,d1
	lsrl    #2,d1           /* convert to longword count */
	jeq     bzbloop         /* no longwords, skip loop */
bzlloop:
	clrl    a0@+            /* clear a longword */
	subql   #1,d1           /* adjust count */
	jne     bzlloop         /* still more, keep going */
	andl    #3,d0           /* what remains */
	jeq     bzdone          /* nothing, all done */
bzbloop:
	clrb    a0@+            /* clear a byte */
	subql   #1,d0           /* adjust count */
	jne     bzbloop         /* still more, keep going */
bzdone:
	rts
");
#endif
