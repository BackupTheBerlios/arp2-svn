#include <stdlib.h>
#include <ctype.h>
//#include <proto/utility.h>
#include <stabs.h>

int strnicmp(unsigned char *s1,unsigned char *s2,size_t len)
{
#ifndef OS_20_ONLY

   unsigned char c1,c2;
   int r;

   r=0;
   if (len != 0)
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
       if (!--len)
         break;
     }
   return r;

#else

  return Strnicmp((char *)s1,(char *)s2,(long)len);

#endif
}
int strncasecmp(unsigned char *s1,unsigned char *s2,size_t len)
{
#ifndef OS_20_ONLY

   unsigned char c1,c2;
   int r;

   r=0;
   if (len != 0)
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
       if (!--len)
         break;
     }
   return r;

#else

  return Strnicmp((char *)s1,(char *)s2,(long)len);

#endif
}

//ALIAS(strncasecmp,strnicmp);
