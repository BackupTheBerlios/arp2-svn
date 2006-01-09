
#include "pcode.h"


/*** State structure *********************************************************/

// Use 32 general-purpose registers
#define GLOBALS	32

// Use 32 local registers
#define LOCALS	32				// Must be a power of two

// Use a stack size so that sizeof (struct state) <= 4096.
#define STACK	(512-(32+GLOBALS+LOCALS+2))	// Total space for
						// registers and stack
						// (longs)
enum special_register {
  rB, rD, rE, rH, rJ, rM, rR, rBB, rC, rN, rO, rS, rI,  rT,  rTT, rK,
  rQ, rU, rV, rG, rL, rA, rF, rP,  rW, rX, rY, rZ, rWW, rXX, rYY, rZZ
};

#define RES	(32+GLOBALS-256+231)
#define ARG0	(32+GLOBALS-256+231)
#define ARG1	(32+GLOBALS-256+232)
#define ARG2	(32+GLOBALS-256+233)
#define ARG3	(32+GLOBALS-256+234)

#define FP	(32+GLOBALS-256+253)
#define SP	(32+GLOBALS-256+254)
#define TMP	(32+GLOBALS-256+255)

#define GLOBAL(s, r) ((s)->g[32 + (r) - (256-GLOBALS)])
#define LOCAL(s, r)  ((s)->l[((s)->alpha + (r)) % LOCALS])

struct state {
    uint64_t          g[32+GLOBALS];		// Global registers (special+normal)
    uint64_t          l[LOCALS];
    uint64_t          s[STACK];			// Local registers/stack

    size_t            alpha;			// (r0 / 8) mod LOCALS
    size_t            beta;			// (alpha + rL) mod LOCALS
    size_t            gamma;			// (rS / 8) mod LOCALS

    struct pcode_ops const* ops;
    uint64_t                text_start;
    uint64_t                text_end;
    uint8_t                 text[0];
};


/*** Memory access functions (memory is big-endian) **************************/

#ifndef ntohll
# if __BYTE_ORDER == __BIG_ENDIAN
#  define ntohll(x) (x);
# else
#  define ntohll(x) (((uint64_t) ntohl((x))) << 32) | ntohl((x) >> 32);
# endif
#endif

#ifndef htonll
# define htonll(x) ntohll((x))
#endif


static inline int8_t read_int8(void* addr) {
  return ((int8_t*) addr)[0];
}

static inline uint8_t read_uint8(void* addr) {
  return ((uint8_t*) addr)[0];
}

static inline void write_int8(void* addr, int8_t value) {
  ((int8_t*) addr)[0] = value;
}

static inline void write_uint8(void* addr, uint8_t value) {
  ((uint8_t*) addr)[0] = value;
}


static inline int16_t read_int16(void* addr) {
  return (int16_t) ntohs(((uint16_t*) addr)[0]);
}

static inline uint16_t read_uint16(void* addr) {
  return ntohs(((uint16_t*) addr)[0]);
}

static inline void write_int16(void* addr, int16_t value) {
  ((uint16_t*) addr)[0] = htons((uint16_t) value);
}

static inline void write_uint16(void* addr, uint16_t value) {
  ((uint16_t*) addr)[0] = htons(value);
}


static inline int32_t read_int32(void* addr) {
  return (int32_t) ntohl(((uint32_t*) addr)[0]);
}

static inline uint32_t read_uint32(void* addr) {
  return ntohl(((uint32_t*) addr)[0]);
}

static inline void write_int32(void* addr, int32_t value) {
  ((uint32_t*) addr)[0] = htonl((uint32_t) value);
}

static inline void write_uint32(void* addr, uint32_t value) {
  ((uint32_t*) addr)[0] = htonl(value);
}


static inline int64_t read_int64(void* addr) {
  return (int64_t) ntohll(((uint64_t*) addr)[0]);
}

static inline uint64_t read_uint64(void* addr) {
  return ntohll(((uint64_t*) addr)[0]);
}

