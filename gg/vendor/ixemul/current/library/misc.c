/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *  Portions Copyright (C) 1996 Jeff Shepherd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Miscellaneous functions */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdlib.h>

int
getpid(void)
{
  return (int)SysBase->ThisTask;
}

int
getppid(void)
{
  usetup;
  
  return (int)u.p_pptr;
}

int
setpgid(int pid, int pgrp)
{
  usetup;

  if (pid)
      getuser(pid)->p_pgrp = pgrp;
  else
      u.p_pgrp = pgrp;
  return 0;
}

int
setpgrp(int pid, int pgrp)
{
  return setpgid(pid, pgrp);
}

pid_t
getpgrp(void)
{
  usetup;
  return u.p_pgrp;
}

pid_t
setsid(void)
{
  struct session *s;
  usetup;

  if (u.u_session && u.u_session->s_count <= 1)
    {
      errno = EPERM;
      return (pid_t)-1;
    }

  s = kmalloc(sizeof(struct session));
  if (s == NULL)
    {
      errno = ENOMEM;
      return (pid_t)-1;
    }
  if (u.u_session)
    u.u_session->s_count--;
  u.u_session = s;
  s->s_count = 1;
  s->pgrp = u.p_pgrp = getpid();

  return u.p_pgrp;
}

int
getrusage(int who, struct rusage *rusage)
{
  usetup;

  if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  *rusage = (who == RUSAGE_SELF) ? u.u_ru : u.u_cru;
  return 0;
}

char hostname[64] = "localhost";

int
gethostname(char *name, int namelen)
{
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_gethostname, name, namelen);
  strncpy (name, hostname, namelen);
  return 0;
}

int
sethostname(char *name, int namelen)
{
  int len;
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_sethostname, name, namelen);

  len = namelen < sizeof (hostname) - 1 ? namelen : sizeof (hostname) - 1;
  strncpy (hostname, name, len);
  hostname[len] = 0;
  return 0;
}

/* not really useful.. but it's there ;-)) */
int
getpagesize(void)
{
  return 2048;
}

void sync (void)
{
  usetup;
  /* could probably walk the entire file table and fsync each, but
     this is too much of a nuisance ;-)) */
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
}

int
fork (void)
{
  usetup;
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

int
mkfifo (const char *path, mode_t mode)
{
  usetup;
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

int
mknod (const char *path, mode_t mode, dev_t dev)
{
  usetup;
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

#if 0 //def NATIVE_MORPHOS
asm("
	.section \".text\"
	.globl	__flush_cache
	.type	__flush_cache,@function
__flush_cache:
	cmpwi	0,4,0
	addi	4,4,31
	mr	5,3
	srwi    4,4,5
	mtctr	4
	beqlr
l1:
	dcbf	0,3	/* flush data cache lines */
	addi	3,3,32
	bdnz	l1

	sync
	mtctr	4
l2:
	icbi	0,5	/* invalidate insn cache lines */
	addi	5,5,32
	bdnz	l2

	sync
	isync
	blr
__end_flush_cache:
	.size	__flush_cache,__end_flush_cache-__flush_cache
");

#else
void __flush_cache(void *addr, int len)
{
#ifdef NATIVE_MORPHOS
  CacheFlushDataInstArea(addr, len);
#else
  CacheClearE(addr, len, CACRF_ClearI | CACRF_ClearD);
#endif
}
#endif


#if 0 //def NATIVE_MORPHOS
asm("
	.section \".text\"
	.globl	ix_flush_insn_cache
	.type	ix_flush_insn_cache,@function
ix_flush_insn_cache:
	cmpwi	0,4,0
	addi	4,4,31
	mr	5,3
	srwi    4,4,5
	mtctr	4
	beqlr
l3:
	dcbst	0,3	/* store dirty data cache lines */
	addi	3,3,32
	bdnz	l3

	sync
	mtctr	4
l4:
	icbi	0,5	/* invalidate insn cache lines */
	addi	5,5,32
	bdnz	l4

	sync
        isync
	blr
__end_ix_flush_insn_cache:
	.size	ix_flush_insn_cache,__end_ix_flush_insn_cache-ix_flush_insn_cache
");

#else
void ix_flush_insn_cache(void *addr, int len)
{
#ifdef NATIVE_MORPHOS
  CacheInvalidInstArea(addr, len);
#else
  CacheClearE(addr, len, CACRF_ClearI);
#endif
}
#endif

#if 0 //def NATIVE_MORPHOS
asm("
	.section \".text\"
	.globl	ix_flush_data_cache
	.type	ix_flush_data_cache,@function
ix_flush_data_cache:
	cmpwi	0,4,0
	addi	4,4,31
	mr	5,3
	srwi    4,4,5
	mtctr	4
	beqlr
l5:
	dcbf    0,3	/* flush data cache lines */
	addi	3,3,32
	bdnz	l5

	sync
	blr
__end_ix_flush_data_cache:
	.size	ix_flush_data_cache,__end_ix_flush_data_cache-ix_flush_data_cache
");

#else
void ix_flush_data_cache(void *addr, int len)
{
#ifdef NATIVE_MORPHOS
  CacheFlushDataArea(addr, len);
#else
  CacheClearE(addr, len, CACRF_ClearD);
#endif
}
#endif

void ix_flush_caches(void)
{
  CacheClearU();
}
