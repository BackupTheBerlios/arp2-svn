
#if defined( __mc68000__ )

asm("
		.text

		.globl	_HookEntry

_HookEntry:	movel	a1,sp@-
		movel	a2,sp@-
		movel	a0,sp@-
		movel	a0@(12:W),a0
		jsr	a0@
		lea	sp@(12:W),sp
		rts
");

#elif defined( __i386__ ) && defined( __amithlon__ ) && defined( __BIG_ENDIAN__ )

__asm("
	.text
	.balign	4
	.type	_HookEntry,@function
_HookEntry:
	movl	%ebp,%eax  /* A1 */
	bswap	%eax
	push	%eax

	movl	%esi,%eax  /* A2 */
	bswap	%eax
	push	%eax

	movl	%ebx,%eax  /* A0 */
	bswap	%eax
	push	%eax
  
	mov	12(%ebx),%eax
	bswap	%eax
	call	*%eax
        add     $12,%esp
        ret

.L_HookEntry:
	.size	_HookEntry,.L_HookEntry-_HookEntry
	
	.globl	HookEntry
	.type	HookEntry,@function
HookEntry = _HookEntry+1
");

#else
# warning No HookEntry implementation
#endif
