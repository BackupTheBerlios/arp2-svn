
#include "pcode-traps.h"

__asm("					\n\
1:					\n\
	.int	init-1b			\n\
	.int	acknowledge-1b		\n\
	.int	release-1b		\n\
");

struct Locals {
    uint32_t intreq;
    uint32_t intreqr;
};


int init(struct Locals* l) {
  l->intreq  = 0xdff09c;
  l->intreqr = 0xdff09a;

  return 0;
}

int acknowledge(struct Locals* l) {
  uint16_t intreq = ReadResource16(13, l->intreqr);

  if (intreq & 0x0080) {
    intreq &= ~0x0080;

    WriteResource16(12, l->intreq, intreq);
    return 1;
  }
  else {
    return 0;
  }
}

int release() { 
  return 0; 
}
