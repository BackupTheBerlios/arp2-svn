#define _KERNEL
#include <exec/resident.h>
#include <exec/libraries.h>
#include "ixemul.h"
#include "kprintf.h"
#include "version.h"

#define STR(x)  #x

struct ixemul_base *ix_init_glue(struct ixemul_base *ixbase);
struct ixemul_base *ix_open(struct ixemul_base *ixbase);
void ix_close(struct ixemul_base *ixbase);
void ix_expunge(struct ixemul_base *ixbase);

int (*funcTable[])();
extern int (*ppcFuncTable[])();

struct ixemul_base* _initRoutine(struct ixemul_base *base,
				 BPTR seglist,
				 struct ExecBase *sysbase);

struct LibInitStruct {
  ULONG LibSize;
  void  *FuncTable;
  void  *DataTable;
  void  (*InitFunc)(void);
};

struct LibInitStruct Init={
  sizeof(struct ixemul_base),
  funcTable,
  NULL,
  (void (*)(void)) &_initRoutine
};


struct Resident initDDescrip={
  RTC_MATCHWORD,
  &initDDescrip,
  &initDDescrip + 1,
  RTF_PPC | RTF_AUTOINIT | RTF_EXTENDED,
  IX_VERSION,
  NT_LIBRARY,
  IX_PRIORITY,
  IX_NAME,
  IX_IDSTRING "\r\n",
  &Init,
  IX_REVISION,
  NULL
};

/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 * ** IMPORTANT **
 */
ULONG   __abox__=1;


int libReserved(void) {
  return 0;
}

/* RTF_PPC flag set, so the initialization routine is called
 * as a normal PPC SysV4 ABI */
struct ixemul_base *_initRoutine(struct ixemul_base *base,
				 BPTR seglist,
				 struct ExecBase *sysbase) {
  base->ix_seg_list = seglist;
  base->basearray = ppcFuncTable;
  KPRINTF(("basearray = %lx\n", ppcFuncTable));
  return ix_init_glue(base);
}


struct ixemul_base *libOpen(void) {
  struct ixemul_base *base = (struct ixemul_base *) REG_A6;

  ++base->ix_lib.lib_OpenCnt;
  base->ix_lib.lib_Flags &= ~LIBF_DELEXP;
  if(!ix_open(base))
    {
      --base->ix_lib.lib_OpenCnt;
      base = NULL;
    }
  return base;
}

ULONG libExpungeFunc(struct ixemul_base *base) {
  ULONG seg;

  /* assume we can't expunge */
  seg = 0;
  base->ix_lib.lib_Flags |= LIBF_DELEXP;

  if (base->ix_lib.lib_OpenCnt == 0)
    {
      Remove(&base->ix_lib.lib_Node);

      ix_expunge(base);

      seg = base->ix_seg_list;

      FreeMem((char *)base - base->ix_lib.lib_NegSize,
	      base->ix_lib.lib_NegSize + base->ix_lib.lib_PosSize);
    }

  return seg;
}

ULONG libExpunge(void) {
  struct ixemul_base *base = (struct ixemul_base *) REG_A6;
  return libExpungeFunc(base);
}

ULONG libClose(void) {
  struct ixemul_base *base = (struct ixemul_base *) REG_A6;

  ix_close(base);

  if (--base->ix_lib.lib_OpenCnt == 0 &&
      base->ix_lib.lib_Flags & LIBF_DELEXP)
    return libExpungeFunc(base);

  return 0;
}


static const struct EmulLibEntry _gate_libOpen={
  TRAP_LIB, 0, (void(*)(void))libOpen
};

static const struct EmulLibEntry _gate_libClose={
  TRAP_LIB, 0, (void(*)(void))libClose
};

static const struct EmulLibEntry _gate_libExpunge={
  TRAP_LIB, 0, (void(*)(void))libExpunge
};

static const struct EmulLibEntry _gate_libReserved={
  TRAP_LIB, 0, (void(*)(void))libReserved
};

void __must_recompile(void);

static const struct EmulLibEntry _gate___must_recompile={
  TRAP_LIBNR, 0, __must_recompile
};

/* declare the gates */

