#include <stdio.h>
#include <string.h>
#include <stabs.h>

#ifndef mc68000

char *strrchr(const char *s,int c)
{ char *c1=NULL;
  do
    if(*s==(char)c)
      c1=(char *)s;
  while(*s++!='\0');
  return c1;
}

#else

asm("
	.globl	_strrchr
_strrchr:
	movel	sp@(4:W),a1
	movel	sp@(8:W),d1
	moveq	#0,d0
L2:	cmpb	a1@,d1
	jne	L1
	movel	a1,d0
L1:	tstb	a1@+
	jne	L2
	rts
");

#endif

