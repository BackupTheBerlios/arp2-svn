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

/* This is a _fast_ block move routine for PowerPC - Piru */

#define USE_INDEX	1
#define USE_READ8WRITE8	1
#define USE_DOUBLECOPY	0 /* experimental, need to be tested */

void bcopy(const void *s1, void *s2, size_t n)
{
  typedef unsigned char u8;
  typedef unsigned int  LTYPE;
  const size_t LSIZE = sizeof(LTYPE);
  const size_t LMASK = sizeof(LTYPE) - 1;

  size_t m, o;

  if (!n)
    return;

  if (s2 < s1)
  {
    if (n >= LSIZE * 4)
    {
      m = (size_t) s1 & LMASK;
      if (m && ((size_t) s2 & LMASK) == m)
      {
        n -= m;
        do
          *((u8 *)s2)++ = *((u8 *)s1)++;
        while (--m);
      }
#if USE_DOUBLECOPY
      /* If source is aligned by 4, destionation is aligned by 4 and not 8 */
      if (!((size_t) s1 & LMASK) && !((size_t) s2 & LMASK) && ((size_t) s2 & LSIZE))
      {
        n -= LSIZE;
        *((LTYPE *)s2)++ = *((LTYPE *)s1)++;
      }
#endif
      m = n / LSIZE;
      n &= LMASK;
      if (m >= 8)
      {
        o = m / 8;
        m &= 7;
#if USE_INDEX

#if USE_DOUBLECOPY
        if (!((size_t) s1 & LMASK) && !((size_t) s2 & (sizeof(double) - 1)))
        {
          do
          {
#if USE_READ8WRITE8

            register double r0,r1,r2,r3;

            r0 = ((double *)s1)[0];
            r1 = ((double *)s1)[1];
            r2 = ((double *)s1)[2];
            ((double *)s1) += 4;
            r3 = ((double *)s1)[-1];

            ((double *)s2)[0] = r0;
            ((double *)s2)[1] = r1;
            ((double *)s2)[2] = r2;
            ((double *)s2) += 4;
            ((double *)s2)[-1] = r3;

#else

            ((double *)s2)[0] = ((double *)s1)[0];
            ((double *)s2)[1] = ((double *)s1)[1];
            ((double *)s2)[2] = ((double *)s1)[2];
            ((double *)s2)[3] = ((double *)s1)[3];

            ((double *)s2) += 4;
            ((double *)s1) += 4;

#endif
          } while (--o);
        }
        else
#endif
        do
        {
#if USE_READ8WRITE8

          register LTYPE r0,r1,r2,r3,r4,r5,r6,r7;

          r0 = ((LTYPE *)s1)[0];
          r1 = ((LTYPE *)s1)[1];
          r2 = ((LTYPE *)s1)[2];
          r3 = ((LTYPE *)s1)[3];
          r4 = ((LTYPE *)s1)[4];
          r5 = ((LTYPE *)s1)[5];
          r6 = ((LTYPE *)s1)[6];
          ((LTYPE *)s1) += 8;
          r7 = ((LTYPE *)s1)[-1];

          ((LTYPE *)s2)[0] = r0;
          ((LTYPE *)s2)[1] = r1;
          ((LTYPE *)s2)[2] = r2;
          ((LTYPE *)s2)[3] = r3;
          ((LTYPE *)s2)[4] = r4;
          ((LTYPE *)s2)[5] = r5;
          ((LTYPE *)s2)[6] = r6;
          ((LTYPE *)s2) += 8;
          ((LTYPE *)s2)[-1] = r7;

#else

          //seems slower here: asm("dcbt 0,%0" : : "b" (s1));

          ((LTYPE *)s2)[0] = ((LTYPE *)s1)[0];
          ((LTYPE *)s2)[1] = ((LTYPE *)s1)[1];
          ((LTYPE *)s2)[2] = ((LTYPE *)s1)[2];
          ((LTYPE *)s2)[3] = ((LTYPE *)s1)[3];

          ((LTYPE *)s2)[4] = ((LTYPE *)s1)[4];
          ((LTYPE *)s2)[5] = ((LTYPE *)s1)[5];
          ((LTYPE *)s2)[6] = ((LTYPE *)s1)[6];
          ((LTYPE *)s2)[7] = ((LTYPE *)s1)[7];

          ((LTYPE *)s2) += 8;
          ((LTYPE *)s1) += 8;

#endif

        } while (--o);

#else

        ((LTYPE *)s1)--; ((LTYPE *)s2)--;
        do
        {
          *++((LTYPE *)s2) = *++((LTYPE *)s1);
          *++((LTYPE *)s2) = *++((LTYPE *)s1);
          *++((LTYPE *)s2) = *++((LTYPE *)s1);
          *++((LTYPE *)s2) = *++((LTYPE *)s1);

          *++((LTYPE *)s2) = *++((LTYPE *)s1);
          *++((LTYPE *)s2) = *++((LTYPE *)s1);
          *++((LTYPE *)s2) = *++((LTYPE *)s1);
          *++((LTYPE *)s2) = *++((LTYPE *)s1);

        } while (--o);
        ((LTYPE *)s1)++; ((LTYPE *)s2)++;

#endif
      }
      while (m--)
        *((LTYPE *)s2)++ = *((LTYPE *)s1)++;
    }
    while (n--)
      *((u8 *)s2)++ = *((u8 *)s1)++;
  }
  else
  {
    (u8 *)s1 += n;
    (u8 *)s2 += n;

    if (n >= LSIZE * 4)
    {
      m = (size_t) s1 & LMASK;
      if (m && ((size_t) s2 & LMASK) == m)
      {
        n -= m;
        do
          *--((u8 *)s2) = *--((u8 *)s1);
        while (--m);
      }
      m = n / LSIZE;
      n &= LMASK;
      if (m >= 8)
      {
        o = m / 8;
        m &= 7;

#if USE_DOUBLECOPY
        if (!((size_t) s1 & LMASK) && !((size_t) s2 & LMASK))
        {
          do
          {
#if USE_READ8WRITE8

            register double r0,r1,r2,r3;

            r0 = ((double *)s1)[-1];
            r1 = ((double *)s1)[-2];
            r2 = ((double *)s1)[-3];
            ((double *)s1) -= 4;
            r3 = ((double *)s1)[0];

            ((double *)s2)[-1] = r0;
            ((double *)s2)[-2] = r1;
            ((double *)s2)[-3] = r2;
            ((double *)s2) -= 4;
            ((double *)s2)[0] = r3;

#else

            ((double *)s2)[-1] = ((double *)s1)[-1];
            ((double *)s2)[-2] = ((double *)s1)[-2];
            ((double *)s2)[-3] = ((double *)s1)[-3];
            ((double *)s2)[-4] = ((double *)s1)[-4];

            ((double *)s2) -= 4;
            ((double *)s1) -= 4;

#endif
          } while (--o);
        }
        else
#endif
        do
        {
#if USE_INDEX

#if USE_READ8WRITE8

          register LTYPE r0,r1,r2,r3,r4,r5,r6,r7;

          r0 = ((LTYPE *)s1)[-1];
          r1 = ((LTYPE *)s1)[-2];
          r2 = ((LTYPE *)s1)[-3];
          r3 = ((LTYPE *)s1)[-4];
          r4 = ((LTYPE *)s1)[-5];
          r5 = ((LTYPE *)s1)[-6];
          r6 = ((LTYPE *)s1)[-7];
          ((LTYPE *)s1) -= 8;
          r7 = ((LTYPE *)s1)[0];

          ((LTYPE *)s2)[-1] = r0;
          ((LTYPE *)s2)[-2] = r1;
          ((LTYPE *)s2)[-3] = r2;
          ((LTYPE *)s2)[-4] = r3;
          ((LTYPE *)s2)[-5] = r4;
          ((LTYPE *)s2)[-6] = r5;
          ((LTYPE *)s2)[-7] = r6;
          ((LTYPE *)s2) -= 8;
          ((LTYPE *)s2)[0] = r7;

#else

          //seems slower here: asm("dcbt 0,%0" : : "b" (((LTYPE *) s1) - 8));

          ((LTYPE *)s2)[-1] = ((LTYPE *)s1)[-1];
          ((LTYPE *)s2)[-2] = ((LTYPE *)s1)[-2];
          ((LTYPE *)s2)[-3] = ((LTYPE *)s1)[-3];
          ((LTYPE *)s2)[-4] = ((LTYPE *)s1)[-4];

          ((LTYPE *)s2)[-5] = ((LTYPE *)s1)[-5];
          ((LTYPE *)s2)[-6] = ((LTYPE *)s1)[-6];
          ((LTYPE *)s2)[-7] = ((LTYPE *)s1)[-7];
          ((LTYPE *)s2)[-8] = ((LTYPE *)s1)[-8];

          ((LTYPE *)s2) -= 8;
          ((LTYPE *)s1) -= 8;

#endif

#else

          *--((LTYPE *)s2) = *--((LTYPE *)s1);
          *--((LTYPE *)s2) = *--((LTYPE *)s1);
          *--((LTYPE *)s2) = *--((LTYPE *)s1);
          *--((LTYPE *)s2) = *--((LTYPE *)s1);

          *--((LTYPE *)s2) = *--((LTYPE *)s1);
          *--((LTYPE *)s2) = *--((LTYPE *)s1);
          *--((LTYPE *)s2) = *--((LTYPE *)s1);
          *--((LTYPE *)s2) = *--((LTYPE *)s1);

#endif
        } while (--o);
      }
      while (m--)
        *--((LTYPE *)s2) = *--((LTYPE *)s1);
    }
    while (n--)
      *--((u8 *)s2) = *--((u8 *)s1);
  }
}

