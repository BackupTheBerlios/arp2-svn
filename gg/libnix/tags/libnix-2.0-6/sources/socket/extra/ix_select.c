
#include "libnix.h"

int ix_select(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd, struct timeval *timeout,long *mask)
{ return lx_select(nfd, ifd, ofd, efd, timeout, (u_long*) mask); }
