#ifndef _HEADERS_POOL_H
#define _HEADERS_POOL_H

#include <exec/types.h>

#if defined(__GNUC__)
#define ASM
#define REG(reg,arg) arg __asm(#reg)
#else
#define ASM __asm
#define REG(reg,arg) register __##reg arg
#endif

#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>

/*
**     our PRIVATE! memory pool structure 
** (_NOT_ compatible with original amiga.lib!)
*/

typedef struct Pool {
  struct MinList PuddleList;
  ULONG MemoryFlags;
  ULONG PuddleSize;
  ULONG ThreshSize;
} POOL;

/*
** required prototypes
*/

APTR _AsmCreatePool(ULONG,ULONG,ULONG,APTR);
APTR _AsmAllocPooled(POOL *,ULONG,APTR);
VOID _AsmFreePooled(POOL *,APTR,ULONG,APTR);
VOID _AsmDeletePool(POOL *,APTR);

#endif /* _HEADERS_POOL_H */
