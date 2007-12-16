/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef PROTO_ARP2_SYSCALL_H
#define PROTO_ARP2_SYSCALL_H

#include <clib/arp2_syscall_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/arp2_syscall.h>
#  else
#   include <inline/arp2_syscall.h>
#  endif
# else
#  include <pragmas/arp2_syscall_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/arp2_syscall.h>
# ifndef __NOGLOBALIFACE__
   extern struct ARP2_SysCallIFace *IARP2_SysCall;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct Library*
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  ARP2_SysCallBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_ARP2_SYSCALL_H */
