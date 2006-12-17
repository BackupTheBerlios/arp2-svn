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
#include "arp2sys.h"

#include <sys/mman.h>

#ifndef DOS_DOSHUNKS_H
# define HUNK_HEADER  1011
# define HUNK_CODE    1001
# define HUNK_DATA    1002
# define HUNK_BSS     1003
# define HUNK_RELOC32 1004
# define HUNK_SYMBOL  1008
# define HUNK_END     1010

# define HUNKF_CHIP   0x40000000
# define HUNKF_FAST   0x80000000
#endif

#ifdef WORDS_BIGENDIAN
# define BE32(x) (x)
#else
# define BE32(x) bswap_32(x)
#endif

//#define ARP2ROM_DEBUG

/*
 * ARP2 ROM memory 
 */


/*** ARP2 ROM memory access **************************************************/

static uae_u8*  arp2rom       = NULL;
static uae_u32  arp2rom_start = 0;	/* Determined by the OS */
static uae_u32  arp2rom_mask  = 0;	/* Determined by arp2rom_init() */
static uae_u32  arp2rom_size  = 0;	/* Determined by arp2rom_init() */


static uae_u32 arp2rom_lget(uaecptr) REGPARAM;
static uae_u32 arp2rom_wget(uaecptr) REGPARAM;
static uae_u32 arp2rom_bget(uaecptr) REGPARAM;
static void arp2rom_lput(uaecptr, uae_u32) REGPARAM;
static void arp2rom_wput(uaecptr, uae_u32) REGPARAM;
static void arp2rom_bput(uaecptr, uae_u32) REGPARAM;

uae_u32 REGPARAM2 arp2rom_lget(uaecptr addr)
{
    uae_u8 *m;
/* #ifdef JIT */
/*     special_mem |= S_READ; */
/* #endif */
#ifdef ARP2ROM_DEBUG
    write_log("ARP2 ROM: arp2rom_lget 0x%08x, PC=%p => ", addr, m68k_getpc(&regs));
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    m = arp2rom + addr;
#ifdef ARP2ROM_DEBUG
    write_log("0x%08x\n", do_get_mem_long((uae_u32 *)m));
#endif
    return do_get_mem_long((uae_u32 *)m);
}

uae_u32 REGPARAM2 arp2rom_wget(uaecptr addr)
{
    uae_u8 *m;
/* #ifdef JIT */
/*     special_mem |= S_READ; */
/* #endif */
#ifdef ARP2ROM_DEBUG
    write_log("ARP2 ROM: arp2rom_wget 0x%08x, PC=%p => ", addr, m68k_getpc(&regs));
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
    m = arp2rom + addr;
#ifdef ARP2ROM_DEBUG
    write_log("0x%04x\n", do_get_mem_word((uae_u16 *)m));
#endif
    return do_get_mem_word((uae_u16 *)m);
}

uae_u32 REGPARAM2 arp2rom_bget(uaecptr addr)
{
/* #ifdef JIT */
/*     special_mem |= S_READ; */
/* #endif */
#ifdef ARP2ROM_DEBUG
    write_log("ARP2 ROM: arp2rom_bget 0x%08x, PC=%p => ", addr, m68k_getpc(&regs));
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
#ifdef ARP2ROM_DEBUG
    write_log("0x%02x\n", arp2rom[addr]);
#endif
    return arp2rom[addr];
}

static void REGPARAM2 arp2rom_lput(uaecptr addr, uae_u32 l)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log("ARP2 ROM: arp2rom_lput 0x%08x into 0x%08x,  PC=%p\n", 
	       l, addr, m68k_getpc(&regs));
}

static void REGPARAM2 arp2rom_wput(uaecptr addr, uae_u32 w)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log("ARP2 ROM: arp2rom_wput 0x%04x into 0x%08x, PC=%p\n", 
	       w & 0xffff, addr, m68k_getpc(&regs));
}

static void REGPARAM2 arp2rom_bput(uaecptr addr, uae_u32 b)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log("ARP2 ROM: arp2rom_bput 0x%02x into 0x%08x, PC=%p\n", 
	       b & 0xff, addr, m68k_getpc(&regs));
}


static int REGPARAM2 arp2rom_check(uaecptr addr, uae_u32 size)
{
#ifdef ARP2ROM_DEBUG
    write_log("arp2rom_check %p, %08x => ", addr, size);
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
#ifdef ARP2ROM_DEBUG
    write_log("%d\n", (addr + size) <= arp2rom_size);
#endif
    return(addr + size) <= arp2rom_size;
}

static uae_u8 REGPARAM2 *arp2rom_xlate(uaecptr addr)
{
#ifdef ARP2ROM_DEBUG
    write_log("arp2rom_xlate %p => ", addr);
#endif
    addr -= arp2rom_start & arp2rom_mask;
    addr &= arp2rom_mask;
#ifdef ARP2ROM_DEBUG
    write_log("%p\n", arp2rom + addr);
#endif
    return arp2rom + addr;
}

