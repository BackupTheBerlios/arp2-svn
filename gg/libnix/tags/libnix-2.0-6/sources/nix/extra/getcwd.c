#include <proto/dos.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

extern void __seterrno();

char *getcwd(char *buf,size_t size)
{
  if (buf!=NULL || (buf=(char *)malloc(size))!=NULL)
  {
    BPTR dir_lock;

    if((dir_lock = Lock("", SHARED_LOCK)))
    {
      NameFromLock(dir_lock, buf, size);
      UnLock(dir_lock);
    }
    else
    {
      __seterrno(); buf=NULL;
    }
  }
  else
    errno=ENOMEM;
  return buf;
}