static inline void write_int64(void* addr, int64_t value) {
  ((uint64_t*) addr)[0] = htonll((uint64_t) value);
}

static inline void write_uint64(void* addr, uint64_t value) {
  ((uint64_t*) addr)[0] = htonll(value);
}


/*** Support functions *******************************************************/

// Check that the two stack pointers are reasonable

static inline bool check_stack(struct state* state) {
  return (state->g[rS] >= (uintptr_t) &state->s[0] &&
	  state->g[rS] <  (uintptr_t) &state->s[STACK] &&
	  state->g[SP] >= state->g[rS] && 
	  state->g[SP] <= (uintptr_t) &state->s[STACK]);
}


// Push the "oldest" local register on the register stack

static enum pcode_error push_local(struct state* state) {
  uint64_t* S = (uint64_t*) (uintptr_t) state->g[rS];

  state->g[rS] += 8;

  if (!check_stack(state)) {
    state->g[rS] -= 8; // Undo
    return pcode_stack_overflow;
  }

  *S = state->l[state->gamma];
  state->gamma = (state->gamma + 1) % LOCALS;

  return pcode_ok;
}


// Pop the latest local register from the register stack

static enum pcode_error pop_local(struct state* state) {
  uint64_t* S;

  state->g[rS] -= 8;

  if (!check_stack(state)) {
    state->g[rS] += 8; // Undo
    return pcode_stack_overflow;
  }

  S = (uint64_t*) (uintptr_t) state->g[rS];

  state->gamma = (state->gamma - 1) % LOCALS;
  state->l[state->gamma]= *S;

  return pcode_ok;
}


// Initialize a previously unused local register for writing

static inline enum pcode_error allocate_local(struct state* state) {
  if (state->g[rL] == (LOCALS-1)) {
    return pcode_local_register_overflow;
  }

  state->l[state->beta] = 0;

  state->beta = (state->beta + 1) % LOCALS;
  state->g[rL] += 1;
  
  if (state->beta == state->gamma) {
    return push_local(state);
  }
  else {
    return pcode_ok;
  }
}


static inline uint64_t shift_imm(uint8_t op, uint64_t yz) {
  return yz << ((~op & 3) * 16);
}


static void* calc_m(struct state* state, uint64_t addr, size_t size) {
  if (addr >= state->text_start && addr < state->text_end) {
    addr += (uintptr_t) &state->text - state->text_start;
  }

  return (void*) (uintptr_t) (addr & ~(size-1));
}


static inline int64_t calc_ra16(uint8_t op, uint64_t yz) {
  if ((op & 0x01) == 0x01) {
    yz -= (1 << 16);
  }

  return 4 * yz;
}


static inline int64_t calc_ra24(uint8_t op, uint64_t xyz) {
  if ((op & 0x01) == 0x01) {
    xyz -= (1 << 24);
  }

  return 4 * xyz;
}


static inline uint64_t mor(uint64_t y, uint64_t z) {
  uint64_t x = 0;

  while (y != 0) {
    uint64_t a = (y & 0xff) * 0x0101010101010101ULL;
    uint64_t b = (z & 0x0101010101010101ULL) * 0xff;

    x |= a & b;

    y >>= 8;
    z >>= 1;
  }

  return x;
}  


static inline uint64_t mxor(uint64_t y, uint64_t z) {
  uint64_t x = 0;

  while (y != 0) {
    uint64_t a = (y & 0xff) * 0x0101010101010101ULL;
    uint64_t b = (z & 0x0101010101010101ULL) * 0xff;

    x ^= a & b;

    y >>= 8;
    z >>= 1;
  }

  return x;
}  


static bool cc(uint8_t op, int64_t y) {
  switch ((op >> 1) & 0x07) {
    case 0:
      return y < 0;

    case 1:
      return y == 0;

    case 2:
      return y > 0;

    case 3:
      return (y & 1) == 1;

    case 4:
      return y >= 0;

    case 5:
      return y != 0;

    case 6:
      return y <= 0;

    case 7:
      return (y & 1) == 0;
  }

  return false; // (Cannot happen)
}


