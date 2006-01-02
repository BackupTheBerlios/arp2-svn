#ifndef pcode_calls_h
#define pcode_calls_h

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;

#define _readres_sc(sc, sysjmp, resource, offset) ({		\
      register uint64_t _res __asm("0");			\
      register uint32_t _a1  __asm("1") = resource;		\
      register uint32_t _a2  __asm("2") = offset;		\
      __asm volatile ("go $255,%1,%2" 				\
		      : "=r" (_res)				\
		      : "r" (sc), "i" (sysjmp),			\
		        "r" (_a1), "r" (_a2)\
		      : "memory", "255");			\
      _res;							\
    })

#define _writeres_sc(sc, sysjmp, resource, offset, value) ({	\
      register uint32_t _a1  __asm("1") = resource;		\
      register uint32_t _a2  __asm("2") = offset;		\
      register uint64_t _a3  __asm("3") = value;		\
      __asm volatile ("go $255,%0,%1" 				\
		      : 					\
		      : "r" (sc), "i" (sysjmp),			\
		        "r" (_a1), "r" (_a2), "r" (_a3)		\
		      : "memory", "255");			\
    })


static inline uint64_t ReadResource8(void* sc, uint32_t resource, uint32_t offset) {
  return _readres_sc(sc, 0, resource, offset);
}

static inline uint64_t ReadResource16(void* sc, uint32_t resource, uint32_t offset) {
  return _readres_sc(sc, 4, resource, offset);
}

static inline uint64_t ReadResource32(void* sc, uint32_t resource, uint32_t offset) {
  return _readres_sc(sc, 8, resource, offset);
}

static inline uint64_t ReadResource64(void* sc, uint32_t resource, uint32_t offset) {
  return _readres_sc(sc, 12, resource, offset);
}

static inline void WriteResource8(void* sc, uint32_t resource, uint32_t offset, uint8_t value) {
  return _writeres_sc(sc, 16, resource, offset, value);
}

static inline void WriteResource16(void* sc, uint32_t resource, uint32_t offset, uint16_t value) {
  return _writeres_sc(sc, 20, resource, offset, value);
}

static inline void WriteResource32(void* sc, uint32_t resource, uint32_t offset, uint32_t value) {
  return _writeres_sc(sc, 24, resource, offset, value);
}

static inline void WriteResource64(void* sc, uint32_t resource, uint32_t offset, uint64_t value) {
  return _writeres_sc(sc, 28, resource, offset, value);
}

#endif /* pcode_calls_h */
