 /*
  * UAE - The Un*x Amiga Emulator
  *
  * ARP2 ROM syscall gateway
  *
  * Copyright 2006 Martin Blom
  *
  * Patches a MakeLibrary()-type function pointer table inside an ARP2
  * ROM image.
  *
  */

#ifdef ARP2ROM


/*** Include prototypes for all functions we export **************************/

#define _GNU_SOURCE
#include <fcntl.h>
#include <mqueue.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <attr/xattr.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/fsuid.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <sys/klog.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/swap.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "arp2sys.h"


#ifdef WORDS_BIGENDIAN
# define BE32(x) (x)
#else
# define BE32(x) bswap_32(x)
#endif


/*** arp2-syscall.resource types *********************************************/

typedef struct {
    uae_u32 d0;
    uae_u32 d1;
    uae_u32 d2;
    uae_u32 d3;
    uae_u32 d4;
    uae_u32 d5;
    uae_u32 d6;
    uae_u32 d7;
    uae_u32 a0;
    uae_u32 a1;
    uae_u32 a2;
    uae_u32 a3;
    uae_u32 a4;
    uae_u32 a5;
    uae_u32 a6;
    uae_u32 a7;
}* regptr;


typedef void* aptr;
typedef char* strptr;
typedef const void* const_aptr;
typedef const char* const_strptr;
typedef uae_s32 arp2_pid_t;
typedef uae_s32 arp2_clock_t;
typedef uae_u32 arp2_uid_t;
typedef uae_u32 arp2_gid_t;
typedef uae_u32 arp2_mode_t;
typedef uae_u32 arp2_priority_which_t;
typedef uae_u32 arp2_id_t;
typedef uae_u64 arp2_dev_t;

/* union arp2_sigval { */
/*     uae_s32 sival_int; */
/*     uae_u32 sival_ptr; */
/* }; */


/*** The actual syscall wrappers *********************************************/


