#include <string.h>

void *memcpy(void *s1,const void *s2,size_t n)
{
  bcopy(s2,s1,n); return s1;
}
