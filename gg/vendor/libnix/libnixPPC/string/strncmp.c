#include <string.h>

int strncmp(const char *s1,const char *s2,size_t n)
{ 
  const unsigned char *p1=s1,*p2=s2;
  unsigned long r,c;

  if((r=n))
  {
    c=0;  
    do
    {
      r=*p1++;
      ((unsigned char)c)=*p2++;
    } while (!(r-=c) && (unsigned char)c && --n);
  }
  return r;
}
