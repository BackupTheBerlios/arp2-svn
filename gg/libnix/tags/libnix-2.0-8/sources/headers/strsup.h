#ifndef _STRSUP_H
#define _STRSUP_H

#if defined(__mc68000__)

#include <sys/types.h>

#ifdef __OPTIMIZE__

extern __inline__ void *memcpy(void *s1,const void *s2,size_t n)
{
  #ifndef __NOLIBBASE__
  extern struct ExecBase *SysBase;
  #endif
  register char *a6 __asm("a6") = (char *)SysBase;
  register const void *a0 __asm("a0") = s2;
  register const void *a1 __asm("a1") = s1;
  register size_t d0 __asm("d0") = n;
  __asm __volatile ("jsr a6@(-0x270)"
  : /* no output */
  : "r" (a6), "r" (a0), "r" (a1), "r" (d0)
  : "a0","a1","d0","d1", "memory");
  return s1;
}

extern __inline__ void *memmove(void *s1,const void *s2,size_t n)
{ extern void bcopy();

  bcopy(s2,s1,n); return s1;
}

extern __inline__ void *memset(void *s,int c,size_t n)
{ 
  if (n) {
    unsigned char *p=(unsigned char *)s;
    do;while(*p++=c,--n);
  }
  return s;
}

extern __inline__ int memcmp(const void *s1,const void *s2,size_t n)
{ const unsigned char *p1=(const unsigned char *)s1,*p2=(const unsigned char *)s2;
  unsigned long r,c;

  if ((r=n))
    do;while(r=*p1++,c=*p2++,!(r-=c) && --n);
  return r;
}

extern __inline__ void *memchr(const void *s,int c,size_t n)
{
  if (n) {
    unsigned char *p=(unsigned char *)s;
    do {
      if (*p++==(unsigned char)c)
        return --p;
    } while(--n);
  }
  return (void *)n;
}

extern __inline__ size_t strlen(const char *string)
{ const char *s=string;

  do;while(*s++); return ~(string-s);
}

extern __inline__ char *strcpy(char *s1,const char *s2)
{ char *s=s1;
#if 0
  do;while(*s1++=*s2,*s2++!='\0');
#else
  do;while((*s1++=*s2++));
#endif
  return s;
}

extern __inline__ char *strncpy(char *s1,const char *s2,size_t n)
{
  if (n) {
    char *s=s1;
    do;while((*s++=*s2++) && --n);
    if (n)
      while(--n) *s++=0;
  }
  return s1;
}

extern __inline__ char *strcat(char *s1,const char *s2)
{ char *s=s1;

  do;while(*s++); --s; do;while((*s++=*s2++)); return s1;
}

extern __inline__ char *strncat(char *s1,const char *s2,size_t n)
{
  if (n) {
    char *s=s1;
    do;while(*s++); --s;
    for(;;) {
      if (!(*s++=*s2++))
        return s1;
      if (!--n) {
        *s=0; return s1;
      }
    }
  }
  return s1;
} 

extern __inline__ int strcmp(const char *s1,const char *s2)
{ unsigned char *p1=(unsigned char *)s1, *p2=(unsigned char *)s2;
  unsigned long r,c;

  do;while(r=*p1++,c=*p2++,!(r-=c) && (char)c); return r;
}

extern __inline__ int strncmp(const char *s1,const char *s2,size_t n)
{ unsigned char *p1=(unsigned char *)s1,*p2=(unsigned char *)s2;
  unsigned long r,c;

  if ((r=n))
    do;while(r=*p1++,c=*p2++,!(r-=c) && (char)c && --n);
  return r;
}
#if 0
extern __inline__ char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!*s++)
      return (char *)0;
  return (char *)s;
}
#endif
extern __inline__ char *strupr(char *s)
{ unsigned char *s1=(unsigned char *)s;

  while(*s1) {
    if ((*s1>('a'-1)) && (*s1<('z'+1)))
      *s1-='a'-'A';
    s1++;
  }
  return s;
}

extern __inline__ char *strlwr(char *s)
{ unsigned char *s1=(unsigned char *)s;

  while(*s1) {
    if ((*s1>('A'-1)) && (*s1<('Z'+1)))
      *s1+='a'-'A';
    s1++;
  }
  return s;
}

extern __inline__ char *stpcpy(char *dst,const char *src)
{
  do;while((*dst++=*src++)); return(--dst);
}

#endif /* __OPTIMIZE__ */

static  __inline__ size_t strlen_plus_one(const char *string)
{ const char *s=string;

  do;while(*s++); return (s-string);
}

#endif

#endif /* _STRSUP_H */
