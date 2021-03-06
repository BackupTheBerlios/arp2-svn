/*      $NetBSD: err.c,v 1.12 1995/02/25 17:19:26 cgd Exp $     */

/*-
 * Copyright (c) 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)err.c       8.1 (Berkeley) 6/4/93";
#else
static char rcsid[] = "$NetBSD: err.c,v 1.12 1995/02/25 17:19:26 cgd Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

extern char *__progname;                /* Program name, from crt0. */

__dead void
_verr(eval, fmt, ap)
	int eval;
	const char *fmt;
	va_list ap;
{
	int sverrno;

	sverrno = errno;
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(sverrno));
	exit(eval);
}

__dead void
#ifdef __STDC__
_err(int eval, const char *fmt, ...)
#else
_err(va_alist)
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	int eval;
	const char *fmt;

	va_start(ap);
	eval = va_arg(ap, int);
	fmt = va_arg(ap, const char *);
#endif
	_verr(eval, fmt, ap);
	va_end(ap);
}

__dead void
_verrx(eval, fmt, ap)
	int eval;
	const char *fmt;
	va_list ap;
{
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fprintf(stderr, "\n");
	exit(eval);
}

__dead void
#if __STDC__
_errx(int eval, const char *fmt, ...)
#else
_errx(va_alist)
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	int eval;
	const char *fmt;

	va_start(ap);
	eval = va_arg(ap, int);
	fmt = va_arg(ap, const char *);
#endif
	_verrx(eval, fmt, ap);
	va_end(ap);
}


void
_vwarn(fmt, ap)
	const char *fmt;
	va_list ap;
{
	int sverrno;

	sverrno = errno;
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(sverrno));
}

void
#if __STDC__
_warn(const char *fmt, ...)
#else
_warn(va_alist)
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	const char *fmt;

	va_start(ap);
	fmt = va_arg(ap, const char *);
#endif
	_vwarn(fmt, ap);
	va_end(ap);
}

void
_vwarnx(fmt, ap)
	const char *fmt;
	va_list ap;
{
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fprintf(stderr, "\n");
}

void
#ifdef __STDC__
_warnx(const char *fmt, ...)
#else
_warnx(va_alist)
	va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	const char *fmt;

	va_start(ap);
	fmt = va_arg(ap, const char *);
#endif
	_vwarnx(fmt, ap);
	va_end(ap);
}
