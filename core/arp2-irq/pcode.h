#ifndef arp2_exp_pcode_h
#define arp2_exp_pcode_h

#if defined(__linux__) && defined(__KERNEL__)

# include <linux/types.h>
# include <asm/string.h>
# include <asm/byteorder.h>
# define ntohll(x) be64_to_cpu(x)
# define htonll(x) cpu_to_be64(x)
 typedef int       bool;
 typedef ptrdiff_t uintptr_t;
# define false 0
# define true  1

#elif defined(__amigaos__)

# include <exec/types.h>
 typedef UBYTE uint8_t;
 typedef UWORD uint16_t;
 typedef ULONG uint32_t;
 typedef UQUAD uint64_t;
 typedef ULONG uintptr_t;
 typedef int   bool;
# define false FALSE
# define true  TRUE

#else

# include <inttypes.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <netinet/in.h>

#endif


enum Endian {	/* SetEndian() flags */
  ENDIAN_LITTLE	= 0,
  ENDIAN_BIG	= 1,
};

enum {		/* MapResource() flags */
  MAPF_READ	= 1,
  MAPF_WRITE	= 2,
};

struct pcode_ops {
    void* user_data;

    void* (*malloc)(struct pcode_ops const* ops, size_t size);
    void  (*free)(struct pcode_ops const* ops, void* addr);
    void  (*kprintf)(struct pcode_ops const* ops, char* fmt, ...);

    uint64_t (*ReadResource8)(struct pcode_ops const* ops, 
			      uint32_t resource, uint32_t offset);
    uint64_t (*ReadResource16)(struct pcode_ops const* ops, 
			       uint32_t resource, uint32_t offset);
    uint64_t (*ReadResource32)(struct pcode_ops const* ops, 
			       uint32_t resource, uint32_t offset);
    uint64_t (*ReadResource64)(struct pcode_ops const* ops, 
			       uint32_t resource, uint32_t offset);
    void (*WriteResource8)(struct pcode_ops const* ops, 
			   uint32_t resource, uint32_t offset, uint8_t value);
    void (*WriteResource16)(struct pcode_ops const* ops, 
			    uint32_t resource, uint32_t offset, uint16_t value);
    void (*WriteResource32)(struct pcode_ops const* ops,
			    uint32_t resource, uint32_t offset, uint32_t value);
    void (*WriteResource64)(struct pcode_ops const* ops, 
			    uint32_t resource, uint32_t offset, uint64_t value);
    void (*SetEndian)(struct pcode_ops const* ops, 
		      enum Endian endian);
    uint64_t (*GetResourceSize)(struct pcode_ops const* ops, 
				uint32_t resource);
    void* (*MapResource)(struct pcode_ops const* ops, 
			 uint32_t resource, uint32_t flags, uint64_t* dma_addr);
    void (*UnmapResource)(struct pcode_ops const* ops, 
			  void* addr);
};


enum pcode_error {
  pcode_ok, 
  pcode_stack_overflow, pcode_local_register_overflow, 
  pcode_illegal_instruction, pcode_priv_instruction
};


typedef void* pcode_handle;

pcode_handle pcode_create(size_t data_size, uint64_t load_address,
			  void const* pcode, size_t pcode_size, 
			  struct pcode_ops const const* ops);

enum pcode_error pcode_execute(pcode_handle handle, uint64_t address);

void pcode_delete(pcode_handle handle);


/* The P-Code is just a subset of Knuth's MMIX ISA. See
   <URL:http://www-cs-faculty.stanford.edu/%7Eknuth/mmix-news.html>
   for further information.
 
   mmix-mmixware-gcc (using the default MMIX ABI, not the GNU variant)
   can be used to compile C code into P-Code. Use
   "mmix-mmixware-objcopy -O binary" to generate a binary image and
   "bin2c", for instance, to include it into your program.
 
   The subset:
 
   * No exceptions. 

   * No overflow checking.

   * No floating point instructions.

   * Only 32 global registers

   * Only 32 local registers

   * 4k shared data/register stack.

   * No segmented memory model.

   * Missing instructions: All FP instrs, CSWAP, LDVTS, BDIF, WDIF,
   TDIF, ODIF, SADD, SYNCD, SYNCID, RESUME, SAVE, UNSAVE, SYNC, TRIP.
   
   * NOP instructions: PRELD, PREGO, PREST.

   * TRAP has predefined meaning. MMIX-sim's stdio traps are not
   supported.

   * The only supported special registers are: rG, rL, rJ, rO and rM.
   rS is set but never actually modified.
*/



#endif /* arp2_exp_pcode_h */