/*** Execute one instruction *************************************************/

static enum pcode_error execute_op(struct state* state, uint64_t* pc) {
  uint8_t op, ox, oy, oz;
  uint64_t x, y, z;
  uint64_t* r = NULL;
  uint8_t* real_pc;

  if (!check_stack(state)) {
    return pcode_stack_overflow;
  }

  // Fetch instruction

  real_pc = calc_m(state, *pc, 4);

  op = real_pc[0];
  ox = real_pc[1];
  oy = real_pc[2];
  oz = real_pc[3];

  state->ops->kprintf(state->ops,
		      "Executing instruction at %p: %02x%02x%02x%02x\n",
		      (void*) *pc, op, ox, oy, oz);

  // Default source arguments

  x = y = z = 0;

  if (ox >= state->g[rG]) {
    x = GLOBAL(state, ox);
  }
  else if (ox < state->g[rL]) {
    x = LOCAL(state, ox);
  }

  if (oy >= state->g[rG]) {
    y = GLOBAL(state, oy);
  }
  else if (oy < state->g[rL]) {
    y = LOCAL(state, oy);
  }

  if (oz >= state->g[rG]) {
    z = GLOBAL(state, oz);
  }
  else if (oz < state->g[rL]) {
    z = LOCAL(state, oz);
  }


  // Now fix arguments

  switch (op) { 
    // Handle instructions with immediate x
    case 0xb4 ... 0xb5:				// stco/stcoi
      x = ox;
      goto imm_z;

    // Handle instructions with immediate y
    case 0x34 ... 0x37:				// neg/negi/negu/negui
      y = oy;
      goto imm_z;

    // Handle instructions with immediate z
    case 0x08 ... 0x0f:
    case 0x18 ... 0x33:
    case 0x38 ... 0x3f:
    case 0x60 ... 0xb3:
    case 0xb6 ... 0xdf:
    case 0xf6 ... 0xf7:
    imm_z:
      if ((op & 0x01) == 0x01) {
	z = oz;
      }
      break;

    // Handle instructions with immediate y & z
    case 0x40 ... 0x5f:				// bcc/pbcc
    case 0xf2 ... 0xf3:				// pushj/pushjb
    case 0xf4 ... 0xf5:				// geta/getab
    case 0xf8:					// pop
      z = (oy << 8) | oz;
      break;

    case 0xe0 ... 0xe3:				// set{h,mh,ml,l}
      y = 0;	// So add can handle it
      z = shift_imm(op, (oy << 8) | oz);
      break;

    case 0xe4 ... 0xef:				// {inc,or,and}{h,mh,ml,l}
      y = x;	// So add/or/and can handle it
      z = shift_imm(op, (oy << 8) | oz);
      break;


    // Handle instructions with immediate x & y & z
    case 0xf0 ... 0xf1:				// jmp/jmpb
    case 0xf9:					// resume
    case 0xfc:					// sync
      z = (ox << 16) | (oy << 8) | oz;
      break;
  }


  // Handle instructions where x is destination
  switch (op) {
    case 0x00:					// trap
    case 0x40 ... 0x5f:				// bcc/pbcc
    case 0x9a ... 0x9d:				// preld/prego
    case 0xa0 ... 0xb7:				// st*
    case 0xb8 ... 0xbd:				// syncd/prest/syncid
    case 0xf0 ... 0xf1:				// jmp
    case 0xf6 ... 0xf9:				// put/pop/resume
    case 0xfb ... 0xfd:				// unsave/sync/swym
    case 0xff:					// trip
      // Don't do anything
      break;

    default:
      // X is destination
      if (ox >= state->g[rG]) {
	r = &GLOBAL(state, ox);
      }
      else {
	while (ox >= state->g[rL]) {
	  enum pcode_error e = allocate_local(state);
	  
	  if (e != pcode_ok) {
	    return e;
	  }
	}
	
	r =  &LOCAL(state, ox);
      }
      break;
  }
  

