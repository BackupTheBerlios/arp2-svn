#ifndef __INLINE_MACROS_H
#define __INLINE_MACROS_H

/*
   General macros for Amiga function calls from native, big-endian x86 code.

   LPX - functions that take X arguments.

   Modifiers (variations are possible):
   NR - no return (void),
   UB - base will be given explicitly by user (see cia.resource).

*/

#ifndef __INLINE_STUB_H
#include <inline/stubs.h>
#endif

#ifndef __INLINE_MACROS_H_REGS
#define __INLINE_MACROS_H_REGS

struct _Regs
{
    ULONG reg_d0;
    ULONG reg_d1;
    ULONG reg_d2;
    ULONG reg_d3;
    ULONG reg_d4;
    ULONG reg_d5;
    ULONG reg_d6;
    ULONG reg_d7;
    ULONG reg_a0;
    ULONG reg_a1;
    ULONG reg_a2;
    ULONG reg_a3;
    ULONG reg_a4;
    ULONG reg_a5;
    ULONG reg_a6;
};

ULONG _CallOS68k(ULONG o, struct _Regs* r);

#endif /* __INLINE_MACROS_H_REGS */


#define LP0(offs, rt, name, bt, bn)				\
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP0NR(offs, name, bt, bn)				\
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP1(offs, rt, name, t1, v1, r1, bt, bn)			\
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP1NR(offs, name, t1, v1, r1, bt, bn)			\
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP2(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn)	\
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP2UB(offs, rt, name, t1, v1, r1, t2, v2, r2)		\
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP2NR(offs, name, t1, v1, r1, t2, v2, r2, bt, bn)	\
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP2NRUB(offs, name, t1, v1, r1, t2, v2, r2)		\
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP3(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP3UB(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP3NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP3NRUB(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3)	\
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP4NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP5(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP5NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP6(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP6NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP7(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP7NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP8(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP8NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP9(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_##r9  = (ULONG) (v9);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP9NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_##r9  = (ULONG) (v9);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP10(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_##r9  = (ULONG) (v9);				\
      _regs.reg_##r10 = (ULONG) (v10);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP10NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_##r9  = (ULONG) (v9);				\
      _regs.reg_##r10 = (ULONG) (v10);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})

#define LP11(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn) \
({								\
   {								\
      rt _##name##_re;                                          \
      struct _Regs _regs; 					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_##r9  = (ULONG) (v9);				\
      _regs.reg_##r10 = (ULONG) (v10);				\
      _regs.reg_##r11 = (ULONG) (v11);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _##name##_re    = (rt) _CallOS68k(offs,&_regs);		\
      _##name##_re;                                             \
   }								\
})

#define LP11NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn) \
({								\
   {								\
      struct _Regs _regs;					\
      _regs.reg_##r1  = (ULONG) (v1);				\
      _regs.reg_##r2  = (ULONG) (v2);				\
      _regs.reg_##r3  = (ULONG) (v3);				\
      _regs.reg_##r4  = (ULONG) (v4);				\
      _regs.reg_##r5  = (ULONG) (v5);				\
      _regs.reg_##r6  = (ULONG) (v6);				\
      _regs.reg_##r7  = (ULONG) (v7);				\
      _regs.reg_##r8  = (ULONG) (v8);				\
      _regs.reg_##r9  = (ULONG) (v9);				\
      _regs.reg_##r10 = (ULONG) (v10);				\
      _regs.reg_##r11 = (ULONG) (v11);				\
      _regs.reg_a6    = (ULONG) (bn);				\
      _CallOS68k(offs,&_regs);					\
   }								\
})


#endif /* __INLINE_MACROS_H */
