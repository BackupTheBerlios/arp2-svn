#include <string.h>

void *memchr(const void *s,int c,size_t n)
{
  if (n != 0)
  {
    unsigned char *p=(unsigned char *)s;
    do
    {
      if (*p++==(unsigned char)c)
        return --p;
    }while(--n != 0);
  }
  return (void *)n;
}
