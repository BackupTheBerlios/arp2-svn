

__asm("
        .text
        .align 4
        .globl __start
        .type  __start,@function
__start:
        lis     9,__commandline@ha
        lis     10,__commandlen@ha
        lis     11,__SaveSP@ha
        stw     3,__commandline@l(9)
        stw     4,__commandlen@l(10)
        stw     1,__SaveSP@l(11)
	mflr	0
        stw     0,4(1)
        b       c_start

        .align 4
        .globl myexit
        .globl _myexit
        .type  myexit,@function
        .type  _myexit,@function
myexit:
_myexit:
        stwu    1,-32(1)
        stw     3,20(1)
        bl      c_exit
        lwz     3,20(1)

        lis     9,__SaveSP@ha
        lwz     1,__SaveSP@l(9)
        lwz     0,4(1)
	mtlr	0
        blr
");

#include <exec/types.h>

ULONG   __abox__=1;
ULONG   __amigappc__=1;  // deprecated, used in MOS 0.4

void*            __SaveSP      = NULL;
const char*      __commandline = NULL;
int              __commandlen  = 0;

static void
c_start( void ) {
  kprintf("############################### c_start\n");

  _myexit( mymain( 0, 0, 0 ) );
}

static void
c_exit( void ) {
  kprintf("############################### c_end\n");
}

int mymain(int argc, char** argv, void* env) {
  kprintf("############################### main\n");
//  test();
  return 99;
}

//static int test(void) {}
//  kprintf("############################### test\n");
//  myexit(99);
//}
