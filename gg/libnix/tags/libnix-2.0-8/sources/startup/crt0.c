/* not baserelative startup code for gcc v2.3.3+
 * Reimplemented from ncrt0.S by Martin Blom <martin@blom.org>
 */


struct Device;
struct IORequest;
struct MemHeader;
struct SemaphoreMessage;
struct SignalSemaphore;

#include <dos/dosextens.h>
#include <proto/exec.h>

struct ExecBase* SysBase       = NULL;
void*            __SaveSP      = NULL;
const char*      __commandline = NULL;
int              __commandlen  = 0;
const char**     __argv        = NULL;
int              __argc        = 0;
void*            __env         = NULL;
struct Message*  _WBenchMsg    = NULL;

extern ULONG __LIB_LIST__[];
extern ULONG __INIT_LIST__[];
extern ULONG __EXIT_LIST__[];
extern ULONG __LIB_END__[];
extern ULONG __INIT_END__[];
extern ULONG __EXIT_END__[];

static void
callfuncs( ULONG* table, ULONG direction );

#if defined(__i386__) && defined(__amithlon__)

__asm( "
	.text
	.align 4

        # If we are loaded as a DOS segment, the first code segment must
        # contain m68k code. This is equivalent to 'jmp (_start+1,pc)':

	.byte 0x4e, 0xfa, 0x00, 0x03

	.globl _start
	.type  _start,@function
_start:
	pushl %ebp
	movl  %esp,%ebp

	bswap %eax;
	bswap %ebx;
	mov   %eax,__commandlen
	mov   %ebx,__commandline
	bswap %ebx;                  # Restore ebx

	mov   %ebp,%eax
	bswap %eax
	mov   %eax,__SaveSP
	jmp   c_start


	.align 4
	.globl exit
	.globl _exit
	.type  exit,@function
	.type  _exit,@function
exit:
_exit:
	call  c_exit

	mov   4(%esp),%eax
	bswap %eax
	mov   __SaveSP,%edx
	bswap %edx
	mov   %edx,%ebp
	leave
	ret

	.text
	.align 4
	.globl geta4
	.type  geta4,@function
geta4:
	ret
" );

#elif defined(__powerpc__) && defined(__MORPHOS__)

__asm( "
	.text
	.align 4
	.globl __start
	.type  __start,@function
__start:
	stwu	1,-256(1)		/* (8+4+19*4+18*8+8+31)&~31 = 256 */
	mflr	0
	stw	0,256+4(1)
	mfcr	12
	stw	12,8(1)
	stmw	13,12(1)
	stfd	14,12+76+0(1)
	stfd	15,12+76+8(1)
	stfd	16,12+76+16(1)
	stfd	17,12+76+24(1)
	stfd	18,12+76+32(1)
	stfd	19,12+76+40(1)
	stfd	20,12+76+48(1)
	stfd	21,12+76+56(1)
	stfd	22,12+76+64(1)
	stfd	23,12+76+72(1)
	stfd	24,12+76+80(1)
	stfd	25,12+76+88(1)
	stfd	26,12+76+96(1)
	stfd	27,12+76+104(1)
	stfd	28,12+76+112(1)
	stfd	29,12+76+120(1)
	stfd	30,12+76+128(1)
	stfd	31,12+76+136(1)

	lis	9,__commandline@ha
	lis	10,__commandlen@ha
	lis	11,__SaveSP@ha
	stw	3,__commandline@l(9)
	stw	4,__commandlen@l(10)
	stw	1,__SaveSP@l(11)
	b	c_start

	.align 4
	.globl exit
	.globl _exit
	.type  exit,@function
	.type  _exit,@function
exit:
_exit:
	stwu	1,-32(1)
	stw	3,8(1)
	bl	c_exit
	lwz	3,8(1)

	lis	12,__SaveSP@ha
	lwz	1,__SaveSP@l(12)

	lwz	0,256+4(1)
	mtlr	0
	lwz	12,8(1)
	lmw	13,12(1)
	lfd	14,12+76+0(1)
	lfd	15,12+76+8(1)
	lfd	16,12+76+16(1)
	lfd	17,12+76+24(1)
	lfd	18,12+76+32(1)
	lfd	19,12+76+40(1)
	lfd	20,12+76+48(1)
	lfd	21,12+76+56(1)
	lfd	22,12+76+64(1)
	lfd	23,12+76+72(1)
	lfd	24,12+76+80(1)
	lfd	25,12+76+88(1)
	lfd	26,12+76+96(1)
	lfd	27,12+76+104(1)
	lfd	28,12+76+112(1)
	lfd	29,12+76+120(1)
	lfd	30,12+76+128(1)
	lfd	31,12+76+136(1)
	mtcr	12
	addi	1,1,256
	blr
");

ULONG   __abox__=1;
ULONG   __amigappc__=1;  // deprecated, used in MOS 0.4

#else
# error Unknown CPU/OS
#endif


static void
c_start( void )
{
  struct Process* me;
  
  SysBase = *(struct ExecBase**) 4;

  // Set first word in array to the number of word (or -pairs) in array
  __LIB_LIST__[ 0 ]  = ( __LIB_END__  - __LIB_LIST__  - 1 );
  __INIT_LIST__[ 0 ] = ( __INIT_END__ - __INIT_LIST__ - 1 ) / 2;
  __EXIT_LIST__[ 0 ] = ( __EXIT_END__ - __EXIT_LIST__ - 1 ) / 2;

  me = (struct Process*) FindTask( NULL );

  if( me->pr_CLI == NULL )
  {
    WaitPort( &me->pr_MsgPort );

    _WBenchMsg = GetMsg( &me->pr_MsgPort );
  }

  callfuncs( & __INIT_LIST__[ 1 ], -1 );

  _exit( main( __argc, __argv, __env ) );
}


static void
c_exit( void )
{
  callfuncs( & __EXIT_LIST__[ 1 ], 0 );

  if( _WBenchMsg != NULL )
  {
    Forbid();
    ReplyMsg( _WBenchMsg );
  }
}

/* This puzzle is similar to the one found in ncrt0.S, but is more
   suitable for children below 7. */

static void
callfuncs( ULONG* table, ULONG direction )
{
  ULONG current_pri = ~direction;

  do
  {
    ULONG* t;
    ULONG  next_pri = 0;

    for( t = table; t[ 0 ] != NULL; t += 2 )
    {
      ULONG c = current_pri ^ direction;
      ULONG p = t[ 1 ]      ^ direction;

      if( p == c )
      {
	( (void (*)(void)) t[ 0 ])();
      }

      if( p < c && p > next_pri )
      {
	next_pri = p;
      }
    }

    current_pri = next_pri ^ direction;

  } while( current_pri != direction );
}

void __register_frame_info( void ) {}
void __deregister_frame_info( void ) {}


/* data area */

__asm( "
	.data
	.long	__nocommandline
	.long	__initlibraries
	.long	__cpucheck

	.section .libnix___INIT_LIST__, \"aw\", @progbits
	.globl	__INIT_LIST__
__INIT_LIST__:
	.long -1


	.section .libnix___EXIT_LIST__, \"aw\", @progbits
	.globl	__EXIT_LIST__
__EXIT_LIST__:
	.long -1


	.section .libnix___LIB_LIST__, \"aw\", @progbits
	.globl	__LIB_LIST__
__LIB_LIST__:
	.long -1

	.text
" );

