/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
static char sccsid[] = "@(#)exec.c      5.9 (Berkeley) 6/17/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include "my_varargs.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

static char **
buildargv(ap, arg, envpp)
	my_va_list ap;
	const char *arg;
	char ***envpp;
{
	register size_t max, off;
	register char **argv = NULL;

	for (off = max = 0;; ++off) {
		if (off >= max) {
			max += 50;      /* Starts out at 0. */
			max *= 2;       /* Ramp up fast. */
			if (!(argv = realloc(argv, max * sizeof(char *))))
				return(NULL);
			if (off == 0) {
				argv[0] = (char *)arg;
				off = 1;
			}
		}
		if (!(argv[off] = my_va_arg(ap, char *)))
			break;
	}
	/* Get environment pointer if user supposed to provide one. */
	if (envpp)
		*envpp = my_va_arg(ap, char **);
	return(argv);
}

int vexecl(const char *name, const char *arg, my_va_list ap)
{
	usetup;
	int sverrno;
	char **argv;

	if ((argv = buildargv(ap, arg, (char ***)NULL)))
		(void)execve(name, argv, *u.u_environ);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return(-1);
}

int execl(const char *name, const char *arg, ...)
{
	int r;
	my_va_list ap;
	my_va_start(ap, arg);
	r = vexecl(name, arg, ap);
	my_va_end(ap);
	return r;
}

#ifdef NATIVE_MORPHOS

int _varargs68k_execl(const char *name, const char *arg, char *ap1)
{
	my_va_list ap;
	my_va_init_68k(ap, ap1);
	return vexecl(name, arg, ap);
}

