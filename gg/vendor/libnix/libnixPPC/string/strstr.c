#include <stdio.h>
#include <string.h>

char *strstr(const char *s1,const char *s2)
{ const char *c1;
  const char *c2;
  do
  { c1=s1;
    c2=s2;
    while(*c1!='\0'&&*c1==*c2)
    { c1++;
      c2++; }
    if(*c2=='\0')
      return (char *)s1;
  }while(*s1++!='\0');
  return NULL;
}
      
