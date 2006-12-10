 /*
  * UAE - The Un*x Amiga Emulator
  *
  * ARP2 ROM support code
  *
  * Copyright 2006 Martin Blom
  *
  * Features:
  *
  * A ZorroII ROM card with loadabe content (config option
  * arp2_rom_file), plus a function pointer vector with pointers to
  * loads of native syscalls (BJMP support required).
  */

#ifdef ARP2ROM

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "zfile.h"
#include "arp2rom.h"

#include <sys/mman.h>


//#define ARP2ROM_DEBUG

/*
 * ARP2 ROM memory 
 */

uae_u8*  arp2rom       = NULL;
uae_u32  arp2rom_start = 0;	/* Determined by the OS */
uae_u32  arp2rom_mask  = 0;	/* Determined by arp2rom_init() */
uae_u32  arp2rom_size  = 0;	/* Determined by arp2rom_init() */


static uae_u32 arp2rom_lget (uaecptr) REGPARAM;
static uae_u32 arp2rom_wget (uaecptr) REGPARAM;
static uae_u32 arp2rom_bget (uaecptr) REGPARAM;
static void arp2rom_lput (uaecptr, uae_u32) REGPARAM;
static void arp2rom_wput (uaecptr, uae_u32) REGPARAM;
static void arp2rom_bput (uaecptr, uae_u32) REGPARAM;

uae_u32 REGPARAM2 arp2rom_lget (uaecptr addr)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_READ;
#endif
#ifdef ARP2ROM_DEBUG
    write_log ("ARP2 ROM: arp2rom_lget 0x%08x, PC=%p\n", addr, m68k_getpc (&regs));
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    m = arp2rom + addr;
    return do_get_mem_long ((uae_u32 *)m);
}

uae_u32 REGPARAM2 arp2rom_wget (uaecptr addr)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_READ;
#endif
#ifdef ARP2ROM_DEBUG
    write_log ("ARP2 ROM: arp2rom_wget 0x%08x, PC=%p\n", addr, m68k_getpc (&regs));
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    m = arp2rom + addr;
    return do_get_mem_word ((uae_u16 *)m);
}

uae_u32 REGPARAM2 arp2rom_bget (uaecptr addr)
{
#ifdef JIT
    special_mem |= S_READ;
#endif
#ifdef ARP2ROM_DEBUG
    write_log ("ARP2 ROM: arp2rom_bget 0x%08x, PC=%p\n", addr, m68k_getpc (&regs));
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    return arp2rom[addr];
}

static void REGPARAM2 arp2rom_lput (uaecptr addr, uae_u32 l)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log ("ARP2 ROM: arp2rom_lput 0x%08x into 0x%08x,  PC=%p\n", 
	       l, addr, m68k_getpc (&regs));
}

static void REGPARAM2 arp2rom_wput (uaecptr addr, uae_u32 w)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log ("ARP2 ROM: arp2rom_wput 0x%04x into 0x%08x, PC=%p\n", 
	       w & 0xffff, addr, m68k_getpc (&regs));
}

static void REGPARAM2 arp2rom_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log ("ARP2 ROM: arp2rom_bput 0x%02x into 0x%08x, PC=%p\n", 
	       b & 0xff, addr, m68k_getpc (&regs));
}


static int REGPARAM2 arp2rom_check (uaecptr addr, uae_u32 size)
{
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    return (addr + size) <= arp2rom_size;
}

static uae_u8 REGPARAM2 *arp2rom_xlate (uaecptr addr)
{
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    return arp2rom + addr;
}

addrbank arp2rom_bank = {
    arp2rom_lget, arp2rom_wget, arp2rom_bget,
    arp2rom_lput, arp2rom_wput, arp2rom_bput,
    arp2rom_xlate, arp2rom_check, NULL
};



int arp2rom_init (void) {
  uae_u32 size, read_size;
  struct zfile* rom;

  arp2rom_free ();

  if (strlen(currprefs.arp2romfile) == 0)
    return 0;

  rom = zfile_fopen(currprefs.arp2romfile,"rb");

  if (rom == NULL) {
    write_log("ARP2 ROM: failed to open ROM image '%s'\n", currprefs.arp2romfile);
    return 0;
  }

  zfile_fseek(rom, 0, SEEK_END);
  size = zfile_ftell(rom);
  zfile_fseek(rom, 0, SEEK_SET);

  for (arp2rom_size = 65536; arp2rom_size < size; arp2rom_size *= 2);
  arp2rom_mask = arp2rom_size - 1;

  arp2rom = mapped_malloc (arp2rom_size, "ARP2 ROM image");

  if (arp2rom == NULL) {
    write_log ("ARP2 ROM: Out of memory for ROM image (%d KB)\n", arp2rom_size / 1024);
    arp2rom = 0;
    arp2rom_free ();
    zfile_fclose (rom);
    return 0;
  }

  read_size = zfile_fread (arp2rom, 1, size, rom);

  if (read_size != size) {
    write_log ("ARP2 ROM: Read error for ROM image (wanted %d, read %d).\n", size, read_size);
    arp2rom_free ();
    zfile_fclose (rom);
    return 0;
  }

  zfile_fclose (rom);

  // Patch ROM with syscall vectors

  // Write-protect ROM
//  mprotect(arp2rom, arp2rom_size, PROT_READ | PROT_EXEC);

  return 1;
}

void arp2rom_free (void) {
  if (arp2rom != NULL) {
    mapped_free (arp2rom);
  }

  arp2rom       = NULL;
  arp2rom_start = 0;
  arp2rom_mask  = 0;
  arp2rom_size  = 0;
}



#endif // ARP2ROM
