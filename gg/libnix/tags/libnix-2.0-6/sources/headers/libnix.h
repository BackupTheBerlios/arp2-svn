#ifndef __LIBNIX_H
#define __LIBNIX_H

#include <sys/types.h>

struct timeval;

/* Like select() but you can pass a pointer to an extra bitmask as the last
 * argument. In that case select() will also wait on these signal bits and
 * return if one of these signals came in. The contents of mask will be set
 * to the signals that arrived.
 *
 * This is the same as ix_select() in ixemul and WaitSelect() in AmiTCP.
 *
 * Note that ix_select() is also aliased in libnix, in case you prefer
 * to include <ix.h> instead of this file. (For ix_select(), the last
 * argument is a long pointer instead of an u_long pointer.)
 */
int lx_select(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd,
              struct timeval *timeout, u_long *mask);

#endif /* __LIBNIX_H */
