/* 10-Apr-94 bug fix M. Fleischer
 * 11-Apr-94 bug fix & readjastment G. Nikl
 * 14-Apr-94 readjustment M. Fleischer
 * 24-Apr-94 cleanup for malloc changed
 */

#include <stdlib.h>
#include <exec/memory.h>
#include <powerup/gcclib/powerup_protos.h>
#include <stabs.h>

void *pool=0l;

void *malloc(size_t size)
{
	if(pool==0l)
		if(!(pool=PPCCreatePool(MEMF_ANY,4096,2048)))
			exit(-1);
	return(PPCAllocVecPooled(pool,size));
}

void __exitmalloc(void)
{
	if(pool)
		PPCDeletePool(pool);
}

ADD2EXIT(__exitmalloc,-50);
