#include <stdio.h>
#include <unistd.h>
#include "strsup.h"

int fputs(const char *s,FILE *stream)
{
  if (*s)
  { if((stream->flags&(__SNBF|__SERR))!=__SNBF)
    { do
      { if(fputc(*s++,stream)==EOF)
          return EOF;
      }while(*s);
    }else if(write(stream->file,s,strlen(s))==EOF)
    { stream->flags|=__SERR;
      return EOF;
    }
  }
  return 0;
}
