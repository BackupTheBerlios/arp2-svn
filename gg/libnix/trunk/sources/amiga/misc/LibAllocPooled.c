#include "pool.h"

APTR LibAllocPooled(APTR poolHeader, ULONG memSize)
{
  return _AsmAllocPooled(poolHeader,memSize,*(APTR *)4L);
}
