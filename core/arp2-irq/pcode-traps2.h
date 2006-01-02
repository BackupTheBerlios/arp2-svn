#ifndef pcode_calls_h
#define pcode_calls_h

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;

#define _syscall(syscall, args) ({			\
      register uint64_t _res __asm("255");		\
      register void*    _arg __asm("255") = args;	\
      __asm volatile ("trap 1,0,%1" 			\
		      : "=r" (_res)			\
		      : "i" (syscall), "r" (_arg)	\
		      : "memory");			\
      _res;						\
    })

#define _syscall_nr(syscall, args) ({			\
      register void* _arg __asm("255") = args;		\
      __asm volatile ("trap 1,0,%0" 			\
		      : 				\
		      : "i" (syscall), "r" (_arg)	\
		      : "memory");			\
    })


struct ReadResourceArgs {
    uint32_t resource;
    uint32_t offset;
};


struct WriteResourceArgs {
    uint32_t resource;
    uint32_t offset;
    uint64_t value;
};


static inline uint64_t ReadResource8(uint32_t resource, uint32_t offset) {
  struct ReadResourceArgs args = { resource, offset };
  return _syscall(0, &args);
}

static inline uint64_t ReadResource16(uint32_t resource, uint32_t offset) {
  struct ReadResourceArgs args = { resource, offset };
  return _syscall(1, &args);
}

static inline uint64_t ReadResource32(uint32_t resource, uint32_t offset) {
  struct ReadResourceArgs args = { resource, offset };
  return _syscall(2, &args);
}

static inline uint64_t ReadResource64(uint32_t resource, uint32_t offset) {
  struct ReadResourceArgs args = { resource, offset };
  return _syscall(3, &args);
}

static inline void WriteResource8(uint32_t resource, uint32_t offset, uint8_t value) {
  struct WriteResourceArgs args = { resource, offset, value };
  _syscall_nr(4, &args);
}

static inline void WriteResource16(uint32_t resource, uint32_t offset, uint16_t value) {
  struct WriteResourceArgs args = { resource, offset, value };
  _syscall_nr(5, &args);
}

static inline void WriteResource32(uint32_t resource, uint32_t offset, uint32_t value) {
  struct WriteResourceArgs args = { resource, offset, value };
  _syscall_nr(6, &args);
}

static inline void WriteResource64(uint32_t resource, uint32_t offset, uint64_t value) {
  struct WriteResourceArgs args = { resource, offset, value };
  _syscall_nr(7, &args);
}

#endif /* pcode_calls_h */
