
#include <stdio.h>
#include <stdarg.h>
#include <byteswap.h>

#include "pcode.h"

#include "pcode-test.h"

extern const char pcode_test[];
extern const int  pcode_test_size;

static void* impl_malloc(struct pcode_ops const* ops, size_t size) {
  (void) ops;

  return malloc(size);
}

static void impl_free(struct pcode_ops const* ops, void* addr) {
  (void) ops;

  free(addr);
}

static void impl_kprintf(struct pcode_ops const* ops, char* fmt, ...) {
  (void) ops;
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

uint64_t impl_ReadResource8(struct pcode_ops const* ops, 
			    uint32_t resource, uint32_t offset){
  ops->kprintf(ops, "ReadResource8(%d, %x)\n", resource, offset);
  return 0;
}

uint64_t impl_ReadResource16(struct pcode_ops const* ops, 
			     uint32_t resource, uint32_t offset){
  ops->kprintf(ops, "ReadResource16(%d, %x)\n", resource, offset);
  return 0;
 
}

uint64_t impl_ReadResource32(struct pcode_ops const* ops, 
			     uint32_t resource, uint32_t offset){
  ops->kprintf(ops, "ReadResource32(%d, %x)\n", resource, offset);
  return 0;
}

uint64_t impl_ReadResource64(struct pcode_ops const* ops, 
			     uint32_t resource, uint32_t offset){
  ops->kprintf(ops, "ReadResource64(%d, %x)\n", resource, offset);
  return 0;
}

void impl_WriteResource8(struct pcode_ops const* ops, 
			 uint32_t resource, uint32_t offset, uint8_t value){
  ops->kprintf(ops, "WriteResource8(%d, %x, %x)\n", resource, offset, value);
}

void impl_WriteResource16(struct pcode_ops const* ops, 
			  uint32_t resource, uint32_t offset, uint16_t value){
  ops->kprintf(ops, "WriteResource16(%d, %x, %x)\n", resource, offset, value);
}

void impl_WriteResource32(struct pcode_ops const* ops,
			  uint32_t resource, uint32_t offset, uint32_t value){
  ops->kprintf(ops, "WriteResource32(%d, %x, %x)\n", resource, offset, value);
}

void impl_WriteResource64(struct pcode_ops const* ops, 
			  uint32_t resource, uint32_t offset, uint64_t value){
  ops->kprintf(ops, "WriteResource32(%d, %x, %x)\n", resource, offset, value);
}

void impl_SetEndian(struct pcode_ops const* ops, 
		    enum Endian endian){
  ops->kprintf(ops, "SetEndian(%d)\n", endian);
}

uint64_t impl_GetResourceSize(struct pcode_ops const* ops, 
			      uint32_t resource){
  ops->kprintf(ops, "GetResourceSize(%d)\n", resource);
  return 0;
}

void* impl_MapResource(struct pcode_ops const* ops, 
		       uint32_t resource, uint32_t flags, uint64_t* dma_addr){
  ops->kprintf(ops, "MapResource(%d, %x, %p)\n", resource, flags, dma_addr);
  return NULL;
}

void impl_UnmapResource(struct pcode_ops const* ops, 
			void* addr){
  ops->kprintf(ops, "UnmapResource(%p)\n", addr);
}

struct pcode_ops const ops = {
  NULL, 

  impl_malloc,
  impl_free,
  impl_kprintf,
  impl_ReadResource8,
  impl_ReadResource16,
  impl_ReadResource32,
  impl_ReadResource64,
  impl_WriteResource8,
  impl_WriteResource16,
  impl_WriteResource32,
  impl_WriteResource64,
  impl_SetEndian,
  impl_GetResourceSize,
  impl_MapResource,
  impl_UnmapResource,
};

int main(void) {
  pcode_handle h = pcode_create(sizeof (struct Locals), 0x100, 
				pcode_test, pcode_test_size, &ops);

  if (h != NULL) {
    uint32_t* vec = (uint32_t*) pcode_test;

    pcode_execute(h, bswap_32(vec[0]));
    pcode_execute(h, bswap_32(vec[1]));
    pcode_execute(h, bswap_32(vec[2]));
    pcode_delete(h);
  }

  return 0;
}
