
/** Note: This file will probably generate junk with any other
 * compiler than gcc 2.95 */

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <libraries/configregs.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "sysresbase.h"
#include "version.h"

#define NIBBLE(byte) ((byte) & 0xf0), 0, (((byte) << 4) & 0xf0), 0
#define INIBBLE(byte) (~(byte) & 0xf0), 0, ((~(byte) << 4) & 0xf0), 0

#define DiagArea(c,f) DiagArea2(c,f)
#define DiagArea2(c,f) \
extern struct DiagArea DiagStart; __asm("\n"	\
"	.globl	_DiagStart \n"		\
"_DiagStart: \n"			\
"	.byte " #c "\n"			\
"	.byte " #f "\n"			\
"	.word _DiagEnd-_DiagStart \n"\
"	.word _DiagEntry-_DiagStart \n"	\
"	.word _BootEntry-_DiagStart \n"	\
"	.word _Name-_DiagStart \n"	\
"	.word 0, 0  \n")

#define PCREL(label) ({ \
      APTR res; \
      __asm("lea.l (%1,pc),%0" : "=a" (res) : "m" (label)); \
      res; \
})

#define Z2_SIZE_64KB	0x01
#define Z2_SIZE_128KB	0x02
#define Z2_SIZE_256KB	0x03
#define Z2_SIZE_512KB	0x04
#define Z2_SIZE_1MB	0x05
#define Z2_SIZE_2MB	0x06
#define Z2_SIZE_4MB	0x07
#define Z2_SIZE_8MB	0x00

struct ROMTag {
    struct Resident res;
    ULONG           data_size;
    APTR            vectors;
    APTR            init_struct;
    APTR            init_func;
};


/*** ExpansionROM  ***********************************************************/

UBYTE CONST ROMStart[128] = {
  // struct ExpansionRom:
  NIBBLE(ERT_ZORROII | ERTF_DIAGVALID | Z2_SIZE_64KB),	// er_Type
  INIBBLE(PRODUCT_ARP2ROM),				// er_Product
  INIBBLE(ERFF_NOSHUTUP),				// er_Flags
  INIBBLE(0x00),					// er_Reserved03
  INIBBLE(MANUFACTURER_HACKER >> 8),			// er_Manufacturer
  INIBBLE(MANUFACTURER_HACKER & 0xff),
  INIBBLE(VERSION >> 8),				// er_SerialNumber
  INIBBLE(VERSION & 0x0ff),
  INIBBLE(REVISION >> 8),
  INIBBLE(REVISION & 0x0ff),
  // (Assume DiagStart is directly after this structure, at 0x80)
  INIBBLE(0x00), INIBBLE(0x80),				// er_InitDiagVec 
  INIBBLE(0x00),					// er_Reserved0c
  INIBBLE(0x00),					// er_Reserved0d
  INIBBLE(0x00),					// er_Reserved0e
  INIBBLE(0x00),					// er_Reserved0f

  // struct ExpansionControl:
  NIBBLE(0x00),						// ec_Interrupt
  INIBBLE(0x00),					// ec_Z3_HighBase
  INIBBLE(0x00),					// ec_BaseAddress
  INIBBLE(0x00),					// ec_Shutup
  INIBBLE(0x00),					// ec_Reserved14
  INIBBLE(0x00),					// ec_Reserved15
  INIBBLE(0x00),					// ec_Reserved16
  INIBBLE(0x00),					// ec_Reserved17
  INIBBLE(0x00),					// ec_Reserved18
  INIBBLE(0x00),					// ec_Reserved19
  INIBBLE(0x00),					// ec_Reserved1a
  INIBBLE(0x00),					// ec_Reserved1b
  INIBBLE(0x00),					// ec_Reserved1c
  INIBBLE(0x00),					// ec_Reserved1d
  INIBBLE(0x00),					// ec_Reserved1e
  INIBBLE(0x00)						// ec_Reserved1f
};

/*** DiagArea  ***************************************************************/