#else

void  bcopy(const void *s1, void *s2, size_t n)
{
  size_t m;
  if(!n)
    return;
  if(s2<s1)
  { if((long)s1&1)
    { *((char *)s2)++=*((char *)s1)++;
      n--; }
    if(!((long)s2&1))
    { m=n/sizeof(long);
      n&=sizeof(long)-1;
      for(;m;m--)
	*((long *)s2)++=*((long *)s1)++;
    }
    for(;n;n--)
      *((char *)s2)++=*((char *)s1)++;
  }else
  { (char *)s1+=n;
    (char *)s2+=n;
    if((long)s1&1)
    { *--((char *)s2)=*--((char *)s1);
      n--; }
    if(!((long)s2&1))
    { m=n/sizeof(long);
      n&=sizeof(long)-1;
      for(;m;m--)
	*--((long *)s2)=*--((long *)s1);
    }
    for(;n;n--)
      *--((char *)s2)=*--((char *)s1);
  }
}

#endif
#else
#include "defs.h"

/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *      - unroll the longword copy loop (might not be good for a 68020)
 *      - longword align when possible (only on the 68020)
 *      - use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(bcopy)
asm("
	movl    sp@(12),d1      /* check count */
	jle     bcdone_bcopy    /* <= 0, don't do anything */
	movl    sp@(4),a0       /* src address */
	movl    sp@(8),a1       /* dest address */
	cmpl    a1,a0           /* src after dest? */
	jlt     bcback          /* yes, must copy backwards */
	movl    a0,d0
	btst    #0,d0           /* src address odd? */
	jeq     bcfeven         /* no, skip alignment */
	movb    a0@+,a1@+       /* yes, copy a byte */
	subql   #1,d1           /* adjust count */
	jeq     bcdone_bcopy    /* count 0, all done  */
bcfeven:
	movl    a1,d0
	btst    #0,d0           /* dest address odd? */
	jne     bcfbloop        /* yes, no hope for alignment, copy bytes */
	movl    d1,d0           /* no, both even */
	lsrl    #2,d0           /* convert count to longword count */
	jeq     bcfbloop        /* count 0, skip longword loop */
bcflloop:
	movl    a0@+,a1@+       /* copy a longword */
	subql   #1,d0           /* adjust count */
	jne     bcflloop        /* still more, keep copying */
	andl    #3,d1           /* what remains */
	jeq     bcdone_bcopy    /* nothing, all done */
bcfbloop:
	movb    a0@+,a1@+       /* copy a byte */
	subql   #1,d1           /* adjust count */
	jne     bcfbloop        /* still more, keep going */
bcdone_bcopy:
	rts
bcback:
	addl    d1,a0           /* src pointer to end */
	addl    d1,a1           /* dest pointer to end */
	movl    a0,d0
	btst    #0,d0           /* src address odd? */
	jeq     bcbeven         /* no, skip alignment */
	movb    a0@-,a1@-       /* yes, copy a byte */
	subql   #1,d1           /* adjust count */
	jeq     bcdone_bcopy    /* count 0, all done  */
bcbeven:
	movl    a1,d0
	btst    #0,d0           /* dest address odd? */
	jne     bcbbloop        /* yes, no hope for alignment, copy bytes */
	movl    d1,d0           /* no, both even */
	lsrl    #2,d0           /* convert count to longword count */
	jeq     bcbbloop        /* count 0, skip longword loop */
bcblloop:
	movl    a0@-,a1@-       /* copy a longword */
	subql   #1,d0           /* adjust count */
	jne     bcblloop        /* still more, keep copying */
	andl    #3,d1           /* what remains */
	jeq     bcdone_bcopy    /* nothing, all done */
bcbbloop:
	movb    a0@-,a1@-       /* copy a byte */
	subql   #1,d1           /* adjust count */
	jne     bcbbloop        /* still more, keep going */
	rts
");
#endif
