#include "pool.h"

APTR LibCreatePool(ULONG requirements, ULONG puddleSize, ULONG threshSize)
{
  return _AsmCreatePool(requirements,puddleSize,threshSize,*(APTR *)4L);
}

VOID LibDeletePool(APTR poolHeader)
{
  _AsmDeletePool(poolHeader,*(APTR *)4L);
}
