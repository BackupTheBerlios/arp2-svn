#ifndef _STRSUP_H /* don't include this twice */

#define _STRSUP_H

#if defined(__GNUC__) && defined(__OPTIMIZE__)

extern __inline char *strcat(char *s1,const char *s2)
{ 
  char *s=s1;

  while(*s++)
    ;
  --s;
  while((*s++=*s2++))
    ;
  return s1;
}

extern __inline char *strcpy(char *s1,const char *s2)
{ char *s=s1;
  do
    *s++=*s2;
  while(*s2++!='\0');
  return s1;
}

extern __inline size_t strlen(const char *string)
{
  const char *s=string;

  while(*s++)
    ;
  return ~(string-s);
}

extern __inline size_t strlen_plus_one(const char *string)
{
  const char *s=string;

  while(*s++)
    ;
  return (s-string);
}

#else

#define strlen_plus_one(s) strlen(s)+1 /* not gnu :( */

#endif
#endif
