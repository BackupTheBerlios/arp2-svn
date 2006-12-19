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

#ifndef __linux
# error Unsupported
#endif

/*** Include prototypes for all functions we export **************************/

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

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

#undef S_WRITE	// Defined in <sys/mount.h>

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


/*** BJMP register argument definition ***************************************/

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


/*** SetIoErr() emulation ****************************************************/

#define NT_PROCESS 0x0d

struct sysresbase {
    uae_u8	_junk[36];
    uae_u32	sysbase;	// At offset 36
};


struct execbase {
    uae_u8	_junk[276];
    uae_u32	thistask;	// At offset 276
};


struct process {
    uae_u8	_junk[8];
    uae_u8	ln_type;	// At offset 8
    uae_u8	_more[139];
    uae_u32	pr_result2;	// At offset 148
};


static void set_io_err(struct sysresbase* sys) {
  struct execbase*   exec = (struct execbase*) (uintptr_t) BE32(sys->sysbase);
  struct process*    pr   = (struct process*) (uintptr_t) BE32(exec->thistask);

  if (pr->ln_type == NT_PROCESS) {
    pr->pr_result2 = BE32(errno == 0 ? 0 : 1000 + errno);
  }
}


/*** arp2-syscall.resource types *********************************************/

typedef char* strptr;
typedef void* aptr;
typedef const char* const_strptr;
typedef const void* const_aptr;

typedef uae_s32 arp2_clock_t;
typedef uae_s32 arp2_clockid_t;
typedef uae_s32 arp2_mqd_t;
typedef uae_s32 arp2_pid_t;
typedef uae_s32 arp2_time_t;
typedef uae_u32 arp2_gid_t;
typedef uae_u32 arp2_id_t;
typedef uae_u32 arp2_mode_t;
typedef uae_u32 arp2_priority_which_t;
typedef uae_u32 arp2_uid_t;
typedef uae_u64 arp2_dev_t;

struct arp2_mq_attr {
    uae_s32 mq_flags;
    uae_s32 mq_maxmsg;
    uae_s32 mq_msgsize;
    uae_s32 mq_curmsgs;
    uae_s32 __pad[4];
};

struct arp2_sched_param {
    uae_s32 sched_priority;
};

struct arp2_utsname {
    uae_u8 sysname[65];
    uae_u8 nodename[65];
    uae_u8 release[65];
    uae_u8 version[65];
    uae_u8 machine[65];
    uae_u8 domainname[65];
};

struct arp2_timespec {
    arp2_time_t tv_sec;
    uae_s32     tv_nsec;
};

struct arp2_timeval {
    arp2_time_t tv_sec;
    uae_s32     tv_usec;
};




#define COPY_sched_param(s,d) (d).sched_priority = BE32((s).sched_priority)

#define COPY_mq_attr(s,d) \
  (d).mq_flags = BE32((s).mq_flags);		\
  (d).mq_maxmsg = BE32((s).mq_maxmsg);		\
  (d).mq_msgsize = BE32((s).mq_msgsize);	\
  (d).mq_curmsgs = BE32((s).mq_curmsgs)

#define COPY_timespec(s,d) \
  (d).tv_sec = BE32((s).tv_sec);		\
  (d).tv_nsec = BE32((s).tv_nsec)

#define COPY_timeval(s,d) \
  (d).tv_sec = BE32((s).tv_sec);		\
  (d).tv_usec = BE32((s).tv_usec)



#define GET(struct_name, src, name)					\
  struct struct_name name ## _struct, *name = NULL;			\
  if (src != NULL) {							\
    COPY_ ## struct_name(*src, name ## _struct);			\
    name = & name ## _struct;						\
  }

#define PUT(struct_name, dst, name)					\
  if (dst != NULL) {							\
    COPY_ ## struct_name(name, *dst);					\
  }

/* union arp2_sigval { */
/*     uae_s32 sival_int; */
/*     uae_u32 sival_ptr; */
/* }; */


static void unimplemented(void) {
  write_log("Unimplemented arp2-syscall vector called!\n");
  abort();
}

#define arp2sys_sync_file_range unimplemented
#define arp2sys_vmsplice unimplemented
#define arp2sys_splice unimplemented
#define arp2sys_tee unimplemented
#define arp2sys_openat unimplemented
//#define arp2sys_mq_open unimplemented
//#define arp2sys_mq_close unimplemented
//#define arp2sys_mq_getattr unimplemented
//#define arp2sys_mq_setattr unimplemented
//#define arp2sys_mq_unlink unimplemented
#define arp2sys_mq_notify unimplemented
//#define arp2sys_mq_receive unimplemented
//#define arp2sys_mq_send unimplemented
//#define arp2sys_mq_timedreceive unimplemented
//#define arp2sys_mq_timedsend unimplemented
#define arp2sys_poll unimplemented
#define arp2sys_ppoll unimplemented
#define arp2sys_clone unimplemented
#define arp2sys_unshare unimplemented
//#define arp2sys_sched_setparam unimplemented
//#define arp2sys_sched_getparam unimplemented
//#define arp2sys_sched_setscheduler unimplemented
//#define arp2sys_sched_rr_get_interval unimplemented
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
//#define arp2sys_time unimplemented
//#define arp2sys_nanosleep unimplemented
//#define arp2sys_clock_getres unimplemented
//#define arp2sys_clock_gettime unimplemented
//#define arp2sys_clock_settime unimplemented
//#define arp2sys_clock_nanosleep unimplemented
//#define arp2sys_clock_getcpuclockid unimplemented
#define arp2sys_timer_create unimplemented
#define arp2sys_timer_delete unimplemented
#define arp2sys_timer_settime unimplemented
#define arp2sys_timer_gettime unimplemented
#define arp2sys_timer_getoverrun unimplemented
#define arp2sys_eaccess unimplemented
#define arp2sys_faccessat unimplemented
//#define arp2sys_pipe unimplemented
#define arp2sys_fchownat unimplemented
#define arp2sys_execve unimplemented
#define arp2sys_execv unimplemented
#define arp2sys_execvp unimplemented
#define arp2sys_getgroups unimplemented
#define arp2sys_setgroups unimplemented
//#define arp2sys_getresuid unimplemented
//#define arp2sys_getresgid unimplemented
#define arp2sys_linkat unimplemented
#define arp2sys_symlinkat unimplemented
#define arp2sys_readlinkat unimplemented
#define arp2sys_unlinkat unimplemented
//#define arp2sys_profil unimplemented
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
/* #define arp2sys_gettimeofday unimplemented */
/* #define arp2sys_settimeofday unimplemented */
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
//#define arp2sys_uname unimplemented
#define arp2sys_wait unimplemented
#define arp2sys_waitid unimplemented
#define arp2sys_wait3 unimplemented
#define arp2sys_wait4 unimplemented


/*** The actual syscall wrappers *********************************************/

