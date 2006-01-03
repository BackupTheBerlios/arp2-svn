#ifndef pcode_calls_h
#define pcode_calls_h

/*** Type definitions ********************************************************/

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;
typedef unsigned long  size_t;
typedef long           ssize_t;

#define NULL ((void*) 0)

/*** Syscall macros **********************************************************/

#define _sc1(sc, t1,a1) ({					\
      register uint64_t _res __asm("231");			\
      register t1 _a1  __asm("231") = (a1);			\
      __asm volatile ("trap 1,0,%1" 				\
		      : "=r" (_res)				\
		      : "i" (sc),				\
		        "r" (_a1)				\
		      : "memory");				\
      _res;							\
    })


#define _sc2(sc, t1, a1, t2, a2) ({				\
      register uint64_t _res __asm("231");			\
      register t1 _a1  __asm("231") = (a1);			\
      register t2 _a2  __asm("232") = (a2);			\
      __asm volatile ("trap 1,0,%1" 				\
		      : "=r" (_res)				\
		      : "i" (sc),				\
		        "r" (_a1), "r" (_a2)			\
		      : "memory");				\
      _res;							\
    })

#define _sc3(sc, t1, a1, t2, a2, t3, a3) ({			\
      register uint64_t _res __asm("231");			\
      register t1 _a1  __asm("231") = (a1);			\
      register t2 _a2  __asm("232") = (a2);			\
      register t3 _a3  __asm("233") = (a3);			\
      __asm volatile ("trap 1,0,%1" 				\
		      : "=r" (_res)				\
		      : "i" (sc),				\
		        "r" (_a1), "r" (_a2), "r" (_a3)		\
		      : "memory", "255");			\
      _res;							\
    })


/*** Byteswap functions ******************************************************/

#define _mor(a1, a2)  ({					\
      register uint64_t _res;					\
      __asm("mor %0,%1,%2" : "=r" (_res) : "r" (a1), "r" (a2));	\
      _res;							\
    })


static inline uint64_t bswap16(uint16_t x) {
  return _mor(x, 0x0102);
}

static inline uint64_t bswap32(uint32_t x) {
  return _mor(x, 0x01020304);
}

static inline uint64_t bswap64(uint64_t x) {
  return _mor(x, 0x0102030405060708);
}


/*** Device functions ********************************************************/

enum Endian {	/* SetEndian() flags */
  ENDIAN_BIG	= 1,
  ENDIAN_LITTLE	= 2,
};

enum {		/* MapResource() flags */
  MAPF_READ	= 1,
  MAPF_WRITE	= 2,
};


static inline uint64_t ReadResource8(uint32_t resource, uint32_t offset) {
  return _sc2(0, uint32_t, resource, uint32_t, offset);
}

static inline uint64_t ReadResource16(uint32_t resource, uint32_t offset) {
  return _sc2(1, uint32_t, resource, uint32_t, offset);
}

static inline uint64_t ReadResource32(uint32_t resource, uint32_t offset) {
  return _sc2(2, uint32_t, resource, uint32_t, offset);
}

static inline uint64_t ReadResource64(uint32_t resource, uint32_t offset) {
  return _sc2(3, uint32_t, resource, uint32_t, offset);
}

static inline void WriteResource8(uint32_t resource, uint32_t offset, uint8_t value) {
  _sc3(4, uint32_t, resource, uint32_t, offset, uint8_t, value);
}

static inline void WriteResource16(uint32_t resource, uint32_t offset, uint16_t value) {
  _sc3(5, uint32_t, resource, uint32_t, offset, uint16_t, value);
}

static inline void WriteResource32(uint32_t resource, uint32_t offset, uint32_t value) {
  _sc3(6, uint32_t, resource, uint32_t, offset, uint32_t, value);
}

static inline void WriteResource64(uint32_t resource, uint32_t offset, uint64_t value) {
  _sc3(7, uint32_t, resource, uint32_t, offset, uint64_t, value);
}

static inline void SetEndian(enum Endian endian) {
  _sc1(8, uint64_t, endian);
}

static inline uint64_t GetResourceSize(uint32_t resource) {
  return _sc1(9, uint32_t, resource);
}

static inline void* MapResource(uint32_t resource, uint32_t flags, uint64_t* dma_addr) {
  return (void*) _sc3(10, uint32_t, resource, uint32_t, flags, uint64_t*, dma_addr);
}

static inline void UnmapResource(void* addr) {
  _sc1(11, void*, addr);
}

/*** Clean up ****************************************************************/

#undef _mor
#undef _sc1
#undef _sc2
#undef _sc3

#endif /* pcode_calls_h */
