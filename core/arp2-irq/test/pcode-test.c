
#include "pcode-traps.h"

#include "pcode-test.h"

__asm("					\n\
	.int	init			\n\
	.int	acknowledge		\n\
	.int	release			\n\
");

int add(int a, int b) {
  return a + b;
}


int init(struct Locals* l) {
  l->intreq  = 0xdff09c;
  l->intreqr = 0xdff09a;

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
