
#if defined( __mc68000__ )

asm("
		.text

		.globl	_CallHook
		.globl	_CallHookA

_CallHook:	lea	sp@(12:W),a1
		jra	L_CallHook

_CallHookA:	movel	sp@(12:W),a1

L_CallHook:	movel	a6,sp@-
		movel	a2,sp@-
		movel	sp@(12:W),a0
		movel	sp@(16:W),a2
		jbsr	L_callit
		movel	sp@+,a2
		movel	sp@+,a6
		rts

L_callit:	movel	a0@(8:W),sp@-
		rts
");

#elif defined( __i386__ ) && defined( __amithlon__ ) && defined( __BIG_ENDIAN__ )

#include <clib/alib_protos.h>

__asm__("
	.globl	CallHookA
	.type   CallHookA,@function
CallHookA:
        subl    $0x40,%esp
	movl	0x48(%esp),%eax
	bswap	%eax
	movl	%eax,0x28(%esp)      # obj -> a2
	movl	0x4c(%esp),%eax
	bswap	%eax
	movl	%eax,0x24(%esp)      # msg -> a1
	movl	0x44(%esp),%eax
	bswap	%eax
	movl	%eax,0x20(%esp)      # hook -> a0
	movl	8(%eax),%edx
	bswap	%edx                 # h_Entry
        movl    %esp,%eax            # struct Regs*
	call	_CallFunc68k
        addl    $0x40,%esp
	ret
");	

ULONG
CallHook( struct Hook *hookPtr, Object *obj, ... )
  
{
  return CallHookA( hookPtr, obj, (APTR) ( &obj + 1 ) );
};

#else
# warning No CallHook implementation
#endif