#define _gate___must_recompile27  _gate___must_recompile
#define _gate___must_recompile28  _gate___must_recompile
#define _gate___must_recompile29  _gate___must_recompile
#define _gate___must_recompile30  _gate___must_recompile
#define _gate___must_recompile31  _gate___must_recompile
#define _gate___must_recompile32  _gate___must_recompile
#define _gate___must_recompile33  _gate___must_recompile
#define _gate___must_recompile34  _gate___must_recompile
#define _gate___must_recompile215 _gate___must_recompile
#define _gate___must_recompile216 _gate___must_recompile
#define _gate___must_recompile217 _gate___must_recompile
#define _gate___must_recompile220 _gate___must_recompile
#define _gate___must_recompile221 _gate___must_recompile
#define _gate___must_recompile222 _gate___must_recompile
#define _gate___must_recompile223 _gate___must_recompile
#define _gate___must_recompile224 _gate___must_recompile
#define _gate___must_recompile226 _gate___must_recompile
#define _gate___must_recompile229 _gate___must_recompile
#define _gate___must_recompile235 _gate___must_recompile
#define _gate___must_recompile237 _gate___must_recompile
#define _gate___must_recompile239 _gate___must_recompile
#define _gate___must_recompile240 _gate___must_recompile
#define _gate___must_recompile241 _gate___must_recompile
#define _gate___must_recompile242 _gate___must_recompile
#define _gate___must_recompile243 _gate___must_recompile
#define _gate___must_recompile244 _gate___must_recompile
#define _gate___must_recompile253 _gate___must_recompile
#define _gate___must_recompile254 _gate___must_recompile
#define _gate___must_recompile255 _gate___must_recompile
#define _gate___must_recompile256 _gate___must_recompile
#define _gate___must_recompile259 _gate___must_recompile
#define _gate___must_recompile262 _gate___must_recompile
#define _gate___must_recompile267 _gate___must_recompile
#define _gate___must_recompile273 _gate___must_recompile
#define _gate___must_recompile274 _gate___must_recompile
#define _gate___must_recompile275 _gate___must_recompile
#define _gate___must_recompile277 _gate___must_recompile
#define _gate___must_recompile279 _gate___must_recompile
#define _gate___must_recompile386 _gate___must_recompile
#define _gate___must_recompile387 _gate___must_recompile
#define _gate___must_recompile388 _gate___must_recompile
#define _gate___must_recompile389 _gate___must_recompile
#define _gate___must_recompile390 _gate___must_recompile
#define _gate___must_recompile447 _gate___must_recompile
#define _gate___must_recompile448 _gate___must_recompile
#define _gate___must_recompile449 _gate___must_recompile
#define _gate___must_recompile450 _gate___must_recompile
#define _gate___must_recompile502 _gate___must_recompile
#define _gate___must_recompile503 _gate___must_recompile
#define _gate___must_recompile504 _gate___must_recompile
#define _gate___must_recompile505 _gate___must_recompile
#define _gate___must_recompile506 _gate___must_recompile
#define _gate___must_recompile507 _gate___must_recompile
#define _gate___must_recompile520 _gate___must_recompile
#define _gate___must_recompile521 _gate___must_recompile
#define _gate___must_recompile523 _gate___must_recompile
#define _gate___must_recompile524 _gate___must_recompile
#define _gate___must_recompile525 _gate___must_recompile
#define _gate___must_recompile526 _gate___must_recompile
#define _gate___must_recompile527 _gate___must_recompile
#define _gate___must_recompile528 _gate___must_recompile
#define _gate___must_recompile529 _gate___must_recompile
#define _gate___must_recompile530 _gate___must_recompile
#define _gate___must_recompile570 _gate___must_recompile

#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec, nargs, stk) extern const struct EmulLibEntry _gate_trace_ ## func;
#else
#define SYSTEM_CALL(func, vec, nargs, stk) extern const struct EmulLibEntry _gate_ ## func;
#endif
#include <sys/syscall.def>
#undef SYSTEM_CALL

/* The library 68k gates table. We use gates that read the function
 * arguments from the stack and put them in registers. */

int (*funcTable[])() = {

  /* standard system routines */
  (int(*)())&_gate_libOpen,
  (int(*)())&_gate_libClose,
  (int(*)())&_gate_libExpunge,
  (int(*)())&_gate_libReserved,

  /* my libraries definitions */

#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec, nargs, stk) (int(*)())&_gate_trace_ ## func,
#else
#define SYSTEM_CALL(func, vec, nargs, stk) (int(*)())&_gate_ ## func,
#endif
#include <sys/syscall.def>
#undef SYSTEM_CALL

  (int(*)())-1
};

#if 0
void prolog(int n) {
  usetup;
  if (u.u_stk_spare)
      KPRINTF(("#### begin(%ld): %lx\n", n, u.u_stk_spare));
}