uae_s32
arp2sys_readahead(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_readahead(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___offset = ((uae_u64) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  uae_u32 ___count = (uae_u32) _regs->d3;
  uae_u32 rc = readahead(___fd, ___offset, ___count);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
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
/*   uae_u32 rc = sync_file_range(___fd, ___from, ___to, ___flags); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_vmsplice(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_vmsplice(regptr _regs) */
/* { */
/*   uae_s32 ___fdout = (uae_s32) _regs->d0; */
/*   const struct arp2_iovec* ___iov = (const struct arp2_iovec*) (uintptr_t) _regs->a0; */
/*   uae_u32 ___count = (uae_u32) _regs->d1; */
/*   uae_u32 ___flags = (uae_u32) _regs->d2; */
/*   uae_u32 rc = vmsplice(___fdout, ___iov, ___count, ___flags); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_splice(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_splice(regptr _regs) */
/* { */
/*   uae_s32 ___fdin = (uae_s32) _regs->d0; */
/*   uae_u64* ___offin = (uae_u64*) (uintptr_t) _regs->a0; */
/*   uae_s32 ___fdout = (uae_s32) _regs->d1; */
/*   uae_u64* ___offout = (uae_u64*) (uintptr_t) _regs->a1; */
/*   uae_u32 ___len = (uae_u32) _regs->d2; */
/*   uae_u32 ___flags = (uae_u32) _regs->d3; */
/*   uae_u32 rc = splice(___fdin, ___offin, ___fdout, ___offout, ___len, ___flags); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_tee(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_tee(regptr _regs) */
/* { */
/*   uae_s32 ___fdin = (uae_s32) _regs->d0; */
/*   uae_s32 ___fdout = (uae_s32) _regs->d1; */
/*   uae_u32 ___len = (uae_u32) _regs->d2; */
/*   uae_u32 ___flags = (uae_u32) _regs->d3; */
/*   uae_u32 rc = tee(___fdin, ___fdout, ___len, ___flags); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_fcntl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fcntl(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___cmd = (uae_s32) _regs->d1;
  uae_s32 ___arg = (uae_s32) _regs->a0;
  uae_u32 rc = fcntl(___fd, ___cmd, ___arg);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_open(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_open(regptr _regs)
{
  const_strptr ___pathname = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 ___mode = (uae_u32) _regs->d1;

  printf("***** open %s %x %x\n", ___pathname, ___flags, ___mode);

  uae_u32 rc = open(___pathname, ___flags, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_openat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_openat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___pathname = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___flags = (uae_s32) _regs->d1; */
/*   uae_u32 ___mode = (uae_u32) _regs->d2; */
/*   uae_u32 rc = openat(___fd, ___pathname, ___flags, ___mode); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_creat(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_creat(regptr _regs)
{
  const_strptr ___pathname = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 ___mode = (uae_u32) _regs->d0;
  uae_u32 rc = creat(___pathname, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
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
  uae_u32 rc = posix_fadvise(___fd, ___offset, ___len, ___advise);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_posix_fallocate(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_posix_fallocate(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___offset = (uae_u64) _regs->d1;
  uae_u64 ___len = (uae_u64) _regs->d3;
  uae_u32 rc = posix_fallocate(___fd, ___offset, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_mqd_t
arp2sys_mq_open(regptr _regs) REGPARAM;

arp2_mqd_t
REGPARAM2 arp2sys_mq_open(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___oflag = (uae_s32) _regs->d0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d1;
  struct arp2_mq_attr* ___attr = (struct arp2_mq_attr*) (uintptr_t) _regs->a1;

  GET(mq_attr, ___attr, attr);
  uae_u32 rc = mq_open(___name, ___oflag, ___mode, attr);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_close(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_close(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  uae_u32 rc = mq_close(___mqdes);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_getattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_getattr(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  struct arp2_mq_attr* ___mqstat = (struct arp2_mq_attr*) (uintptr_t) _regs->a0;

  struct mq_attr mqstat;
  uae_u32 rc = mq_getattr(___mqdes, &mqstat);
  PUT(mq_attr, ___mqstat, mqstat);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_setattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_setattr(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  const struct arp2_mq_attr* ___mqstat = (const struct arp2_mq_attr*) (uintptr_t) _regs->a0;
  struct arp2_mq_attr* ___omqstat = (struct arp2_mq_attr*) (uintptr_t) _regs->a1;

  GET(mq_attr, ___mqstat, mqstat);
  struct mq_attr omqstat;
  uae_u32 rc = mq_setattr(___mqdes, mqstat, &omqstat);
  PUT(mq_attr, ___omqstat, omqstat);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = mq_unlink(___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_mq_notify(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mq_notify(regptr _regs) */
/* { */
/*   arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0; */
/*   const struct arp2_sigevent* ___notification = (const struct arp2_sigevent*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = mq_notify(___mqdes, ___notification); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_mq_receive(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_receive(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  char* ___msg_ptr = (char*) (uintptr_t) _regs->a0;
  uae_u32 ___msg_len = (uae_u32) _regs->d1;
  uae_u32* ___msg_prio = (uae_u32*) (uintptr_t) _regs->a1;

  unsigned msg_prio;
  uae_u32 rc = mq_receive(___mqdes, ___msg_ptr, ___msg_len, &msg_prio);
  if (___msg_prio != NULL) {
    *___msg_prio = BE32(msg_prio);
  }
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_send(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_send(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  const char* ___msg_ptr = (const char*) (uintptr_t) _regs->a0;
  uae_u32 ___msg_len = (uae_u32) _regs->d1;
  uae_u32 ___msg_prio = (uae_u32) _regs->d2;
  uae_u32 rc = mq_send(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_timedreceive(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_timedreceive(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  char* ___msg_ptr = (char*) (uintptr_t) _regs->a0;
  uae_u32 ___msg_len = (uae_u32) _regs->d1;
  uae_u32* ___msg_prio = (uae_u32*) (uintptr_t) _regs->a1;
  const struct arp2_timespec* ___abs_timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a2;

  GET(timespec, ___abs_timeout, abs_timeout);
  unsigned msg_prio;
  uae_u32 rc = mq_timedreceive(___mqdes, ___msg_ptr, ___msg_len, &msg_prio, abs_timeout);
  if (___msg_prio != NULL) {
    *___msg_prio = BE32(msg_prio);
  }
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mq_timedsend(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_timedsend(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  const char* ___msg_ptr = (const char*) (uintptr_t) _regs->a0;
  uae_u32 ___msg_len = (uae_u32) _regs->d1;
  uae_u32 ___msg_prio = (uae_u32) _regs->d2;
  const struct arp2_timespec* ___abs_timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a1;

  GET(timespec, ___abs_timeout, abs_timeout);
  uae_u32 rc = mq_timedsend(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio, abs_timeout);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_poll(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_poll(regptr _regs) */
/* { */
/*   struct arp2_pollfd* ___fds = (struct arp2_pollfd*) (uintptr_t) _regs->a0; */
/*   uae_u32 ___nfds = (uae_u32) _regs->d0; */
/*   uae_u32 rc = poll(___fds, ___nfds); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_ppoll(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_ppoll(regptr _regs) */
/* { */
/*   struct arp2_pollfd* ___fds = (struct arp2_pollfd*) (uintptr_t) _regs->a0; */
/*   uae_u32 ___nfds = (uae_u32) _regs->d0; */
/*   const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a1; */
/*   const arp2_sigset_t* ___sigmask = (const arp2_sigset_t*) (uintptr_t) _regs->a2; */
/*   uae_u32 rc = ppoll(___fds, ___nfds, ___timeout, ___sigmask); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_clone(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_clone(regptr _regs) */
/* { */
/*   uae_s32 (*___fn) (aptr arg) = (uae_s32 (*)(aptr arg)) _regs->a0; */
/*   aptr ___child_stack = (aptr) (uintptr_t) _regs->a1; */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   aptr ___arg = (aptr) (uintptr_t) _regs->a2; */
/*   arp2_pid_t* ___ptid = (arp2_pid_t*) (uintptr_t) _regs->d1; */
/*   struct arp2_user_desc* ___tls = (struct arp2_user_desc*) (uintptr_t) _regs->d2; */
/*   arp2_pid_t* ___ctid = (arp2_pid_t*) (uintptr_t) _regs->d3; */
/*   uae_u32 rc = clone(___fn, ___child_stack, ___flags, ___arg, ___ptid, ___tls, ___ctid); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_unshare(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_unshare(regptr _regs) */
/* { */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   uae_u32 rc = unshare(___flags); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_sched_setparam(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_setparam(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  const struct arp2_sched_param* ___param = (const struct arp2_sched_param*) (uintptr_t) _regs->a0;

  GET(sched_param, ___param, param);
  uae_u32 rc = sched_setparam(___pid, param);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_getparam(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_getparam(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  struct arp2_sched_param* ___param = (struct arp2_sched_param*) (uintptr_t) _regs->a0;

  struct sched_param param;
  uae_s32 rc =  sched_getparam(___pid, &param);
  PUT(sched_param, ___param, param);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_setscheduler(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_setscheduler(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32 ___policy = (uae_s32) _regs->d1;
  const struct arp2_sched_param* ___param = (const struct arp2_sched_param*) (uintptr_t) _regs->a0;

  GET(sched_param, ___param, param);
  uae_u32 rc = sched_setscheduler(___pid, ___policy, param);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_getscheduler(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_getscheduler(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 rc = sched_getscheduler(___pid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_yield(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_yield(regptr _regs)
{
  uae_u32 rc = sched_yield();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_get_priority_max(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_get_priority_max(regptr _regs)
{
  uae_s32 ___algorithm = (uae_s32) _regs->d0;
  uae_u32 rc = sched_get_priority_max(___algorithm);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_get_priority_min(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_get_priority_min(regptr _regs)
{
  uae_s32 ___algorithm = (uae_s32) _regs->d0;
  uae_u32 rc = sched_get_priority_min(___algorithm);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sched_rr_get_interval(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_rr_get_interval(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  struct arp2_timespec* ___t = (struct arp2_timespec*) (uintptr_t) _regs->a0;
  
  struct timespec t;
  uae_u32 rc = sched_rr_get_interval(___pid, &t);
  PUT(timespec, ___t, t);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_sched_setaffinity(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_setaffinity(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_u32 ___cpusetsize = (uae_u32) _regs->d1; */
/*   const arp2_cpu_set_t* ___cpuset = (const arp2_cpu_set_t*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = sched_setaffinity(___pid, ___cpusetsize, ___cpuset); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sched_getaffinity(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sched_getaffinity(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_u32 ___cpusetsize = (uae_u32) _regs->d1; */
/*   arp2_cpu_set_t* ___cpuset = (arp2_cpu_set_t*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = sched_getaffinity(___pid, ___cpusetsize, ___cpuset); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_kill(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_kill(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
  uae_u32 rc = kill(___pid, ___sig);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_killpg(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_killpg(regptr _regs)
{
  arp2_pid_t ___pgrp = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
  uae_u32 rc = killpg(___pgrp, ___sig);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_raise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_raise(regptr _regs)
{
  uae_s32 ___sig = (uae_s32) _regs->d0;
  uae_u32 rc = raise(___sig);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_sigprocmask(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigprocmask(regptr _regs) */
/* { */
/*   uae_s32 ___how = (uae_s32) _regs->d0; */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0; */
/*   arp2_sigset_t* ___oldset = (arp2_sigset_t*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = sigprocmask(___how, ___set, ___oldset); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sigsuspend(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigsuspend(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = sigsuspend(___set); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sigaction(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigaction(regptr _regs) */
/* { */
/*   uae_s32 ___sig = (uae_s32) _regs->d0; */
/*   const struct arp2_sigaction* ___act = (const struct arp2_sigaction*) (uintptr_t) _regs->a0; */
/*   struct arp2_sigaction* ___oldact = (struct arp2_sigaction*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = sigaction(___sig, ___act, ___oldact); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sigpending(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigpending(regptr _regs) */
/* { */
/*   arp2_sigset_t* ___set = (arp2_sigset_t*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = sigpending(___set); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sigwait(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigwait(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0; */
/*   uae_s32* ___sig = (uae_s32*) (uintptr_t) (uintptr_t) _regs->d0; */
/*   uae_u32 rc = sigwait(___set, ___sig); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sigwaitinfo(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigwaitinfo(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0; */
/*   arp2_siginfo_t* ___info = (arp2_siginfo_t*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = sigwaitinfo(___set, ___info); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_sigtimedwait(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigtimedwait(regptr _regs) */
/* { */
/*   const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0; */
/*   arp2_siginfo_t* ___info = (arp2_siginfo_t*) (uintptr_t) _regs->a1; */
/*   const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a2; */
/*   uae_u32 rc = sigtimedwait(___set, ___info, ___timeout); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_sigqueue(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigqueue(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
/*   const union arp2_sigval ___val = (const union arp2_sigval) _regs->d2; */
#warning Make sure this is right
  union sigval ___val;
  ___val.sival_ptr = (void*) (uintptr_t) _regs->d2;
  uae_u32 rc = sigqueue(___pid, ___sig, ___val);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_siginterrupt(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_siginterrupt(regptr _regs)
{
  uae_s32 ___sig = (uae_s32) _regs->d0;
  uae_s32 ___interrupt = (uae_s32) _regs->d1;
  uae_u32 rc = siginterrupt(___sig, ___interrupt);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_sigaltstack(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigaltstack(regptr _regs) */
/* { */
/*   const struct arp2_sigaltstack* ___ss = (const struct arp2_sigaltstack*) (uintptr_t) _regs->a0; */
/*   struct arp2_sigaltstack* ___oss = (struct arp2_sigaltstack*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = sigaltstack(___ss, ___oss); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_rename(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_rename(regptr _regs)
{
  const_strptr ___old = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___new = (const_strptr) (uintptr_t) _regs->a1;
  uae_u32 rc = rename(___old, ___new);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_renameat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_renameat(regptr _regs) */
/* { */
/*   uae_s32 ___oldfd = (uae_s32) _regs->d0; */
/*   const_strptr ___old = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___newfd = (uae_s32) _regs->d1; */
/*   const_strptr ___new = (const_strptr) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = renameat(___oldfd, ___old, ___newfd, ___new); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

arp2_clock_t
arp2sys_clock(regptr _regs) REGPARAM;

arp2_clock_t
REGPARAM2 arp2sys_clock(regptr _regs)
{
  uae_u32 rc = clock();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_time_t
arp2sys_time(regptr _regs) REGPARAM;

arp2_time_t
REGPARAM2 arp2sys_time(regptr _regs)
{
  arp2_time_t rc;
  arp2_time_t* ___time = (arp2_time_t*) (uintptr_t) _regs->a0;

  rc = time(NULL);

  if (___time != NULL) {
    *___time = BE32(rc);
  }
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_nanosleep(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_nanosleep(regptr _regs)
{
  const struct arp2_timespec* ___requested_time = (const struct arp2_timespec*) (uintptr_t) _regs->a0;
  struct arp2_timespec* ___remaining = (struct arp2_timespec*) (uintptr_t) _regs->a1;

  GET(timespec, ___requested_time, requested_time);
  struct timespec remaining;
  uae_u32 rc = nanosleep(requested_time, &remaining);
  PUT(timespec, ___remaining, remaining)
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_clock_getres(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_clock_getres(regptr _regs)
{
  arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0;
  struct arp2_timespec* ___res = (struct arp2_timespec*) (uintptr_t) _regs->a0;

  struct timespec res;
  uae_u32 rc = clock_getres(___clock_id, &res);
  PUT(timespec, ___res, res);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_clock_gettime(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_clock_gettime(regptr _regs)
{
  arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0;
  struct arp2_timespec* ___tp = (struct arp2_timespec*) (uintptr_t) _regs->a0;

  struct timespec tp;
  uae_u32 rc = clock_gettime(___clock_id, &tp);
  PUT(timespec, ___tp, tp);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_clock_settime(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_clock_settime(regptr _regs)
{
  arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0;
  const struct arp2_timespec* ___tp = (const struct arp2_timespec*) (uintptr_t) _regs->a0;

  GET(timespec, ___tp, tp);
  uae_u32 rc = clock_settime(___clock_id, tp);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_clock_nanosleep(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_clock_nanosleep(regptr _regs)
{
  arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  const struct arp2_timespec* ___req = (const struct arp2_timespec*) (uintptr_t) _regs->a0;
  struct arp2_timespec* ___rem = (struct arp2_timespec*) (uintptr_t) _regs->a1;

  GET(timespec, ___req, req);
  struct timespec rem;
  uae_u32 rc = clock_nanosleep(___clock_id, ___flags, req, &rem);
  PUT(timespec, ___rem, rem)
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_clock_getcpuclockid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_clock_getcpuclockid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  arp2_clockid_t* ___clock_id = (arp2_clockid_t*) (uintptr_t) _regs->a0;

  clockid_t clock_id;
  uae_u32 rc = clock_getcpuclockid(___pid, &clock_id);
  *___clock_id = BE32(clock_id);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_timer_create(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_create(regptr _regs) */
/* { */
/*   arp2_clockid_t ___clock_id = (arp2_clockid_t) _regs->d0; */
/*   struct arp2_sigevent* ___evp = (struct arp2_sigevent*) (uintptr_t) _regs->a0; */
/*   arp2_timer_t* ___timerid = (arp2_timer_t*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = timer_create(___clock_id, ___evp, ___timerid); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_timer_delete(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_delete(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   uae_u32 rc = timer_delete(___timerid); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_timer_settime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_settime(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   const struct arp2_itimerspec* ___value = (const struct arp2_itimerspec*) (uintptr_t) _regs->a1; */
/*   struct arp2_itimerspec* ___ovalue = (struct arp2_itimerspec*) (uintptr_t) _regs->a2; */
/*   uae_u32 rc = timer_settime(___timerid, ___flags, ___value, ___ovalue); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_timer_gettime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_gettime(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   struct arp2_itimerspec* ___value = (struct arp2_itimerspec*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = timer_gettime(___timerid, ___value); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_timer_getoverrun(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_getoverrun(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   uae_u32 rc = timer_getoverrun(___timerid); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_access(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_access(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___type = (uae_s32) _regs->d0;
  uae_u32 rc = access(___name, ___type);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_euidaccess(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_euidaccess(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___type = (uae_s32) _regs->d0;
  uae_u32 rc = euidaccess(___name, ___type);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_eaccess(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_eaccess(regptr _regs) */
/* { */
/*   const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___type = (uae_s32) _regs->d0; */
/*   uae_u32 rc = eaccess(___name, ___type); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_faccessat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_faccessat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___type = (uae_s32) _regs->d1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d2; */
/*   uae_u32 rc = faccessat(___fd, ___file, ___type, ___flag); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s64
arp2sys_lseek(regptr _regs) REGPARAM;

uae_s64
REGPARAM2 arp2sys_lseek(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s64 ___offset = ((uae_s64) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  uae_s32 ___whence = (uae_s32) _regs->d3;
  uae_u32 rc = lseek(___fd, ___offset, ___whence);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_close(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_close(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = close(___fd);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_read(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_read(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  aptr ___buf = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___count = (uae_u32) _regs->d1;
  uae_u32 rc = read(___fd, ___buf, ___count);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_write(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_write(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const_aptr ___buf = (const_aptr) (uintptr_t) _regs->a0;
  uae_u32 ___count = (uae_u32) _regs->d1;
  uae_u32 rc = write(___fd, ___buf, ___count);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_pread(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pread(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  aptr ___buf = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___nbytes = (uae_u32) _regs->d1;
  uae_u64 ___offset = (uae_u64) _regs->d2;
  uae_u32 rc = pread(___fd, ___buf, ___nbytes, ___offset);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_pwrite(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pwrite(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const_aptr ___buf = (const_aptr) (uintptr_t) _regs->a0;
  uae_u32 ___n = (uae_u32) _regs->d1;
  uae_u64 ___offset = (uae_u64) _regs->d2;
  uae_u32 rc = pwrite(___fd, ___buf, ___n, ___offset);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_pipe(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pipe(regptr _regs)
{
  uae_s32* ___pipedes = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0;

  int pipedes[2];
  uae_u32 rc = pipe(pipedes);
  ___pipedes[0] = BE32(pipedes[0]);
  ___pipedes[1] = BE32(pipedes[1]);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_u32
arp2sys_alarm(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_alarm(regptr _regs)
{
  uae_u32 ___seconds = (uae_u32) _regs->d0;
  uae_u32 rc = alarm(___seconds);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_u32
arp2sys_sleep(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_sleep(regptr _regs)
{
  uae_u32 ___seconds = (uae_u32) _regs->d0;
  uae_u32 rc = sleep(___seconds);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_pause(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pause(regptr _regs)
{
  uae_u32 rc = pause();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_chown(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chown(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  arp2_uid_t ___owner = (arp2_uid_t) _regs->d0;
  arp2_gid_t ___group = (arp2_gid_t) _regs->d1;
  uae_u32 rc = chown(___file, ___owner, ___group);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fchown(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchown(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  arp2_uid_t ___owner = (arp2_uid_t) _regs->d1;
  arp2_gid_t ___group = (arp2_gid_t) _regs->d2;
  uae_u32 rc = fchown(___fd, ___owner, ___group);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_lchown(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lchown(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  arp2_uid_t ___owner = (arp2_uid_t) _regs->d0;
  arp2_gid_t ___group = (arp2_gid_t) _regs->d1;
  uae_u32 rc = lchown(___file, ___owner, ___group);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_fchownat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fchownat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   arp2_uid_t ___owner = (arp2_uid_t) _regs->d1; */
/*   arp2_gid_t ___group = (arp2_gid_t) _regs->d2; */
/*   uae_s32 ___flag = (uae_s32) _regs->d3; */
/*   uae_u32 rc = fchownat(___fd, ___file, ___owner, ___group, ___flag); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_chdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = chdir(___path);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fchdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchdir(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = fchdir(___fd);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

strptr
arp2sys_getcwd(regptr _regs) REGPARAM;

strptr
REGPARAM2 arp2sys_getcwd(regptr _regs)
{
  strptr ___buf = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d0;
  strptr rc = getcwd(___buf, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_dup(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_dup(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = dup(___fd);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_dup2(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_dup2(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___fd2 = (uae_s32) _regs->d1;
  uae_u32 rc = dup2(___fd, ___fd2);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_execve(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_execve(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   strptr const* ___argv = (strptr const*) (uintptr_t) _regs->a1; */
/*   strptr const* ___envp = (strptr const*) (uintptr_t) _regs->a2; */
/*   uae_u32 rc = execve(___path, ___argv, ___envp); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_execv(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_execv(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   strptr const* ___argv = (strptr const*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = execv(___path, ___argv); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_execvp(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_execvp(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   strptr const* ___argv = (strptr const*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = execvp(___file, ___argv); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_nice(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_nice(regptr _regs)
{
  uae_s32 ___inc = (uae_s32) _regs->d0;
  uae_u32 rc = nice(___inc);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_pathconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pathconf(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___name = (uae_s32) _regs->d0;
  uae_u32 rc = pathconf(___path, ___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fpathconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fpathconf(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___name = (uae_s32) _regs->d1;
  uae_u32 rc = fpathconf(___fd, ___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sysconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sysconf(regptr _regs)
{
  uae_s32 ___name = (uae_s32) _regs->d0;
  uae_u32 rc = sysconf(___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_u32
arp2sys_confstr(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_confstr(regptr _regs)
{
  uae_s32 ___name = (uae_s32) _regs->d0;
  strptr ___buf = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d1;
  uae_u32 rc = confstr(___name, ___buf, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_getpid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpid(regptr _regs)
{
  uae_u32 rc = getpid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_getppid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getppid(regptr _regs)
{
  uae_u32 rc = getppid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_getpgrp(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpgrp(regptr _regs)
{
  uae_u32 rc = getpgrp();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_getpgid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpgid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 rc = getpgid(___pid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setpgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpgid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  arp2_pid_t ___pgid = (arp2_pid_t) _regs->d1;
  uae_u32 rc = setpgid(___pid, ___pgid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setpgrp(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpgrp(regptr _regs)
{
  uae_u32 rc = setpgrp();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_setsid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_setsid(regptr _regs)
{
  uae_u32 rc = setsid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_getsid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getsid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 rc = getsid(___pid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_uid_t
arp2sys_getuid(regptr _regs) REGPARAM;

arp2_uid_t
REGPARAM2 arp2sys_getuid(regptr _regs)
{
  uae_u32 rc = getuid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_uid_t
arp2sys_geteuid(regptr _regs) REGPARAM;

arp2_uid_t
REGPARAM2 arp2sys_geteuid(regptr _regs)
{
  uae_u32 rc = geteuid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_gid_t
arp2sys_getgid(regptr _regs) REGPARAM;

arp2_gid_t
REGPARAM2 arp2sys_getgid(regptr _regs)
{
  uae_u32 rc = getgid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_gid_t
arp2sys_getegid(regptr _regs) REGPARAM;

arp2_gid_t
REGPARAM2 arp2sys_getegid(regptr _regs)
{
  uae_u32 rc = getegid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_getgroups(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getgroups(regptr _regs) */
/* { */
/*   uae_s32 ___size = (uae_s32) _regs->d0; */
/*   arp2_gid_t* ___list = (arp2_gid_t*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = getgroups(___size, ___list); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_setgroups(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_setgroups(regptr _regs) */
/* { */
/*   uae_u32 ___n = (uae_u32) _regs->d0; */
/*   const arp2_gid_t* ___groups = (const arp2_gid_t*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = setgroups(___n, ___groups); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_group_member(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_group_member(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = group_member(___gid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  uae_u32 rc = setuid(___uid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setreuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setreuid(regptr _regs)
{
  arp2_uid_t ___ruid = (arp2_uid_t) _regs->d0;
  arp2_uid_t ___euid = (arp2_uid_t) _regs->d1;
  uae_u32 rc = setreuid(___ruid, ___euid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_seteuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_seteuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  uae_u32 rc = seteuid(___uid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setgid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = setgid(___gid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setregid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setregid(regptr _regs)
{
  arp2_gid_t ___rgid = (arp2_gid_t) _regs->d0;
  arp2_gid_t ___egid = (arp2_gid_t) _regs->d1;
  uae_u32 rc = setregid(___rgid, ___egid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setegid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setegid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = setegid(___gid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_getresuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getresuid(regptr _regs)
{
  arp2_uid_t* ___ruid = (arp2_uid_t*) (uintptr_t) _regs->a0;
  arp2_uid_t* ___euid = (arp2_uid_t*) (uintptr_t) _regs->a1;
  arp2_uid_t* ___suid = (arp2_uid_t*) (uintptr_t) _regs->a2;

  uid_t ruid, euid, suid;
  uae_u32 rc = getresuid(&ruid, &euid, &suid);
  *___ruid = BE32(ruid);
  *___euid = BE32(euid);
  *___suid = BE32(suid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_getresgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getresgid(regptr _regs)
{
  arp2_gid_t* ___rgid = (arp2_gid_t*) (uintptr_t) _regs->a0;
  arp2_gid_t* ___egid = (arp2_gid_t*) (uintptr_t) _regs->a1;
  arp2_gid_t* ___sgid = (arp2_gid_t*) (uintptr_t) _regs->a2;

  uid_t rgid, egid, sgid;
  uae_u32 rc = getresgid(&rgid, &egid, &sgid);
  *___rgid = BE32(rgid);
  *___egid = BE32(egid);
  *___sgid = BE32(sgid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setresuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setresuid(regptr _regs)
{
  arp2_uid_t ___ruid = (arp2_uid_t) _regs->d0;
  arp2_uid_t ___euid = (arp2_uid_t) _regs->d1;
  arp2_uid_t ___suid = (arp2_uid_t) _regs->d2;
  uae_u32 rc = setresuid(___ruid, ___euid, ___suid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setresgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setresgid(regptr _regs)
{
  arp2_gid_t ___rgid = (arp2_gid_t) _regs->d0;
  arp2_gid_t ___egid = (arp2_gid_t) _regs->d1;
  arp2_gid_t ___sgid = (arp2_gid_t) _regs->d2;
  uae_u32 rc = setresgid(___rgid, ___egid, ___sgid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_fork(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_fork(regptr _regs)
{
  uae_u32 rc = fork();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_vfork(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_vfork(regptr _regs)
{
  uae_u32 rc = vfork();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_ttyname_r(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ttyname_r(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  strptr ___buf = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___buflen = (uae_u32) _regs->d1;
  uae_u32 rc = ttyname_r(___fd, ___buf, ___buflen);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_isatty(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_isatty(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = isatty(___fd);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_link(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_link(regptr _regs)
{
  const_strptr ___from = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___to = (const_strptr) (uintptr_t) _regs->a1;
  uae_u32 rc = link(___from, ___to);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_linkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_linkat(regptr _regs) */
/* { */
/*   uae_s32 ___fromfd = (uae_s32) _regs->d0; */
/*   const_strptr ___from = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___tofd = (uae_s32) _regs->d1; */
/*   const_strptr ___to = (const_strptr) (uintptr_t) _regs->a1; */
/*   uae_s32 ___flags = (uae_s32) _regs->d2; */
/*   uae_u32 rc = linkat(___fromfd, ___from, ___tofd, ___to, ___flags); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_symlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_symlink(regptr _regs)
{
  const_strptr ___from = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___to = (const_strptr) (uintptr_t) _regs->a1;
  uae_u32 rc = symlink(___from, ___to);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_readlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_readlink(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  strptr ___buf = (strptr) (uintptr_t) _regs->a1;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = readlink(___path, ___buf, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_symlinkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_symlinkat(regptr _regs) */
/* { */
/*   const_strptr ___from = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___tofd = (uae_s32) _regs->d0; */
/*   const_strptr ___to = (const_strptr) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = symlinkat(___from, ___tofd, ___to); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_readlinkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_readlinkat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   strptr ___buf = (strptr) (uintptr_t) _regs->a1; */
/*   uae_u32 ___len = (uae_u32) _regs->d1; */
/*   uae_u32 rc = readlinkat(___fd, ___path, ___buf, ___len); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = unlink(___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_unlinkat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_unlinkat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0; */
/*   uae_s32 ___flag = (uae_s32) _regs->d1; */
/*   uae_u32 rc = unlinkat(___fd, ___name, ___flag); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_rmdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_rmdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = rmdir(___path);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_pid_t
arp2sys_tcgetpgrp(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_tcgetpgrp(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = tcgetpgrp(___fd);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_tcsetpgrp(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_tcsetpgrp(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  arp2_pid_t ___pgrp_id = (arp2_pid_t) _regs->d1;
  uae_u32 rc = tcsetpgrp(___fd, ___pgrp_id);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_getlogin_r(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getlogin_r(regptr _regs)
{
  strptr ___name = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___name_len = (uae_u32) _regs->d0;
  uae_u32 rc = getlogin_r(___name, ___name_len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setlogin(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setlogin(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = setlogin(___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_gethostname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_gethostname(regptr _regs)
{
  strptr ___name = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = gethostname(___name, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sethostname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sethostname(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = sethostname(___name, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_sethostid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sethostid(regptr _regs)
{
  uae_s32 ___id = (uae_s32) _regs->d0;
  uae_u32 rc = sethostid(___id);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_getdomainname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getdomainname(regptr _regs)
{
  strptr ___name = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = getdomainname(___name, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setdomainname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setdomainname(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = setdomainname(___name, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_vhangup(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_vhangup(regptr _regs)
{
  uae_u32 rc = vhangup();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_revoke(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_revoke(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = revoke(___file);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_profil(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_profil(regptr _regs)
{
  uae_u16* ___sample_buffer = (uae_u16*) (uintptr_t) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_u32 ___offset = (uae_u32) _regs->d1;
  uae_u32 ___scale = (uae_u32) _regs->d2;
#warning There is no way to convert buffer to big-endian!
  uae_u32 rc = profil(___sample_buffer, ___size, ___offset, ___scale);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_acct(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_acct(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = acct(___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_daemon(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_daemon(regptr _regs)
{
  uae_s32 ___nochdir = (uae_s32) _regs->d0;
  uae_s32 ___noclose = (uae_s32) _regs->d1;
  uae_u32 rc = daemon(___nochdir, ___noclose);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_chroot(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chroot(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = chroot(___path);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fsync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fsync(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = fsync(___fd);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_gethostid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_gethostid(regptr _regs)
{
  uae_u32 rc = gethostid();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
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
  uae_u32 rc = getpagesize();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_getdtablesize(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getdtablesize(regptr _regs)
{
  uae_u32 rc = getdtablesize();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_truncate(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_truncate(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  uae_u64 ___length = (uae_u64) _regs->d0;
  uae_u32 rc = truncate(___file, ___length);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_ftruncate(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ftruncate(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u64 ___length = (uae_u64) _regs->d1;
  uae_u32 rc = ftruncate(___fd, ___length);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_brk(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_brk(regptr _regs)
{
  aptr ___end_data_segment = (aptr) (uintptr_t) _regs->a0;
  uae_u32 rc = brk(___end_data_segment);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

aptr
arp2sys_sbrk(regptr _regs) REGPARAM;

aptr
REGPARAM2 arp2sys_sbrk(regptr _regs)
{
  uae_s32 ___increment = (uae_s32) _regs->d0;
  aptr rc = sbrk(___increment);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_lockf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lockf(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___cmd = (uae_s32) _regs->d1;
  uae_u64 ___len = (uae_u64) _regs->d2;
  uae_u32 rc = lockf(___fd, ___cmd, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fdatasync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fdatasync(regptr _regs)
{
  uae_s32 ___fildes = (uae_s32) _regs->d0;
  uae_u32 rc = fdatasync(___fildes);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_getdents(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getdents(regptr _regs) */
/* { */
/*   uae_u32 ___fd = (uae_u32) _regs->d0; */
/*   struct arp2_dirent* ___dirp = (struct arp2_dirent*) (uintptr_t) _regs->a0; */
/*   uae_u32 ___count = (uae_u32) _regs->d1; */
/*   uae_u32 rc = getdents(___fd, ___dirp, ___count); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_utime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_utime(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   const struct arp2_utimbuf* ___file_times = (const struct arp2_utimbuf*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = utime(___file, ___file_times); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_setxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a1;
  const_aptr ___value = (const_aptr) (uintptr_t) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  uae_u32 rc = setxattr(___path, ___name, ___value, ___size, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_lsetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lsetxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a1;
  const_aptr ___value = (const_aptr) (uintptr_t) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  uae_u32 rc = lsetxattr(___path, ___name, ___value, ___size, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fsetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fsetxattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  const_aptr ___value = (const_aptr) (uintptr_t) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  uae_u32 rc = fsetxattr(___filedes, ___name, ___value, ___size, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_getxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a1;
  aptr ___value = (aptr) (uintptr_t) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_u32 rc = getxattr(___path, ___name, ___value, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_lgetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lgetxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a1;
  aptr ___value = (aptr) (uintptr_t) _regs->a2;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_u32 rc = lgetxattr(___path, ___name, ___value, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fgetxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fgetxattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  aptr ___value = (aptr) (uintptr_t) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d1;
  uae_u32 rc = fgetxattr(___filedes, ___name, ___value, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_listxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_listxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  strptr ___list = (strptr) (uintptr_t) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_u32 rc = listxattr(___path, ___list, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_llistxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_llistxattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  strptr ___list = (strptr) (uintptr_t) _regs->a1;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_u32 rc = llistxattr(___path, ___list, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_flistxattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_flistxattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  strptr ___list = (strptr) (uintptr_t) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d1;
  uae_u32 rc = flistxattr(___filedes, ___list, ___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_removexattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_removexattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a1;
  uae_u32 rc = removexattr(___path, ___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_lremovexattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lremovexattr(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a1;
  uae_u32 rc = lremovexattr(___path, ___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fremovexattr(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fremovexattr(regptr _regs)
{
  uae_s32 ___filedes = (uae_s32) _regs->d0;
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = fremovexattr(___filedes, ___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_epoll_create(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_epoll_create(regptr _regs)
{
  uae_s32 ___size = (uae_s32) _regs->d0;
  uae_u32 rc = epoll_create(___size);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_epoll_ctl(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_epoll_ctl(regptr _regs) */
/* { */
/*   uae_s32 ___epfd = (uae_s32) _regs->d0; */
/*   uae_s32 ___op = (uae_s32) _regs->d1; */
/*   uae_s32 ___fd = (uae_s32) _regs->d2; */
/*   struct arp2_epoll_event* ___event = (struct arp2_epoll_event*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = epoll_ctl(___epfd, ___op, ___fd, ___event); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_epoll_wait(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_epoll_wait(regptr _regs) */
/* { */
/*   uae_s32 ___epfd = (uae_s32) _regs->d0; */
/*   struct arp2_epoll_event* ___events = (struct arp2_epoll_event*) (uintptr_t) _regs->a0; */
/*   uae_s32 ___maxevents = (uae_s32) _regs->d1; */
/*   uae_s32 ___timeout = (uae_s32) _regs->d2; */
/*   uae_u32 rc = epoll_wait(___epfd, ___events, ___maxevents, ___timeout); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_flock(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_flock(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s32 ___operation = (uae_s32) _regs->d1;
  uae_u32 rc = flock(___fd, ___operation);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setfsuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setfsuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  uae_u32 rc = setfsuid(___uid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setfsgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setfsgid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = setfsgid(___gid);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_ioperm(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ioperm(regptr _regs)
{
  uae_u32 ___from = (uae_u32) _regs->d0;
  uae_u32 ___num = (uae_u32) _regs->d1;
  uae_s32 ___turn_on = (uae_s32) _regs->d2;
  uae_u32 rc = ioperm(___from, ___num, ___turn_on);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_iopl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_iopl(regptr _regs)
{
  uae_s32 ___level = (uae_s32) _regs->d0;
  uae_u32 rc = iopl(___level);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_ioctl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_ioctl(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 ___request = (uae_u32) _regs->d1;
  aptr ___arg = (aptr) (uintptr_t) _regs->a0;
  uae_u32 rc = ioctl(___fd, ___request, ___arg);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_klogctl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_klogctl(regptr _regs)
{
  uae_s32 ___type = (uae_s32) _regs->d0;
  strptr ___bufp = (strptr) (uintptr_t) _regs->a0;
  uae_s32 ___len = (uae_s32) _regs->d1;
  uae_u32 rc = klogctl(___type, ___bufp, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

aptr
arp2sys_mmap(regptr _regs) REGPARAM;

aptr
REGPARAM2 arp2sys_mmap(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  uae_s32 ___fd = (uae_s32) _regs->d3;
  uae_s64 ___offset = ((uae_s64) _regs->d4 << 32) | ((uae_u32) _regs->d5);
  aptr rc = mmap(___start, ___length, ___prot, ___flags, ___fd, ___offset);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_munmap(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munmap(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_u32 rc = munmap(___start, ___length);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mprotect(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mprotect(regptr _regs)
{
  const_aptr ___addr = (const_aptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  uae_u32 rc = mprotect((void*) ___addr, ___len, ___prot);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_msync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_msync(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___flags = (uae_s32) _regs->d1;
  uae_u32 rc = msync(___start, ___length, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_madvise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_madvise(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___advice = (uae_s32) _regs->d1;
  uae_u32 rc = madvise(___start, ___length, ___advice);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_posix_madvise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_posix_madvise(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___advice = (uae_s32) _regs->d1;
  uae_u32 rc = posix_madvise(___start, ___length, ___advice);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mlock(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mlock(regptr _regs)
{
  const_aptr ___addr = (const_aptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = mlock(___addr, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_munlock(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munlock(regptr _regs)
{
  const_aptr ___addr = (const_aptr) (uintptr_t) _regs->a0;
  uae_u32 ___len = (uae_u32) _regs->d0;
  uae_u32 rc = munlock(___addr, ___len);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mlockall(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mlockall(regptr _regs)
{
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 rc = mlockall(___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_munlockall(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munlockall(regptr _regs)
{
  uae_u32 rc = munlockall();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mincore(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mincore(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_u8* ___vec = (uae_u8*) (uintptr_t) _regs->a1;
  uae_u32 rc = mincore(___start, ___length, ___vec);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

aptr
arp2sys_mremap(regptr _regs) REGPARAM;

aptr
REGPARAM2 arp2sys_mremap(regptr _regs)
{
  aptr ___old_address = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___old_size = (uae_u32) _regs->d0;
  uae_u32 ___new_size = (uae_u32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  aptr rc = mremap(___old_address, ___old_size, ___new_size, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_remap_file_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_remap_file_pages(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___size = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  uae_u32 ___pgoff = (uae_u32) _regs->d2;
  uae_s32 ___flags = (uae_s32) _regs->d3;
  uae_u32 rc = remap_file_pages(___start, ___size, ___prot, ___pgoff, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_shm_open(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_shm_open(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___oflag = (uae_s32) _regs->d0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d1;
  uae_u32 rc = shm_open(___name, ___oflag, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_shm_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_shm_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = shm_unlink(___name);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mount(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mount(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) (uintptr_t) _regs->a0;
  const_strptr ___dir = (const_strptr) (uintptr_t) _regs->a1;
  const_strptr ___fstype = (const_strptr) (uintptr_t) _regs->a2;
  uae_u32 ___rwflag = (uae_u32) _regs->d0;
  const_aptr ___data = (const_aptr) (uintptr_t) _regs->a3;
  uae_u32 rc = mount(___special_file, ___dir, ___fstype, ___rwflag, ___data);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_umount(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_umount(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = umount(___special_file);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_umount2(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_umount2(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 rc = umount2(___special_file, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_getrlimit64(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getrlimit64(regptr _regs) */
/* { */
/*   arp2_rlimit_resource_t ___resource = (arp2_rlimit_resource_t) _regs->d0; */
/*   struct arp2_rlimit* ___rlimits = (struct arp2_rlimit*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = getrlimit64(___resource, ___rlimits); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_setrlimit64(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_setrlimit64(regptr _regs) */
/* { */
/*   arp2_rlimit_resource_t ___resource = (arp2_rlimit_resource_t) _regs->d0; */
/*   const struct arp2_rlimit* ___rlimits = (const struct arp2_rlimit*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = setrlimit64(___resource, ___rlimits); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_getrusage(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getrusage(regptr _regs) */
/* { */
/*   arp2_rusage_who_t ___who = (arp2_rusage_who_t) _regs->d0; */
/*   struct arp2_rusage* ___usage = (struct arp2_rusage*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = getrusage(___who, ___usage); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_getpriority(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getpriority(regptr _regs)
{
  arp2_priority_which_t ___which = (arp2_priority_which_t) _regs->d0;
  arp2_id_t ___who = (arp2_id_t) _regs->d1;
  uae_u32 rc = getpriority(___which, ___who);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_setpriority(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpriority(regptr _regs)
{
  arp2_priority_which_t ___which = (arp2_priority_which_t) _regs->d0;
  arp2_id_t ___who = (arp2_id_t) _regs->d1;
  uae_s32 ___prio = (uae_s32) _regs->d2;
  uae_u32 rc = setpriority(___which, ___who, ___prio);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_select(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_select(regptr _regs) */
/* { */
/*   uae_s32 ___nfds = (uae_s32) _regs->d0; */
/*   arp2_fd_set* ___readfds = (arp2_fd_set*) (uintptr_t) _regs->a0; */
/*   arp2_fd_set* ___writefds = (arp2_fd_set*) (uintptr_t) _regs->a1; */
/*   arp2_fd_set* ___exceptfds = (arp2_fd_set*) (uintptr_t) _regs->a2; */
/*   struct arp2_timeval* ___timeout = (struct arp2_timeval*) (uintptr_t) _regs->a3; */
/*   uae_u32 rc = select(___nfds, ___readfds, ___writefds, ___exceptfds, ___timeout); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_pselect(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_pselect(regptr _regs) */
/* { */
/*   uae_s32 ___nfds = (uae_s32) _regs->d0; */
/*   arp2_fd_set* ___readfds = (arp2_fd_set*) (uintptr_t) _regs->a0; */
/*   arp2_fd_set* ___writefds = (arp2_fd_set*) (uintptr_t) _regs->a1; */
/*   arp2_fd_set* ___exceptfds = (arp2_fd_set*) (uintptr_t) _regs->a2; */
/*   const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a3; */
/*   const arp2_sigset_t* ___sigmask = (const arp2_sigset_t*) (uintptr_t) _regs->a4; */
/*   uae_u32 rc = pselect(___nfds, ___readfds, ___writefds, ___exceptfds, ___timeout, ___sigmask); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_stat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_stat(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = stat(___path, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_fstat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = fstat(___fd, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_fstatat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d1; */
/*   uae_u32 rc = fstatat(___fd, ___file, ___buf, ___flag); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_lstat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_lstat(regptr _regs) */
/* { */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = lstat(___path, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_chmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chmod(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  uae_u32 rc = chmod(___file, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_lchmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lchmod(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  uae_u32 rc = lchmod(___file, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_fchmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchmod(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d1;
  uae_u32 rc = fchmod(___fd, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_fchmodat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fchmodat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d2; */
/*   uae_u32 rc = fchmodat(___fd, ___file, ___mode, ___flag); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

arp2_mode_t
arp2sys_umask(regptr _regs) REGPARAM;

arp2_mode_t
REGPARAM2 arp2sys_umask(regptr _regs)
{
  arp2_mode_t ___mask = (arp2_mode_t) _regs->d0;
  uae_u32 rc = umask(___mask);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

arp2_mode_t
arp2sys_getumask(regptr _regs) REGPARAM;

arp2_mode_t
REGPARAM2 arp2sys_getumask(regptr _regs)
{
#warning Check if this has been implemented yet
//  uae_u32 rc = getumask();
  arp2_mode_t mask = umask(0);
  umask(mask);
  uae_u32 rc = mask;
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_mkdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mkdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  uae_u32 rc = mkdir(___path, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_mkdirat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mkdirat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   uae_u32 rc = mkdirat(___fd, ___path, ___mode); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_mknod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mknod(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  arp2_dev_t ___dev = ((arp2_dev_t) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  uae_u32 rc = mknod(___path, ___mode, ___dev);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_mknodat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mknodat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   arp2_dev_t ___dev = ((arp2_dev_t) _regs->d2 << 32) | ((uae_u32) _regs->d3); */
/*   uae_u32 rc = mknodat(___fd, ___path, ___mode, ___dev); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_mkfifo(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mkfifo(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  uae_u32 rc = mkfifo(___path, ___mode);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_mkfifoat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_mkfifoat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0; */
/*   arp2_mode_t ___mode = (arp2_mode_t) _regs->d1; */
/*   uae_u32 rc = mkfifoat(___fd, ___path, ___mode); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_statfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_statfs(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   struct arp2_statfs* ___buf = (struct arp2_statfs*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = statfs(___file, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_fstatfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatfs(regptr _regs) */
/* { */
/*   uae_s32 ___fildes = (uae_s32) _regs->d0; */
/*   struct arp2_statfs* ___buf = (struct arp2_statfs*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = fstatfs(___fildes, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_statvfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_statvfs(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   struct arp2_statvfs* ___buf = (struct arp2_statvfs*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = statvfs(___file, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_fstatvfs(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatvfs(regptr _regs) */
/* { */
/*   uae_s32 ___fildes = (uae_s32) _regs->d0; */
/*   struct arp2_statvfs* ___buf = (struct arp2_statvfs*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = fstatvfs(___fildes, ___buf); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_swapon(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_swapon(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 rc = swapon(___path, ___flags);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_swapoff(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_swapoff(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = swapoff(___path);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_sysinfo(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sysinfo(regptr _regs) */
/* { */
/*   struct arp2_sysinfo* ___info = (struct arp2_sysinfo*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = sysinfo(___info); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_get_nprocs_conf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_nprocs_conf(regptr _regs)
{
  uae_u32 rc = get_nprocs_conf();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_get_nprocs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_nprocs(regptr _regs)
{
  uae_u32 rc = get_nprocs();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_get_phys_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_phys_pages(regptr _regs)
{
  uae_u32 rc = get_phys_pages();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_get_avphys_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_avphys_pages(regptr _regs)
{
  uae_u32 rc = get_avphys_pages();
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_gettimeofday(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_gettimeofday(regptr _regs)
{
  struct arp2_timeval* ___tv = (struct arp2_timeval*) (uintptr_t) _regs->a0;
  struct arp2_timezone* ___tz = (struct arp2_timezone*) (uintptr_t) _regs->a1;

  struct timeval tv;
  uae_s32 rc = gettimeofday(&tv, NULL);
  PUT(timeval, ___tv, tv);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

uae_s32
arp2sys_settimeofday(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_settimeofday(regptr _regs)
{
  const struct arp2_timeval* ___tv = (const struct arp2_timeval*) (uintptr_t) _regs->a0;
  const struct arp2_timezone* ___tz = (const struct arp2_timezone*) (uintptr_t) _regs->a1;

  GET(timeval, ___tv, tv);
  uae_u32 rc = settimeofday(tv, NULL);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* uae_s32 */
/* arp2sys_adjtime(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_adjtime(regptr _regs) */
/* { */
/*   const struct arp2_timeval* ___delta = (const struct arp2_timeval*) (uintptr_t) _regs->a0; */
/*   struct arp2_timeval* ___olddelta = (struct arp2_timeval*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = adjtime(___delta, ___olddelta); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_getitimer(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_getitimer(regptr _regs) */
/* { */
/*   arp2_itimer_which_t ___which = (arp2_itimer_which_t) _regs->d0; */
/*   struct arp2_itimerval* ___value = (struct arp2_itimerval*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = getitimer(___which, ___value); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_setitimer(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_setitimer(regptr _regs) */
/* { */
/*   arp2_itimer_which_t ___which = (arp2_itimer_which_t) _regs->d0; */
/*   const struct arp2_itimerval* ___new = (const struct arp2_itimerval*) (uintptr_t) _regs->a0; */
/*   struct arp2_itimerval* ___old = (struct arp2_itimerval*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = setitimer(___which, ___new, ___old); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_utimes(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_utimes(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = utimes(___file, ___tvp); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_lutimes(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_lutimes(regptr _regs) */
/* { */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = lutimes(___file, ___tvp); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_futimes(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_futimes(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = futimes(___fd, ___tvp); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_futimesat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_futimesat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = futimesat(___fd, ___file, ___tvp); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* arp2_clock_t */
/* arp2sys_times(regptr _regs) REGPARAM; */

/* arp2_clock_t */
/* REGPARAM2 arp2sys_times(regptr _regs) */
/* { */
/*   struct arp2_tms* ___buffer = (struct arp2_tms*) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = times(___buffer); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_readv(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_readv(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const struct arp2_iovec* ___iovec = (const struct arp2_iovec*) (uintptr_t) _regs->a0; */
/*   uae_s32 ___count = (uae_s32) _regs->d1; */
/*   uae_u32 rc = readv(___fd, ___iovec, ___count); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_writev(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_writev(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const struct arp2_iovec* ___iovec = (const struct arp2_iovec*) (uintptr_t) _regs->a0; */
/*   uae_s32 ___count = (uae_s32) _regs->d1; */
/*   uae_u32 rc = writev(___fd, ___iovec, ___count); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

uae_s32
arp2sys_uname(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_uname(regptr _regs)
{
  struct arp2_utsname* ___name = (struct arp2_utsname*) (uintptr_t) _regs->a0;
  
  uae_s32 rc;
  struct utsname name;
  rc =  uname(&name);
  strcpy((char*) ___name->sysname, name.sysname);
  strcpy((char*) ___name->nodename, name.nodename);
  strcpy((char*) ___name->release, name.release);
  strcpy((char*) ___name->version, name.version);
  strcpy((char*) ___name->machine, name.machine);
  strcpy((char*) ___name->domainname, name.domainname);
  set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
}

/* arp2_pid_t */
/* arp2sys_wait(regptr _regs) REGPARAM; */

/* arp2_pid_t */
/* REGPARAM2 arp2sys_wait(regptr _regs) */
/* { */
/*   uae_s32* ___stat_loc = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0; */
/*   uae_u32 rc = wait(___stat_loc); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* uae_s32 */
/* arp2sys_waitid(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_waitid(regptr _regs) */
/* { */
/*   arp2_idtype_t ___idtype = (arp2_idtype_t) _regs->d0; */
/*   arp2_id_t ___id = (arp2_id_t) _regs->d1; */
/*   arp2_siginfo_t* ___infop = (arp2_siginfo_t*) (uintptr_t) _regs->a0; */
/*   uae_s32 ___options = (uae_s32) _regs->d2; */
/*   uae_u32 rc = waitid(___idtype, ___id, ___infop, ___options); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* arp2_pid_t */
/* arp2sys_wait3(regptr _regs) REGPARAM; */

/* arp2_pid_t */
/* REGPARAM2 arp2sys_wait3(regptr _regs) */
/* { */
/*   uae_s32* ___stat_loc = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0; */
/*   uae_s32 ___options = (uae_s32) _regs->d0; */
/*   struct arp2_rusage* ___usage = (struct arp2_rusage*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = wait3(___stat_loc, ___options, ___usage); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */

/* arp2_pid_t */
/* arp2sys_wait4(regptr _regs) REGPARAM; */

/* arp2_pid_t */
/* REGPARAM2 arp2sys_wait4(regptr _regs) */
/* { */
/*   arp2_pid_t ___pid = (arp2_pid_t) _regs->d0; */
/*   uae_s32* ___stat_loc = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0; */
/*   uae_s32 ___options = (uae_s32) _regs->d1; */
/*   struct arp2_rusage* ___usage = (struct arp2_rusage*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = wait4(___pid, ___stat_loc, ___options, ___usage); */
/*   set_io_err((struct sysresbase*) (uintptr_t) _regs->a6);
  return rc;
} */


/*** A NULL-terminated array of function pointers to the syscall wrappers ****/

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
  arp2sys_sched_yield,
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
  uae_u32* rom = (uae_u32*) (uintptr_t) arp2rom;

  if (BE32(rom[1024]) == 0xdeadc0de &&
      BE32(rom[1025]) == ~0U) {
    int i;
    write_log("ARP2 ROM: arp2sys patching ROM image\n");

    for (i = 0; arp2sys_functions[i] != 0; ++i) {
      assert ((((uae_u64) (uintptr_t) arp2sys_functions[i]) & 0xffffffff00000000ULL) == 0);
      
      rom[1024+i] = BE32((uae_u32) (uintptr_t) arp2sys_functions[i]);
    }

    rom[1024+i] = BE32(~0U);
  }

  return 1;
}

void arp2sys_free(void) {
  // Do what?
}

#endif
