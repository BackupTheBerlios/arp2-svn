#include <stdio.h>
#include <errno.h>
//#include <dos/dos.h>
//#include <proto/dos.h>
//#include <syscalls.h>
#include <powerup/ppcproto/dos.h>

extern void __seterrno(void);
extern char *__amigapath(const char *path);

int remove(const char *filename)
{ 
#ifdef IXPATHS
  if((filename=__amigapath(filename))==NULL)
    return -1;
#endif

  if(DeleteFile((char *)filename))
    return 0;
  else
  { __seterrno();
    return -1; }
}

int unlink(const char *s)
{
	return remove(s);
}
