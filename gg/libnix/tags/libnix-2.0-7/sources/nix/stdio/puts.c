#include <stdio.h>

int puts(const char *s)
{
  while(*s)
    if(fputc(*s++,stdout)==EOF)
      return EOF;
  if(fputc('\n',stdout)==EOF)
    return EOF;
  return '\n';
}
