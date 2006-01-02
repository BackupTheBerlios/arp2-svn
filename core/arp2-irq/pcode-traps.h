#ifndef pcode_calls_h
#define pcode_calls_h

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;

#define _readres_sc(sc, resource, offset) ({			\
      register uint64_t _res __asm("231");			\
      register uint32_t _a1  __asm("231") = resource;		\
      register uint32_t _a2  __asm("232") = offset;		\
      __asm volatile ("trap 1,0,%1" 				\
		      : "=r" (_res)				\
		      : "i" (sc),				\
		        "r" (_a1), "r" (_a2)			\
		      : "memory");				\
      _res;							\
    })

#define _writeres_sc(sc, resource, offset, value) ({		\
      register uint32_t _a1  __asm("231") = resource;		\
      register uint32_t _a2  __asm("232") = offset;		\
      register uint64_t _a3  __asm("233") = value;		\
      __asm volatile ("trap 1,0,%0" 				\
		      : 					\
		      : "i" (sc),				\
		        "r" (_a1), "r" (_a2), "r" (_a3)		\
		      : "memory", "255");			\
    })

static inline uint64_t ReadResource8(uint32_t resource, uint32_t offset) {
  return _readres_sc(0, resource, offset);
}

static inline uint64_t ReadResource16(uint32_t resource, uint32_t offset) {
  return _readres_sc(1, resource, offset);
}

static inline uint64_t ReadResource32(uint32_t resource, uint32_t offset) {
  return _readres_sc(2, resource, offset);
}

static inline uint64_t ReadResource64(uint32_t resource, uint32_t offset) {
  return _readres_sc(3, resource, offset);
}

static inline void WriteResource8(uint32_t resource, uint32_t offset, uint8_t value) {
  return _writeres_sc(4, resource, offset, value);
}

static inline void WriteResource16(uint32_t resource, uint32_t offset, uint16_t value) {
  return _writeres_sc(5, resource, offset, value);
}

static inline void WriteResource32(uint32_t resource, uint32_t offset, uint32_t value) {
  return _writeres_sc(6, resource, offset, value);
}

static inline void WriteResource64(uint32_t resource, uint32_t offset, uint64_t value) {
  return _writeres_sc(7, resource, offset, value);
}

#undef _readres_sc
#undef _writeres_sc

#endif /* pcode_calls_h */
