
#include "pcode.h"

// Use 32 general-purpose registers
#define GLOBALS	32

// Use 32 local registers
#define LOCALS	32				// Must be a power of two

// Use a stack size so that sizeof (struct state) <= 4096.
#define STACK	(512-(32+GLOBALS+LOCALS+2))	// Total space for
						// registers and stack
						// (longs)
#define SP	(32+GLOBALS-3)

enum special_register {
  rB, rD, rE, rH, rJ, rM, rR, rBB, rC, rN, rO, rS, rI,  rT,  rTT, rK,
  rQ, rU, rV, rG, rL, rA, rF, rP,  rW, rX, rY, rZ, rWW, rXX, rYY, rZZ
};

enum error {
  ok, stack_overflow, local_register_overflow, illegal_instruction
};


struct state {
    uint64_t  g[32+GLOBALS];		// Global registers (special+normal)
    uint64_t  l[LOCALS];		// Local registers
    uint64_t  s[STACK];			// Spilled registers/stack

    size_t    alpha;			// (r0 / 8) mod LOCALS
    size_t    beta;			// (alpha + rL) mod LOCALS
    size_t    gamma;			// (rS / 8) mod LOCALS
};


// Check that the two stack pointers are reasonable

static inline bool check_stack(struct state* state) {
  return (state->g[rS] >= (uintptr_t) &state->s[0] &&
	  state->g[rS] <  (uintptr_t) &state->s[STACK] &&
	  state->g[SP] >= state->g[rS] && 
	  state->g[SP] <= (uintptr_t) &state->s[STACK]);
}


// Push the "oldest" local register on the register stack

static enum error push_local(struct state* state) {
  uint64_t* S = (uint64_t*) (uintptr_t) state->g[rS];

  state->g[rS] += 8;

  if (!check_stack(state)) {
    state->g[rS] -= 8; // Undo
    return stack_overflow;
  }

  *S = state->l[state->gamma];
  state->gamma = (state->gamma + 1) % LOCALS;

  return ok;
}


// Pop the latest local register from the register stack

static enum error pop_local(struct state* state) {
  uint64_t* S;

  state->g[rS] -= 8;

  if (!check_stack(state)) {
    state->g[rS] += 8; // Undo
    return stack_overflow;
  }

  S = (uint64_t*) (uintptr_t) state->g[rS];

  state->gamma = (state->gamma - 1) % LOCALS;
  state->l[state->gamma]= *S;

  return ok;
}


// Initialize a previously unused local register for writing

static enum error allocate_local(struct state* state) {
  if (state->g[rL] == (LOCALS-1)) {
    return local_register_overflow;
  }

  state->l[state->beta] = 0;

  state->beta = state->beta % LOCALS;
  state->g[rL] += 1;
  
  if (state->beta == state->gamma) {
    return push_local(state);
  }
  else {
    return ok;
  }
}


static inline uint64_t shift_imm(uint8_t op, uint64_t yz) {
  return yz << ((~op & 3) * 16);
}


