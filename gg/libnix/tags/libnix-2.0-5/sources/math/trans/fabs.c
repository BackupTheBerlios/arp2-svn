
#if defined( __mc68000__ )

#include <proto/mathieeedoubbas.h>

double fabs(double x)
{ return IEEEDPAbs(x); }

#else

/* @(#)s_fabs.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: s_fabs.c,v 1.9 1999/07/02 15:37:42 simonb Exp $");
#endif

/*
 * fabs(x) returns the absolute value of x.
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	double fabs(double x)
#else
	double fabs(x)
	double x;
#endif
{
	u_int32_t high;
	GET_HIGH_WORD(high,x);
	SET_HIGH_WORD(x,high&0x7fffffff);
        return x;
}

#endif