uae_s32
arp2sys_readahead(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_readahead(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___offset = ((uae_u64) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  uae_u32 ___count = (uae_u32) _regs->d3;
  return readahead(___fd, ___offset, ___count);
}

/* uae_s32 */
/* arp2sys_sync_file_range(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sync_file_range(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   uae_u64 ___from = (uae_u64) _regs->d1; */
/*   uae_u64 ___to = (uae_u64) _regs->d3; */
/*   uae_u32 ___flags = (uae_u32) _regs->d5; */
/*   return sync_file_range(___fd, ___from, ___to, ___flags); */
/* } */

/* uae_s32 */
/* arp2sys_vmsplice(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_vmsplice(regptr _regs) */
/* { */
/*   uae_s32 ___fdout = (uae_s32) _regs->d0; */
/*   const struct arp2_iovec* ___iov = (const struct arp2_iovec*) _regs->a0; */
/*   uae_u32 ___count = (uae_u32) _regs->d1; */
/*   uae_u32 ___flags = (uae_u32) _regs->d2; */
/*   return vmsplice(___fdout, ___iov, ___count, ___flags); */
/* } */

/* uae_s32 */
/* arp2sys_splice(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_splice(regptr _regs) */
/* { */
/*   uae_s32 ___fdin = (uae_s32) _regs->d0; */
/*   uae_u64* ___offin = (uae_u64*) _regs->a0; */
/*   uae_s32 ___fdout = (uae_s32) _regs->d1; */
/*   uae_u64* ___offout = (uae_u64*) _regs->a1; */
/*   uae_u32 ___len = (uae_u32) _regs->d2; */
/*   uae_u32 ___flags = (uae_u32) _regs->d3; */
/*   return splice(___fdin, ___offin, ___fdout, ___offout, ___len, ___flags); */
/* } */

/* uae_s32 */
/* arp2sys_tee(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_tee(regptr _regs) */
/* { */
/*   uae_s32 ___fdin = (uae_s32) _regs->d0; */
/*   uae_s32 ___fdout = (uae_s32) _regs->d1; */
/*   uae_u32 ___len = (uae_u32) _regs->d2; */
/*   uae_u32 ___flags = (uae_u32) _regs->d3; */
/*   return tee(___fdin, ___fdout, ___len, ___flags); */
/* } */

uae_s32
arp2sys_fcntl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fcntl(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___cmd = (uae_s32) _regs->d1;
  uae_s32 ___arg = (uae_s32) _regs->a0;
  return fcntl(___fd, ___cmd, ___arg);
}

uae_s32
arp2sys_open(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_open(regptr _regs)
{
  const_strptr ___pathname = (const_strptr) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 ___mode = (uae_u32) _regs->d1;

  printf("***** open %s %x %x\n", ___pathname, ___flags, ___mode);

  return open(___pathname, ___flags, ___mode);
}

/* uae_s32 */
/* arp2sys_openat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_openat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___pathname = (const_strptr) _regs->a0; */
/*   uae_s32 ___flags = (uae_s32) _regs->d1; */
/*   uae_u32 ___mode = (uae_u32) _regs->d2; */
/*   return openat(___fd, ___pathname, ___flags, ___mode); */
/* } */

uae_s32
arp2sys_creat(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_creat(regptr _regs)
{
  const_strptr ___pathname = (const_strptr) _regs->a0;
  uae_u32 ___mode = (uae_u32) _regs->d0;
  return creat(___pathname, ___mode);
}

uae_s32
arp2sys_posix_fadvise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_posix_fadvise(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___offset = (uae_u64) _regs->d1;
  uae_u64 ___len = (uae_u64) _regs->d3;
  uae_s32 ___advise = (uae_s32) _regs->d5;
  return posix_fadvise(___fd, ___offset, ___len, ___advise);
}

uae_s32
arp2sys_posix_fallocate(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_posix_fallocate(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___offset = (uae_u64) _regs->d1;
  uae_u64 ___len = (uae_u64) _regs->d3;
  return posix_fallocate(___fd, ___offset, ___len);
}

/* arp2_mqd_t */
/* arp2sys_mq_open(regptr _regs) REGPARAM; */

/* arp2_mqd_t */
/* REGPARAM2 arp2sys_mq_open(regptr _regs) */
/* { */
/*   const_strptr ___name = (const_strptr) _regs->a0; */
/*   uae_s32 ___oflag = (uae_s32) _regs->d0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   struct arp2_mq_attr* ___attr = (struct arp2_mq_attr*) _regs->a1; */
/*   return mq_open(___name, ___oflag, ___mode, ___attr); */
/* } */

/* uae_s32 */
/* arp2sys_mq_close(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_close(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   return mq_close(___mqdes); */
/* } */

/* uae_s32 */
/* arp2sys_mq_getattr(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_getattr(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   struct arp2_mq_attr* ___mqstat = (struct arp2_mq_attr*) _regs->a0; */
/*   return mq_getattr(___mqdes, ___mqstat); */
/* } */

/* uae_s32 */
/* arp2sys_mq_setattr(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_setattr(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   const struct arp2_mq_attr* ___mqstat = (const struct arp2_mq_attr*) _regs->a0; */
/*   struct arp2_mq_attr* ___omqstat = (struct arp2_mq_attr*) _regs->a1; */
/*   return mq_setattr(___mqdes, ___mqstat, ___omqstat); */
/* } */

/* uae_s32 */
/* arp2sys_mq_unlink(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_unlink(regptr _regs) */
/* { */
/*   const_strptr ___name = (const_strptr) _regs->a0; */
/*   return mq_unlink(___name); */
/* } */

/* uae_s32 */
/* arp2sys_mq_notify(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_notify(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   const struct arp2_sigevent* ___notification = (const struct arp2_sigevent*) _regs->a0; */
/*   return mq_notify(___mqdes, ___notification); */
/* } */

/* uae_s32 */
/* arp2sys_mq_receive(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_receive(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   uae_s8* ___msg_ptr = (uae_s8*) _regs->a0; */
/*   uae_u32 ___msg_len = (uae_u32) _regs->d1; */
/*   uae_u32* ___msg_prio = (uae_u32*) _regs->a1; */
/*   return mq_receive(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio); */
/* } */

/* uae_s32 */
/* arp2sys_mq_send(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_send(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   const uae_s8* ___msg_ptr = (const uae_s8*) _regs->a0; */
/*   uae_u32 ___msg_len = (uae_u32) _regs->d1; */
/*   uae_u32 ___msg_prio = (uae_u32) _regs->d2; */
/*   return mq_send(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio); */
/* } */

/* uae_s32 */
/* arp2sys_mq_timedreceive(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_timedreceive(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   uae_s8* ___msg_ptr = (uae_s8*) _regs->a0; */
/*   uae_u32 ___msg_len = (uae_u32) _regs->d1; */
/*   uae_u32* ___msg_prio = (uae_u32*) _regs->a1; */
/*   const struct arp2_timespec* ___abs_timeout = (const struct arp2_timespec*) _regs->a2; */
/*   return mq_timedreceive(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio, ___abs_timeout); */
/* } */

/* uae_s32 */
/* arp2sys_mq_timedsend(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_timedsend(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   const uae_s8* ___msg_ptr = (const uae_s8*) _regs->a0; */
/*   uae_u32 ___msg_len = (uae_u32) _regs->d1; */
/*   uae_u32 ___msg_prio = (uae_u32) _regs->d2; */
/*   const struct arp2_timespec* ___abs_timeout = (const struct arp2_timespec*) _regs->a1; */
/*   return mq_timedsend(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio, ___abs_timeout); */
/* } */

/* uae_s32 */
/* arp2sys_poll(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_poll(regptr _regs) */
/* { */
/*   struct arp2_pollfd* ___fds = (struct arp2_pollfd*) _regs->a0; */
/*   uae_u32 ___nfds = (uae_u32) _regs->d0; */
/*   return poll(___fds, ___nfds); */
/* } */

/* uae_s32 */
/* arp2sys_ppoll(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_ppoll(regptr _regs) */
/* { */
/*   struct arp2_pollfd* ___fds = (struct arp2_pollfd*) _regs->a0; */
/*   uae_u32 ___nfds = (uae_u32) _regs->d0; */
/*   const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) _regs->a1; */
/*   const arp2_sigset_t* ___sigmask = (const arp2_sigset_t*) _regs->a2; */
/*   return ppoll(___fds, ___nfds, ___timeout, ___sigmask); */
/* } */

/* uae_s32 */
/* arp2sys_clone(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clone(regptr _regs) */
/* { */
/*   uae_s32 (*___fn) (aptr arg) = (uae_s32 (*)(aptr arg)) _regs->a0; */
/*   aptr ___child_stack = (aptr) _regs->a1; */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   aptr ___arg = (aptr) _regs->a2; */
/*   arp2_pid_t* ___ptid = (arp2_pid_t*) _regs->d1; */
/*   struct arp2_user_desc* ___tls = (struct arp2_user_desc*) _regs->d2; */
/*   arp2_pid_t* ___ctid = (arp2_pid_t*) _regs->d3; */
/*   return clone(___fn, ___child_stack, ___flags, ___arg, ___ptid, ___tls, ___ctid); */
/* } */

/* uae_s32 */
/* arp2sys_unshare(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_unshare(regptr _regs) */
/* { */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   return unshare(___flags); */
/* } */

/* uae_s32 */
/* arp2sys_sched_setparam(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_setparam(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   const struct arp2_sched_param* ___param = (const struct arp2_sched_param*) _regs->a0; */
/*   return sched_setparam(___pid, ___param); */
/* } */

/* uae_s32 */
/* arp2sys_sched_getparam(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_getparam(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   struct arp2_sched_param* ___param = (struct arp2_sched_param*) _regs->a0; */
/*   return sched_getparam(___pid, ___param); */
/* } */

/* uae_s32 */
/* arp2sys_sched_setscheduler(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_setscheduler(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_s32 ___policy = (uae_s32) _regs->d1; */
/*   const struct arp2_sched_param* ___param = (const struct arp2_sched_param*) _regs->a0; */
/*   return sched_setscheduler(___pid, ___policy, ___param); */
/* } */

uae_s32
arp2sys_sched_getscheduler(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_getscheduler(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  return sched_getscheduler(___pid);
}

uae_s32
arp2sys_sched_yield(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_yield(regptr _regs)
{
  return sched_yield();
}

uae_s32
arp2sys_sched_get_priority_max(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_get_priority_max(regptr _regs)
{
  uae_s32 ___algorithm = (uae_s32) _regs->d0;
  return sched_get_priority_max(___algorithm);
}

uae_s32
arp2sys_sched_get_priority_min(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_get_priority_min(regptr _regs)
{
  uae_s32 ___algorithm = (uae_s32) _regs->d0;
  return sched_get_priority_min(___algorithm);
}

/* uae_s32 */
/* arp2sys_sched_rr_get_interval(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_rr_get_interval(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   struct arp2_timespec* ___t = (struct arp2_timespec*) _regs->a0; */
/*   return sched_rr_get_interval(___pid, ___t); */
/* } */

/* uae_s32 */
/* arp2sys_sched_setaffinity(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_setaffinity(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_u32 ___cpusetsize = (uae_u32) _regs->d1; */
/*   const arp2_cpu_set_t* ___cpuset = (const arp2_cpu_set_t*) _regs->a0; */
/*   return sched_setaffinity(___pid, ___cpusetsize, ___cpuset); */
/* } */

/* uae_s32 */
/* arp2sys_sched_getaffinity(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_getaffinity(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_u32 ___cpusetsize = (uae_u32) _regs->d1; */
/*   arp2_cpu_set_t* ___cpuset = (arp2_cpu_set_t*) _regs->a0; */
/*   return sched_getaffinity(___pid, ___cpusetsize, ___cpuset); */
/* } */

uae_s32
arp2sys_kill(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_kill(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
  return kill(___pid, ___sig);
}

uae_s32
arp2sys_killpg(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_killpg(regptr _regs)
{
  arp2_pid_t ___pgrp = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
  return killpg(___pgrp, ___sig);
}

uae_s32
arp2sys_raise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_raise(regptr _regs)
{
  uae_s32 ___sig = (uae_s32) _regs->d0;
  return raise(___sig);
}

/* uae_s32 */
/* arp2sys_sigprocmask(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigprocmask(regptr _regs) */
/* { */
/*   uae_s32 ___how = (uae_s32) _regs->d0; */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) _regs->a0; */
/*   arp2_sigset_t* ___oldset = (arp2_sigset_t*) _regs->a1; */
/*   return sigprocmask(___how, ___set, ___oldset); */
/* } */

/* uae_s32 */
/* arp2sys_sigsuspend(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigsuspend(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) _regs->a0; */
/*   return sigsuspend(___set); */
/* } */

/* uae_s32 */
/* arp2sys_sigaction(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigaction(regptr _regs) */
/* { */
/*   uae_s32 ___sig = (uae_s32) _regs->d0; */
/*   const struct arp2_sigaction* ___act = (const struct arp2_sigaction*) _regs->a0; */
/*   struct arp2_sigaction* ___oldact = (struct arp2_sigaction*) _regs->a1; */
/*   return sigaction(___sig, ___act, ___oldact); */
/* } */

/* uae_s32 */
/* arp2sys_sigpending(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigpending(regptr _regs) */
/* { */
/*   arp2_sigset_t* ___set = (arp2_sigset_t*) _regs->a0; */
/*   return sigpending(___set); */
/* } */

/* uae_s32 */
/* arp2sys_sigwait(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigwait(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) _regs->a0; */
/*   uae_s32* ___sig = (uae_s32*) _regs->d0; */
/*   return sigwait(___set, ___sig); */
/* } */

/* uae_s32 */
/* arp2sys_sigwaitinfo(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigwaitinfo(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) _regs->a0; */
/*   arp2_siginfo_t* ___info = (arp2_siginfo_t*) _regs->a1; */
/*   return sigwaitinfo(___set, ___info); */
/* } */

/* uae_s32 */
/* arp2sys_sigtimedwait(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigtimedwait(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) _regs->a0; */
/*   arp2_siginfo_t* ___info = (arp2_siginfo_t*) _regs->a1; */
/*   const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) _regs->a2; */
/*   return sigtimedwait(___set, ___info, ___timeout); */
/* } */

uae_s32
arp2sys_sigqueue(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigqueue(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
/*   const union arp2_sigval ___val = (const union arp2_sigval) _regs->d2; */
  union sigval ___val = { _regs->d2 };
#warning Make sure this is right
  return sigqueue(___pid, ___sig, ___val);
}

uae_s32
arp2sys_siginterrupt(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_siginterrupt(regptr _regs)
{
  uae_s32 ___sig = (uae_s32) _regs->d0;
  uae_s32 ___interrupt = (uae_s32) _regs->d1;
  return siginterrupt(___sig, ___interrupt);
}

/* uae_s32 */
/* arp2sys_sigaltstack(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigaltstack(regptr _regs) */
/* { */
/*   const struct arp2_sigaltstack* ___ss = (const struct arp2_sigaltstack*) _regs->a0; */
/*   struct arp2_sigaltstack* ___oss = (struct arp2_sigaltstack*) _regs->a1; */
/*   return sigaltstack(___ss, ___oss); */
/* } */

uae_s32
arp2sys_rename(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_rename(regptr _regs)
{
  const_strptr ___old = (const_strptr) _regs->a0;
  const_strptr ___new = (const_strptr) _regs->a1;
  return rename(___old, ___new);
}

/* uae_s32 */
/* arp2sys_renameat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_renameat(regptr _regs) */
/* { */
/*   uae_s32 ___oldfd = (uae_s32) _regs->d0; */
/*   const_strptr ___old = (const_strptr) _regs->a0; */
/*   uae_s32 ___newfd = (uae_s32) _regs->d1; */
/*   const_strptr ___new = (const_strptr) _regs->a1; */
/*   return renameat(___oldfd, ___old, ___newfd, ___new); */
/* } */

arp2_clock_t
arp2sys_clock(regptr _regs) REGPARAM;

arp2_clock_t
REGPARAM2 arp2sys_clock(regptr _regs)
{
  return clock();
}

/* arp2_time_t */
/* arp2sys_time(regptr _regs) REGPARAM; */

/* arp2_time_t */
/* REGPARAM2 arp2sys_time(regptr _regs) */
/* { */
/*   arp2_time_t* ___time = (arp2_time_t*) _regs->a0; */
/*   return time(___time); */
/* } */

/* uae_s32 */
/* arp2sys_nanosleep(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_nanosleep(regptr _regs) */
/* { */
/*   const struct arp2_timespec* ___requested_time = (const struct arp2_timespec*) _regs->a0; */
/*   struct arp2_timespec* ___remaining = (struct arp2_timespec*) _regs->a1; */
/*   return nanosleep(___requested_time, ___remaining); */
/* } */

/* uae_s32 */
/* arp2sys_clock_getres(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clock_getres(regptr _regs) */
/* { */
/*   arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0; */
/*   struct arp2_timespec* ___res = (struct arp2_timespec*) _regs->a0; */
/*   return clock_getres(___clock_id, ___res); */
/* } */

/* uae_s32 */
/* arp2sys_clock_gettime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clock_gettime(regptr _regs) */
/* { */
/*   arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0; */
/*   struct arp2_timespec* ___tp = (struct arp2_timespec*) _regs->a0; */
/*   return clock_gettime(___clock_id, ___tp); */
/* } */

/* uae_s32 */
/* arp2sys_clock_settime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clock_settime(regptr _regs) */
/* { */
/*   arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0; */
/*   const struct arp2_timespec* ___tp = (const struct arp2_timespec*) _regs->a0; */
/*   return clock_settime(___clock_id, ___tp); */
/* } */

/* uae_s32 */
/* arp2sys_clock_nanosleep(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clock_nanosleep(regptr _regs) */
/* { */
/*   arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0; */
/*   uae_s32 ___flags = (uae_s32) _regs->d1; */
/*   const struct arp2_timespec* ___req = (const struct arp2_timespec*) _regs->a0; */
/*   struct arp2_timespec* ___rem = (struct arp2_timespec*) _regs->a1; */
/*   return clock_nanosleep(___clock_id, ___flags, ___req, ___rem); */
/* } */

/* uae_s32 */
/* arp2sys_clock_getcpuclockid(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clock_getcpuclockid(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   arp2_clockid_t* ___clock_id = (arp2_clockid_t*) _regs->a0; */
/*   return clock_getcpuclockid(___pid, ___clock_id); */
/* } */

/* uae_s32 */
/* arp2sys_timer_create(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_create(regptr _regs) */
/* { */
/*   arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0; */
/*   struct arp2_sigevent* ___evp = (struct arp2_sigevent*) _regs->a0; */
/*   arp2_timer_t* ___timerid = (arp2_timer_t*) _regs->a1; */
/*   return timer_create(___clock_id, ___evp, ___timerid); */
/* } */

/* uae_s32 */
/* arp2sys_timer_delete(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_delete(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   return timer_delete(___timerid); */
/* } */

/* uae_s32 */
/* arp2sys_timer_settime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_settime(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   const struct arp2_itimerspec* ___value = (const struct arp2_itimerspec*) _regs->a1; */
/*   struct arp2_itimerspec* ___ovalue = (struct arp2_itimerspec*) _regs->a2; */
/*   return timer_settime(___timerid, ___flags, ___value, ___ovalue); */
/* } */

/* uae_s32 */
/* arp2sys_timer_gettime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_gettime(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   struct arp2_itimerspec* ___value = (struct arp2_itimerspec*) _regs->a1; */
/*   return timer_gettime(___timerid, ___value); */
/* } */

/* uae_s32 */
/* arp2sys_timer_getoverrun(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_getoverrun(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   return timer_getoverrun(___timerid); */
/* } */

uae_s32
arp2sys_access(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_access(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  uae_s32 ___type = (uae_s32) _regs->d0;
  return access(___name, ___type);
}

uae_s32
arp2sys_euidaccess(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_euidaccess(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  uae_s32 ___type = (uae_s32) _regs->d0;
  return euidaccess(___name, ___type);
}

/* uae_s32 */
/* arp2sys_eaccess(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_eaccess(regptr _regs) */
/* { */
/*   const_strptr ___name = (const_strptr) _regs->a0; */
/*   uae_s32 ___type = (uae_s32) _regs->d0; */
/*   return eaccess(___name, ___type); */
/* } */

/* uae_s32 */
/* arp2sys_faccessat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_faccessat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   uae_s32 ___type = (uae_s32) _regs->d1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d2; */
/*   return faccessat(___fd, ___file, ___type, ___flag); */
/* } */

uae_s64
arp2sys_lseek(regptr _regs) REGPARAM;

uae_s64
REGPARAM2 arp2sys_lseek(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s64 ___offset = ((uae_s64) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  uae_s32 ___whence = (uae_s32) _regs->d3;
  return lseek(___fd, ___offset, ___whence);
}

uae_s32
arp2sys_close(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_close(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  return close(___fd);
}

uae_s32
arp2sys_read(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_read(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  aptr ___buf = (aptr) _regs->a0;
  uae_u32 ___count = (uae_u32) _regs->d1;
  return read(___fd, ___buf, ___count);
}

uae_s32
arp2sys_write(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_write(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const_aptr ___buf = (const_aptr) _regs->a0;
  uae_u32 ___count = (uae_u32) _regs->d1;
  return write(___fd, ___buf, ___count);
}

uae_s32
arp2sys_pread(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pread(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  aptr ___buf = (aptr) _regs->a0;
  uae_u32 ___nbytes = (uae_u32) _regs->d1;
  uae_u64 ___offset = (uae_u64) _regs->d2;
  return pread(___fd, ___buf, ___nbytes, ___offset);
}

uae_s32
arp2sys_pwrite(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pwrite(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const_aptr ___buf = (const_aptr) _regs->a0;
  uae_u32 ___n = (uae_u32) _regs->d1;
  uae_u64 ___offset = (uae_u64) _regs->d2;
  return pwrite(___fd, ___buf, ___n, ___offset);
}

/* uae_s32 */
/* arp2sys_pipe(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_pipe(regptr _regs) */
/* { */
/*   uae_s32* ___pipedes = (uae_s32*) _regs->a0; */
/*   return pipe(___pipedes); */
/* } */

uae_u32
arp2sys_alarm(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_alarm(regptr _regs)
{
  uae_u32 ___seconds = (uae_u32) _regs->d0;
  return alarm(___seconds);
}

uae_u32
arp2sys_sleep(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_sleep(regptr _regs)
{
  uae_u32 ___seconds = (uae_u32) _regs->d0;
  return sleep(___seconds);
}

uae_s32
arp2sys_pause(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pause(regptr _regs)
{
  return pause();
}

uae_s32
arp2sys_chown(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chown(regptr _regs)
{
  const_strptr ___file = (const_strptr) _regs->a0;
  arp2_uid_t ___owner = (arp2_uid_t) _regs->d0;
  arp2_gid_t ___group = (arp2_gid_t) _regs->d1;
  return chown(___file, ___owner, ___group);
}

uae_s32
arp2sys_fchown(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchown(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  arp2_uid_t ___owner = (arp2_uid_t) _regs->d1;
  arp2_gid_t ___group = (arp2_gid_t) _regs->d2;
  return fchown(___fd, ___owner, ___group);
}

uae_s32
arp2sys_lchown(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lchown(regptr _regs)
{
  const_strptr ___file = (const_strptr) _regs->a0;
  arp2_uid_t ___owner = (arp2_uid_t) _regs->d0;
  arp2_gid_t ___group = (arp2_gid_t) _regs->d1;
  return lchown(___file, ___owner, ___group);
}

/* uae_s32 */
/* arp2sys_fchownat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fchownat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   arp2_uid_t ___owner = (arp2_uid_t) _regs->d1; */
/*   arp2_gid_t ___group = (arp2_gid_t) _regs->d2; */
/*   uae_s32 ___flag = (uae_s32) _regs->d3; */
/*   return fchownat(___fd, ___file, ___owner, ___group, ___flag); */
/* } */

uae_s32
arp2sys_chdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  return chdir(___path);
}

uae_s32
arp2sys_fchdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchdir(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  return fchdir(___fd);
}

strptr
arp2sys_getcwd(regptr _regs) REGPARAM;

strptr
REGPARAM2 arp2sys_getcwd(regptr _regs)
{
  strptr ___buf = (strptr) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d0;
  return getcwd(___buf, ___size);
}

uae_s32
arp2sys_dup(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_dup(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  return dup(___fd);
}

uae_s32
arp2sys_dup2(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_dup2(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___fd2 = (uae_s32) _regs->d1;
  return dup2(___fd, ___fd2);
}

/* uae_s32 */
/* arp2sys_execve(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_execve(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   strptr const* ___argv = (strptr const*) _regs->a1; */
/*   strptr const* ___envp = (strptr const*) _regs->a2; */
/*   return execve(___path, ___argv, ___envp); */
/* } */

/* uae_s32 */
/* arp2sys_execv(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_execv(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   strptr const* ___argv = (strptr const*) _regs->a1; */
/*   return execv(___path, ___argv); */
/* } */

/* uae_s32 */
/* arp2sys_execvp(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_execvp(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   strptr const* ___argv = (strptr const*) _regs->a1; */
/*   return execvp(___file, ___argv); */
/* } */

uae_s32
arp2sys_nice(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_nice(regptr _regs)
{
  uae_s32 ___inc = (uae_s32) _regs->d0;
  return nice(___inc);
}

uae_s32
arp2sys_pathconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pathconf(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  uae_s32 ___name = (uae_s32) _regs->d0;
  return pathconf(___path, ___name);
}

uae_s32
arp2sys_fpathconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fpathconf(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___name = (uae_s32) _regs->d1;
  return fpathconf(___fd, ___name);
}

uae_s32
arp2sys_sysconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sysconf(regptr _regs)
{
  uae_s32 ___name = (uae_s32) _regs->d0;
  return sysconf(___name);
}

uae_u32
arp2sys_confstr(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_confstr(regptr _regs)
{
  uae_s32 ___name = (uae_s32) _regs->d0;
  strptr ___buf = (strptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d1;
  return confstr(___name, ___buf, ___len);
}

arp2_pid_t
arp2sys_getpid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpid(regptr _regs)
{
  return getpid();
}

arp2_pid_t
arp2sys_getppid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getppid(regptr _regs)
{
  return getppid();
}

arp2_pid_t
arp2sys_getpgrp(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpgrp(regptr _regs)
{
  return getpgrp();
}

arp2_pid_t
arp2sys_getpgid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpgid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  return getpgid(___pid);
}

uae_s32
arp2sys_setpgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpgid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  arp2_pid_t ___pgid = (arp2_pid_t) _regs->d1;
  return setpgid(___pid, ___pgid);
}

uae_s32
arp2sys_setpgrp(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpgrp(regptr _regs)
{
  return setpgrp();
}

arp2_pid_t
arp2sys_setsid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_setsid(regptr _regs)
{
  return setsid();
}

arp2_pid_t
arp2sys_getsid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getsid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  return getsid(___pid);
}

arp2_uid_t
arp2sys_getuid(regptr _regs) REGPARAM;

arp2_uid_t
REGPARAM2 arp2sys_getuid(regptr _regs)
{
  return getuid();
}

arp2_uid_t
arp2sys_geteuid(regptr _regs) REGPARAM;

arp2_uid_t
REGPARAM2 arp2sys_geteuid(regptr _regs)
{
  return geteuid();
}

arp2_gid_t
arp2sys_getgid(regptr _regs) REGPARAM;

arp2_gid_t
REGPARAM2 arp2sys_getgid(regptr _regs)
{
  return getgid();
}

arp2_gid_t
arp2sys_getegid(regptr _regs) REGPARAM;

arp2_gid_t
REGPARAM2 arp2sys_getegid(regptr _regs)
{
  return getegid();
}

/* uae_s32 */
/* arp2sys_getgroups(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getgroups(regptr _regs) */
/* { */
/*   uae_s32 ___size = (uae_s32) _regs->d0; */
/*   arp2_gid_t* ___list = (arp2_gid_t*) _regs->a0; */
/*   return getgroups(___size, ___list); */
/* } */

/* uae_s32 */
/* arp2sys_setgroups(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_setgroups(regptr _regs) */
/* { */
/*   uae_u32 ___n = (uae_u32) _regs->d0; */
/*   const arp2_gid_t* ___groups = (const arp2_gid_t*) _regs->a0; */
/*   return setgroups(___n, ___groups); */
/* } */

uae_s32
arp2sys_group_member(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_group_member(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  return group_member(___gid);
}

uae_s32
arp2sys_setuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  return setuid(___uid);
}

uae_s32
arp2sys_setreuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setreuid(regptr _regs)
{
  arp2_uid_t ___ruid = (arp2_uid_t) _regs->d0;
  arp2_uid_t ___euid = (arp2_uid_t) _regs->d1;
  return setreuid(___ruid, ___euid);
}

uae_s32
arp2sys_seteuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_seteuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  return seteuid(___uid);
}

uae_s32
arp2sys_setgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setgid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  return setgid(___gid);
}

uae_s32
arp2sys_setregid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setregid(regptr _regs)
{
  arp2_gid_t ___rgid = (arp2_gid_t) _regs->d0;
  arp2_gid_t ___egid = (arp2_gid_t) _regs->d1;
  return setregid(___rgid, ___egid);
}

uae_s32
arp2sys_setegid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setegid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  return setegid(___gid);
}

/* uae_s32 */
/* arp2sys_getresuid(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getresuid(regptr _regs) */
/* { */
/*   arp2_uid_t* ___ruid = (arp2_uid_t*) _regs->a0; */
/*   arp2_uid_t* ___euid = (arp2_uid_t*) _regs->a1; */
/*   arp2_uid_t* ___suid = (arp2_uid_t*) _regs->a2; */
/*   return getresuid(___ruid, ___euid, ___suid); */
/* } */

/* uae_s32 */
/* arp2sys_getresgid(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getresgid(regptr _regs) */
/* { */
/*   arp2_gid_t* ___rgid = (arp2_gid_t*) _regs->a0; */
/*   arp2_gid_t* ___egid = (arp2_gid_t*) _regs->a1; */
/*   arp2_gid_t* ___sgid = (arp2_gid_t*) _regs->a2; */
/*   return getresgid(___rgid, ___egid, ___sgid); */
/* } */

uae_s32
arp2sys_setresuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setresuid(regptr _regs)
{
  arp2_uid_t ___ruid = (arp2_uid_t) _regs->d0;
  arp2_uid_t ___euid = (arp2_uid_t) _regs->d1;
  arp2_uid_t ___suid = (arp2_uid_t) _regs->d2;
  return setresuid(___ruid, ___euid, ___suid);
}

uae_s32
arp2sys_setresgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setresgid(regptr _regs)
{
  arp2_gid_t ___rgid = (arp2_gid_t) _regs->d0;
  arp2_gid_t ___egid = (arp2_gid_t) _regs->d1;
  arp2_gid_t ___sgid = (arp2_gid_t) _regs->d2;
  return setresgid(___rgid, ___egid, ___sgid);
}

arp2_pid_t
arp2sys_fork(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_fork(regptr _regs)
{
  return fork();
}

arp2_pid_t
arp2sys_vfork(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_vfork(regptr _regs)
{
  return vfork();
}

uae_s32
arp2sys_ttyname_r(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ttyname_r(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  strptr ___buf = (strptr) _regs->a0;
  uae_u32 ___buflen = (uae_u32) _regs->d1;
  return ttyname_r(___fd, ___buf, ___buflen);
}

uae_s32
arp2sys_isatty(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_isatty(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  return isatty(___fd);
}

uae_s32
arp2sys_link(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_link(regptr _regs)
{
  const_strptr ___from = (const_strptr) _regs->a0;
  const_strptr ___to = (const_strptr) _regs->a1;
  return link(___from, ___to);
}

/* uae_s32 */
/* arp2sys_linkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_linkat(regptr _regs) */
/* { */
/*   uae_s32 ___fromfd = (uae_s32) _regs->d0; */
/*   const_strptr ___from = (const_strptr) _regs->a0; */
/*   uae_s32 ___tofd = (uae_s32) _regs->d1; */
/*   const_strptr ___to = (const_strptr) _regs->a1; */
/*   uae_s32 ___flags = (uae_s32) _regs->d2; */
/*   return linkat(___fromfd, ___from, ___tofd, ___to, ___flags); */
/* } */

uae_s32
arp2sys_symlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_symlink(regptr _regs)
{
  const_strptr ___from = (const_strptr) _regs->a0;
  const_strptr ___to = (const_strptr) _regs->a1;
  return symlink(___from, ___to);
}

uae_s32
arp2sys_readlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_readlink(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  strptr ___buf = (strptr) _regs->a1;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return readlink(___path, ___buf, ___len);
}

/* uae_s32 */
/* arp2sys_symlinkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_symlinkat(regptr _regs) */
/* { */
/*   const_strptr ___from = (const_strptr) _regs->a0; */
/*   uae_s32 ___tofd = (uae_s32) _regs->d0; */
/*   const_strptr ___to = (const_strptr) _regs->a1; */
/*   return symlinkat(___from, ___tofd, ___to); */
/* } */

/* uae_s32 */
/* arp2sys_readlinkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_readlinkat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   strptr ___buf = (strptr) _regs->a1; */
/*   uae_u32 ___len = (uae_u32) _regs->d1; */
/*   return readlinkat(___fd, ___path, ___buf, ___len); */
/* } */

uae_s32
arp2sys_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  return unlink(___name);
}

/* uae_s32 */
/* arp2sys_unlinkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_unlinkat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___name = (const_strptr) _regs->a0; */
/*   uae_s32 ___flag = (uae_s32) _regs->d1; */
/*   return unlinkat(___fd, ___name, ___flag); */
/* } */

uae_s32
arp2sys_rmdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_rmdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  return rmdir(___path);
}

arp2_pid_t
arp2sys_tcgetpgrp(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_tcgetpgrp(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  return tcgetpgrp(___fd);
}

uae_s32
arp2sys_tcsetpgrp(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_tcsetpgrp(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  arp2_pid_t ___pgrp_id = (arp2_pid_t) _regs->d1;
  return tcsetpgrp(___fd, ___pgrp_id);
}

uae_s32
arp2sys_getlogin_r(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getlogin_r(regptr _regs)
{
  strptr ___name = (strptr) _regs->a0;
  uae_u32 ___name_len = (uae_u32) _regs->d0;
  return getlogin_r(___name, ___name_len);
}

uae_s32
arp2sys_setlogin(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setlogin(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  return setlogin(___name);
}

uae_s32
arp2sys_gethostname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_gethostname(regptr _regs)
{
  strptr ___name = (strptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return gethostname(___name, ___len);
}

uae_s32
arp2sys_sethostname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sethostname(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return sethostname(___name, ___len);
}

uae_s32
arp2sys_sethostid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sethostid(regptr _regs)
{
  uae_s32 ___id = (uae_s32) _regs->d0;
  return sethostid(___id);
}

uae_s32
arp2sys_getdomainname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getdomainname(regptr _regs)
{
  strptr ___name = (strptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return getdomainname(___name, ___len);
}

uae_s32
arp2sys_setdomainname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setdomainname(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return setdomainname(___name, ___len);
}

uae_s32
arp2sys_vhangup(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_vhangup(regptr _regs)
{
  return vhangup();
}

uae_s32
arp2sys_revoke(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_revoke(regptr _regs)
{
  const_strptr ___file = (const_strptr) _regs->a0;
  return revoke(___file);
}

/* uae_s32 */
/* arp2sys_profil(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_profil(regptr _regs) */
/* { */
/*   UWORD* ___sample_buffer = (UWORD*) _regs->a0; */
/*   uae_u32 ___size = (uae_u32) _regs->d0; */
/*   uae_u32 ___offset = (uae_u32) _regs->d1; */
/*   uae_u32 ___scale = (uae_u32) _regs->d2; */
/*   return profil(___sample_buffer, ___size, ___offset, ___scale); */
/* } */

uae_s32
arp2sys_acct(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_acct(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  return acct(___name);
}

uae_s32
arp2sys_daemon(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_daemon(regptr _regs)
{
  uae_s32 ___nochdir = (uae_s32) _regs->d0;
  uae_s32 ___noclose = (uae_s32) _regs->d1;
  return daemon(___nochdir, ___noclose);
}

uae_s32
arp2sys_chroot(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chroot(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  return chroot(___path);
}

uae_s32
arp2sys_fsync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fsync(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  return fsync(___fd);
}

uae_s32
arp2sys_gethostid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_gethostid(regptr _regs)
{
  return gethostid();
}

void
arp2sys_sync(regptr _regs) REGPARAM;

void
REGPARAM2 arp2sys_sync(regptr _regs)
{
  return sync();
}

uae_s32
arp2sys_getpagesize(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getpagesize(regptr _regs)
{
  return getpagesize();
}

uae_s32
arp2sys_getdtablesize(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getdtablesize(regptr _regs)
{
  return getdtablesize();
}

uae_s32
arp2sys_truncate(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_truncate(regptr _regs)
{
  const_strptr ___file = (const_strptr) _regs->a0;
  uae_u64 ___length = (uae_u64) _regs->d0;
  return truncate(___file, ___length);
}

uae_s32
arp2sys_ftruncate(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ftruncate(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___length = (uae_u64) _regs->d1;
  return ftruncate(___fd, ___length);
}

uae_s32
arp2sys_brk(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_brk(regptr _regs)
{
  aptr ___end_data_segment = (aptr) _regs->a0;
  return brk(___end_data_segment);
}

aptr
arp2sys_sbrk(regptr _regs) REGPARAM;

aptr
REGPARAM2 arp2sys_sbrk(regptr _regs)
{
  uae_s32 ___increment = (uae_s32) _regs->d0;
  return sbrk(___increment);
}

uae_s32
arp2sys_lockf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lockf(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___cmd = (uae_s32) _regs->d1;
  uae_u64 ___len = (uae_u64) _regs->d2;
  return lockf(___fd, ___cmd, ___len);
}

uae_s32
arp2sys_fdatasync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fdatasync(regptr _regs)
{
  uae_s32 ___fildes = (uae_s32) _regs->d0;
  return fdatasync(___fildes);
}

/* uae_s32 */
/* arp2sys_getdents(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getdents(regptr _regs) */
/* { */
/*   uae_u32 ___fd = (uae_u32) _regs->d0; */
/*   struct arp2_dirent* ___dirp = (struct arp2_dirent*) _regs->a0; */
/*   uae_u32 ___count = (uae_u32) _regs->d1; */
/*   return getdents(___fd, ___dirp, ___count); */
/* } */

/* uae_s32 */
/* arp2sys_utime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_utime(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   const struct arp2_utimbuf* ___file_times = (const struct arp2_utimbuf*) _regs->a1; */
/*   return utime(___file, ___file_times); */
/* } */

uae_s32
arp2sys_setxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  const_strptr ___name = (const_strptr) _regs->a1;
  const_aptr ___value = (const_aptr) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  return setxattr(___path, ___name, ___value, ___size, ___flags);
}

uae_s32
arp2sys_lsetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lsetxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  const_strptr ___name = (const_strptr) _regs->a1;
  const_aptr ___value = (const_aptr) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  return lsetxattr(___path, ___name, ___value, ___size, ___flags);
}

uae_s32
arp2sys_fsetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fsetxattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  const_strptr ___name = (const_strptr) _regs->a0;
  const_aptr ___value = (const_aptr) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  return fsetxattr(___filedes, ___name, ___value, ___size, ___flags);
}

uae_s32
arp2sys_getxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  const_strptr ___name = (const_strptr) _regs->a1;
  aptr ___value = (aptr) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  return getxattr(___path, ___name, ___value, ___size);
}

uae_s32
arp2sys_lgetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lgetxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  const_strptr ___name = (const_strptr) _regs->a1;
  aptr ___value = (aptr) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  return lgetxattr(___path, ___name, ___value, ___size);
}

uae_s32
arp2sys_fgetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fgetxattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  const_strptr ___name = (const_strptr) _regs->a0;
  aptr ___value = (aptr) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d1;
  return fgetxattr(___filedes, ___name, ___value, ___size);
}

uae_s32
arp2sys_listxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_listxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  strptr ___list = (strptr) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d0;
  return listxattr(___path, ___list, ___size);
}

uae_s32
arp2sys_llistxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_llistxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  strptr ___list = (strptr) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d0;
  return llistxattr(___path, ___list, ___size);
}

uae_s32
arp2sys_flistxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_flistxattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  strptr ___list = (strptr) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d1;
  return flistxattr(___filedes, ___list, ___size);
}

uae_s32
arp2sys_removexattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_removexattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  const_strptr ___name = (const_strptr) _regs->a1;
  return removexattr(___path, ___name);
}

uae_s32
arp2sys_lremovexattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lremovexattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  const_strptr ___name = (const_strptr) _regs->a1;
  return lremovexattr(___path, ___name);
}

uae_s32
arp2sys_fremovexattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fremovexattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  const_strptr ___name = (const_strptr) _regs->a0;
  return fremovexattr(___filedes, ___name);
}

uae_s32
arp2sys_epoll_create(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_epoll_create(regptr _regs)
{
  uae_s32 ___size = (uae_s32) _regs->d0;
  return epoll_create(___size);
}

/* uae_s32 */
/* arp2sys_epoll_ctl(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_epoll_ctl(regptr _regs) */
/* { */
/*   uae_s32 ___epfd = (uae_s32) _regs->d0; */
/*   uae_s32 ___op = (uae_s32) _regs->d1; */
/*   uae_s32 ___fd = (uae_s32) _regs->d2; */
/*   struct arp2_epoll_event* ___event = (struct arp2_epoll_event*) _regs->a0; */
/*   return epoll_ctl(___epfd, ___op, ___fd, ___event); */
/* } */

/* uae_s32 */
/* arp2sys_epoll_wait(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_epoll_wait(regptr _regs) */
/* { */
/*   uae_s32 ___epfd = (uae_s32) _regs->d0; */
/*   struct arp2_epoll_event* ___events = (struct arp2_epoll_event*) _regs->a0; */
/*   uae_s32 ___maxevents = (uae_s32) _regs->d1; */
/*   uae_s32 ___timeout = (uae_s32) _regs->d2; */
/*   return epoll_wait(___epfd, ___events, ___maxevents, ___timeout); */
/* } */

uae_s32
arp2sys_flock(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_flock(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___operation = (uae_s32) _regs->d1;
  return flock(___fd, ___operation);
}

uae_s32
arp2sys_setfsuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setfsuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  return setfsuid(___uid);
}

uae_s32
arp2sys_setfsgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setfsgid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  return setfsgid(___gid);
}

uae_s32
arp2sys_ioperm(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ioperm(regptr _regs)
{
  uae_u32 ___from = (uae_u32) _regs->d0;
  uae_u32 ___num = (uae_u32) _regs->d1;
  uae_s32 ___turn_on = (uae_s32) _regs->d2;
  return ioperm(___from, ___num, ___turn_on);
}

uae_s32
arp2sys_iopl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_iopl(regptr _regs)
{
  uae_s32 ___level = (uae_s32) _regs->d0;
  return iopl(___level);
}

uae_s32
arp2sys_ioctl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ioctl(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 ___request = (uae_u32) _regs->d1;
  aptr ___arg = (aptr) _regs->a0;
  return ioctl(___fd, ___request, ___arg);
}

uae_s32
arp2sys_klogctl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_klogctl(regptr _regs)
{
  uae_s32 ___type = (uae_s32) _regs->d0;
  strptr ___bufp = (strptr) _regs->a0;
  uae_s32 ___len = (uae_s32) _regs->d1;
  return klogctl(___type, ___bufp, ___len);
}

aptr
arp2sys_mmap(regptr _regs) REGPARAM;

aptr
REGPARAM2 arp2sys_mmap(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  uae_s32 ___fd = (uae_s32) _regs->d3;
  uae_s64 ___offset = ((uae_s64) _regs->d4 << 32) | ((uae_u32) _regs->d5);
  return mmap(___start, ___length, ___prot, ___flags, ___fd, ___offset);
}

uae_s32
arp2sys_munmap(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munmap(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  return munmap(___start, ___length);
}

uae_s32
arp2sys_mprotect(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mprotect(regptr _regs)
{
  const_aptr ___addr = (const_aptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  return mprotect(___addr, ___len, ___prot);
}

uae_s32
arp2sys_msync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_msync(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  return msync(___start, ___length, ___flags);
}

uae_s32
arp2sys_madvise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_madvise(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___advice = (uae_s32) _regs->d1;
  return madvise(___start, ___length, ___advice);
}

uae_s32
arp2sys_posix_madvise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_posix_madvise(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___advice = (uae_s32) _regs->d1;
  return posix_madvise(___start, ___length, ___advice);
}

uae_s32
arp2sys_mlock(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mlock(regptr _regs)
{
  const_aptr ___addr = (const_aptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return mlock(___addr, ___len);
}

uae_s32
arp2sys_munlock(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munlock(regptr _regs)
{
  const_aptr ___addr = (const_aptr) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  return munlock(___addr, ___len);
}

uae_s32
arp2sys_mlockall(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mlockall(regptr _regs)
{
  uae_s32 ___flags = (uae_s32) _regs->d0;
  return mlockall(___flags);
}

uae_s32
arp2sys_munlockall(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munlockall(regptr _regs)
{
  return munlockall();
}

uae_s32
arp2sys_mincore(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mincore(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_u8* ___vec = (uae_u8*) _regs->a1;
  return mincore(___start, ___length, ___vec);
}

aptr
arp2sys_mremap(regptr _regs) REGPARAM;

aptr
REGPARAM2 arp2sys_mremap(regptr _regs)
{
  aptr ___old_address = (aptr) _regs->a0;
  uae_u32 ___old_size = (uae_u32) _regs->d0;
  uae_u32 ___new_size = (uae_u32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  return mremap(___old_address, ___old_size, ___new_size, ___flags);
}

uae_s32
arp2sys_remap_file_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_remap_file_pages(regptr _regs)
{
  aptr ___start = (aptr) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  uae_u32 ___pgoff = (uae_u32) _regs->d2;
  uae_s32 ___flags = (uae_s32) _regs->d3;
  return remap_file_pages(___start, ___size, ___prot, ___pgoff, ___flags);
}

uae_s32
arp2sys_shm_open(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_shm_open(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  uae_s32 ___oflag = (uae_s32) _regs->d0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d1;
  return shm_open(___name, ___oflag, ___mode);
}

uae_s32
arp2sys_shm_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_shm_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) _regs->a0;
  return shm_unlink(___name);
}

uae_s32
arp2sys_mount(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mount(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) _regs->a0;
  const_strptr ___dir = (const_strptr) _regs->a1;
  const_strptr ___fstype = (const_strptr) _regs->a2;
  uae_u32 ___rwflag = (uae_u32) _regs->d0;
  const_aptr ___data = (const_aptr) _regs->a3;
  return mount(___special_file, ___dir, ___fstype, ___rwflag, ___data);
}

uae_s32
arp2sys_umount(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_umount(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) _regs->a0;
  return umount(___special_file);
}

uae_s32
arp2sys_umount2(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_umount2(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  return umount2(___special_file, ___flags);
}

/* uae_s32 */
/* arp2sys_getrlimit64(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getrlimit64(regptr _regs) */
/* { */
/*   arp2_rlimit_resource_t ___resource = (arp2_rlimit_resource_t) _regs->d0; */
/*   struct arp2_rlimit* ___rlimits = (struct arp2_rlimit*) _regs->a0; */
/*   return getrlimit64(___resource, ___rlimits); */
/* } */

/* uae_s32 */
/* arp2sys_setrlimit64(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_setrlimit64(regptr _regs) */
/* { */
/*   arp2_rlimit_resource_t ___resource = (arp2_rlimit_resource_t) _regs->d0; */
/*   const struct arp2_rlimit* ___rlimits = (const struct arp2_rlimit*) _regs->a0; */
/*   return setrlimit64(___resource, ___rlimits); */
/* } */

/* uae_s32 */
/* arp2sys_getrusage(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getrusage(regptr _regs) */
/* { */
/*   arp2_rusage_who_t ___who = (arp2_rusage_who_t) _regs->d0; */
/*   struct arp2_rusage* ___usage = (struct arp2_rusage*) _regs->a0; */
/*   return getrusage(___who, ___usage); */
/* } */

uae_s32
arp2sys_getpriority(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getpriority(regptr _regs)
{
  arp2_priority_which_t ___which = (arp2_priority_which_t) _regs->d0;
  arp2_id_t ___who = (arp2_id_t) _regs->d1;
  return getpriority(___which, ___who);
}

uae_s32
arp2sys_setpriority(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpriority(regptr _regs)
{
  arp2_priority_which_t ___which = (arp2_priority_which_t) _regs->d0;
  arp2_id_t ___who = (arp2_id_t) _regs->d1;
  uae_s32 ___prio = (uae_s32) _regs->d2;
  return setpriority(___which, ___who, ___prio);
}

/* uae_s32 */
/* arp2sys_select(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_select(regptr _regs) */
/* { */
/*   uae_s32 ___nfds = (uae_s32) _regs->d0; */
/*   arp2_fd_set* ___readfds = (arp2_fd_set*) _regs->a0; */
/*   arp2_fd_set* ___writefds = (arp2_fd_set*) _regs->a1; */
/*   arp2_fd_set* ___exceptfds = (arp2_fd_set*) _regs->a2; */
/*   struct arp2_timeval* ___timeout = (struct arp2_timeval*) _regs->a3; */
/*   return select(___nfds, ___readfds, ___writefds, ___exceptfds, ___timeout); */
/* } */

/* uae_s32 */
/* arp2sys_pselect(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_pselect(regptr _regs) */
/* { */
/*   uae_s32 ___nfds = (uae_s32) _regs->d0; */
/*   arp2_fd_set* ___readfds = (arp2_fd_set*) _regs->a0; */
/*   arp2_fd_set* ___writefds = (arp2_fd_set*) _regs->a1; */
/*   arp2_fd_set* ___exceptfds = (arp2_fd_set*) _regs->a2; */
/*   const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) _regs->a3; */
/*   const arp2_sigset_t* ___sigmask = (const arp2_sigset_t*) _regs->a4; */
/*   return pselect(___nfds, ___readfds, ___writefds, ___exceptfds, ___timeout, ___sigmask); */
/* } */

/* uae_s32 */
/* arp2sys_stat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_stat(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) _regs->a1; */
/*   return stat(___path, ___buf); */
/* } */

/* uae_s32 */
/* arp2sys_fstat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) _regs->a0; */
/*   return fstat(___fd, ___buf); */
/* } */

/* uae_s32 */
/* arp2sys_fstatat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) _regs->a1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d1; */
/*   return fstatat(___fd, ___file, ___buf, ___flag); */
/* } */

/* uae_s32 */
/* arp2sys_lstat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_lstat(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) _regs->a1; */
/*   return lstat(___path, ___buf); */
/* } */

uae_s32
arp2sys_chmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chmod(regptr _regs)
{
  const_strptr ___file = (const_strptr) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  return chmod(___file, ___mode);
}

uae_s32
arp2sys_lchmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lchmod(regptr _regs)
{
  const_strptr ___file = (const_strptr) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  return lchmod(___file, ___mode);
}

uae_s32
arp2sys_fchmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchmod(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d1;
  return fchmod(___fd, ___mode);
}

/* uae_s32 */
/* arp2sys_fchmodat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fchmodat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d2; */
/*   return fchmodat(___fd, ___file, ___mode, ___flag); */
/* } */

arp2_mode_t
arp2sys_umask(regptr _regs) REGPARAM;

arp2_mode_t
REGPARAM2 arp2sys_umask(regptr _regs)
{
  arp2_mode_t ___mask = (arp2_mode_t) _regs->d0;
  return umask(___mask);
}

arp2_mode_t
arp2sys_getumask(regptr _regs) REGPARAM;

arp2_mode_t
REGPARAM2 arp2sys_getumask(regptr _regs)
{
#warning Check if this has been implemented yet
//  return getumask();
  arp2_mode_t mask = umask(0);
  umask(mask);
  return mask;
}

uae_s32
arp2sys_mkdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mkdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  return mkdir(___path, ___mode);
}

/* uae_s32 */
/* arp2sys_mkdirat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mkdirat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   return mkdirat(___fd, ___path, ___mode); */
/* } */

uae_s32
arp2sys_mknod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mknod(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  arp2_dev_t ___dev = ((arp2_dev_t) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  return mknod(___path, ___mode, ___dev);
}

/* uae_s32 */
/* arp2sys_mknodat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mknodat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   arp2_dev_t ___dev = ((arp2_dev_t) _regs->d2 << 32) | ((uae_u32) _regs->d3); */
/*   return mknodat(___fd, ___path, ___mode, ___dev); */
/* } */

uae_s32
arp2sys_mkfifo(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mkfifo(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  return mkfifo(___path, ___mode);
}

/* uae_s32 */
/* arp2sys_mkfifoat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mkfifoat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   return mkfifoat(___fd, ___path, ___mode); */
/* } */

/* uae_s32 */
/* arp2sys_statfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_statfs(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   struct arp2_statfs* ___buf = (struct arp2_statfs*) _regs->a1; */
/*   return statfs(___file, ___buf); */
/* } */

/* uae_s32 */
/* arp2sys_fstatfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatfs(regptr _regs) */
/* { */
/*   uae_s32 ___fildes = (uae_s32) _regs->d0; */
/*   struct arp2_statfs* ___buf = (struct arp2_statfs*) _regs->a0; */
/*   return fstatfs(___fildes, ___buf); */
/* } */

/* uae_s32 */
/* arp2sys_statvfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_statvfs(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   struct arp2_statvfs* ___buf = (struct arp2_statvfs*) _regs->a1; */
/*   return statvfs(___file, ___buf); */
/* } */

/* uae_s32 */
/* arp2sys_fstatvfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatvfs(regptr _regs) */
/* { */
/*   uae_s32 ___fildes = (uae_s32) _regs->d0; */
/*   struct arp2_statvfs* ___buf = (struct arp2_statvfs*) _regs->a0; */
/*   return fstatvfs(___fildes, ___buf); */
/* } */

uae_s32
arp2sys_swapon(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_swapon(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  return swapon(___path, ___flags);
}

uae_s32
arp2sys_swapoff(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_swapoff(regptr _regs)
{
  const_strptr ___path = (const_strptr) _regs->a0;
  return swapoff(___path);
}

/* uae_s32 */
/* arp2sys_sysinfo(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sysinfo(regptr _regs) */
/* { */
/*   struct arp2_sysinfo* ___info = (struct arp2_sysinfo*) _regs->a0; */
/*   return sysinfo(___info); */
/* } */

uae_s32
arp2sys_get_nprocs_conf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_nprocs_conf(regptr _regs)
{
  return get_nprocs_conf();
}

uae_s32
arp2sys_get_nprocs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_nprocs(regptr _regs)
{
  return get_nprocs();
}

uae_s32
arp2sys_get_phys_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_phys_pages(regptr _regs)
{
  return get_phys_pages();
}

uae_s32
arp2sys_get_avphys_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_avphys_pages(regptr _regs)
{
  return get_avphys_pages();
}

/* uae_s32 */
/* arp2sys_gettimeofday(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_gettimeofday(regptr _regs) */
/* { */
/*   struct arp2_timeval* ___tv = (struct arp2_timeval*) _regs->a0; */
/*   struct arp2_timezone* ___tz = (struct arp2_timezone*) _regs->a1; */
/*   return gettimeofday(___tv, ___tz); */
/* } */

/* uae_s32 */
/* arp2sys_settimeofday(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_settimeofday(regptr _regs) */
/* { */
/*   const struct arp2_timeval* ___tv = (const struct arp2_timeval*) _regs->a0; */
/*   const struct arp2_timezone* ___tz = (const struct arp2_timezone*) _regs->a1; */
/*   return settimeofday(___tv, ___tz); */
/* } */

/* uae_s32 */
/* arp2sys_adjtime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_adjtime(regptr _regs) */
/* { */
/*   const struct arp2_timeval* ___delta = (const struct arp2_timeval*) _regs->a0; */
/*   struct arp2_timeval* ___olddelta = (struct arp2_timeval*) _regs->a1; */
/*   return adjtime(___delta, ___olddelta); */
/* } */

/* uae_s32 */
/* arp2sys_getitimer(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getitimer(regptr _regs) */
/* { */
/*   arp2_itimer_which_t ___which = (arp2_itimer_which_t) _regs->d0; */
/*   struct arp2_itimerval* ___value = (struct arp2_itimerval*) _regs->a0; */
/*   return getitimer(___which, ___value); */
/* } */

/* uae_s32 */
/* arp2sys_setitimer(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_setitimer(regptr _regs) */
/* { */
/*   arp2_itimer_which_t ___which = (arp2_itimer_which_t) _regs->d0; */
/*   const struct arp2_itimerval* ___new = (const struct arp2_itimerval*) _regs->a0; */
/*   struct arp2_itimerval* ___old = (struct arp2_itimerval*) _regs->a1; */
/*   return setitimer(___which, ___new, ___old); */
/* } */

/* uae_s32 */
/* arp2sys_utimes(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_utimes(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) _regs->a1; */
/*   return utimes(___file, ___tvp); */
/* } */

/* uae_s32 */
/* arp2sys_lutimes(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_lutimes(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) _regs->a1; */
/*   return lutimes(___file, ___tvp); */
/* } */

/* uae_s32 */
/* arp2sys_futimes(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_futimes(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) _regs->a0; */
/*   return futimes(___fd, ___tvp); */
/* } */

/* uae_s32 */
/* arp2sys_futimesat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_futimesat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) _regs->a1; */
/*   return futimesat(___fd, ___file, ___tvp); */
/* } */

/* arp2_clock_t */
/* arp2sys_times(regptr _regs) REGPARAM; */

/* arp2_clock_t */
/* REGPARAM2 arp2sys_times(regptr _regs) */
/* { */
/*   struct arp2_tms* ___buffer = (struct arp2_tms*) _regs->a0; */
/*   return times(___buffer); */
/* } */

/* uae_s32 */
/* arp2sys_readv(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_readv(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const struct arp2_iovec* ___iovec = (const struct arp2_iovec*) _regs->a0; */
/*   uae_s32 ___count = (uae_s32) _regs->d1; */
/*   return readv(___fd, ___iovec, ___count); */
/* } */

/* uae_s32 */
/* arp2sys_writev(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_writev(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const struct arp2_iovec* ___iovec = (const struct arp2_iovec*) _regs->a0; */
/*   uae_s32 ___count = (uae_s32) _regs->d1; */
/*   return writev(___fd, ___iovec, ___count); */
/* } */

/* uae_s32 */
/* arp2sys_uname(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_uname(regptr _regs) */
/* { */
/*   struct arp2_utsname* ___name = (struct arp2_utsname*) _regs->a0; */
/*   return uname(___name); */
/* } */

/* arp2_pid_t */
/* arp2sys_wait(regptr _regs) REGPARAM; */

/* arp2_pid_t */
/* REGPARAM2 arp2sys_wait(regptr _regs) */
/* { */
/*   uae_s32* ___stat_loc = (uae_s32*) _regs->a0; */
/*   return wait(___stat_loc); */
/* } */

/* uae_s32 */
/* arp2sys_waitid(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_waitid(regptr _regs) */
/* { */
/*   arp2_idtype_t ___idtype = (arp2_idtype_t) _regs->d0; */
/*   arp2_id_t ___id = (arp2_id_t) _regs->d1; */
/*   arp2_siginfo_t* ___infop = (arp2_siginfo_t*) _regs->a0; */
/*   uae_s32 ___options = (uae_s32) _regs->d2; */
/*   return waitid(___idtype, ___id, ___infop, ___options); */
/* } */

/* arp2_pid_t */
/* arp2sys_wait3(regptr _regs) REGPARAM; */

/* arp2_pid_t */
/* REGPARAM2 arp2sys_wait3(regptr _regs) */
/* { */
/*   uae_s32* ___stat_loc = (uae_s32*) _regs->a0; */
/*   uae_s32 ___options = (uae_s32) _regs->d0; */
/*   struct arp2_rusage* ___usage = (struct arp2_rusage*) _regs->a1; */
/*   return wait3(___stat_loc, ___options, ___usage); */
/* } */

/* arp2_pid_t */
/* arp2sys_wait4(regptr _regs) REGPARAM; */

/* arp2_pid_t */
/* REGPARAM2 arp2sys_wait4(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_s32* ___stat_loc = (uae_s32*) _regs->a0; */
/*   uae_s32 ___options = (uae_s32) _regs->d1; */
/*   struct arp2_rusage* ___usage = (struct arp2_rusage*) _regs->a1; */
/*   return wait4(___pid, ___stat_loc, ___options, ___usage); */
/* } */


/*** A NULL-terminated array of function pointers to the syscall wrappers ****/

void unimplemented() {
  write_log("Unimplemented arp2-syscall vector called!\n");
  abort();
}

#define arp2sys_sync_file_range unimplemented
#define arp2sys_vmsplice unimplemented
#define arp2sys_splice unimplemented
#define arp2sys_tee unimplemented
#define arp2sys_openat unimplemented
#define arp2sys_mq_open unimplemented
#define arp2sys_mq_close unimplemented
#define arp2sys_mq_getattr unimplemented
#define arp2sys_mq_setattr unimplemented
#define arp2sys_mq_unlink unimplemented
#define arp2sys_mq_notify unimplemented
#define arp2sys_mq_receive unimplemented
#define arp2sys_mq_send unimplemented
#define arp2sys_mq_timedreceive unimplemented
#define arp2sys_mq_timedsend unimplemented
#define arp2sys_poll unimplemented
#define arp2sys_ppoll unimplemented
#define arp2sys_clone unimplemented
#define arp2sys_unshare unimplemented
#define arp2sys_sched_setparam unimplemented
#define arp2sys_sched_getparam unimplemented
#define arp2sys_sched_setscheduler unimplemented
#define arp2sys_ched_yield unimplemented
#define arp2sys_sched_rr_get_interval unimplemented
#define arp2sys_sched_setaffinity unimplemented
#define arp2sys_sched_getaffinity unimplemented
#define arp2sys_sigprocmask unimplemented
#define arp2sys_sigsuspend unimplemented
#define arp2sys_sigaction unimplemented
#define arp2sys_sigpending unimplemented
#define arp2sys_sigwait unimplemented
#define arp2sys_sigwaitinfo unimplemented
#define arp2sys_sigtimedwait unimplemented
#define arp2sys_sigaltstack unimplemented
#define arp2sys_renameat unimplemented
#define arp2sys_time unimplemented
#define arp2sys_nanosleep unimplemented
#define arp2sys_clock_getres unimplemented
#define arp2sys_clock_gettime unimplemented
#define arp2sys_clock_settime unimplemented
#define arp2sys_clock_nanosleep unimplemented
#define arp2sys_clock_getcpuclockid unimplemented
#define arp2sys_timer_create unimplemented
#define arp2sys_timer_delete unimplemented
#define arp2sys_timer_settime unimplemented
#define arp2sys_timer_gettime unimplemented
#define arp2sys_timer_getoverrun unimplemented
#define arp2sys_eaccess unimplemented
#define arp2sys_faccessat unimplemented
#define arp2sys_pipe unimplemented
#define arp2sys_fchownat unimplemented
#define arp2sys_execve unimplemented
#define arp2sys_execv unimplemented
#define arp2sys_execvp unimplemented
#define arp2sys_getgroups unimplemented
#define arp2sys_setgroups unimplemented
#define arp2sys_getresuid unimplemented
#define arp2sys_getresgid unimplemented
#define arp2sys_linkat unimplemented
#define arp2sys_symlinkat unimplemented
#define arp2sys_readlinkat unimplemented
#define arp2sys_unlinkat unimplemented
#define arp2sys_profil unimplemented
#define arp2sys_getdents unimplemented
#define arp2sys_utime unimplemented
#define arp2sys_epoll_ctl unimplemented
#define arp2sys_epoll_wait unimplemented
#define arp2sys_getrlimit64 unimplemented
#define arp2sys_setrlimit64 unimplemented
#define arp2sys_getrusage unimplemented
#define arp2sys_select unimplemented
#define arp2sys_pselect unimplemented
#define arp2sys_stat unimplemented
#define arp2sys_fstat unimplemented
#define arp2sys_fstatat unimplemented
#define arp2sys_lstat unimplemented
#define arp2sys_fchmodat unimplemented
#define arp2sys_mkdirat unimplemented
#define arp2sys_mknodat unimplemented
#define arp2sys_mkfifoat unimplemented
#define arp2sys_statfs unimplemented
#define arp2sys_fstatfs unimplemented
#define arp2sys_statvfs unimplemented
#define arp2sys_fstatvfs unimplemented
#define arp2sys_sysinfo unimplemented
#define arp2sys_gettimeofday unimplemented
#define arp2sys_settimeofday unimplemented
#define arp2sys_adjtime unimplemented
#define arp2sys_getitimer unimplemented
#define arp2sys_setitimer unimplemented
#define arp2sys_utimes unimplemented
#define arp2sys_lutimes unimplemented
#define arp2sys_futimes unimplemented
#define arp2sys_futimesat unimplemented
#define arp2sys_times unimplemented
#define arp2sys_readv unimplemented
#define arp2sys_writev unimplemented
#define arp2sys_uname unimplemented
#define arp2sys_wait unimplemented
#define arp2sys_waitid unimplemented
#define arp2sys_wait3 unimplemented
#define arp2sys_wait4 unimplemented


void* const arp2sys_functions[251] = {
  arp2sys_sync_file_range,
  arp2sys_vmsplice,
  arp2sys_splice,
  arp2sys_tee,
  arp2sys_fcntl,
  arp2sys_open,
  arp2sys_openat,
  arp2sys_creat,
  arp2sys_posix_fadvise,
  arp2sys_posix_fallocate,
  arp2sys_mq_open,
  arp2sys_mq_close,
  arp2sys_mq_getattr,
  arp2sys_mq_setattr,
  arp2sys_mq_unlink,
  arp2sys_mq_notify,
  arp2sys_mq_receive,
  arp2sys_mq_send,
  arp2sys_mq_timedreceive,
  arp2sys_mq_timedsend,
  arp2sys_poll,
  arp2sys_ppoll,
  arp2sys_clone,
  arp2sys_unshare,
  arp2sys_sched_setparam,
  arp2sys_sched_getparam,
  arp2sys_sched_setscheduler,
  arp2sys_sched_getscheduler,
  arp2sys_ched_yield,
  arp2sys_sched_get_priority_max,
  arp2sys_sched_get_priority_min,
  arp2sys_sched_rr_get_interval,
  arp2sys_sched_setaffinity,
  arp2sys_sched_getaffinity,
  arp2sys_kill,
  arp2sys_killpg,
  arp2sys_raise,
  arp2sys_sigprocmask,
  arp2sys_sigsuspend,
  arp2sys_sigaction,
  arp2sys_sigpending,
  arp2sys_sigwait,
  arp2sys_sigwaitinfo,
  arp2sys_sigtimedwait,
  arp2sys_sigqueue,
  arp2sys_siginterrupt,
  arp2sys_sigaltstack,
  arp2sys_rename,
  arp2sys_renameat,
  arp2sys_clock,
  arp2sys_time,
  arp2sys_nanosleep,
  arp2sys_clock_getres,
  arp2sys_clock_gettime,
  arp2sys_clock_settime,
  arp2sys_clock_nanosleep,
  arp2sys_clock_getcpuclockid,
  arp2sys_timer_create,
  arp2sys_timer_delete,
  arp2sys_timer_settime,
  arp2sys_timer_gettime,
  arp2sys_timer_getoverrun,
  arp2sys_access,
  arp2sys_euidaccess,
  arp2sys_eaccess,
  arp2sys_faccessat,
  arp2sys_lseek,
  arp2sys_close,
  arp2sys_read,
  arp2sys_write,
  arp2sys_pread,
  arp2sys_pwrite,
  arp2sys_pipe,
  arp2sys_alarm,
  arp2sys_sleep,
  arp2sys_pause,
  arp2sys_chown,
  arp2sys_fchown,
  arp2sys_lchown,
  arp2sys_fchownat,
  arp2sys_chdir,
  arp2sys_fchdir,
  arp2sys_getcwd,
  arp2sys_dup,
  arp2sys_dup2,
  arp2sys_execve,
  arp2sys_execv,
  arp2sys_execvp,
  arp2sys_nice,
  arp2sys_pathconf,
  arp2sys_fpathconf,
  arp2sys_sysconf,
  arp2sys_confstr,
  arp2sys_getpid,
  arp2sys_getppid,
  arp2sys_getpgrp,
  arp2sys_getpgid,
  arp2sys_setpgid,
  arp2sys_setpgrp,
  arp2sys_setsid,
  arp2sys_getsid,
  arp2sys_getuid,
  arp2sys_geteuid,
  arp2sys_getgid,
  arp2sys_getegid,
  arp2sys_getgroups,
  arp2sys_setgroups,
  arp2sys_group_member,
  arp2sys_setuid,
  arp2sys_setreuid,
  arp2sys_seteuid,
  arp2sys_setgid,
  arp2sys_setregid,
  arp2sys_setegid,
  arp2sys_getresuid,
  arp2sys_getresgid,
  arp2sys_setresuid,
  arp2sys_setresgid,
  arp2sys_fork,
  arp2sys_vfork,
  arp2sys_ttyname_r,
  arp2sys_isatty,
  arp2sys_link,
  arp2sys_linkat,
  arp2sys_symlink,
  arp2sys_readlink,
  arp2sys_symlinkat,
  arp2sys_readlinkat,
  arp2sys_unlink,
  arp2sys_unlinkat,
  arp2sys_rmdir,
  arp2sys_tcgetpgrp,
  arp2sys_tcsetpgrp,
  arp2sys_getlogin_r,
  arp2sys_setlogin,
  arp2sys_gethostname,
  arp2sys_sethostname,
  arp2sys_sethostid,
  arp2sys_getdomainname,
  arp2sys_setdomainname,
  arp2sys_vhangup,
  arp2sys_revoke,
  arp2sys_profil,
  arp2sys_acct,
  arp2sys_daemon,
  arp2sys_chroot,
  arp2sys_fsync,
  arp2sys_gethostid,
  arp2sys_sync,
  arp2sys_getpagesize,
  arp2sys_getdtablesize,
  arp2sys_truncate,
  arp2sys_ftruncate,
  arp2sys_brk,
  arp2sys_sbrk,
  arp2sys_lockf,
  arp2sys_fdatasync,
  arp2sys_getdents,
  arp2sys_utime,
  arp2sys_setxattr,
  arp2sys_lsetxattr,
  arp2sys_fsetxattr,
  arp2sys_getxattr,
  arp2sys_lgetxattr,
  arp2sys_fgetxattr,
  arp2sys_listxattr,
  arp2sys_llistxattr,
  arp2sys_flistxattr,
  arp2sys_removexattr,
  arp2sys_lremovexattr,
  arp2sys_fremovexattr,
  arp2sys_epoll_create,
  arp2sys_epoll_ctl,
  arp2sys_epoll_wait,
  arp2sys_flock,
  arp2sys_setfsuid,
  arp2sys_setfsgid,
  arp2sys_ioperm,
  arp2sys_iopl,
  arp2sys_ioctl,
  arp2sys_klogctl,
  arp2sys_mmap,
  arp2sys_munmap,
  arp2sys_mprotect,
  arp2sys_msync,
  arp2sys_madvise,
  arp2sys_posix_madvise,
  arp2sys_mlock,
  arp2sys_munlock,
  arp2sys_mlockall,
  arp2sys_munlockall,
  arp2sys_mincore,
  arp2sys_mremap,
  arp2sys_remap_file_pages,
  arp2sys_shm_open,
  arp2sys_shm_unlink,
  arp2sys_mount,
  arp2sys_umount,
  arp2sys_umount2,
  arp2sys_getrlimit64,
  arp2sys_setrlimit64,
  arp2sys_getrusage,
  arp2sys_getpriority,
  arp2sys_setpriority,
  arp2sys_select,
  arp2sys_pselect,
  arp2sys_stat,
  arp2sys_fstat,
  arp2sys_fstatat,
  arp2sys_lstat,
  arp2sys_chmod,
  arp2sys_lchmod,
  arp2sys_fchmod,
  arp2sys_fchmodat,
  arp2sys_umask,
  arp2sys_getumask,
  arp2sys_mkdir,
  arp2sys_mkdirat,
  arp2sys_mknod,
  arp2sys_mknodat,
  arp2sys_mkfifo,
  arp2sys_mkfifoat,
  arp2sys_statfs,
  arp2sys_fstatfs,
  arp2sys_statvfs,
  arp2sys_fstatvfs,
  arp2sys_swapon,
  arp2sys_swapoff,
  arp2sys_sysinfo,
  arp2sys_get_nprocs_conf,
  arp2sys_get_nprocs,
  arp2sys_get_phys_pages,
  arp2sys_get_avphys_pages,
  arp2sys_gettimeofday,
  arp2sys_settimeofday,
  arp2sys_adjtime,
  arp2sys_getitimer,
  arp2sys_setitimer,
  arp2sys_utimes,
  arp2sys_lutimes,
  arp2sys_futimes,
  arp2sys_futimesat,
  arp2sys_times,
  arp2sys_readv,
  arp2sys_writev,
  arp2sys_uname,
  arp2sys_wait,
  arp2sys_waitid,
  arp2sys_wait3,
  arp2sys_wait4,
  NULL
};


/*** Initialization and patching code ****************************************/

int arp2sys_init(void) {
  return 1;
}

int arp2sys_reset(uae_u8* arp2rom) {
  uae_u32* rom = (uae_u32*) arp2rom;

  if (BE32(rom[1024]) == 0xdeadc0de &&
      BE32(rom[1025]) == ~0U) {
    int i;
    write_log("ARP2 ROM: arp2sys patching ROM image\n");

    for (i = 0; arp2sys_functions[i] != 0; ++i) {
      assert ((((uae_u64) (uintptr_t) arp2sys_functions[i]) & 0xffffffff00000000ULL) == 0);
      
      rom[1024+i] = BE32((uae_u32) (uintptr_t) arp2sys_functions[i]);
    }

    rom[1024+i] = BE32(~0UL);
  }

  return 1;
}

void arp2sys_free(void) {
  // Do what?
}

#endif
