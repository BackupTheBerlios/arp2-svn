
#include <exec/io.h>
#include <exec/devices.h>

#if defined( __mc68000__ )

void BeginIO(struct IORequest *iorequest)
{ register struct IORequest *a1 asm("a1")=iorequest;
  register struct Device    *a6 asm("a6")=iorequest->io_Device;
  __asm __volatile ("jsr a6@(-30:W)"::"r"(a1),"r"(a6):"a0","a1","d0","d1");
}

#else

#include <inline/macros.h>

#ifdef LP1NR

void
BeginIO( struct IORequest *iorequest )
{
#if defined(__MORPHOS__)
  LP1NR( 30, BeginIO, struct IORequest *, iorequest, a1, ,iorequest->io_Device,
	 IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0);
#else
  LP1NR( 30, BeginIO, struct IORequest *, iorequest, a1, ,iorequest->io_Device );
#endif
}

#else
# error No BeginIO implementation
#endif 

#endif
