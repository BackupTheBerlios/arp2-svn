#include <stdlib.h>
#include <exec/memory.h>
#include <powerup/gcclib/powerup_protos.h>
#include "stabs.h"

extern int    __argc; /* Defined in startup */
extern char **__argv;
extern char  *__commandline;
extern unsigned long __commandlen;

static char *cline=NULL; /* Copy of commandline */

/* This guarantees that this module gets linked in.
   If you replace this by an own reference called
   __nocommandline you get no commandline arguments */

void __nocommandline(void)
{
    char **av,*a,*cl=__commandline;
    size_t i=__commandlen;
    int ac;

    if(!(cline=(char *)PPCAllocVec(i+1,MEMF_ANY))) /* get buffer */
      exit(RETURN_FAIL);
  
    for(a=cline,ac=1;;) /* and parse commandline */
    {
      while(i&&(*cl==' '||*cl=='\t'||*cl=='\n'))
      { cl++;
        i--; }
      if(!i)
        break;
      if(*cl=='\"')
      {
        cl++;
        i--;
        while(i)
        {
          if(*cl=='\"')
          {
            cl++;
            i--;
            break;
          }
          if(*cl=='*')
          {
            cl++;
            i--;
            if(!i)
              break;
          }
          *a++=*cl++;
          i--;
        }
      }
      else
        while(i&&(*cl!=' '&&*cl!='\t'&&*cl!='\n'))
        { *a++=*cl++;
          i--; }
      *a++='\0';
      ac++;
    }
      /* NULL Terminated */
    if(!(__argv=av=(char **)PPCAllocVec(((__argc=ac-1)+1)*sizeof(char *),MEMF_ANY|MEMF_CLEAR)))
      exit(RETURN_FAIL);

    for(a=cline,i=1;i<ac;i++)
    { 
      av[i-1]=a;
      while(*a++)
        ; 
    }
}
  
void __exitcommandline(void)
{
  char *cl=cline;

    if(cl!=NULL)
    { char **av=__argv;

      if(av!=NULL)
      { 
        PPCFreeVec(av);
      }
      PPCFreeVec(cl);
    }
}
  
/* Add these two functions to the lists */
ADD2INIT(__nocommandline,-40);
ADD2EXIT(__exitcommandline,-40);
