#include <debuglib.h>
#include <stdlib.h>
#include <powerup/gcclib/powerup_protos.h>

extern void *pool;

void free(void *ptr)
{
  if(ptr==NULL) /* What does that mean ????? */
  {
#ifdef DEBUG_LIB
    FATALERROR("NULL pointer free'd\n");
#endif
    return;
  }
  if(pool!=0l)
	PPCFreeVecPooled(pool,ptr);
}
