#include "pool.h"

VOID LibFreePooled(APTR poolHeader, APTR memory, ULONG memSize)
{
  _AsmFreePooled(poolHeader,memory,memSize,*(APTR *)4L);
}