  // Default next address
  *pc += 4; 

  // Now handle the instructions.  Since we don't handle overflow,
  // lots of op-codes will be handled the same.

  switch (op) {
    case 0x18 ... 0x1b:				// mul/muli/mulu/mului
      *r = y * z;
      break;

    case 0x1c ... 0x1d:				// div/divi
      *r = (z == 0 ? 0 : (int64_t) y / (int64_t) z);
      break;

    case 0x1e ... 0x1f:				// div/divi
      *r = (z == 0 ? 0 : y / z);
      break;


    case 0x2e ... 0x2f:				// 16addu/16addui
      y *= 2;
    case 0x2c ... 0x2d:				// 8addu/8addui
      y *= 2;
    case 0x2a ... 0x2b:				// 4addu/4addui
      y *= 2;
    case 0x28 ... 0x29:				// 2addu/2addui
      y *= 2;
    case 0x20 ... 0x23:				// add/addi/addu/addui
    case 0xe0 ... 0xe7:				// {set,inc}{h,mh,ml,l}
      *r = y + z;
      break;

    case 0x24 ... 0x27:				// sub/subi/subu/subui
    case 0x34 ... 0x37:				// neg/negi/negu/negui
      *r = y - z;
      break;


    case 0x30 ... 0x31:				// cmp/cmpi
      *r = ((int64_t) y > (int64_t) z) - ((int64_t) y < (int64_t) z);
      break;

    case 0x32 ... 0x33:				// cmpu/cmpui
      *r = (y > z) - (y < z);
      break;

      
    case 0x38 ... 0x3b:				// sl/sli/slu/slui
      *r = y << z;
      break;

    case 0x3c ... 0x3d:				// sr/sri
      *r = (int64_t) y >> z;
      break;

    case 0x3e ... 0x3f:				// sru/srui
      *r = y >> z;
      break;


    case 0x40 ... 0x5f:				// bcc/pbcc
      if (cc(op, (int64_t) x)) {
	*pc = *pc - 4 /* undo */ + calc_ra16(op, z);
      }
      break;

    case 0x60 ... 0x6f:				// cscc
      if (cc(op, (int64_t) y)) {
	*r = z;
      }
      break;

    case 0x70 ... 0x7f:				// zscc
      if (cc(op, (int64_t) y)) {
	*r = z;
      }
      else {
	*r = 0;
      }
      break;


    case 0x80 ... 0x81:				// ldb/ldbi
      *r = read_int8(calc_m(state, y+z, 1));
      break;

    case 0x82 ... 0x83:				// ldbu/ldbui
      *r = read_uint8(calc_m(state, y+z, 1));
      break;

    case 0x84 ... 0x85:				// ldw/ldwi
      *r = read_int16(calc_m(state, y+z, 2));
      break;

    case 0x86 ... 0x87:				// ldwu/ldwui
      *r = read_uint16(calc_m(state, y+z, 2));
      break;

    case 0x88 ... 0x89:				// ldt/ldti
      *r = read_int32(calc_m(state, y+z, 4));
      break;

    case 0x8a ... 0x8b:				// ldtu/ldtui
      *r = read_uint32(calc_m(state, y+z, 4));
      break;

    case 0x8c ... 0x8f:				// ldo/ldoi/ldou/ldoui
    case 0x96 ... 0x97:				// ldunc/ldunci
      *r = read_uint64(calc_m(state, y+z, 8));
      break;

    case 0x92 ... 0x93:				// ldht/ldhti
      *r = ((uint64_t) read_uint32(calc_m(state, y+z, 4))) << 32;
      break;

    case 0x9a ... 0x9d:				// preld/prego
      // nop
      break;

    case 0x9e ... 0x9f:				// go/goi
      *r = *pc;
      *pc = (uintptr_t) calc_m(state, y+z, 4);
      break;

    case 0xa0 ... 0xa3:				// stb/stbi/stbu/stbui
    case 0xb4 ... 0xb5:				// stco/stcoi
      write_uint8(calc_m(state, y+z, 1), x);
      break;

    case 0xa4 ... 0xa7:				// stw/stwi/stwu/stwui
      write_uint16(calc_m(state, y+z, 2), x);
      break;

    case 0xa8 ... 0xab:				// stt/stti/sttu/sttui
      write_uint32(calc_m(state, y+z, 4), x);
      break;

    case 0xac ... 0xaf:				// sto/stoi/stou/stoui
    case 0xb6 ... 0xb7:				// stunc/stunci
      write_uint64(calc_m(state, y+z, 8), x);
      break;

    case 0xb2 ... 0xb3:				// stht/sthti
      write_uint32(calc_m(state, y+z, 4), x >> 32);
      break;

    case 0xba ... 0xbb:				// prest
      // nop
      break;

    case 0xbe ... 0xbf:				// pushgo/pushgoi
      state->g[rJ] = *pc;
      *pc = (uintptr_t) calc_m(state, y+z, 4);
      goto push;

    case 0xc0 ... 0xc1:				// or/ori
    case 0xe8 ... 0xeb:				// or{h,mh,ml,l}
      *r = y | z;
      break;

    case 0xc2 ... 0xc3:				// orn/orni
      *r = y | ~z;
      break;

    case 0xc4 ... 0xc5:				// nor/nori
      *r = ~(y | z);
      break;

    case 0xc6 ... 0xc7:				// xor/xori
      *r = y ^ z;
      break;

    case 0xc8 ... 0xc9:				// and/andi
    case 0xec ... 0xef:				// and{h,mh,ml,l}
      *r = y | z;
      break;

    case 0xca ... 0xcb:				// andn/andni
      *r = y & ~z;
      break;

    case 0xcc ... 0xcd:				// nand/nandi
      *r = ~(y & z);
      break;

    case 0xce ... 0xcf:				// nxor/nxori
      *r = ~(y ^ z);
      break;

    case 0xd8 ... 0xd9:				// mux/muxi
      *r = (y & state->g[rM]) | (z & ~state->g[rM]);
      break;

    case 0xdc ... 0xdd:				// mor/mori
      *r = mor(y, z);
      break;

    case 0xde ... 0xdf:				// mxor/mxori
      *r = mxor(y, z);
      break;

    case 0xf0 ... 0xf1:				// jmp/jmpb
      *pc = *pc - 4 /* undo */ + calc_ra24(op, z);
      break;

    case 0xf2 ... 0xf3:				// pushj/pushjb
      state->g[rJ] = *pc;
      *pc = *pc - 4 /* undo */ + calc_ra16(op, z);

    push:
      if (ox >= state->g[rG]) {
	ox = state->g[rL];
	allocate_local(state);
      }

      LOCAL(state, ox) = ox;

      state->alpha  = (state->alpha + (ox + 1)) % LOCALS;
      state->g[rO] += (ox + 1) * 8;

      state->beta   = (state->beta - (ox + 1)) % LOCALS;
      state->g[rL] -= (ox + 1);
      break;

    case 0xf4 ... 0xf5:				// geta/getab
      *r = *pc - 4 /* undo */ + calc_ra16(op, z);
      break;

    case 0xf6 ... 0xf7:				// put/puti
      if (ox >= rC && ox <= rS) {
	// rC, rN, rO and rS can't be changed
	return pcode_illegal_instruction;
      }
      else if (ox >= rI && ox <= rV) {
	// rI, rT, rTT, rK rQ, rU and rV changes not allowed
	return pcode_priv_instruction;
      }
      else if (ox == rG) {
	if (z < (256-GLOBALS) || z > 255 || z < state->g[rL]) {
	  // rG must not be out of bounds or less than rL
	  return pcode_illegal_instruction;
	}
	else {
	  int i;

	  for (i = z; i < (int) state->g[rG]; ++i) {
	    // Clear new global registers
	    GLOBAL(state, i) = 0;
	  }
	}
      }
      else if (ox == rL) {
	if (z > state->g[rL]) {
	  // Can't increase rL
	  z = state->g[rL];
	}
      }

      state->g[ox] = z;
      break;

    case 0xf8: {				// pop
      uint64_t main_res = 0;

      if (ox > 0 && ox < state->g[rL]) {
	main_res = LOCAL(state, ox - 1);
      }

      not finished!

      *pc = state->g[rJ] + 4 * z;
      break;
    }
     
    case 0xfd:					// swym
      // nop
      break;

    case 0xfe:					// get
      if (z >= 32) {
	return pcode_illegal_instruction;
      }

      *r = state->g[z];
      break;

    case 0x01 ... 0x17:				// (fp insructions)
    case 0x90 ... 0x91:				// ldsf/ldsfi
    case 0x94 ... 0x95:				// cswap/cswapi
    case 0x98 ... 0x99:				// ldvts/ldvtsi
    case 0xb0 ... 0xb1:				// stsf/stsfi
    case 0xd0 ... 0xd7:				// {b,w,t,o}dif/{b,w,t,o}difi
    case 0xda ... 0xdb:				// sadd/saddi
    case 0xb8 ... 0xb9:				// syncd
    case 0xbc ... 0xbd:				// syncid
    case 0xf9:					// resume
    case 0xfa ... 0xfb:				// save/unsave
    case 0xfc:					// sync
    case 0xff:					// trip
    default:
      return pcode_illegal_instruction;
  }
  