void epilog(int n) {
  usetup;
  if (u.u_stk_spare)
      KPRINTF(("#### end(%ld): %lx\n", n, u.u_stk_spare));
}

void __obsolete_div(void) {}
void __obsolete_ldiv(void) {}
void __obsolete_inet_makeaddr(void) {}
void __must_recompile27 (void) {}
void __must_recompile28 (void) {}
void __must_recompile29 (void) {}
void __must_recompile30 (void) {}
void __must_recompile31 (void) {}
void __must_recompile32 (void) {}
void __must_recompile33 (void) {}
void __must_recompile34 (void) {}
void __must_recompile215(void) {}
void __must_recompile216(void) {}
void __must_recompile217(void) {}
void __must_recompile220(void) {}
void __must_recompile221(void) {}
void __must_recompile222(void) {}
void __must_recompile223(void) {}
void __must_recompile224(void) {}
void __must_recompile226(void) {}
void __must_recompile229(void) {}
void __must_recompile235(void) {}
void __must_recompile237(void) {}
void __must_recompile239(void) {}
void __must_recompile240(void) {}
void __must_recompile241(void) {}
void __must_recompile242(void) {}
void __must_recompile243(void) {}
void __must_recompile244(void) {}
void __must_recompile253(void) {}
void __must_recompile254(void) {}
void __must_recompile255(void) {}
void __must_recompile256(void) {}
void __must_recompile259(void) {}
void __must_recompile262(void) {}
void __must_recompile267(void) {}
void __must_recompile273(void) {}
void __must_recompile274(void) {}
void __must_recompile275(void) {}
void __must_recompile277(void) {}
void __must_recompile279(void) {}
void __must_recompile386(void) {}
void __must_recompile387(void) {}
void __must_recompile388(void) {}
void __must_recompile389(void) {}
void __must_recompile390(void) {}
void __must_recompile447(void) {}
void __must_recompile448(void) {}
void __must_recompile449(void) {}
void __must_recompile450(void) {}
void __must_recompile502(void) {}
void __must_recompile503(void) {}
void __must_recompile504(void) {}
void __must_recompile505(void) {}
void __must_recompile506(void) {}
void __must_recompile507(void) {}
void __must_recompile520(void) {}
void __must_recompile521(void) {}
void __must_recompile523(void) {}
void __must_recompile524(void) {}
void __must_recompile525(void) {}
void __must_recompile526(void) {}
void __must_recompile527(void) {}
void __must_recompile528(void) {}
void __must_recompile529(void) {}
void __must_recompile530(void) {}
void __must_recompile570(void) {}

asm(".section \".text\"; .align 2");

#define SYSTEM_CALL(func, vec, nargs) \
    int _t_ ## func(); \
    asm(".globl _t_" STR(func) "\n" \
	"_t_" STR(func) ":\n" \
	"stwu   1,-256(1)\n" \
	"mflr   0\n" \
	"stw    0,260(1)\n" \
	"stmw   3,8(1)\n" \
	"li     3," STR(vec) "\n" \
	"bl     prolog\n" \
	"lmw    3,8(1)\n" \
	"bl     " STR(func) "\n" \
	"stw    3,8(1)\n" \
	"stw    4,12(1)\n" \
	"li     3," STR(vec) "\n" \
	"bl     epilog\n" \
	"lwz    0,260(1)\n" \
	"lmw    3,8(1)\n" \
	"mtlr   0\n" \
	"addi   1,1,256\n" \
	"blr");

#include <sys/syscall.def>
SYSTEM_CALL(__must_recompile,0,X)
#undef SYSTEM_CALL

#endif

