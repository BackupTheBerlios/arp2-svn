
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

static inline double floor(double x)
{ return IEEEDPFloor(x); }

static inline double ceil(double x)
{ return IEEEDPCeil(x); }

double fmod(double x,double y)
{
  double a=x/y;
  if(a>=0)
    return x-y*floor(a);
  else
    return x-y*ceil(a);
}

#elif defined( __i386__ )

__asm("
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include <abi.h>


ENTRY(__ieee754_fmod)
	XMM_TWO_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_TWO
	fldl	ARG_DOUBLE_ONE
1:	fprem
	fstsw	%ax
	btw	$10,%ax
	jc	1b
	fstp	%st(1)
	XMM_DOUBLE_EPILOGUE
	ret

	WEAK_ALIAS(fmod,__ieee754_fmod)
");
#else

/* @(#)e_fmod.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: e_fmod.c,v 1.11 2002/05/26 22:01:49 wiz Exp $");
#endif

/*
 * __ieee754_fmod(x,y)
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */

#include "math.h"
#include "math_private.h"

static const double one = 1.0, Zero[] = {0.0, -0.0,};

double
__ieee754_fmod(double x, double y)
{
	int32_t n,hx,hy,hz,ix,iy,sx,i;
	u_int32_t lx,ly,lz;

	EXTRACT_WORDS(hx,lx,x);
	EXTRACT_WORDS(hy,ly,y);
	sx = hx&0x80000000;		/* sign of x */
	hx ^=sx;		/* |x| */
	hy &= 0x7fffffff;	/* |y| */

    /* purge off exception values */
	if((hy|ly)==0||(hx>=0x7ff00000)||	/* y=0,or x not finite */
	  ((hy|((ly|-ly)>>31))>0x7ff00000))	/* or y is NaN */
	    return (x*y)/(x*y);
	if(hx<=hy) {
	    if((hx<hy)||(lx<ly)) return x;	/* |x|<|y| return x */
	    if(lx==ly)
		return Zero[(u_int32_t)sx>>31];	/* |x|=|y| return x*0*/
	}

    /* determine ix = ilogb(x) */
	if(hx<0x00100000) {	/* subnormal x */
	    if(hx==0) {
		for (ix = -1043, i=lx; i>0; i<<=1) ix -=1;
	    } else {
		for (ix = -1022,i=(hx<<11); i>0; i<<=1) ix -=1;
	    }
	} else ix = (hx>>20)-1023;

    /* determine iy = ilogb(y) */
	if(hy<0x00100000) {	/* subnormal y */
	    if(hy==0) {
		for (iy = -1043, i=ly; i>0; i<<=1) iy -=1;
	    } else {
		for (iy = -1022,i=(hy<<11); i>0; i<<=1) iy -=1;
	    }
	} else iy = (hy>>20)-1023;

    /* set up {hx,lx}, {hy,ly} and align y to x */
	if(ix >= -1022)
	    hx = 0x00100000|(0x000fffff&hx);
	else {		/* subnormal x, shift x to normal */
	    n = -1022-ix;
	    if(n<=31) {
	        hx = (hx<<n)|(lx>>(32-n));
	        lx <<= n;
	    } else {
		hx = lx<<(n-32);
		lx = 0;
	    }
	}
	if(iy >= -1022)
	    hy = 0x00100000|(0x000fffff&hy);
	else {		/* subnormal y, shift y to normal */
	    n = -1022-iy;
	    if(n<=31) {
	        hy = (hy<<n)|(ly>>(32-n));
	        ly <<= n;
	    } else {
		hy = ly<<(n-32);
		ly = 0;
	    }
	}

    /* fix point fmod */
	n = ix - iy;
	while(n--) {
	    hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	    if(hz<0){hx = hx+hx+(lx>>31); lx = lx+lx;}
	    else {
	    	if((hz|lz)==0) 		/* return sign(x)*0 */
		    return Zero[(u_int32_t)sx>>31];
	    	hx = hz+hz+(lz>>31); lx = lz+lz;
	    }
	}
	hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	if(hz>=0) {hx=hz;lx=lz;}

    /* convert back to floating value and restore the sign */
	if((hx|lx)==0) 			/* return sign(x)*0 */
	    return Zero[(u_int32_t)sx>>31];
	while(hx<0x00100000) {		/* normalize x */
	    hx = hx+hx+(lx>>31); lx = lx+lx;
	    iy -= 1;
	}
	if(iy>= -1022) {	/* normalize output */
	    hx = ((hx-0x00100000)|((iy+1023)<<20));
	    INSERT_WORDS(x,hx|sx,lx);
	} else {		/* subnormal output */
	    n = -1022 - iy;
	    if(n<=20) {
		lx = (lx>>n)|((u_int32_t)hx<<(32-n));
		hx >>= n;
	    } else if (n<=31) {
		lx = (hx<<(32-n))|(lx>>n); hx = sx;
	    } else {
		lx = hx>>(n-32); hx = sx;
	    }
	    INSERT_WORDS(x,hx|sx,lx);
	    x *= one;		/* create necessary signal */
	}
	return x;		/* exact output */
}

#include "stabs.h"
WEAK_ALIAS(fmod,__ieee754_fmod);

#endif