DiagArea(DAC_WORDWIDE+DAC_CONFIGTIME, 0);

TEXT CONST Name[]  = "ARP2 ROM" ;
TEXT CONST DosName[]  = DOSNAME;

/*** Resident structures *****************************************************/

extern TEXT CONST MainName[];
extern TEXT CONST MainID[];
extern UWORD CONST MainVectors[];
extern void MainInit(void);

struct ROMTag CONST MainRomTag = {
  { RTC_MATCHWORD,
    (struct Resident*) &MainRomTag.res,
    (APTR) (&MainRomTag + 1),
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION,
    NT_RESOURCE,
    0,
    (STRPTR) MainName,
    (STRPTR) MainID,
    (APTR) &MainRomTag.data_size
  },

  sizeof (struct Library),
  (APTR) MainVectors,
  NULL,
  MainInit
};

extern TEXT CONST SysName[];
extern TEXT CONST SysID[];
extern CONST_APTR CONST SysVectors[];
extern void SysInit(void);

struct ROMTag CONST SysRomTag = {
  { RTC_MATCHWORD,
    (struct Resident*) &SysRomTag.res,
    (APTR) (&SysRomTag + 1),
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION,
    NT_RESOURCE,
    0,
    (STRPTR) SysName,
    (STRPTR) SysID,
    (APTR) &SysRomTag.data_size
  },

  sizeof (struct SysResBase),
  (APTR) SysVectors,
  NULL,
  SysInit
};


/*** DiagEntry, patches ROMTags **********************************************/

void patch_romtag(struct ROMTag* rt, ULONG board_base, ULONG diag_copy) {
  ULONG diag_patch = diag_copy - (ULONG) &DiagStart;
  ULONG rom_patch  = board_base - (ULONG) &ROMStart;

  rt->res.rt_MatchTag = (APTR) rt->res.rt_MatchTag + diag_patch;
  rt->res.rt_EndSkip  = (APTR) rt->res.rt_EndSkip  + diag_patch;
  rt->res.rt_Name     = (APTR) rt->res.rt_Name     + rom_patch;
  rt->res.rt_IdString = (APTR) rt->res.rt_IdString + rom_patch;
  rt->res.rt_Init     = (APTR) rt->res.rt_Init     + diag_patch;
  rt->vectors         = (APTR) rt->vectors         + rom_patch;
  rt->init_struct     = (APTR) rt->init_struct     + rom_patch;
  rt->init_func       = (APTR) rt->init_func       + rom_patch;
}


ULONG DiagEntry(APTR board_base __asm("a0"), 
		APTR diag_copy __asm("a2"),
		struct ConfigDev* config_dev __asm("a3"),
		struct ExpansionBase* ExpansionBase __asm("a5"),
		struct ExecBase* SysBase __asm("a6")) {
  (void) config_dev;
  (void) ExpansionBase;
  (void) SysBase;

  patch_romtag(PCREL(MainRomTag), (ULONG) board_base, (ULONG) diag_copy);
  patch_romtag(PCREL(SysRomTag), (ULONG) board_base, (ULONG) diag_copy);

  return TRUE;
}


/*** BootEntry ***************************************************************/

typedef VOID rt_init_t(struct ExecBase* SysBase __asm("a6"));

VOID BootEntry(struct ExecBase* SysBase __asm("a6")) {
  struct Resident* dos = FindResident(PCREL(DosName));
  /* volatile */ rt_init_t* init = (/* volatile */ rt_init_t*) dos->rt_Init;

  init(SysBase); // Never returns ... or?
  
  while(1) __asm("nop; nop; nop");
}


/*** End of DiagArea *********************************************************/

__asm("_DiagEnd:");


/*** SysCall function pointers ***********************************************/

__asm(".balign 4096");

/** The ROM area between 4096-8192 will be filled in by UAE when the
 * ROM is loaded, if this signature is present. */
CONST_APTR CONST SysVectors[1024] = { 
  (APTR) 0xdeadc0de,
  (APTR) -1,
  // ...
};
