#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <graphics/graphint.h>
#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/exec.h>

#if defined( __mc68000__ )

STATIC int stub(struct Isrvstr *intr asm("a1"))
{
  (*intr->ccode)(intr->Carg); return 0;
}

#elif defined( __i386__ )

STATIC UBYTE stub[] __attribute__ ((aligned (4))) =
{
  0x2F, 0x29, 0x00, 0x1A,      //       MOVE.L      ($001A,A1),-(A7)
  0x20, 0x29, 0x00, 0x16,      //       MOVEA.L     ($0016,A1),A0
  0x02, 0x00, 0x00, 0xFD,      //       ANDI.B      #$FD,D0
  0x52, 0x00,                  //       ADDQ.B      #1,D0
  0x20, 0x40,                  //       MOVEA.L     D0,A0
  0x4E, 0xD0                   //       JMP         (A0)
};

#else
# error Unsupported CPU
#endif

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
