#include <exec/types.h>
#include <emul/emulinterface.h>
#include <emul/emulregs.h>
/* Don't include any std include, or gcc will report that prototypes
   don't match. */

#define FUNC_D_D(func) \
	double func(double); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  volatile union { double d; int i[2]; } r; \
	  a1.i[0] = p[1]; a1.i[1] = p[2]; \
	  r.d = func(a1.d); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_D_DD(func) \
	double func(double, double); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  volatile union { double d; int i[2]; } a2; \
	  volatile union { double d; int i[2]; } r; \
	  a1.i[0] = p[1]; a1.i[1] = p[2]; \
	  a2.i[0] = p[3]; a2.i[1] = p[4]; \
	  r.d = func(a1.d, a2.d); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_D_DI(func) \
	double func(double, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  volatile union { double d; int i[2]; } r; \
	  a1.i[0] = p[1]; a1.i[1] = p[2]; \
	  r.d = func(a1.d, p[3]); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_D_F(func) \
	double func(float); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  float *p = (float *)REG_A7; \
	  volatile union { double d; int i[2]; } r; \
	  r.d = func(p[1]); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_D_I(func) \
	double func(int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } r; \
	  r.d = func(p[1]); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_D_ID(func) \
	double func(int, double); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  volatile union { double d; int i[2]; } r; \
	  a1.i[0] = p[2]; a1.i[1] = p[3]; \
	  r.d = func(p[1], a1.d); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_D_II(func) \
	double func(int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } r; \
	  r.d = func(p[1], p[2]); \
	  REG_D1 = r.i[1]; \
	  return r.i[0];   \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_F_D(func) \
	float func(double); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  volatile union { float f; int i; } r; \
	  a1.i[0] = p[1]; a1.i[1] = p[2]; \
	  r.f = func(a1.d); \
	  return r.i; \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_F_F(func) \
	float func(float); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  float *p = (float *)REG_A7; \
	  volatile union { float f; int i; } r; \
	  r.f = func(p[1]); \
	  return r.i; \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_F_FF(func) \
	float func(float, float); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  float *p = (float *)REG_A7; \
	  volatile union { float f; int i; } r; \
	  r.f = func(p[1], p[2]); \
	  return r.i; \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_F_I(func) \
	float func(int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { float f; int i; } r; \
	  r.f = func(p[1]); \
	  return r.i; \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_(func) \
	int func(void); \
	int _trampoline_ ## func(void) { \
	  return func(); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_D(func) \
	int func(double); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  a1.i[0] = p[1]; a1.i[1] = p[2]; \
	  return func(a1.d); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_DD(func) \
	int func(double, double); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  volatile union { double d; int i[2]; } a1; \
	  volatile union { double d; int i[2]; } a2; \
	  a1.i[0] = p[1]; a1.i[1] = p[2]; \
	  a2.i[0] = p[3]; a2.i[1] = p[4]; \
	  return func(a1.d, a2.d); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_F(func) \
	int func(float); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  float *p = (float *)REG_A7; \
	  return func(p[1]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_FF(func) \
	int func(float, float); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  float *p = (float *)REG_A7; \
	  return func(p[1], p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_I(func) \
	int func(int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IA(func) \
	int _varargs68k_ ## func(int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], (void *)p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_II(func) \
	int func(int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIA(func) \
	int _varargs68k_ ## func(int, int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], p[2], (void *)p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_III(func) \
	int func(int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIA(func) \
	int _varargs68k_ ## func(int, int, int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], p[2], p[3], (void *)p[4]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIII(func) \
	int func(int, int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p[4]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIII(func) \
	int func(int, int, int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p[4], p[5]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIIII(func) \
	int func(int, int, int, int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p[4], p[5], p[6]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIIV(func) \
	int _varargs68k_ ## func(int, int, int, int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], p[2], p[3], p[4], p + 5); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIS(func) \
	int func(int, int, int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p + 4); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIV(func) \
	int _varargs68k_ ## func(int, int, int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], p[2], p[3], p + 4); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIV(func) \
	int _varargs68k_ ## func(int, int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], p[2], p + 3); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IV(func) \
	int _varargs68k_ ## func(int, void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return _varargs68k_ ## func(p[1], p + 2); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_S(func) \
	int func(void *); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p + 1); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_P_(func) \
	void func(void); \
	void _trampoline_ ## func(void) { \
	  func(); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_Q_III(func) \
	long long func(int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  long long r = func(p[1], p[2], p[3]); \
	  REG_D1 = (LONG) r; \
	  return r >> 32; \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBD0_D1, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_S_II(func) \
	void *func(void *, int, int); \
	void *_trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  void *r = (void *)REG_A1; \
	  return func(r, p[1], p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_(func) \
	void func(void); \
	void _trampoline_ ## func(void) { \
	  func(); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_I(func) \
	void func(int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_II(func) \
	void func(int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIA(func) \
	void _varargs68k_ ## func(int, int, void *); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  _varargs68k_ ## func(p[1], p[2], (void *)p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_III(func) \
	void func(int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIII(func) \
	void func(int, int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3], p[4]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIIII(func) \
	void func(int, int, int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3], p[4], p[5]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIIIIIIIIIIII(func) \
	void func(int, int, int, int, int, int, int, int, int, int, int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIV(func) \
	void _varargs68k_ ## func(int, int, void *); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  _varargs68k_ ## func(p[1], p[2], p + 3); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_X(func) \
	extern struct EmulLibEntry _gate_ ## func;

#undef getchar
#undef setjmp
#undef sigsetjmp
#undef _setjmp
#define SYSTEM_CALL(func, vec, args, stk)  FUNC_##args(func)
#include <sys/syscall.def>
#undef SYSTEM_CALL