addrbank arp2rom_bank = {
    arp2rom_lget, arp2rom_wget, arp2rom_bget,
    arp2rom_lput, arp2rom_wput, arp2rom_bput,
    arp2rom_xlate, arp2rom_check, NULL
};


/*** ARP2 ROM LoadSeg() support **********************************************/

/* I added LoadSeg() support for the ROM image, since there seem to be
 * NO decent compiler for m68k that can produce true PC-relative,
 * ROM-able code. Pathetic! (I need AmigaOS-style registered argument
 * and 64-bit integer support.)
 *
 * But so what? This isn't a physical ROM, so I can relocate the code
 * whenever I want and use good old gcc 2.95 for the ROM.
 */

struct segment {
    uae_u32 size_spec;
    int image_reloc_count;
    int image_reloc_index;
};

static uae_u32*        loadseg_image = NULL;
static int             loadseg_image_words = 0;
static struct segment* loadseg_segments = NULL;


static int loadseg_check_size(struct zfile* image_file) {
  int i;
  uae_u32 header[5] = { 0, 0, 0, 0, 0 };
  int rom_size = 0;

  if (zfile_fread(header, sizeof(uae_u32), 5, image_file) != 5) {
    return 0;
  }

  for(i = 0; i < 5; ++i) {
    header[i] = BE32(header[i]);
  }

  if (header[0] == HUNK_HEADER && 
      header[1] == 0 && 
      header[3] == 0 && 
      header[4] == header[2] - 1) {
    int segments = header[4] - header[3] + 1;

    loadseg_segments = calloc(segments, sizeof(struct segment));

    if (loadseg_segments == NULL) {
      return 0;
    }

    // Load size specification into segment array

    for(i = 0; i < segments; ++i) {
      if (zfile_fread(&loadseg_segments[i].size_spec, sizeof(uae_u32), 1, 
		      image_file) != 1) {
	return 0;
      }

      loadseg_segments[i].size_spec = BE32(loadseg_segments[i].size_spec);

      if ((loadseg_segments[i].size_spec &(HUNKF_CHIP | HUNKF_FAST)) != 0) {
	write_log("ARP2 ROM: LoadSeg() hunks must load into MEMF_ANY memory.\n");
	return 0;
      }

      rom_size += loadseg_segments[i].size_spec * sizeof(uae_u32);
    }
  }

  return rom_size;
}


static int loadseg_load(void) {
  int segments = BE32(loadseg_image[4]) - BE32(loadseg_image[3]) + 1;
  uae_u32 type;

  int segment = 0;
  int rom_index = 0;
  int index = 5 + segments;

  for (type = BE32(loadseg_image[index]); 
       type != HUNK_END && index < loadseg_image_words;
       type = BE32(loadseg_image[index]) ) {
    
    switch (type) {
      case HUNK_CODE:
      case HUNK_DATA:
      case HUNK_BSS: {
	uae_u32 longs = BE32(loadseg_image[index + 1]);
	int     size  = longs * sizeof (uae_u32);

	if (segment >= segments) {
	  write_log("ARP2 ROM: Illegal LoadSeg() code/data hunk: %08x.\n", type);
	  return 0;
	}

	if (type != HUNK_BSS) {
	  memcpy(&arp2rom[rom_index], &loadseg_image[index + 2], size);
	  index += 2 + longs;
	}
	else {
	  // Memory already cleared
	  index += 2;
	}

	rom_index += loadseg_segments[segment].size_spec & ~(HUNKF_CHIP | HUNKF_FAST);
	++segment;
	break;
      }

      case HUNK_RELOC32: {
	++index;

	while (index < loadseg_image_words && BE32(loadseg_image[index] != 0)) {
	  int s = BE32(loadseg_image[index + 1]);
	  
	  if (s >= segments) {
	    write_log("ARP2 ROM: Illegal LoadSeg() RELOC32 reference to hunk %d.\n", s);
	    return 0;
	  }
	  
	  loadseg_segments[s].image_reloc_count = BE32(loadseg_image[index]);
	  loadseg_segments[s].image_reloc_index = index + 2;
	  index += 2 + BE32(loadseg_image[index]);
	}

	++index;
	break;
      }

      case HUNK_SYMBOL: {
	++index;

	while (index < loadseg_image_words && BE32(loadseg_image[index] != 0)) {
	  index += 2 + BE32(loadseg_image[index]);
	}

	++index;
	break;
      }

      default:
	write_log("ARP2 ROM: Unsupported LoadSeg() hunk: %08x.\n", type);
	return 0;
    }
  }

  return (type == HUNK_END);
}


