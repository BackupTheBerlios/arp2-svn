#include <ctype.h>
//#include <proto/utility.h>
#include <stabs.h>

int stricmp(const char *s1,const char *s2)
{
#ifndef OS_20_ONLY

  unsigned char c1,c2;
  int r;

  for(;;)
  {
    c1=*s1++;
    if (isupper(c1))
      c1+='a'-'A';
    c2=*s2;
    if (isupper(c2))
      c2+='a'-'A';
    if ((r=(char)c1-(char)c2)!=0)
      break;
    if (!*s2++)
      break;
  }
  return r;

#else

  return Stricmp((char *)s1,(char *)s2);

#endif
}
int strcasecmp(const char *s1,const char *s2)
{
#ifndef OS_20_ONLY

  unsigned char c1,c2;
  int r;

  for(;;)
  {
    c1=*s1++;
    if (isupper(c1))
      c1+='a'-'A';
    c2=*s2;
    if (isupper(c2))
      c2+='a'-'A';
    if ((r=(char)c1-(char)c2)!=0)
      break;
    if (!*s2++)
      break;
  }
  return r;

#else

  return Stricmp((char *)s1,(char *)s2);

#endif
}

//ALIAS(strcasecmp,stricmp);
