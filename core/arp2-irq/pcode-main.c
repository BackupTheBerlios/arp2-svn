
#include <stdio.h>
#include <stdarg.h>

#include "pcode.h"

#include "pcode-test.h"

extern const char pcode_test[];
extern const int  pcode_test_size;

void* impl_malloc(struct pcode_ops const* ops, size_t size) {
  (void) ops;

  return malloc(size);
}

void impl_free(struct pcode_ops const* ops, void* addr) {
  (void) ops;

  free(addr);
}

void impl_kprintf(struct pcode_ops const* ops, char* fmt, ...) {
  (void) ops;
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}


struct pcode_ops const ops = {
  .malloc  = impl_malloc,
  .free    = impl_free,
  .kprintf = impl_kprintf,
};

int main(void) {
  pcode_handle h = pcode_create(sizeof (struct Locals), 0x100, 
				pcode_test, pcode_test_size, &ops);

  if (h != NULL) {
    pcode_execute(h, 0x10c);
    pcode_delete(h);
  }

  return 0;
}
