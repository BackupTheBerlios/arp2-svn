#ifndef RDESKTOP_AMIGA_CLIPBOARD_H
#define RDESKTOP_AMIGA_CLIPBOARD_H

#include <exec/ports.h>

BOOL amiga_clip_init(void);
BOOL amiga_clip_shutdown(void);
void amiga_clip_deinit(void);
void amiga_clip_handle_signals();

extern ULONG amiga_clip_signals;

#endif /* RDESKTOP_AMIGA_CLIPBOARD_H */
