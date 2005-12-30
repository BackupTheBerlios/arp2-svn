
#ifdef __KERNEL__
#include <linux/types.h>
typedef int       bool;
typedef ptrdiff_t uintptr_t;
#define false 0
#define true  1
#else

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#endif
