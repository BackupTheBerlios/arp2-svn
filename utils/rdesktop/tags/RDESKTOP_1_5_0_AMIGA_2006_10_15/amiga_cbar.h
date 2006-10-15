#ifndef RDESKTOP_AMIGA_CBAR_H
#define RDESKTOP_AMIGA_CBAR_H

#include <utility/tagitem.h>

ULONG amiga_cbar_get_signal_mask();
BOOL amiga_cbar_open(struct TagItem const* common_window_tags);
void amiga_cbar_close();
void amiga_cbar_hide(void) ;
void amiga_cbar_set_mouse(int x, int y) ;
void amiga_cbar_handle_events(ULONG mask);

#endif /* RDESKTOP_AMIGA_CBAR_H */
