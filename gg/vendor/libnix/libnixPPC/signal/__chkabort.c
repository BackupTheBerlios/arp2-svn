#include <signal.h>
#include <powerup/gcclib/powerup_protos.h>

void __chkabort(void)
{
  if (PPCSetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    raise(SIGINT);
}