static inline void* calc_m(uint64_t addr, size_t size) {
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


static enum error execute_op(struct state* state, uint8_t** pc) {
  uint8_t op, ox, oy, oz;
  uint64_t x, y, z;
  uint64_t* r = NULL;

  if (!check_stack(state)) {
    return stack_overflow;
  }

  // Fetch instruction

  op = (*pc)[0];
  ox = (*pc)[1];
  oy = (*pc)[2];
  oz = (*pc)[3];


  // Default source arguments

  x = y = z = 0;

  if (ox >= state->g[rG]) {
    x = state->g[32 + ox - (256-GLOBALS)];
  }
  else if (ox < state->g[rL]) {
    x =  state->l[(state->alpha + ox) % LOCALS];
  }

  if (oy >= state->g[rG]) {
    y = state->g[32 + oy - (256-GLOBALS)];
  }
  else if (oy < state->g[rL]) {
    y =  state->l[(state->alpha + oy) % LOCALS];
  }

  if (oz >= state->g[rG]) {
    z = state->g[32 + oz - (256-GLOBALS)];
  }
  else if (oz < state->g[rL]) {
    z =  state->l[(state->alpha + oz) % LOCALS];
  }


  // Now fix arguments

  switch (op) { 
    // Handle instructions with immediate y
    case 0x34 ... 0x37:		// neg/negi/negu/negui
      y = oy;
      break;

    // Handle instructions with immediate y & z
    case 0x40 ... 0x5f:		// bcc/pbcc
    case 0xf2 ... 0xf3:		// pushj/pushjb
    case 0xf4 ... 0xf5:		// geta/getab
    case 0xf8:			// pop
      z = (oy << 8) | oz;
      break;

    case 0xb4 ... 0xb5:		// stco/stcoi
      x = ox;
      break;

    case 0xe0 ... 0xe3:		// set{h,mh,ml,l}
      y = 0;	// So add can handle it
      z = shift_imm(op, (oy << 8) | oz);
      break;

    case 0xe4 ... 0xef:		// {inc,or,and}{h,mh,ml,l}
      y = x;	// So add/or/and can handle it
      z = shift_imm(op, (oy << 8) | oz);
      break;


    // Handle instructions with immediate x & y & z
    case 0xf0 ... 0xf1:		// jmp/jmpb
    case 0xf9:			// resume
    case 0xfc:			// sync
      z = (ox << 16) | (oy << 8) | oz;
      break;
  }

  switch (op) {
    // Handle instructions with immediate z
    case 0x08 ... 0x0f:
    case 0x18 ... 0x3f:
    case 0x60 ... 0xdf:
    case 0xf6 ... 0xf7:
      if ((op & 0x01) == 0x01) {
	z = oz;
      }
      break;
  }

  switch (op) {
    case 0x00:			// trap
    case 0x40 ... 0x5f:		// bcc/pbcc
    case 0x9a ... 0x9d:		// preld/prego
    case 0xa0 ... 0xb7:		// st*
    case 0xb8 ... 0xbd:		// syncd/prest/syncid
    case 0xf0 ... 0xf1:		// jmp
    case 0xf6 ... 0xf9:		// put/pop/resume
    case 0xfb ... 0xfd:		// unsave/sync/swym
    case 0xff:			// trip
      // Don't do anything
      break;

    default:
      // X is destination
      if (ox >= state->g[rG]) {
	x = state->g[32 + ox - (256-GLOBALS)];
      }
      else {
	while (ox >= state->g[rL]) {
	  enum error e = allocate_local(state);
	  
	  if (e != ok) {
	    return e;
	  }
	}
	
	r =  &state->l[(state->alpha + ox) % LOCALS];
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
      if (z != 0) {
	*r = (int64_t) y / (int64_t) z;
      }
      else {
	*r = 0;
      }
      break;

    case 0x1e ... 0x1f:				// div/divi
      if (z != 0) {
	*r = y / z;
      }
      else {
	*r = 0;
      }
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
      *r = *(int8_t*) calc_m(y+z, 1);
      break;

    case 0x82 ... 0x83:				// ldbu/ldbui
      *r = *(uint8_t*) calc_m(y+z, 1);
      break;

    case 0x84 ... 0x85:				// ldw/ldwi
      *r = *(int16_t*) calc_m(y+z, 2);
      break;

    case 0x86 ... 0x87:				// ldwu/ldwui
      *r = *(uint16_t*) calc_m(y+z, 2);
      break;

    case 0x88 ... 0x89:				// ldt/ldti
      *r = *(int32_t*) calc_m(y+z, 4);
      break;

    case 0x8a ... 0x8b:				// ldtu/ldtui
      *r = *(uint32_t*) calc_m(y+z, 4);
      break;

    case 0x8c ... 0x8f:				// ldo/ldoi/ldou/ldoui
    case 0x96 ... 0x97:				// ldunc/ldunci
      *r = *(uint64_t*) calc_m(y+z, 8);
      break;

    case 0x92 ... 0x93:				// ldht/ldhti
      *r = ((uint64_t) *(uint32_t*) calc_m(y+z, 4)) << 32;
      break;

    case 0x9e ... 0x9f:				// go/goi
      *r = (uint64_t) (uintptr_t) *pc;
      *pc = calc_m(y+z, 4);
      break;

    case 0xa0 ... 0xa3:				// stb/stbi/stbu/stbui
    case 0xb4 ... 0xb5:				// stco/stcoi
      *(uint8_t*) calc_m(y+z, 1) = x;
      break;

    case 0xa4 ... 0xa7:				// stw/stwi/stwu/stwui
      *(uint16_t*) calc_m(y+z, 2)= x;
      break;

    case 0xa8 ... 0xab:				// stt/stti/sttu/sttui
      *(uint32_t*) calc_m(y+z, 4) = x;
      break;

    case 0xac ... 0xaf:				// sto/stoi/stou/stoui
    case 0xb6 ... 0xb7:				// stunc/stunci
      *(uint64_t*) calc_m(y+z, 8) = x;
      break;

    case 0xb2 ... 0xb3:				// stht/sthti
      *(uint32_t*) calc_m(y+z, 4) = x >> 32;
      break;


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


    case 0xf4 ... 0xf5:				// geta/getab
      *r = (uintptr_t) *pc - 4 /* undo */ + calc_ra16(op, z);
      break;

    default:
      return illegal_instruction;
  }
  
  return ok;
}




/* struct state* pcode_create(void) { */
/*   struct state* state = (struct state*) calloc(sizeof (struct state), 1); */

/*   if (state != NULL) { */
/*     // Initialize stack/register pointers */
/*     state->g[rO] = (uintptr_t) &state->l[0]; */
/*     state->g[rS] = (uintptr_t) &state->s[0]; */
/*     state->g[SP] = (uintptr_t) &state->s[STACK]; */
/*   } */
/* } */
