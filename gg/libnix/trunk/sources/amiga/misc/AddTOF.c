#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <graphics/graphint.h>
#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/exec.h>

STATIC int stub(struct Isrvstr *intr asm("a1"))
{
  (*intr->ccode)(intr->Carg); return 0;
}

VOID AddTOF(struct Isrvstr *intr,LONG (*code)(APTR args),APTR a)
{ APTR SysBase = *(APTR *)4L;

  intr->Iptr  = intr;
  intr->code  = (int (*)())stub;
  intr->ccode = (int (*)())code;
  intr->Carg  = (int)a;
  AddIntServer(INTB_VERTB,(struct Interrupt *)intr);
}

VOID RemTOF(struct Isrvstr *intr)
{ APTR SysBase = *(APTR *)4L;

  RemIntServer(INTB_VERTB,(struct Interrupt *)intr);
}
