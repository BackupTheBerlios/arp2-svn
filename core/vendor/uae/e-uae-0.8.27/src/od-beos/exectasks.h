 /*
  * UAE - The Un*x Amiga Emulator
  *
  * exec.library multitasking emulation
  *
  * Copyright 1996 Bernd Schmidt
  */

#if 0
struct switch_struct {
	int	__ss__dummy;
};
#define EXEC_SWITCH_TASKS(run, ready) do { ; } while(0)

#define EXEC_SETUP_SWS(t) do { ; } while(0)
#endif

#include <machdep/exectasks.h>
