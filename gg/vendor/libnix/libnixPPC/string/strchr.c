#include <stdio.h>
#include <string.h>
#include <stabs.h>

#ifndef mc68000

char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!(*s++))
    {
      s=NULL;
      break;
    }
  return (char *)s;
}

#else

asm("
	.globl	_strchr
_strchr:
	movel	sp@(4:W),a0
	movel	sp@(8:W),d0
L2:	cmpb	a0@,d0
	jeq	L1
	tstb	a0@+
	jne	L2
	subal	a0,a0
L1:	movel	a0,d0
	rts
");

#endif

