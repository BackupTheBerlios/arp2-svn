#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int getpeername(int s, struct sockaddr *asa, int *alen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);
  int rc;

  if (fp == NULL) {
    return -1;
  }
  
  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {
    case LX_AS225:
      rc = SOCK_getpeername(fp->lx_sock,asa,alen);
    break;

    case LX_AMITCP:
      rc = TCP_GetPeerName(fp->lx_sock,asa,(LONG*) alen);
    break;

    default:
      rc = -1;
    break;
  }

  return rc;
}
