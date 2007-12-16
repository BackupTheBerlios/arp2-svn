
#include <dos/dos.h>
#include <exec/interrupts.h>
#include <exec/types.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <proto/arp2_syscall.h>

#include "sysresbase.h"
#include "version.h"

extern struct Resident CONST SysRomTag;


TEXT CONST  MainName[]    = "arp2.resource";
TEXT CONST  MainID[]      = "arp2.resource " MAKEVERSTR(VERSION,REVISION);
UWORD CONST MainVectors[] = { ~0, ~0 };

struct Library* MainInit(struct Library* library __asm("d0"),
			 BPTR seg_list __asm("a0"),
			 struct ExecBase* SysBase __asm("a6")) {
  (void) seg_list;

  library->lib_Revision = REVISION;

  // Bring up everything
  InitResident(&SysRomTag, 0);

  return library;
}


TEXT CONST SysName[] = "arp2-syscall.resource";
TEXT CONST SysID[]   = "arp2-syscall.resource " MAKEVERSTR(VERSION,REVISION);


ULONG __interrupt exter_handler(struct SysResBase* ARP2_SysCallBase __asm("a1")) {
  struct Task* ready;

  // Clear interrupt flag and return a Task we should signal
  ready = arp2sys_arp2_inthandler();

  if (ready != NULL) {
    struct ExecBase* SysBase = ARP2_SysCallBase->sysbase;

    // Wake up up ready task
    Signal(ready, SIGF_SINGLE);
    return TRUE;
  }
  else {
    // Not for us
    return FALSE;
  }
}


struct LibVec {
    UWORD jmp;
    APTR  addr;
};

enum {
  OP_BJMP  = 0xfe00,
  OP_BJMPQ = 0xfe01,
};


struct SysResBase* SysInit(struct SysResBase* sysres __asm("d0"),
			   BPTR seg_list __asm("a0"),
			   struct ExecBase* SysBase __asm("a6")) {
  int i;
  (void) seg_list;

  // Create SysResBase
  sysres->lib.lib_Revision = REVISION;

  sysres->sysbase = SysBase;

  sysres->exter_int.is_Node.ln_Type = NT_INTERRUPT;
  sysres->exter_int.is_Node.ln_Pri  = 0;
  sysres->exter_int.is_Node.ln_Name = (STRPTR) SysName;
  sysres->exter_int.is_Code = (void (*)()) exter_handler;
  sysres->exter_int.is_Data = sysres;

  // Patch jump-table to use the m68k-to-native BJMP instruction
  sysres->lib.lib_Flags |= LIBF_CHANGED;

  for (i = sizeof (struct LibVec); 
       i <= sysres->lib.lib_NegSize; 
       i += sizeof (struct LibVec)) {
    struct LibVec* vec = (struct LibVec*) ((APTR) sysres - i);

    if ((ULONG) vec->addr & 1) {
      vec->jmp = OP_BJMPQ;
      vec->addr = (APTR) ((ULONG) vec->addr & ~1);
    }
    else {
      vec->jmp = OP_BJMP;
    }
  }

  CacheClearU();
  SumLibrary(&sysres->lib);

  // Add the EXTER server to the system server chain
  AddIntServer(INTB_EXTER, &sysres->exter_int);
  
  return sysres;
}