static void loadseg_reloc(uae_u32 base) {
  int segment, reloc;
  int segments = BE32(loadseg_image[4]) - BE32(loadseg_image[3]) + 1;

  int rom_index = 0;

  for (segment = 0; segment < segments; ++segment) { 
    int count = loadseg_segments[segment].image_reloc_count;
    int index = loadseg_segments[segment].image_reloc_index;

    for (reloc = 0; reloc < count; ++reloc) {
      uae_u32  offset = BE32(loadseg_image[index + reloc]);
      uae_u32* mempos = (uae_u32*) &arp2rom[rom_index + offset];

      mempos[0] = BE32(BE32(mempos[0]) + base);
    }

    rom_index += loadseg_segments[segment].size_spec & ~(HUNKF_CHIP | HUNKF_FAST);
  }
}


/*** ARP2 ROM main code ******************************************************/

int arp2rom_init(void) {
  arp2rom_size = arp2rom_mask = arp2rom_start = 0;

  return arp2sys_init();
}


int arp2rom_reset(void) {
  int image_size = 0, rom_size = 0;
  struct zfile* image_file = NULL;
  int is_supported_loadseg = 0;

  arp2rom_free();

  if (strlen(currprefs.arp2romfile) == 0)
    return 0;

  image_file = zfile_fopen(currprefs.arp2romfile,"rb");

  if (image_file == NULL) {
    write_log("ARP2 ROM: Failed to open ROM image '%s'\n", currprefs.arp2romfile);
    return 0;
  }

  // Check for supported LoadSeg()-style module
  
  rom_size = loadseg_check_size(image_file);
  is_supported_loadseg = (rom_size != 0);

  zfile_fseek(image_file, 0, SEEK_END);
  image_size = zfile_ftell(image_file);
  zfile_fseek(image_file, 0, SEEK_SET);

  if (is_supported_loadseg) {
    loadseg_image_words = image_size / sizeof (uae_u32);
    loadseg_image = malloc(image_size);

    if (loadseg_image == NULL) {
      write_log("ARP2 ROM: ROM image memory allocation failed(%d KB)\n", 
		image_size / 1024);
      zfile_fclose(image_file);
      arp2rom_free();
      return 0;
    }
  }
  else {
    rom_size = image_size;
  }

  write_log("ARP2 ROM: ROM image is a %s, %ld bytes large\n", 
	    is_supported_loadseg ? "LoadSeg() module" : "binary ROM image",
	    (long) rom_size);
     
  // Calculate Zorro card memory size and allocate ROM

  for(arp2rom_size = 65536; arp2rom_size <(uae_u32) rom_size; arp2rom_size *= 2);
  arp2rom_mask = arp2rom_size - 1;

  arp2rom = mapped_malloc(arp2rom_size, "ARP2 ROM image");

  arp2rom_bank.baseaddr = arp2rom;

  if (arp2rom == NULL) {
    write_log("ARP2 ROM: ROM image memory allocation failed(%d KB)\n", 
	      arp2rom_size / 1024);
    zfile_fclose(image_file);
    arp2rom_free();
    return 0;
  }

  // Read in ROM image

  int read_size = zfile_fread(is_supported_loadseg ? (uae_u8*) loadseg_image : arp2rom, 
			      1, image_size, image_file);
    
  if (read_size != image_size) {
    write_log("ARP2 ROM: ROM image read error(wanted %d, read %d).\n", 
	      image_size, read_size);
    zfile_fclose(image_file);
    arp2rom_free();
    return 0;
  }

  zfile_fclose(image_file);

  if (is_supported_loadseg) {
    if (!loadseg_load()) {
      arp2rom_free();
      return 0;
    }
  }

  // Patch ROM with syscall vectors
  if (!arp2sys_reset(arp2rom)) {
    arp2rom_free();
    return 0;
  }

  // Write-protect ROM
//  mprotect(arp2rom, arp2rom_size, PROT_READ | PROT_EXEC);

  return 1;
}

void arp2rom_free(void) {
  arp2sys_free();

  if (arp2rom != NULL) {
    mapped_free(arp2rom);
  }

  arp2rom       = NULL;
  arp2rom_start = 0;
  arp2rom_mask  = 0;
  arp2rom_size  = 0;

  free(loadseg_image);
  free(loadseg_segments);

  loadseg_image    = NULL;
  loadseg_segments = NULL;
}


void arp2rom_config(uae_u8* configmem) {
  // Map in ROM at configmem, but never more than 64 KiB.
  memcpy(configmem, arp2rom,(arp2rom_mask & 0xFFFF) + 1);
  write_log("ARP2 ROM: Configuring ROM @$e80000: %d KB.\n", 
	    ((arp2rom_mask & 0xFFFF) + 1) / 1024);
}


void arp2rom_map(uae_u32 start) {
  // Unrelocate ROM image from the old address
  if (loadseg_image != NULL) {
    loadseg_reloc(-arp2rom_start);
  }

  arp2rom_start = start;
  map_banks(&arp2rom_bank, arp2rom_start >> 16, arp2rom_size >> 16, arp2rom_size);

  if (loadseg_image != NULL) {
    loadseg_reloc(arp2rom_start);
  }

  write_log("ARP2 ROM: Mapped ROM @$%lx: %d KB.\n",  arp2rom_start, arp2rom_size / 1024);
}


#endif // ARP2ROM