  return pcode_ok;
}


/*** The public API **********************************************************/

pcode_handle pcode_create(size_t data_size, uint64_t load_address,
			  void const* pcode, size_t pcode_size, 
			  struct pcode_ops const* ops) {
  size_t        state_size = sizeof (struct state) + pcode_size + data_size;
  struct state* state      = ops->malloc(ops, state_size);

  if (state != NULL) {
    memset(state, 0, sizeof (*state));

    state->ops = ops;

    // Load P-Code
    state->text_start = load_address;
    state->text_end   = load_address + pcode_size;
    memcpy(state->text, pcode, pcode_size);
  }

  return state;
}


enum pcode_error pcode_execute(pcode_handle handle, uint64_t address) {
  struct state*    state = (struct state*) handle;
  enum pcode_error error;

  // Initialize stack/register pointers
//  state->g[rO] = (uintptr_t) &state->s[0];
  state->g[rS] = (uintptr_t) &state->s[0];
  state->g[SP] = (uintptr_t) &state->s[STACK - 1];

  state->s[STACK - 1] = (uint64_t) -1; // Return address

  state->g[rG] = 256 - GLOBALS;
  state->g[rL] = 0;

  allocate_local(state);  
  state->l[0]  = (uintptr_t) &state->text[state->text_end - state->text_start];

  do {
    uint64_t* d = (uint64_t*) (uintptr_t) &state->text[state->text_end - state->text_start];
    state->ops->kprintf(state->ops,
			"R: %016lx %016lx %016lx %016lx\n",
			state->l[0], state->l[1], state->l[2], state->l[3]);
    state->ops->kprintf(state->ops,
			"R: %016lx %016lx %016lx %016lx\n",
			state->l[4], state->l[5], state->l[6], state->l[7]);


    state->ops->kprintf(state->ops,
			"D: %016lx %016lx %016lx %016lx\n",
			read_uint64(d+0), read_uint64(d+1), read_uint64(d+2), read_uint64(d+3));
    error = execute_op(state, &address);
    state->ops->kprintf(state->ops,"return: %d\n", error);
  } while (error == pcode_ok && address != (uint64_t) -1);

  return error;
}


void pcode_delete(pcode_handle handle) {
  struct state* state = (struct state*) handle;

  if (state != NULL) {
    state->ops->free(state->ops, state);
  }
}
