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

#elif defined( __MORPHOS__ )

# include <emul/emulregs.h>

STATIC int _stub(void)
{
  struct Isrvstr* intr = (struct Isrvstr*) REG_A1;

  (*intr->ccode)(intr->Carg); return 0;
}

static struct EmulLibEntry __stub =
{
  TRAP_LIB, 0, (void (*)(void)) _stub
};

__asm__("stub=__stub");

int stub();

#elif defined( __i386__ ) && defined( __amithlon__ )

__asm__( "

	.balign	4
_stub:
	pushl	%ebp
	movl	0x1a(%ebp),%eax
	pushl	%eax
	movl	0x16(%ebp),%eax
	bswap	%eax
	call	*%eax
	popl	%eax
	xorl	%eax,%eax
	popl	%ebp
	ret

stub=_stub+1

");

int stub();

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
