#ifndef mc68000

#include <string.h>

char *strncpy(char *s1,const char *s2,size_t n)
{
  if (n != 0)
  {
    char *s=s1;

    while ((*s++=*s2++) && (--n != 0))
      ;
    if (n != 0)
    {
      do
        *s++=0;
      while (--n != 0);
    }
  }
  return s1;
}

#else

asm("
	.globl	_strncpy
_strncpy:
	moveml	sp@(4:W),d0/a0
	movel	d0,a1
	movel	sp@(12:W),d1
L4:	subql	#1,d1
	bcs	L1
	moveb	a0@+,a1@+
	bne	L4
	jra	L2
L3:	clrb	a1@+
L2:	subql	#1,d1
	bcc	L3
L1:	rts
");

#endif