#define __obsolete_div __must_recompile
#define __obsolete_inet_makeaddr __must_recompile
#define __obsolete_ldiv __must_recompile
#define __must_recompile27  __must_recompile
#define __must_recompile28  __must_recompile
#define __must_recompile29  __must_recompile
#define __must_recompile30  __must_recompile
#define __must_recompile31  __must_recompile
#define __must_recompile32  __must_recompile
#define __must_recompile33  __must_recompile
#define __must_recompile34  __must_recompile
#define __must_recompile215 __must_recompile
#define __must_recompile216 __must_recompile
#define __must_recompile217 __must_recompile
#define __must_recompile220 __must_recompile
#define __must_recompile221 __must_recompile
#define __must_recompile222 __must_recompile
#define __must_recompile223 __must_recompile
#define __must_recompile224 __must_recompile
#define __must_recompile226 __must_recompile
#define __must_recompile229 __must_recompile
#define __must_recompile235 __must_recompile
#define __must_recompile237 __must_recompile
#define __must_recompile239 __must_recompile
#define __must_recompile240 __must_recompile
#define __must_recompile241 __must_recompile
#define __must_recompile242 __must_recompile
#define __must_recompile243 __must_recompile
#define __must_recompile244 __must_recompile
#define __must_recompile253 __must_recompile
#define __must_recompile254 __must_recompile
#define __must_recompile255 __must_recompile
#define __must_recompile256 __must_recompile
#define __must_recompile259 __must_recompile
#define __must_recompile262 __must_recompile
#define __must_recompile267 __must_recompile
#define __must_recompile273 __must_recompile
#define __must_recompile274 __must_recompile
#define __must_recompile275 __must_recompile
#define __must_recompile277 __must_recompile
#define __must_recompile279 __must_recompile
#define __must_recompile386 __must_recompile
#define __must_recompile387 __must_recompile
#define __must_recompile388 __must_recompile
#define __must_recompile389 __must_recompile
#define __must_recompile390 __must_recompile
#define __must_recompile447 __must_recompile
#define __must_recompile448 __must_recompile
#define __must_recompile449 __must_recompile
#define __must_recompile450 __must_recompile
#define __must_recompile502 __must_recompile
#define __must_recompile503 __must_recompile
#define __must_recompile504 __must_recompile
#define __must_recompile505 __must_recompile
#define __must_recompile506 __must_recompile
#define __must_recompile507 __must_recompile
#define __must_recompile520 __must_recompile
#define __must_recompile521 __must_recompile
#define __must_recompile523 __must_recompile
#define __must_recompile524 __must_recompile
#define __must_recompile525 __must_recompile
#define __must_recompile526 __must_recompile
#define __must_recompile527 __must_recompile
#define __must_recompile528 __must_recompile
#define __must_recompile529 __must_recompile
#define __must_recompile530 __must_recompile
#define __must_recompile570 __must_recompile

/* Glues that align the stack to a 16 bytes boundary if needed. */
#ifdef __MORPHOS__
#define SYSTEM_CALL_STK(func, vec, nargs) \
	void _safe_ ## func(void); \
	asm("	.section \".text\"\n" \
	"	.type	_safe_"STR(func)",@function\n" \
	"_safe_"STR(func)":\n" \
	"	andi.	11,1,15\n" \
	"	bne-	._align_"STR(func)"\n" \
	"	b	"STR(func)"\n" \
	"._align_"STR(func)":\n" \
	"	addi	11,11,16\n" \
	"	mflr	0\n" \
	"	neg	11,11\n" \
	"	stw	0,4(1)\n" \
	"	stwux	1,1,11\n" \
	"	bl	"STR(func)"\n" \
	"	lwz	1,0(1)\n" \
	"	lwz	0,4(1)\n" \
	"	mtlr	0\n" \
	"	blr");
#define SYSTEM_CALL_NOSTK(func, vec, nargs)
#define SYSTEM_CALL_VSTK(func, vec, nargs) void _stk_ ## func(void);
#define SYSTEM_CALL(func, vec, nargs, stk) SYSTEM_CALL_ ## stk(func, vec, nargs)
#include <sys/syscall.def>
#undef SYSTEM_CALL
#endif

/* The library ppc functions table. */
/* Hack: build that table in asm to avoid getting errors about
 * bad prototypes... */

asm("
	.globl      ppcFuncTable
	.section    \".rodata\"
	.align      2
	.type       ppcFuncTable,@object
ppcFuncTable:"

#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec, nargs, stk) ".long " STR(_trace_ ## func) "\n"
#else
#ifndef __MORPHOS__
#define SYSTEM_CALL(func, vec, nargs, stk) ".long " STR(func) "\n"
#else
#define SAFE_STK(func) STR(_safe_ ## func)
#define SAFE_NOSTK(func) STR(func)
#define SAFE_VSTK(func) STR(_stk_ ## func)
#define SYSTEM_CALL(func, vec, nargs, stk) ".long " SAFE_ ## stk(func) "\n"
#endif
#endif
#include <sys/syscall.def>
#undef SYSTEM_CALL

"endppcFuncTable:
	.size    ppcFuncTable,endppcFuncTable-ppcFuncTable
");

int (**_ixbasearray)() = ppcFuncTable;
int (**ixnetarray)();

