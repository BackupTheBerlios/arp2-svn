
#include "pcode-traps.h"

#include "pcode-test.h"

__asm("					\n\
	.int	init			\n\
	.int	acknowledge		\n\
	.int	release			\n\
");

long add(long a, long b) {
  return a + b;
}

long sub(long a, long b) {
  return a - b;
}

long neg(long a) {
  return -a;
}

long not(long a) {
  return ~a;
}

long and(long a, long b) {
  return a & b;
}

long andn(long a, long b) {
  return a & b;
}

long or(long a, long b) {
  return a | b;
}

long orn(long a, long b) {
  return a | ~b;
}

void test() {
  if (add(10, 99) != 109)          __asm("trap 0,0,0");
  if (sub(10, 99) != -89)          __asm("trap 0,0,1");
  if (neg(-10) != 10)              __asm("trap 0,0,2");
  if (not(0x80) != ~0x80)          __asm("trap 0,0,3");
  if (and(0x180,0x100) != 0x100)   __asm("trap 0,0,5");
  if (or(0x80,0x100) != 0x180)     __asm("trap 0,0,6");
  if (andn(0x180,0x100) != 0x100)  __asm("trap 0,0,7");
  if (orn(0x80,0x100) != ~0x100)   __asm("trap 0,0,8");
}

int init(struct Locals* l) {
  l->intreq  = 0xdff09c;
  l->intreqr = 0xdff09a;

  test();
  return add(l->intreq, l->intreqr);
}

int acknowledge(struct Locals* l) {
  uint16_t intreq = ReadResource16(13, l->intreqr);
  intreq = bswap16(intreq);

  if (intreq & 0x0080) {
    intreq &= ~0x0080;

    SetEndian(ENDIAN_BIG);
    WriteResource16(12, l->intreq, intreq);
    return 1;
  }
  else {
    return 0;
  }
}

int release() { 
  uint64_t  dummy;
  uint64_t* fb = MapResource(0, MAPF_WRITE, &dummy);
  uint64_t  sz = GetResourceSize(0) / sizeof (uint64_t);

  // Clear frame buffer
  if (fb != NULL) {
    size_t i;
    for (i = 0; i < sz; ++i) {
      fb[i] = 0;
    }
  }

  UnmapResource(fb);
  
  return 0; 
}
