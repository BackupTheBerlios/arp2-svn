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

extern long __nowbmsg;

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


	.text
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

  if( me->pr_CLI == NULL && !__nowbmsg )
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

#include <proto/dos.h>

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

