#include <stdlib.h>
#include <string.h>
#include "strsup.h"

char *strdup(const char *s)
{ char *s1=malloc(strlen(s)+1);
  if (s1)
    strcpy(s1,s);
  return s1;
}