asm("	.section \".text\"
	.type	_stk_execl,@function
	.globl	_stk_execl
_stk_execl:
	andi.	11,1,15
	mr	12,1
	bne-	.align_execl
	b	execl
.align_execl:
	addi	11,11,128
	mflr	0
	neg	11,11
	stw	0,4(1)
	stwux	1,1,11

	stw	5,16(1)
	stw	6,20(1)
	stw	7,24(1)
	stw	8,28(1)
	stw	9,32(1)
	stw	10,36(1)
	bc	4,6,.nofloat_execl
	stfd	1,40(1)
	stfd	2,48(1)
	stfd	3,56(1)
	stfd	4,64(1)
	stfd	5,72(1)
	stfd	6,80(1)
	stfd	7,88(1)
	stfd	8,96(1)
.nofloat_execl:

	addi	5,1,104
	lis	0,0x200
	addi	12,12,8
	addi	11,1,8
	stw	0,0(5)
	stw	12,4(5)
	stw	11,8(5)
	bl	vexecl
	lwz	1,0(1)
	lwz	0,4(1)
	mtlr	0
	blr
");
#endif

int vexecle(const char *name, const char *arg, my_va_list ap)
{
	usetup;
	int sverrno;
	char **argv, **envp;

	if ((argv = buildargv(ap, arg, &envp)))
		(void)execve(name, argv, envp);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return(-1);
}

int execle(const char *name, const char *arg, ...)
{
	int r;
	my_va_list ap;
	my_va_start(ap, arg);
	r = vexecle(name, arg, ap);
	my_va_end(ap);
	return r;
}

#ifdef NATIVE_MORPHOS

int _varargs68k_execle(const char *name, const char *arg, char *ap1)
{
	my_va_list ap;
	my_va_init_68k(ap, ap1);
	return vexecle(name, arg, ap);
}

asm("	.section \".text\"
	.type	_stk_execle,@function
	.globl	_stk_execle
_stk_execle:
	andi.	11,1,15
	mr	12,1
	bne-	.align_execle
	b	execle
.align_execle:
	addi	11,11,128
	mflr	0
	neg	11,11
	stw	0,4(1)
	stwux	1,1,11

	stw	5,16(1)
	stw	6,20(1)
	stw	7,24(1)
	stw	8,28(1)
	stw	9,32(1)
	stw	10,36(1)
	bc	4,6,.nofloat_execle
	stfd	1,40(1)
	stfd	2,48(1)
	stfd	3,56(1)
	stfd	4,64(1)
	stfd	5,72(1)
	stfd	6,80(1)
	stfd	7,88(1)
	stfd	8,96(1)
.nofloat_execle:

	addi	5,1,104
	lis	0,0x200
	addi	12,12,8
	addi	11,1,8
	stw	0,0(5)
	stw	12,4(5)
	stw	11,8(5)
	bl	vexecle
	lwz	1,0(1)
	lwz	0,4(1)
	mtlr	0
	blr
");
#endif

int vexeclp(const char *name, const char *arg, my_va_list ap)
{
	usetup;
	int sverrno;
	char **argv;

	if ((argv = buildargv(ap, arg, (char ***)NULL)))
		(void)execvp(name, argv);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return(-1);
}

int execlp(const char *name, const char *arg, ...)
{
	int r;
	my_va_list ap;
	my_va_start(ap, arg);
	r = vexeclp(name, arg, ap);
	my_va_end(ap);
	return r;
}

#ifdef NATIVE_MORPHOS

int _varargs68k_execlp(const char *name, const char *arg, char *ap1)
{
	my_va_list ap;
	my_va_init_68k(ap, ap1);
	return vexeclp(name, arg, ap);
}

asm("	.section \".text\"
	.type	_stk_execlp,@function
	.globl	_stk_execlp
_stk_execlp:
	andi.	11,1,15
	mr	12,1
	bne-	.align_execlp
	b	execlp
.align_execlp:
	addi	11,11,128
	mflr	0
	neg	11,11
	stw	0,4(1)
	stwux	1,1,11

	stw	5,16(1)
	stw	6,20(1)
	stw	7,24(1)
	stw	8,28(1)
	stw	9,32(1)
	stw	10,36(1)
	bc	4,6,.nofloat_execlp
	stfd	1,40(1)
	stfd	2,48(1)
	stfd	3,56(1)
	stfd	4,64(1)
	stfd	5,72(1)
	stfd	6,80(1)
	stfd	7,88(1)
	stfd	8,96(1)
.nofloat_execlp:

	addi	5,1,104
	lis	0,0x200
	addi	12,12,8
	addi	11,1,8
	stw	0,0(5)
	stw	12,4(5)
	stw	11,8(5)
	bl	vexeclp
	lwz	1,0(1)
	lwz	0,4(1)
	mtlr	0
	blr
");
#endif

int execv(const char *name, char * const *argv)
{
	usetup;

	execve(name, argv, *u.u_environ);
	return(-1);
}

int execvp(const char *name, char * const *argv)
{
	usetup;
	register int lp, ln;
	register char *p;
	int eacces, etxtbsy;
	char *bp, *cur, *path, buf[MAXPATHLEN];

	KPRINTF(("execvp(%s)\n", name));

	eacces = etxtbsy = 0;
	/* If it's an absolute or relative path name, it's easy. */
	if (index(name, '/') || index (name, ':')) {
		bp = (char *)name;
		cur = path = NULL;
		goto retry;
	}
	bp = buf;

	/* first try to call execve() without path lookup. execve() walks
	   a (possibly provided) CLI PATH, and also supports resident
	   programs. If this fails, we can still try to find the program
	   along the $PATH */
	execv (name, argv);
	/* if we get here, the execv() failed. So start magic ;-)) */

	KPRINTF(("execv failed\n"));

	/* only continue if execv didn't find the program. */
	if (errno != ENOENT)
	  return -1;

	KPRINTF(("searching path\n"));

	/* Get the path we're searching. */
	if (!(path = getenv("PATH")))
		path = _PATH_DEFPATH;
	cur = path = strdup(path);

	while ((p = strsep(&cur, ",:"))) {
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (!*p) {
			p = ".";
			lp = 1;
		} else
			lp = strlen(p);
		ln = strlen(name);

		/*
		 * If the path is too long complain.  This is a possible
		 * security issue; given a way to make the path too long
		 * the user may execute the wrong program.
		 */
		if (lp + ln + 2 > sizeof(buf)) {
			syscall(SYS_write, STDERR_FILENO, "execvp: ", 8);
			syscall(SYS_write, STDERR_FILENO, p, lp);
			syscall(SYS_write, STDERR_FILENO, ": path too long\n", 16);
			continue;
		}
		bcopy(p, buf, lp);
		buf[lp] = '/';
		bcopy(name, buf + lp + 1, ln);
		buf[lp + ln + 1] = '\0';
retry:          execve(bp, argv, *u.u_environ);
		switch(errno) {
		case EACCES:
			eacces = 1;
			break;
		case ENOENT:
			break;
		case ENOEXEC: {
			register size_t cnt;
			register char **ap;

			for (cnt = 0, ap = (char **)argv; *ap; ++ap, ++cnt);
			if ((ap = malloc((cnt + 2) * sizeof(char *)))) {
				bcopy(argv + 1, ap + 2, cnt * sizeof(char *));
				ap[0] = "sh";
				ap[1] = bp;
				execve(_PATH_BSHELL, ap, *u.u_environ);
				free(ap);
			}
			goto done;
		}
		case ETXTBSY:
			if (etxtbsy < 3)
				sleep(++etxtbsy);
			goto retry;
		default:
			goto done;
		}
	}
	if (eacces)
		errno = EACCES;
	else if (!errno)
		errno = ENOENT;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
done:   if (path)
		free(path);
	return(-1);
}
