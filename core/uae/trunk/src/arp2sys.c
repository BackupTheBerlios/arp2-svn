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

#define IS_32BIT(x) ((((uae_u64) (uintptr_t) (x)) & 0xffffffff00000000ULL) == 0)

/*** Include prototypes for all functions we export **************************/

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <fcntl.h>
#include <grp.h>
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

#include <sys/syscall.h>
#include <pthread.h>

#undef S_WRITE	// Defined in <sys/mount.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "blomcall.h"
#include "arp2sys.h"
#include "lists.h"
#include "uae_endian.h"


#ifdef WORDS_BIGENDIAN
# define BE16(x) (x)
# define BE32(x) (x)
# define BE64(x) (x)
#else
# define BE16(x) bswap_16(x)
# define BE32(x) bswap_32(x)
# define BE64(x) bswap_64(x)
#endif


/*** BJMP register argument definition ***************************************/

typedef struct regs {
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


/*** AmigaOS glue ************************************************************/

#define NT_PROCESS	0x0d
#define SIGF_SINGLE	0x10

#define INTF_SETCLR	0x8000
#define INTF_EXTER	0x2000

#define LVO_SetSignal	-306
#define LVO_Wait	-318
#define LVO_Signal	-324


struct sysresbase {
    uae_u8     _junk[36];
    uae_u32    sysbase;        // At offset 36
};


struct execbase {
    uae_u8     _junk[276];
    uae_u32    thistask;       // At offset 276
};


struct process {
    uae_u8     _junk[8];
    uae_u8     ln_type;        // At offset 8
    uae_u8     _more[139];
    uae_u32    pr_result2;     // At offset 148
};


static inline void clear_signal(struct execbase* exec) {
  struct regs regs;
  
  regs.d0 = 0;
  regs.d1 = SIGF_SINGLE;
  regs.a6 = (uae_u32) (uintptr_t) exec;

  blomcall_calllib68k((uae_u32*) &regs, NULL, LVO_SetSignal);
}


static inline void wait_signal(struct execbase* exec) {
  struct regs regs;
  
  regs.d0 = SIGF_SINGLE;
  regs.a6 = (uae_u32) (uintptr_t) exec;

  blomcall_calllib68k((uae_u32*) &regs, NULL, LVO_Wait);
}


static inline void set_signal(struct process* proc, struct execbase* exec) {
  struct regs regs;
  
  regs.d0 = SIGF_SINGLE;
  regs.a1 = (uae_u32) (uintptr_t) proc;
  regs.a6 = (uae_u32) (uintptr_t) exec;

  puts("LVO_Signal\n");
  blomcall_calllib68k((uae_u32*) &regs, NULL, LVO_Signal);
}


static void set_io_err(int code, struct sysresbase* sysresbase) {
  struct execbase*   exec = (struct execbase*) (uintptr_t) BE32(sysresbase->sysbase);
  struct process*    pr   = (struct process*) (uintptr_t) BE32(exec->thistask);

  if (pr->ln_type == NT_PROCESS) {
    pr->pr_result2 = BE32(code == 0 ? 0 : 1000 + code);
  }
}



/*** arp2-syscall.resource types *********************************************/

typedef char* strptr;
typedef void* aptr;
typedef const char* const_strptr;
typedef const void* const_aptr;

typedef uae_u32 aptr32;

typedef uae_s32 arp2_clock_t;
typedef uae_s32 arp2_clockid_t;
typedef uae_s32 arp2_mqd_t;
typedef uae_s32 arp2_pid_t;
typedef uae_s32 arp2_time_t;
typedef uae_u32 arp2_gid_t;
typedef uae_u32 arp2_id_t;
typedef uae_u32 arp2_idtype_t;
typedef uae_u32 arp2_itimer_which_t;
typedef uae_u32 arp2_mode_t;
typedef uae_u32 arp2_priority_which_t;
typedef uae_u32 arp2_rusage_who_t;
typedef uae_u32 arp2_rlimit_resource_t;
typedef uae_u32 arp2_uid_t;
typedef uae_u64 arp2_dev_t;
typedef uae_u64 arp2_rlim_t;


struct arp2_dirent {
    uae_u64  d_ino;
    uae_s64  d_off;
    uae_u16  d_reclen;
    uae_u8   d_type;
    uae_u8   d_name[256];
};

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

struct arp2_sysinfo {
    uae_s32  uptime;
    uae_u32 loads[3];
    uae_u64 totalram;
    uae_u64 freeram;
    uae_u64 sharedram;
    uae_u64 bufferram;
    uae_u64 totalswap;
    uae_u64 freeswap;
    uae_u64 totalhigh;
    uae_u64 freehigh;
    uae_u32 procs;
    uae_u32 __extra[3];
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

struct arp2_timeval2 {
    struct arp2_timeval timeval[2];
};

struct arp2_itimerval {
    struct arp2_timeval it_interval;
    struct arp2_timeval it_value;
};

struct arp2_tms{
    arp2_clock_t tms_utime;
    arp2_clock_t tms_stime;

    arp2_clock_t tms_cutime;
    arp2_clock_t tms_cstime;
};

struct arp2_utimbuf {
    arp2_time_t actime;
    arp2_time_t modtime;
};


typedef union arp2_sigval {
    uae_s32 sival_int;
    uae_u32 sival_ptr;
} arp2_sigval_t;


#undef si_pid
#undef si_uid
#undef si_tid
#undef si_overrun
#undef si_sigval
#undef si_status
#undef si_utime
#undef si_stime
#undef si_addr
#undef si_band
#undef si_fd

typedef struct arp2_siginfo {
    uae_s32 si_signo;
    uae_s32 si_errno;

    uae_s32 si_code;

    union {
        uae_s32 _pad[((128 / sizeof (uae_s32)) - 4)];

        struct {
            arp2_pid_t si_pid;
            arp2_uid_t si_uid;
        } _kill;

        struct {
            uae_s32 si_tid;
            uae_s32 si_overrun;
            arp2_sigval_t si_sigval;
        } _timer;

        struct {
            arp2_pid_t si_pid;
            arp2_uid_t si_uid;
            arp2_sigval_t si_sigval;
        } _rt;

        struct {
            arp2_pid_t si_pid;
            arp2_uid_t si_uid;
            uae_s32 si_status;
            arp2_clock_t si_utime;
            arp2_clock_t si_stime;
        } _sigchld;

        struct {
            uae_u32 si_addr;
        } _sigfault;

        struct {
            uae_s32  si_band;
            uae_s32 si_fd;
        } _sigpoll;
    } _sifields;
} arp2_siginfo_t;

typedef struct arp2_sigset {
    uae_u8 __val[1024 / (8 * sizeof (uae_u8))];
} arp2_sigset_t;

typedef struct arp2_cpu_set {
    uae_u8 __val[1024 / (8 * sizeof (uae_u8))];
} arp2_cpu_set_t;

typedef struct arp2_fd_set {
    uae_u8 __val[1024 / (8 * sizeof (uae_u8))];
} arp2_fd_set;

struct sigset {
    sigset_t ss;
};

struct cpu_set {
    cpu_set_t cs;
};

struct fd_set {
    fd_set fs;
};

struct timeval2 {
    struct timeval timeval[2];
};


typedef struct arp2_sigevent {
    arp2_sigval_t sigev_value;
    uae_s32 sigev_signo;
    uae_s32 sigev_notify;

    union {
        uae_s32 _pad[((64 / sizeof (uae_s32)) - 4)];

        arp2_pid_t _tid;

        struct {
//            VOID (*_function) (arp2_sigval_t);
//            APTR _attribute;
	    uae_u32 _function;
            uae_u32 _attribute;
        } _sigev_thread;
    } _sigev_un;
} arp2_sigevent_t;


struct arp2_stat {
    arp2_dev_t           st_dev;
    uae_u64                st_ino;
    arp2_mode_t          st_mode;
    uae_u32                st_nlink;
    arp2_uid_t           st_uid;
    arp2_gid_t           st_gid;
    arp2_dev_t           st_rdev;
    uae_u64                st_size;
    uae_u64                st_blocks;
    uae_u32                st_blksize;
    struct arp2_timespec st_atim;
    struct arp2_timespec st_mtim;
    struct arp2_timespec st_ctim;
};

struct arp2_statfs {
    uae_s32  f_type;
    uae_s32  f_bsize;
    uae_u64 f_blocks;
    uae_u64 f_bfree;
    uae_u64 f_bavail;
    uae_u64 f_files;
    uae_u64 f_ffree;
    uae_s32  f_fsid[2];
    uae_s32  f_namelen;
    uae_s32  f_frsize;
    uae_s32  f_spare[5];
};


struct arp2_statvfs {
    uae_u32  f_bsize;
    uae_u32  f_frsize;
    uae_u64  f_blocks;
    uae_u64  f_bfree;
    uae_u64  f_bavail;
    uae_u64  f_files;
    uae_u64  f_ffree;
    uae_u64  f_favail;
    uae_u32  f_fsid;
    uae_s32  __f_unused;
    uae_u32  f_flag;
    uae_u32  f_namemax;
    uae_s32  __f_spare[6];
};

struct arp2_rusage {
    struct arp2_timeval ru_utime;
    struct arp2_timeval ru_stime;
    uae_s32                ru_maxrss;
    uae_s32                ru_ixrss;
    uae_s32                ru_idrss;
    uae_s32                ru_isrss;
    uae_s32                ru_minflt;
    uae_s32                ru_majflt;
    uae_s32                ru_nswap;
    uae_s32                ru_inblock;
    uae_s32                ru_oublock;
    uae_s32                ru_msgsnd;
    uae_s32                ru_msgrcv;
    uae_s32                ru_nsignals;
    uae_s32                ru_nvcsw;
    uae_s32                ru_nivcsw;
};

struct arp2_rlimit {
    arp2_rlim_t rlim_cur;
    arp2_rlim_t rlim_max;
 };

struct arp2_iovec {
    uae_u32 iov_base;
    uae_u32 iov_len;
};



// FIXME: Will only work m68k->native, not the other way around, on 64-bit hosts
#define COPY_sigevent(s,d) \
  d.sigev_value.sival_ptr = (void*) (uintptr_t) BE32(s.sigev_value.sival_ptr); \
  d.sigev_signo = BE32(s.sigev_signo);		\
  d.sigev_notify = BE32(s.sigev_notify);	\
  if (d.sigev_notify == (SIGEV_SIGNAL | SIGEV_THREAD_ID)) { \
    d._sigev_un._tid = BE32(s._sigev_un._tid);	\
  }						\
  else if (d.sigev_notify == SIGEV_THREAD) {	\
    d.sigev_notify_function = (void*) (uintptr_t) BE32(s.sigev_notify_function); \
    d.sigev_notify_attributes = (void*) (uintptr_t) BE32(s.sigev_notify_attributes); \
  }

#define COPY_dirent(s,d) \
  d.d_ino = BE64(s.d_ino);			\
  d.d_off = BE64(s.d_off);			\
  d.d_reclen = BE16(s.d_reclen);

#define COPY_sched_param(s,d) \
  d.sched_priority = BE32(s.sched_priority)

#define COPY_mq_attr(s,d) \
  d.mq_flags = BE32(s.mq_flags);		\
  d.mq_maxmsg = BE32(s.mq_maxmsg);		\
  d.mq_msgsize = BE32(s.mq_msgsize);		\
  d.mq_curmsgs = BE32(s.mq_curmsgs)

#define COPY_timespec(s,d) \
  d.tv_sec = BE32(s.tv_sec);			\
  d.tv_nsec = BE32(s.tv_nsec)

#define COPY_timeval(s,d) \
  d.tv_sec = BE32(s.tv_sec);			\
  d.tv_usec = BE32(s.tv_usec)

#define COPY_timeval2(s,d) \
  COPY_timeval((s.timeval[0]), (d.timeval[0]));	\
  COPY_timeval((s.timeval[1]), (d.timeval[1]));

#define COPY_itimerspec(s,d) \
  COPY_timespec((s.it_interval), (d.it_interval)); \
  COPY_timespec((s.it_value), (d.it_interval));

#define COPY_itimerval(s,d) \
  COPY_timeval((s.it_interval), (d.it_interval)); \
  COPY_timeval((s.it_value), (d.it_interval));

#define COPY_tms(s,d) \
  d.tms_utime = BE32(s.tms_utime);		\
  d.tms_stime = BE32(s.tms_stime);		\
  d.tms_cutime = BE32(s.tms_cutime);		\
  d.tms_cstime = BE32(s.tms_cstime);

#define COPY_utimbuf(s,d) \
  d.actime = BE32(s.actime);			\
  d.modtime = BE32(s.modtime);

// FIXME: use UQUAD for si_addr?
#define COPY_siginfo(s,d) \
  d.si_signo = BE32(s.si_signo);		\
  d.si_errno = BE32(s.si_errno);		\
  d.si_code = BE32(s.si_code);			\
  switch (d.si_signo) {				\
    case SIGILL: case SIGFPE:			\
    case SIGSEGV: case SIGBUS:			\
      d._sifields._sigfault.si_addr = BE32((uae_u32) (uintptr_t) s._sifields._sigfault.si_addr); \
      break;					\
    default: {					\
      size_t i;					\
      for (i = 0; i < 6; ++i) { 		\
	d._sifields._pad[i] = BE32(d._sifields._pad[i]);	\
      }						\
      break;					\
    }						\
  }

#define COPY_sysinfo(s,d) \
  d.uptime = BE32(s.uptime);			\
  d.loads[0] = BE32(s.loads[0]);		\
  d.loads[1] = BE32(s.loads[1]);		\
  d.loads[2] = BE32(s.loads[2]);		\
  d.totalram = BE64(s.totalram);		\
  d.freeram = BE64(s.freeram);			\
  d.sharedram = BE64(s.sharedram);		\
  d.bufferram = BE64(s.bufferram);		\
  d.totalswap = BE64(s.totalswap);		\
  d.freeswap = BE64(s.freeswap);		\
  d.totalhigh = BE64(s.totalhigh);		\
  d.freehigh = BE64(s.freehigh);		\
  d.procs = BE32(s.procs);			\
  d.__extra[0] = 0;				\
  d.__extra[1] = 0;				\
  d.__extra[2] = 0;


#define arp2_stat64 arp2_stat
#define COPY_stat64(s,d) \
  d.st_dev = BE64(s.st_dev);			\
  d.st_ino = BE64(s.st_ino);			\
  d.st_mode = BE32(s.st_mode);			\
  d.st_nlink = BE32(s.st_nlink);		\
  d.st_uid = BE32(s.st_uid);			\
  d.st_gid = BE32(s.st_gid);			\
  d.st_rdev = BE64(s.st_rdev);			\
  d.st_size = BE64(s.st_size);			\
  d.st_blocks = BE64(s.st_blocks);		\
  d.st_blksize = BE32(s.st_blksize);		\
  COPY_timespec((s.st_atim), (d.st_atim));	\
  COPY_timespec((s.st_mtim), (d.st_mtim));	\
  COPY_timespec((s.st_ctim), (d.st_ctim));

#define arp2_statfs64 arp2_statfs
#define COPY_statfs64(s,d) \
  d.f_type = BE32((uae_u32) s.f_type);		\
  d.f_bsize  = BE32((uae_u32) s.f_bsize);	\
  d.f_blocks = BE64(s.f_blocks);		\
  d.f_bfree = BE64(s.f_bfree);			\
  d.f_bavail = BE64(s.f_bavail);		\
  d.f_files = BE64(s.f_files);			\
  d.f_ffree = BE64(s.f_ffree);			\
  d.f_fsid[0] = BE32(s.f_fsid.__val[0]);	\
  d.f_fsid[1] = BE32(s.f_fsid.__val[1]);	\
  d.f_namelen = BE32((uae_u32) s.f_namelen);	\
  d.f_frsize = BE32((uae_u32) s.f_frsize);

#define arp2_statvfs64 arp2_statvfs
#define COPY_statvfs64(s,d) \
  d.f_bsize  = BE32((uae_u32) s.f_bsize);	\
  d.f_frsize = BE32((uae_u32) s.f_frsize);	\
  d.f_blocks = BE64(s.f_blocks);		\
  d.f_bfree = BE64(s.f_bfree);			\
  d.f_bavail = BE64(s.f_bavail);		\
  d.f_files = BE64(s.f_files);			\
  d.f_ffree = BE64(s.f_ffree);			\
  d.f_favail = BE64(s.f_favail);		\
  d.f_fsid = BE32((uae_u32) s.f_fsid);		\
  d.__f_unused = 0;				\
  d.f_flag = BE32((uae_u32) s.f_flag);		\
  d.f_namemax = BE32((uae_u32) s.f_namemax);
  
#define COPY_rusage(s,d) \
  COPY_timeval((s.ru_utime), (d.ru_utime));	\
  COPY_timeval((s.ru_stime), (d.ru_stime));	\
  d.ru_maxrss = BE32(s.ru_maxrss);		\
  d.ru_ixrss = BE32(s.ru_ixrss);		\
  d.ru_idrss = BE32(s.ru_idrss);		\
  d.ru_isrss = BE32(s.ru_isrss);		\
  d.ru_minflt = BE32(s.ru_minflt);		\
  d.ru_majflt = BE32(s.ru_majflt);		\
  d.ru_nswap = BE32(s.ru_nswap);		\
  d.ru_inblock = BE32(s.ru_inblock);		\
  d.ru_oublock = BE32(s.ru_oublock);		\
  d.ru_msgsnd = BE32(s.ru_msgsnd);		\
  d.ru_msgrcv = BE32(s.ru_msgrcv);		\
  d.ru_nsignals = BE32(s.ru_nsignals);		\
  d.ru_nvcsw = BE32(s.ru_nvcsw);		\
  d.ru_nivcsw = BE32(s.ru_nivcsw);

#define arp2_rlimit64 arp2_rlimit
#define COPY_rlimit64(s,d) \
  d.rlim_cur = BE64(s.rlim_cur);		\
  d.rlim_max = BE64(s.rlim_max);
  


#ifndef WORDS_BIGENDIAN
    // Binary compatibe
# define COPY_sigset(s,d) memcpy(&d, &s, sizeof (d));
# define COPY_cpu_set(s,d) memcpy(&d, &s, sizeof (d));
# define COPY_fd_set(s,d) memcpy(&d, &s, sizeof (d));
#else
# error sigset_t, cpu_set_t and fd_set unimplemented for big endian
#endif


#define GET(struct_name, src, name)					\
  struct struct_name name ## _struct, *name = NULL;			\
  if (src != NULL) {							\
    COPY_ ## struct_name((*src), (name ## _struct));			\
    name = & name ## _struct;						\
  }

#define PUT(struct_name, dst, name)					\
  if (dst != NULL) {							\
    COPY_ ## struct_name((name), (*dst));				\
  }

/* union arp2_sigval { */
/*     uae_s32 sival_int; */
/*     uae_u32 sival_ptr; */
/* }; */




/*** Threaded syscall support code *******************************************/

struct workload {
    struct node     node;
    struct regs     regs;
    blomcall_func*  func;
    struct process* owner;
    int             cancelled;
};

struct worker_thread {
    struct node node;
    pthread_t   thread;
};

static int worker_quit = 0;
static int worker_availcnt = 0;
static int worker_result_ready = 0;

static pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  worker_cond  = PTHREAD_COND_INITIALIZER;

static struct list worker_threads;

static struct list workload_waiting;
static struct list workload_processing;
static struct list workload_ready;
static struct list workload_done;


/** The worker thread function.
 *
 *  This function waits on worker_cond until there is a workload to
 *  process in the workload_waiting list. During processing, the
 *  workload will be put in the workload_processing list and once
 *  completed, it will be moved to the workload_ready list and the
 *  "INT6" signal will be asserted so arp2sys_hsync_handler() asserts
 *  EXTER (unless the workload has been cancelled, in which case the
 *  workload is moved directly to the workload_done list.
 *
 *  If worker_quit is set to true, this function will remove the
 *  worker_thread struct from the worker_threads list and exit.
 *
 *  @param arg A pointer to this thread's worker_thread struct.
 *
 *  @return Always NULL.
 */

static void* worker_entry(void* arg) {
  struct worker_thread* worker_thread = (struct worker_thread*) arg;

  pthread_mutex_lock(&worker_mutex);

  while (!worker_quit) {
    // Fetch a workload
    struct workload* workload = (struct workload*) list_remhead(&workload_waiting);

    if (workload == NULL) {
      pthread_cond_wait(&worker_cond, &worker_mutex);
    }
    else {
      // Execute workload
      list_addtail(&workload_processing, &workload->node);

      --worker_availcnt;
      pthread_mutex_unlock(&worker_mutex);

      workload->regs.d0 = workload->func((uae_u32*) &workload->regs, NULL);
      pthread_mutex_lock(&worker_mutex);
      ++worker_availcnt;

      if (workload->cancelled) {
	// Return workload to workload cache
	list_remove(&workload->node);
	list_addtail(&workload_done, &workload->node);
      }
      else {
	// Queue result
	list_remove(&workload->node);
	list_addtail(&workload_ready, &workload->node);

	// Make AmigaOS fetch it (see arp2sys_arp2_inthandler() and
	// arp2sys_hsync_handler() below)
	worker_result_ready = 1;
      }
    }
  }

  // Remove me from the worker thread list
  list_remove(&worker_thread->node);

  pthread_mutex_unlock(&worker_mutex);
  return NULL;
}


/** Add a workload to a free worker thread.
 *
 *  Queues a workload for processing. If no free worker thread is
 *  available, create a new one (workers can be busy forever).
 *
 *  @param regs The register array used for in- and output parameters.
 *
 *  @param func The actual function to be called by the worker thread.
 *
 *  @param owner A Task pointer used to identify the workload.
 */

static void workload_add(regptr regs, blomcall_func* func, struct process* owner) {
  struct workload* workload;

  pthread_mutex_lock(&worker_mutex);

  // Create a new or reuse an old workload struct
  workload = (struct workload*) list_remhead(&workload_done);

  if (workload == NULL) {
    workload = malloc(sizeof *workload);
  }

  // Set up workload and add it to the waiting list
  workload->regs      = *regs;
  workload->func      = func;
  workload->owner     = owner;
  workload->cancelled = 0;

  list_addtail(&workload_waiting, &workload->node);

  // Create a new worker thread if there are none ready
  if (worker_availcnt == 0) {
    struct worker_thread* worker_thread = malloc (sizeof *worker_thread);
    sigset_t usr1sigset;

    // Creating threads from a BJMP context is a bit tricky ...
    sigemptyset(&usr1sigset);
    sigaddset(&usr1sigset, SIGUSR1);

    pthread_sigmask(SIG_BLOCK, &usr1sigset, NULL);

    if (pthread_create(&worker_thread->thread, NULL, worker_entry, worker_thread) == 0) {
      list_addtail(&worker_threads, &worker_thread->node);
    }
    else {
      abort();
    }

    pthread_sigmask(SIG_UNBLOCK, &usr1sigset, NULL);

    ++worker_availcnt;
  }

  // Make one of the worker threads wake up
  pthread_cond_signal(&worker_cond);

  pthread_mutex_unlock(&worker_mutex);
}


/** Return a workload started by a specified process.
 *
 *  Unlinks a specified workload and returns the result. If there are
 *  more loads ready, send the first owner task a SIGF_SINGLE signal
 *  to wake it up.
 *
 *  @param regs Will be loaded with the workload result.
 *
 *  @param exec A copy of SysBase (for calling Signal()).
 *
 *  @param owner The Task pointer that identifies the wanted workload.
 *
 *  @result true of the workload was found, else 0.
 */

static int workload_get(regptr regs, struct execbase* exec, struct process* owner) {
  struct workload* workload = NULL;
  int              rc = 0;

  pthread_mutex_lock(&worker_mutex);

  LIST_FOR(&workload_ready, workload) {
    if (workload->owner == owner) {
      *regs = workload->regs;

      // Return workload to workload cache
      list_remove(&workload->node);
      list_addtail(&workload_done, &workload->node);
      rc = 1;
      break;
    }
  }

  // Wake up next ready task, if any
  if (!list_isempty(&workload_ready)) {
    set_signal(((struct workload*) workload_ready.head)->owner, exec);
  }

  pthread_mutex_unlock(&worker_mutex);

  return rc;
}


/** Execute a workload without appearing to busywait from AmigaOS'
 *  point of view.
 *
 *  Queues the workload to a free worker thread and goes to sleep (the
 *  AmigaOS way, using Wait()), waiting for a SIGF_SINGLE signal. Once
 *  woken up, fetch the result of the completed workload and return.
 *
 *  @param regs The register set to work on (contains both input and
 *  output parameters).
 *
 *  @param func A function that will be called by the worker thread.
 *
 *  @result A copy of regs->d0. pr_Result2 will be set too, if the
 *  caller was a Process.
 */

static uae_u32 workload_run(regptr regs, blomcall_func* func) {
  struct sysresbase* sysresbase = (struct sysresbase*) (uintptr_t) regs->a6;
  struct execbase*   exec = (struct execbase*) (uintptr_t) BE32(sysresbase->sysbase);
  struct process*    pr   = (struct process*) (uintptr_t) BE32(exec->thistask);

  clear_signal(exec);

  workload_add(regs, func, pr);
  
  do {
    wait_signal(exec);
  }
  while (!workload_get(regs, exec, pr));

  set_io_err(regs->a0, (struct sysresbase*) (uintptr_t) regs->a6);

  return regs->d0;
}


/** The native part of the interrupt handler.
 *
 *  Releases the "INT6" signal, which stops arp2sys_hsync_handler()
 *  from asserting EXTER.
 *
 *  arp2sys_arp2_inthandler is a "quick" call, since it's called from
 *  Exec's interrupt context, which has a rather tiny stack. This *
 *  implies that it must NOT make any calls back to the m68k *
 *  emulation.
 *
 *  @return A pointer to a Task that should be sent a SIGF_SINGLE
 *  signal, or NULL if this interrupt was not for us.
 */

uae_u32
arp2sys_arp2_inthandler(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_arp2_inthandler(regptr _regs)
{
  uae_u32 rc = 0;

  pthread_mutex_lock(&worker_mutex);

  if (worker_result_ready) {
    // Release "INT6" signal
    worker_result_ready = 0;

    // New workloads are added
    rc = (uae_u32) (uintptr_t) ((struct workload*) workload_ready.head)->owner;
  }

  pthread_mutex_unlock(&worker_mutex);

  return rc;
}


/**  An AmigaOS wrapper for workload_run().
  *
  *  Makes the functionality provied by workload_run available to
  *  AmigaOS programs. Useful for calling "dlopen'ed" objects or
  *  for taking advantage of multi-core processors.
  */

uae_u32
arp2sys_arp2_run(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_arp2_run(regptr _regs)
{
  regptr ___regs = (regptr) (uintptr_t) _regs->a0;
  aptr ___native_func = (aptr) (uintptr_t) _regs->a1;
  uae_u32 rc;

  struct regs regs;

  uae_u32* m68k   = (uae_u32*) ___regs;
  uae_u32* native = (uae_u32*) &regs;

  int i;

  for (i = 0; i < 16; ++i) {
    native[i] = BE32(m68k[i]);
  }

  rc = workload_run(&regs, ___native_func);

  for (i = 0; i < 16; ++i) {
    m68k[i] = BE32(native[i]);
  }

  return rc;
}


/*** Unimplemented syscall wrappers ******************************************/

uae_s32
unimplemented(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 unimplemented(regptr _regs)
{
  write_log("Unimplemented arp2-syscall vector called!\n");

  errno = ENOSYS;
  _regs->a0 = errno;
  return -1;
}


// Not supported yet
#define arp2sys_sigaction unimplemented
#define arp2sys_sigaltstack unimplemented
#define arp2sys_timer_create unimplemented
#define arp2sys_timer_delete unimplemented
#define arp2sys_timer_settime unimplemented
#define arp2sys_timer_gettime unimplemented
#define arp2sys_timer_getoverrun unimplemented

// Not supported yet
#define arp2sys_openat unimplemented
#define arp2sys_renameat unimplemented
#define arp2sys_faccessat unimplemented
#define arp2sys_fchownat unimplemented
#define arp2sys_linkat unimplemented
#define arp2sys_symlinkat unimplemented
#define arp2sys_readlinkat unimplemented
#define arp2sys_unlinkat unimplemented
#define arp2sys_fstatat unimplemented
#define arp2sys_fchmodat unimplemented
#define arp2sys_mkdirat unimplemented
#define arp2sys_mknodat unimplemented
#define arp2sys_mkfifoat unimplemented
#define arp2sys_futimesat unimplemented

// Not supported yet
#define arp2sys_sync_file_range unimplemented
#define arp2sys_vmsplice unimplemented
#define arp2sys_splice unimplemented
#define arp2sys_tee unimplemented
#define arp2sys_unshare unimplemented

// Not supported yet
#define arp2sys_poll unimplemented
#define arp2sys_ppoll unimplemented
#define arp2sys_epoll_ctl unimplemented
#define arp2sys_epoll_wait unimplemented


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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  uae_u32 rc = open(___pathname, ___flags, ___mode);
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_mq_close(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_close(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  uae_u32 rc = mq_close(___mqdes);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_mq_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = mq_unlink(___name);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_mq_notify(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mq_notify(regptr _regs)
{
  arp2_mqd_t ___mqdes = (arp2_mqd_t) _regs->d0;
  const struct arp2_sigevent* ___notification = (const struct arp2_sigevent*) (uintptr_t) _regs->a0;
  GET(sigevent, ___notification, notification);
  uae_u32 rc = mq_notify(___mqdes, notification);
  _regs->a0 = errno;
  return rc;
}

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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_clone(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_clone(regptr _regs)
{
  uae_s32 (*___fn) (aptr arg) = (uae_s32 (*)(aptr arg)) (uintptr_t) _regs->a0;
  aptr ___child_stack = (aptr) (uintptr_t) _regs->a1;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  aptr ___arg = (aptr) (uintptr_t) _regs->a2;
  arp2_pid_t* ___ptid = (arp2_pid_t*) (uintptr_t) _regs->d1;
  struct arp2_user_desc* ___tls = (struct arp2_user_desc*) (uintptr_t) _regs->d2;
  arp2_pid_t* ___ctid = (arp2_pid_t*) (uintptr_t) _regs->d3;
#warning Use clone() arguments ptid, tls, ctid
//  uae_u32 rc = clone(___fn, ___child_stack, ___flags, ___arg, &ptid, ___tls, &ctid);
  uae_u32 rc = clone(___fn, ___child_stack, ___flags, ___arg);

/*   if (___flags & CLONE_PARENT_SETTID) { */
/*     *___ptid = BE32(ptid); */
/*   } */
/*   if (___flags & CLONE_CHILD_SETTID) { */
/*     *___ctid = BE32(ctid); */
/*   } */

  _regs->a0 = errno;
  return rc;
}

/* uae_s32 */
/* arp2sys_unshare(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_unshare(regptr _regs) */
/* { */
/*   uae_s32 ___flags = (uae_s32) _regs->d0; */
/*   uae_u32 rc = unshare(___flags); */
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sched_getscheduler(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_getscheduler(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 rc = sched_getscheduler(___pid);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sched_yield(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_yield(regptr _regs)
{
  uae_u32 rc = sched_yield();
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sched_get_priority_max(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_get_priority_max(regptr _regs)
{
  uae_s32 ___algorithm = (uae_s32) _regs->d0;
  uae_u32 rc = sched_get_priority_max(___algorithm);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sched_get_priority_min(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_get_priority_min(regptr _regs)
{
  uae_s32 ___algorithm = (uae_s32) _regs->d0;
  uae_u32 rc = sched_get_priority_min(___algorithm);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sched_setaffinity(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_setaffinity(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 ___cpusetsize = (uae_u32) _regs->d1;
  const arp2_cpu_set_t* ___cpuset = (const arp2_cpu_set_t*) (uintptr_t) _regs->a0;
  uae_u32 rc;
  if (___cpusetsize == sizeof (arp2_cpu_set_t)) {
    GET(cpu_set, ___cpuset, cpuset);
    rc = sched_setaffinity(___pid, ___cpusetsize, (cpu_set_t*) cpuset);
  }
  else {
    rc = -1;
    errno = EINVAL;
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sched_getaffinity(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sched_getaffinity(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 ___cpusetsize = (uae_u32) _regs->d1;
  arp2_cpu_set_t* ___cpuset = (arp2_cpu_set_t*) (uintptr_t) _regs->a0;
  cpu_set_t cpuset;
  uae_u32 rc;
  if (___cpusetsize == sizeof (arp2_cpu_set_t)) {
    rc = sched_getaffinity(___pid, ___cpusetsize, &cpuset);
    PUT(cpu_set, ___cpuset, cpuset);
  }
  else {
    rc = -1;
    errno = EINVAL;
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_kill(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_kill(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32 ___sig = (uae_s32) _regs->d1;
  uae_u32 rc = kill(___pid, ___sig);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_raise(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_raise(regptr _regs)
{
  uae_s32 ___sig = (uae_s32) _regs->d0;
  uae_u32 rc = raise(___sig);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sigprocmask(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigprocmask(regptr _regs)
{
  uae_s32 ___how = (uae_s32) _regs->d0;
  const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0;
  arp2_sigset_t* ___oldset = (arp2_sigset_t*) (uintptr_t) _regs->a1;
  GET(sigset, ___set, set);
  struct sigset oldset;
  uae_u32 rc = sigprocmask(___how, (sigset_t*) set, (sigset_t*) &oldset);
  PUT(sigset, ___oldset, oldset);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sigsuspend(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigsuspend(regptr _regs)
{
  const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0;
  GET(sigset, ___set, set);
  uae_u32 rc = sigsuspend((sigset_t*) set);
  _regs->a0 = errno;
  return rc;
}

/* uae_s32 */
/* arp2sys_sigaction(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_sigaction(regptr _regs) */
/* { */
/*   uae_s32 ___sig = (uae_s32) _regs->d0; */
/*   const struct arp2_sigaction* ___act = (const struct arp2_sigaction*) (uintptr_t) _regs->a0; */
/*   struct arp2_sigaction* ___oldact = (struct arp2_sigaction*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = sigaction(___sig, ___act, ___oldact); */
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_sigpending(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigpending(regptr _regs)
{
  arp2_sigset_t* ___set = (arp2_sigset_t*) (uintptr_t) _regs->a0;
  struct sigset set;
  uae_u32 rc = sigpending((sigset_t*) &set);
  PUT(sigset, ___set, set);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sigwait(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigwait(regptr _regs)
{
  const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0;
  uae_s32* ___sig = (uae_s32*) (uintptr_t) (uintptr_t) _regs->d0;
  GET(sigset, ___set, set);
  int sig;
  uae_u32 rc = sigwait((sigset_t*) set, &sig);
  if (___sig != NULL) {
    *___sig = BE32(sig);
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sigwaitinfo(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigwaitinfo(regptr _regs)
{
  const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0;
  arp2_siginfo_t* ___info = (arp2_siginfo_t*) (uintptr_t) _regs->a1;
  GET(sigset, ___set, set);
  struct siginfo info;
  uae_u32 rc = sigwaitinfo((sigset_t*) set, &info);
  PUT(siginfo, ___info, info)
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sigtimedwait(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sigtimedwait(regptr _regs)
{
  const arp2_sigset_t* ___set = (const arp2_sigset_t*) (uintptr_t) _regs->a0;
  arp2_siginfo_t* ___info = (arp2_siginfo_t*) (uintptr_t) _regs->a1;
  const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a2;
  GET(sigset, ___set, set);
  GET(timespec, ___timeout, timeout);
  struct siginfo info;
  uae_u32 rc = sigtimedwait((sigset_t*) set, &info, timeout);
  PUT(siginfo, ___info, info)
  _regs->a0 = errno;
  return rc;
}

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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

arp2_clock_t
arp2sys_clock(regptr _regs) REGPARAM;

arp2_clock_t
REGPARAM2 arp2sys_clock(regptr _regs)
{
  uae_u32 rc = clock();
  _regs->a0 = errno;
  return rc;
}

arp2_time_t
arp2sys_time(regptr _regs) REGPARAM;

arp2_time_t
REGPARAM2 arp2sys_time(regptr _regs)
{
  arp2_time_t* ___time = (arp2_time_t*) (uintptr_t) _regs->a0;
  arp2_time_t rc = time(NULL);
  if (___time != NULL) {
    *___time = BE32(rc);
  }
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  if (___clock_id != NULL) {
    *___clock_id = BE32(clock_id);
  }
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

/* uae_s32 */
/* arp2sys_timer_delete(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_delete(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   uae_u32 rc = timer_delete(___timerid); */
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

/* uae_s32 */
/* arp2sys_timer_getoverrun(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_timer_getoverrun(regptr _regs) */
/* { */
/*   arp2_timer_t ___timerid = (arp2_timer_t) _regs->a0; */
/*   uae_u32 rc = timer_getoverrun(___timerid); */
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

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
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_lseek(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lseek(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_s64 ___offset = ((uae_s64) _regs->d1 << 32) | ((uae_u32) _regs->d2);
  uae_s32 ___whence = (uae_s32) _regs->d3;
  uae_s64 rc = lseek(___fd, ___offset, ___whence);
  _regs->a0 = errno;
  _regs->d1 = rc >> 32;
  return (uae_s32) rc;
}

uae_s32
arp2sys_close(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_close(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = close(___fd);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_u32
arp2sys_alarm(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_alarm(regptr _regs)
{
  uae_u32 ___seconds = (uae_u32) _regs->d0;
  uae_u32 rc = alarm(___seconds);
  _regs->a0 = errno;
  return rc;
}

uae_u32
arp2sys_sleep(regptr _regs) REGPARAM;

uae_u32
REGPARAM2 arp2sys_sleep(regptr _regs)
{
  uae_u32 ___seconds = (uae_u32) _regs->d0;
  uae_u32 rc = sleep(___seconds);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_pause(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pause(regptr _regs)
{
  uae_u32 rc = pause();
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_chdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = chdir(___path);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_fchdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fchdir(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = fchdir(___fd);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_dup(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_dup(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = dup(___fd);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_execve(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_execve(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 const* ___argv = (uae_u32 const*) (uintptr_t) _regs->a1;
  uae_u32 const* ___envp = (uae_u32 const*) (uintptr_t) _regs->a2;

  int na, ne;
  for (na = 0; ___argv[na] != 0; ++na);
  for (ne = 0; ___envp[ne] != 0; ++ne);
  
  char* argv[na+1];
  char* envp[ne+1];

  while (na-- >= 0) argv[na] = (char*) (uintptr_t) BE32(___argv[na]);
  while (ne-- >= 0) envp[ne] = (char*) (uintptr_t) BE32(___envp[ne]);


  uae_u32 rc = execve(___path, argv, envp);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_execv(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_execv(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 const* ___argv = (uae_u32 const*) (uintptr_t) _regs->a1;

  int na;
  for (na = 0; ___argv[na] != 0; ++na);
  char* argv[na+1];
  while (na-- >= 0) argv[na] = (char*) (uintptr_t) BE32(___argv[na]);

  uae_u32 rc = execv(___path, argv);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_execvp(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_execvp(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 const* ___argv = (uae_u32 const*) (uintptr_t) _regs->a1;

  int na;
  for (na = 0; ___argv[na] != 0; ++na);
  char* argv[na+1];
  while (na-- >= 0) argv[na] = (char*) (uintptr_t) BE32(___argv[na]);

  uae_u32 rc = execvp(___file, argv);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_nice(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_nice(regptr _regs)
{
  uae_s32 ___inc = (uae_s32) _regs->d0;
  uae_u32 rc = nice(___inc);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sysconf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sysconf(regptr _regs)
{
  uae_s32 ___name = (uae_s32) _regs->d0;
  uae_u32 rc = sysconf(___name);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_getpid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpid(regptr _regs)
{
  uae_u32 rc = getpid();
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_getppid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getppid(regptr _regs)
{
  uae_u32 rc = getppid();
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_getpgrp(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpgrp(regptr _regs)
{
  uae_u32 rc = getpgrp();
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_getpgid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getpgid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 rc = getpgid(___pid);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setpgrp(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setpgrp(regptr _regs)
{
  uae_u32 rc = setpgrp();
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_setsid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_setsid(regptr _regs)
{
  uae_u32 rc = setsid();
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_getsid(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_getsid(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_u32 rc = getsid(___pid);
  _regs->a0 = errno;
  return rc;
}

arp2_uid_t
arp2sys_getuid(regptr _regs) REGPARAM;

arp2_uid_t
REGPARAM2 arp2sys_getuid(regptr _regs)
{
  uae_u32 rc = getuid();
  _regs->a0 = errno;
  return rc;
}

arp2_uid_t
arp2sys_geteuid(regptr _regs) REGPARAM;

arp2_uid_t
REGPARAM2 arp2sys_geteuid(regptr _regs)
{
  uae_u32 rc = geteuid();
  _regs->a0 = errno;
  return rc;
}

arp2_gid_t
arp2sys_getgid(regptr _regs) REGPARAM;

arp2_gid_t
REGPARAM2 arp2sys_getgid(regptr _regs)
{
  uae_u32 rc = getgid();
  _regs->a0 = errno;
  return rc;
}

arp2_gid_t
arp2sys_getegid(regptr _regs) REGPARAM;

arp2_gid_t
REGPARAM2 arp2sys_getegid(regptr _regs)
{
  uae_u32 rc = getegid();
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getgroups(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getgroups(regptr _regs)
{
  uae_s32 ___size = (uae_s32) _regs->d0;
  arp2_gid_t* ___list = (arp2_gid_t*) (uintptr_t) _regs->a0;
  gid_t list[___size];
  uae_u32 rc = getgroups(___size, list);
  if (___list != NULL) {
    int i;
    for (i = 0; i < ___size; ++i) {
      ___list[i] = BE32(list[i]);
    }
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setgroups(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setgroups(regptr _regs)
{
  uae_u32 ___n = (uae_u32) _regs->d0;
  const arp2_gid_t* ___groups = (const arp2_gid_t*) (uintptr_t) _regs->a0;
  gid_t groups[___n];
  if (___groups != NULL) {
    size_t i;
    for (i = 0; i < ___n; ++i) {
      groups[i] = BE32(___groups[i]);
    }
  }
  uae_u32 rc = setgroups(___n, groups);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_group_member(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_group_member(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = group_member(___gid);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  uae_u32 rc = setuid(___uid);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_seteuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_seteuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  uae_u32 rc = seteuid(___uid);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setgid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = setgid(___gid);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setegid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setegid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = setegid(___gid);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_fork(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_fork(regptr _regs)
{
  uae_u32 rc = fork();
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_vfork(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_vfork(regptr _regs)
{
  uae_u32 rc = vfork();
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_isatty(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_isatty(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = isatty(___fd);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = unlink(___name);
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_rmdir(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_rmdir(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = rmdir(___path);
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_tcgetpgrp(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_tcgetpgrp(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = tcgetpgrp(___fd);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setlogin(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setlogin(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = setlogin(___name);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sethostid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sethostid(regptr _regs)
{
  uae_s32 ___id = (uae_s32) _regs->d0;
  uae_u32 rc = sethostid(___id);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_vhangup(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_vhangup(regptr _regs)
{
  uae_u32 rc = vhangup();
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_revoke(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_revoke(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = revoke(___file);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_acct(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_acct(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = acct(___name);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_chroot(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chroot(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = chroot(___path);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_fsync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fsync(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  uae_u32 rc = fsync(___fd);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_gethostid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_gethostid(regptr _regs)
{
  uae_u32 rc = gethostid();
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getdtablesize(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getdtablesize(regptr _regs)
{
  uae_u32 rc = getdtablesize();
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_brk(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_brk(regptr _regs)
{
  aptr ___end_data_segment = (aptr) (uintptr_t) _regs->a0;
  uae_u32 rc = brk(___end_data_segment);
  _regs->a0 = errno;
  return rc;
}

aptr32
arp2sys_sbrk(regptr _regs) REGPARAM;

aptr32
REGPARAM2 arp2sys_sbrk(regptr _regs)
{
  uae_s32 ___increment = (uae_s32) _regs->d0;
  aptr rc = sbrk(___increment);
  if (rc != (aptr) -1 && !IS_32BIT(rc)) {
    errno = ENOMEM;
    rc = (aptr) -1;
  }
  _regs->a0 = errno;
  return (aptr32) (uintptr_t) rc;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_fdatasync(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fdatasync(regptr _regs)
{
  uae_s32 ___fildes = (uae_s32) _regs->d0;
  uae_u32 rc = fdatasync(___fildes);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getdents(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getdents(regptr _regs)
{
  uae_u32 ___fd = (uae_u32) _regs->d0;
  struct arp2_dirent* ___dirp = (struct arp2_dirent*) (uintptr_t) _regs->a0;
  uae_u32 ___count = (uae_u32) _regs->d1;
  uae_u32 rc = syscall(__NR_getdents64, (int) ___fd, (void*) ___dirp, (size_t) ___count);
  if (rc > 0) {
    uae_u32 i;
    for (i = 0; i < rc; /* not here */) {
      struct dirent64* dirp = (struct dirent64*) ((uae_u8*) ___dirp + i);
      COPY_dirent((*dirp), (*dirp));
      i += dirp->d_reclen;
    }
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_utime(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_utime(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  const struct arp2_utimbuf* ___file_times = (const struct arp2_utimbuf*) (uintptr_t) _regs->a1;

  GET(utimbuf, ___file_times, file_times);
  uae_u32 rc = utime(___file, file_times);
  _regs->a0 = errno;
  return rc;
}

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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_epoll_create(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_epoll_create(regptr _regs)
{
  uae_s32 ___size = (uae_s32) _regs->d0;
  uae_u32 rc = epoll_create(___size);
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setfsuid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setfsuid(regptr _regs)
{
  arp2_uid_t ___uid = (arp2_uid_t) _regs->d0;
  uae_u32 rc = setfsuid(___uid);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setfsgid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setfsgid(regptr _regs)
{
  arp2_gid_t ___gid = (arp2_gid_t) _regs->d0;
  uae_u32 rc = setfsgid(___gid);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_iopl(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_iopl(regptr _regs)
{
  uae_s32 ___level = (uae_s32) _regs->d0;
  uae_u32 rc = iopl(___level);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

aptr32
arp2sys_mmap(regptr _regs) REGPARAM;

aptr32
REGPARAM2 arp2sys_mmap(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_s32 ___prot = (uae_s32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  uae_s32 ___fd = (uae_s32) _regs->d3;
  uae_u64 ___offset = ((uae_u64) _regs->d4 << 32) | ((uae_u32) _regs->d5);
#ifdef MAP_32BIT
  // Place mapping in 32-bit memory
  ___flags |= MAP_32BIT;
#endif
  aptr rc = mmap64(___start, ___length, ___prot, ___flags, ___fd, ___offset);
  if (rc != MAP_FAILED && !IS_32BIT(rc)) {
    munmap(rc, ___length);
    errno = ENOMEM;
    rc = MAP_FAILED;
  }
  _regs->a0 = errno;
  return (aptr32) (uintptr_t) rc;
}

uae_s32
arp2sys_munmap(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munmap(regptr _regs)
{
  aptr ___start = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___length = (uae_u32) _regs->d0;
  uae_u32 rc = munmap(___start, ___length);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_mlockall(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_mlockall(regptr _regs)
{
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 rc = mlockall(___flags);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_munlockall(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_munlockall(regptr _regs)
{
  uae_u32 rc = munlockall();
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

aptr32
arp2sys_mremap(regptr _regs) REGPARAM;

aptr32
REGPARAM2 arp2sys_mremap(regptr _regs)
{
  aptr ___old_address = (aptr) (uintptr_t) _regs->a0;
  uae_u32 ___old_size = (uae_u32) _regs->d0;
  uae_u32 ___new_size = (uae_u32) _regs->d1;
  uae_s32 ___flags = (uae_s32) _regs->d2;
  aptr rc = mremap(___old_address, ___old_size, ___new_size, ___flags);
  if (rc != MAP_FAILED && !IS_32BIT(rc)) {
    munmap(rc, ___new_size);
    errno = ENOMEM;
    rc = MAP_FAILED;
  }
  _regs->a0 = errno;
  return (aptr32) (uintptr_t) rc;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_shm_unlink(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_shm_unlink(regptr _regs)
{
  const_strptr ___name = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = shm_unlink(___name);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_umount(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_umount(regptr _regs)
{
  const_strptr ___special_file = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = umount(___special_file);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getrlimit(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getrlimit(regptr _regs)
{
  arp2_rlimit_resource_t ___resource = (arp2_rlimit_resource_t) _regs->d0;
  struct arp2_rlimit* ___rlimits = (struct arp2_rlimit*) (uintptr_t) _regs->a0;
  struct rlimit64 rlimits;
  uae_u32 rc = getrlimit64(___resource, &rlimits);
  PUT(rlimit64, ___rlimits, rlimits);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setrlimit(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setrlimit(regptr _regs)
{
  arp2_rlimit_resource_t ___resource = (arp2_rlimit_resource_t) _regs->d0;
  const struct arp2_rlimit* ___rlimits = (const struct arp2_rlimit*) (uintptr_t) _regs->a0;
  GET(rlimit64, ___rlimits, rlimits);
  uae_u32 rc = setrlimit64(___resource, rlimits);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getrusage(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getrusage(regptr _regs)
{
  arp2_rusage_who_t ___who = (arp2_rusage_who_t) _regs->d0;
  struct arp2_rusage* ___usage = (struct arp2_rusage*) (uintptr_t) _regs->a0;
  struct rusage usage;
  uae_u32 rc = getrusage(___who, &usage);
  PUT(rusage, ___usage, usage);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getpriority(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getpriority(regptr _regs)
{
  arp2_priority_which_t ___which = (arp2_priority_which_t) _regs->d0;
  arp2_id_t ___who = (arp2_id_t) _regs->d1;
  uae_u32 rc = getpriority(___which, ___who);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_select(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_select(regptr _regs)
{
  uae_s32 ___nfds = (uae_s32) _regs->d0;
  arp2_fd_set* ___readfds = (arp2_fd_set*) (uintptr_t) _regs->a0;
  arp2_fd_set* ___writefds = (arp2_fd_set*) (uintptr_t) _regs->a1;
  arp2_fd_set* ___exceptfds = (arp2_fd_set*) (uintptr_t) _regs->a2;
  struct arp2_timeval* ___timeout = (struct arp2_timeval*) (uintptr_t) _regs->a3;
  GET(timeval, ___timeout, timeout);

#ifdef WORDS_BIGENDIAN
# error Fix this casting crap
#endif

  uae_u32 rc = select(___nfds, (fd_set*) ___readfds, (fd_set*) ___writefds, (fd_set*)___exceptfds,
		      timeout);
  if (___timeout != NULL) {
    COPY_timeval((*timeout), (*___timeout));
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_pselect(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_pselect(regptr _regs)
{
  uae_s32 ___nfds = (uae_s32) _regs->d0;
  arp2_fd_set* ___readfds = (arp2_fd_set*) (uintptr_t) _regs->a0;
  arp2_fd_set* ___writefds = (arp2_fd_set*) (uintptr_t) _regs->a1;
  arp2_fd_set* ___exceptfds = (arp2_fd_set*) (uintptr_t) _regs->a2;
  const struct arp2_timespec* ___timeout = (const struct arp2_timespec*) (uintptr_t) _regs->a3;
  const arp2_sigset_t* ___sigmask = (const arp2_sigset_t*) (uintptr_t) _regs->a4;
  GET(timespec, ___timeout, timeout);

#ifdef WORDS_BIGENDIAN
# error Fix this casting crap
#endif

  uae_u32 rc = pselect(___nfds, (fd_set*) ___readfds, (fd_set*) ___writefds, 
		       (fd_set*) ___exceptfds, timeout, (sigset_t*) ___sigmask);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_stat(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_stat(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a1;
  struct stat64 buf;
  uae_u32 rc = stat64(___path, &buf);
  PUT(stat64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_fstat(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fstat(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a0;
  struct stat64 buf;
  uae_u32 rc = fstat64(___fd, &buf);
  PUT(stat64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

/* uae_s32 */
/* arp2sys_fstatat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_fstatat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a1; */
/*   uae_s32 ___flag = (uae_s32) _regs->d1; */
/*   uae_u32 rc = fstatat64(___fd, ___file, ___buf, ___flag); */
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_lstat(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lstat(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  struct arp2_stat* ___buf = (struct arp2_stat*) (uintptr_t) _regs->a1;
  struct stat64 buf;
  uae_u32 rc = lstat64(___path, &buf);
  PUT(stat64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_chmod(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_chmod(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  arp2_mode_t ___mode = (arp2_mode_t) _regs->d0;
  uae_u32 rc = chmod(___file, ___mode);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

arp2_mode_t
arp2sys_umask(regptr _regs) REGPARAM;

arp2_mode_t
REGPARAM2 arp2sys_umask(regptr _regs)
{
  arp2_mode_t ___mask = (arp2_mode_t) _regs->d0;
  uae_u32 rc = umask(___mask);
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
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
  _regs->a0 = errno;
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
/*   _regs->a0 = errno;
  return rc;
} */

uae_s32
arp2sys_statfs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_statfs(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  struct arp2_statfs* ___buf = (struct arp2_statfs*) (uintptr_t) _regs->a1;
  struct statfs64 buf;
  uae_u32 rc = statfs64(___file, &buf);
  PUT(statfs64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_fstatfs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fstatfs(regptr _regs)
{
  uae_s32 ___fildes = (uae_s32) _regs->d0;
  struct arp2_statfs* ___buf = (struct arp2_statfs*) (uintptr_t) _regs->a0;
  struct statfs64 buf;
  uae_u32 rc = fstatfs64(___fildes, &buf);
  PUT(statfs64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_statvfs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_statvfs(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  struct arp2_statvfs* ___buf = (struct arp2_statvfs*) (uintptr_t) _regs->a1;
  struct statvfs64 buf;
  uae_u32 rc = statvfs64(___file, &buf);
  PUT(statvfs64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_fstatvfs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_fstatvfs(regptr _regs)
{
  uae_s32 ___fildes = (uae_s32) _regs->d0;
  struct arp2_statvfs* ___buf = (struct arp2_statvfs*) (uintptr_t) _regs->a0;
  struct statvfs64 buf;
  uae_u32 rc = fstatvfs64(___fildes, &buf);
  PUT(statvfs64, ___buf, buf);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_swapon(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_swapon(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_s32 ___flags = (uae_s32) _regs->d0;
  uae_u32 rc = swapon(___path, ___flags);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_swapoff(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_swapoff(regptr _regs)
{
  const_strptr ___path = (const_strptr) (uintptr_t) _regs->a0;
  uae_u32 rc = swapoff(___path);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_sysinfo(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_sysinfo(regptr _regs)
{
  struct arp2_sysinfo* ___info = (struct arp2_sysinfo*) (uintptr_t) _regs->a0;
  struct sysinfo info;
  uae_u32 rc = sysinfo(&info);
  PUT(sysinfo, ___info, info);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_get_nprocs_conf(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_nprocs_conf(regptr _regs)
{
  uae_u32 rc = get_nprocs_conf();
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_get_nprocs(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_nprocs(regptr _regs)
{
  uae_u32 rc = get_nprocs();
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_get_phys_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_phys_pages(regptr _regs)
{
  uae_u32 rc = get_phys_pages();
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_get_avphys_pages(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_get_avphys_pages(regptr _regs)
{
  uae_u32 rc = get_avphys_pages();
  _regs->a0 = errno;
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
  _regs->a0 = errno;
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
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_adjtime(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_adjtime(regptr _regs)
{
  const struct arp2_timeval* ___delta = (const struct arp2_timeval*) (uintptr_t) _regs->a0;
  struct arp2_timeval* ___olddelta = (struct arp2_timeval*) (uintptr_t) _regs->a1;

  GET(timeval, ___delta, delta);
  struct timeval olddelta;
  uae_u32 rc = adjtime(delta, &olddelta);
  PUT(timeval, ___olddelta, olddelta);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_getitimer(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_getitimer(regptr _regs)
{
  arp2_itimer_which_t ___which = (arp2_itimer_which_t) _regs->d0;
  struct arp2_itimerval* ___value = (struct arp2_itimerval*) (uintptr_t) _regs->a0;

  struct itimerval value;
  uae_u32 rc = getitimer(___which, &value);
  PUT(itimerval, ___value, value);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_setitimer(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_setitimer(regptr _regs)
{
  arp2_itimer_which_t ___which = (arp2_itimer_which_t) _regs->d0;
  const struct arp2_itimerval* ___new = (const struct arp2_itimerval*) (uintptr_t) _regs->a0;
  struct arp2_itimerval* ___old = (struct arp2_itimerval*) (uintptr_t) _regs->a1;

  GET(itimerval, ___new, new);
  struct itimerval old;
  uae_u32 rc = setitimer(___which, new, &old);
  PUT(itimerval, ___old, old);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_utimes(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_utimes(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  const struct arp2_timeval2* ___tvp = (const struct arp2_timeval2*) (uintptr_t) _regs->a1;
  
  GET(timeval2, ___tvp, tvp);
  uae_u32 rc = utimes(___file, tvp->timeval);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_lutimes(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_lutimes(regptr _regs)
{
  const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0;
  const struct arp2_timeval2* ___tvp = (const struct arp2_timeval2*) (uintptr_t) _regs->a1;

  GET(timeval2, ___tvp, tvp);
  uae_u32 rc = lutimes(___file, tvp->timeval);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_futimes(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_futimes(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const struct arp2_timeval2* ___tvp = (const struct arp2_timeval2*) (uintptr_t) _regs->a0;

  GET(timeval2, ___tvp, tvp);
  uae_u32 rc = futimes(___fd, tvp->timeval);
  _regs->a0 = errno;
  return rc;
}

/* uae_s32 */
/* arp2sys_futimesat(regptr _regs) REGPARAM; */

/* uae_s32 */
/* REGPARAM2 arp2sys_futimesat(regptr _regs) */
/* { */
/*   uae_s32 ___fd = (uae_s32) _regs->d0; */
/*   const_strptr ___file = (const_strptr) (uintptr_t) _regs->a0; */
/*   const struct arp2_timeval* ___tvp = (const struct arp2_timeval*) (uintptr_t) _regs->a1; */
/*   uae_u32 rc = futimesat(___fd, ___file, ___tvp); */
/*   _regs->a0 = errno;
  return rc;
} */

arp2_clock_t
arp2sys_times(regptr _regs) REGPARAM;

arp2_clock_t
REGPARAM2 arp2sys_times(regptr _regs)
{
  struct arp2_tms* ___buffer = (struct arp2_tms*) (uintptr_t) _regs->a0;
  
  GET(tms, ___buffer, buffer);
  uae_u32 rc = times(buffer);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_readv(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_readv(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const struct arp2_iovec* ___iovec = (const struct arp2_iovec*) (uintptr_t) _regs->a0;
  uae_s32 ___count = (uae_s32) _regs->d1;

  int nv;
  struct iovec iovec[___count];
  for (nv = 0; nv < ___count; ++nv) {
    iovec[nv].iov_base = (void*) (uintptr_t) BE32(___iovec[nv].iov_base);
    iovec[nv].iov_len = BE32(___iovec[nv].iov_len);
  }

  uae_u32 rc = readv(___fd, iovec, ___count);
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_writev(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_writev(regptr _regs)
{
  uae_s32 ___fd = (uae_s32) _regs->d0;
  const struct arp2_iovec* ___iovec = (const struct arp2_iovec*) (uintptr_t) _regs->a0;
  uae_s32 ___count = (uae_s32) _regs->d1;

  int nv;
  struct iovec iovec[___count];
  for (nv = 0; nv < ___count; ++nv) {
    iovec[nv].iov_base = (void*) (uintptr_t) BE32(___iovec[nv].iov_base);
    iovec[nv].iov_len = BE32(___iovec[nv].iov_len);
  }

  uae_u32 rc = writev(___fd, iovec, ___count);
  _regs->a0 = errno;
  return rc;
}

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
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_wait(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_wait(regptr _regs)
{
  uae_s32* ___stat_loc = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0;
  int stat_loc;
  uae_u32 rc = wait(&stat_loc);
  if (___stat_loc != NULL) {
    *___stat_loc = BE32(stat_loc);
  }
  _regs->a0 = errno;
  return rc;
}

uae_s32
arp2sys_waitid(regptr _regs) REGPARAM;

uae_s32
REGPARAM2 arp2sys_waitid(regptr _regs)
{
  arp2_idtype_t ___idtype = (arp2_idtype_t) _regs->d0;
  arp2_id_t ___id = (arp2_id_t) _regs->d1;
  arp2_siginfo_t* ___infop = (arp2_siginfo_t*) (uintptr_t) _regs->a0;
  uae_s32 ___options = (uae_s32) _regs->d2;
  struct siginfo infop;
  uae_u32 rc = waitid(___idtype, ___id, &infop, ___options);
  PUT(siginfo, ___infop, infop);
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_wait3(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_wait3(regptr _regs)
{
  uae_s32* ___stat_loc = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0;
  uae_s32 ___options = (uae_s32) _regs->d0;
  struct arp2_rusage* ___usage = (struct arp2_rusage*) (uintptr_t) _regs->a1;
  int stat_loc;
  struct rusage usage;
  uae_u32 rc = wait3(&stat_loc, ___options, &usage);
  PUT(rusage, ___usage, usage);
  if (___stat_loc != NULL) {
    *___stat_loc = BE32(stat_loc);
  }
  _regs->a0 = errno;
  return rc;
}

arp2_pid_t
arp2sys_wait4(regptr _regs) REGPARAM;

arp2_pid_t
REGPARAM2 arp2sys_wait4(regptr _regs)
{
  arp2_pid_t ___pid = (arp2_pid_t) _regs->d0;
  uae_s32* ___stat_loc = (uae_s32*) (uintptr_t) (uintptr_t) _regs->a0;
  uae_s32 ___options = (uae_s32) _regs->d1;
  struct arp2_rusage* ___usage = (struct arp2_rusage*) (uintptr_t) _regs->a1;
  int stat_loc;
  struct rusage usage;
  uae_u32 rc = wait4(___pid, &stat_loc, ___options, &usage);
  PUT(rusage, ___usage, usage);
  if (___stat_loc != NULL) {
    *___stat_loc = BE32(stat_loc);
  }
  _regs->a0 = errno;
  return rc;
}


/*** Async wrappers **********************************************************/

#define ASYNC_GW(name) \
  static uae_u32 async_ ## name(regptr regs) REGPARAM;				\
  static uae_u32 REGPARAM2 gw_ ## name(regptr regs) {				\
    return workload_run(regs, (blomcall_func*) name);	\
  }


#define DIRECT_GW(name) \
  static uae_u32 direct_ ## name(regptr regs) REGPARAM;				\
  static uae_u32 REGPARAM2 gw_ ## name(regptr regs) {				\
    uae_u32 rc = name(regs);							\
    set_io_err(regs->a0, (struct sysresbase*) (uintptr_t) regs->a6);		\
    return rc;									\
  }

// (The general idea is that function calls that might block should be
// executed by a worker thread.)

ASYNC_GW(arp2sys_readahead);
ASYNC_GW(arp2sys_sync_file_range);
ASYNC_GW(arp2sys_vmsplice);
ASYNC_GW(arp2sys_splice);
ASYNC_GW(arp2sys_tee);
ASYNC_GW(arp2sys_fcntl);
ASYNC_GW(arp2sys_open);
ASYNC_GW(arp2sys_openat);
ASYNC_GW(arp2sys_creat);
ASYNC_GW(arp2sys_posix_fadvise);
ASYNC_GW(arp2sys_posix_fallocate);
ASYNC_GW(arp2sys_mq_open);
ASYNC_GW(arp2sys_mq_close);
ASYNC_GW(arp2sys_mq_getattr);
ASYNC_GW(arp2sys_mq_setattr);
ASYNC_GW(arp2sys_mq_unlink);
ASYNC_GW(arp2sys_mq_notify);
ASYNC_GW(arp2sys_mq_receive);
ASYNC_GW(arp2sys_mq_send);
ASYNC_GW(arp2sys_mq_timedreceive);
ASYNC_GW(arp2sys_mq_timedsend);
ASYNC_GW(arp2sys_poll);
ASYNC_GW(arp2sys_ppoll);
ASYNC_GW(arp2sys_clone);
ASYNC_GW(arp2sys_unshare);
DIRECT_GW(arp2sys_sched_setparam);
DIRECT_GW(arp2sys_sched_getparam);
DIRECT_GW(arp2sys_sched_setscheduler);
DIRECT_GW(arp2sys_sched_getscheduler);
DIRECT_GW(arp2sys_sched_yield);
DIRECT_GW(arp2sys_sched_get_priority_max);
DIRECT_GW(arp2sys_sched_get_priority_min);
DIRECT_GW(arp2sys_sched_rr_get_interval);
DIRECT_GW(arp2sys_sched_setaffinity);
DIRECT_GW(arp2sys_sched_getaffinity);
DIRECT_GW(arp2sys_kill);
DIRECT_GW(arp2sys_killpg);
DIRECT_GW(arp2sys_raise);
DIRECT_GW(arp2sys_sigprocmask);
DIRECT_GW(arp2sys_sigsuspend);
DIRECT_GW(arp2sys_sigaction);
DIRECT_GW(arp2sys_sigpending);
DIRECT_GW(arp2sys_sigwait);
DIRECT_GW(arp2sys_sigwaitinfo);
DIRECT_GW(arp2sys_sigtimedwait);
DIRECT_GW(arp2sys_sigqueue);
DIRECT_GW(arp2sys_siginterrupt);
DIRECT_GW(arp2sys_sigaltstack);
ASYNC_GW(arp2sys_rename);
ASYNC_GW(arp2sys_renameat);
DIRECT_GW(arp2sys_clock);
DIRECT_GW(arp2sys_time);
ASYNC_GW(arp2sys_nanosleep);
DIRECT_GW(arp2sys_clock_getres);
DIRECT_GW(arp2sys_clock_gettime);
DIRECT_GW(arp2sys_clock_settime);
ASYNC_GW(arp2sys_clock_nanosleep);
DIRECT_GW(arp2sys_clock_getcpuclockid);
DIRECT_GW(arp2sys_timer_create);
DIRECT_GW(arp2sys_timer_delete);
DIRECT_GW(arp2sys_timer_settime);
DIRECT_GW(arp2sys_timer_gettime);
DIRECT_GW(arp2sys_timer_getoverrun);
ASYNC_GW(arp2sys_access);
ASYNC_GW(arp2sys_euidaccess);
ASYNC_GW(arp2sys_faccessat);
ASYNC_GW(arp2sys_lseek);
ASYNC_GW(arp2sys_close);
ASYNC_GW(arp2sys_read);
ASYNC_GW(arp2sys_write);
ASYNC_GW(arp2sys_pread);
ASYNC_GW(arp2sys_pwrite);
DIRECT_GW(arp2sys_pipe);
DIRECT_GW(arp2sys_alarm);
ASYNC_GW(arp2sys_sleep);
ASYNC_GW(arp2sys_pause);
ASYNC_GW(arp2sys_chown);
ASYNC_GW(arp2sys_fchown);
ASYNC_GW(arp2sys_lchown);
ASYNC_GW(arp2sys_fchownat);
ASYNC_GW(arp2sys_chdir);
ASYNC_GW(arp2sys_fchdir);
ASYNC_GW(arp2sys_getcwd);
ASYNC_GW(arp2sys_dup);
ASYNC_GW(arp2sys_dup2);
ASYNC_GW(arp2sys_execve);
ASYNC_GW(arp2sys_execv);
ASYNC_GW(arp2sys_execvp);
DIRECT_GW(arp2sys_nice);
DIRECT_GW(arp2sys_pathconf);
DIRECT_GW(arp2sys_fpathconf);
DIRECT_GW(arp2sys_sysconf);
DIRECT_GW(arp2sys_confstr);
DIRECT_GW(arp2sys_getpid);
DIRECT_GW(arp2sys_getppid);
DIRECT_GW(arp2sys_getpgrp);
DIRECT_GW(arp2sys_getpgid);
DIRECT_GW(arp2sys_setpgid);
DIRECT_GW(arp2sys_setpgrp);
DIRECT_GW(arp2sys_setsid);
DIRECT_GW(arp2sys_getsid);
DIRECT_GW(arp2sys_getuid);
DIRECT_GW(arp2sys_geteuid);
DIRECT_GW(arp2sys_getgid);
DIRECT_GW(arp2sys_getegid);
DIRECT_GW(arp2sys_getgroups);
DIRECT_GW(arp2sys_setgroups);
DIRECT_GW(arp2sys_group_member);
DIRECT_GW(arp2sys_setuid);
DIRECT_GW(arp2sys_setreuid);
DIRECT_GW(arp2sys_seteuid);
DIRECT_GW(arp2sys_setgid);
DIRECT_GW(arp2sys_setregid);
DIRECT_GW(arp2sys_setegid);
DIRECT_GW(arp2sys_getresuid);
DIRECT_GW(arp2sys_getresgid);
DIRECT_GW(arp2sys_setresuid);
DIRECT_GW(arp2sys_setresgid);
DIRECT_GW(arp2sys_fork);
DIRECT_GW(arp2sys_vfork);
DIRECT_GW(arp2sys_ttyname_r);
DIRECT_GW(arp2sys_isatty);
ASYNC_GW(arp2sys_link);
ASYNC_GW(arp2sys_linkat);
ASYNC_GW(arp2sys_symlink);
ASYNC_GW(arp2sys_readlink);
ASYNC_GW(arp2sys_symlinkat);
ASYNC_GW(arp2sys_readlinkat);
ASYNC_GW(arp2sys_unlink);
ASYNC_GW(arp2sys_unlinkat);
ASYNC_GW(arp2sys_rmdir);
DIRECT_GW(arp2sys_tcgetpgrp);
DIRECT_GW(arp2sys_tcsetpgrp);
DIRECT_GW(arp2sys_getlogin_r);
DIRECT_GW(arp2sys_setlogin);
DIRECT_GW(arp2sys_gethostname);
DIRECT_GW(arp2sys_sethostname);
DIRECT_GW(arp2sys_sethostid);
DIRECT_GW(arp2sys_getdomainname);
DIRECT_GW(arp2sys_setdomainname);
DIRECT_GW(arp2sys_vhangup);
DIRECT_GW(arp2sys_revoke);
DIRECT_GW(arp2sys_profil);
DIRECT_GW(arp2sys_acct);
DIRECT_GW(arp2sys_daemon);
ASYNC_GW(arp2sys_chroot);
ASYNC_GW(arp2sys_fsync);
DIRECT_GW(arp2sys_gethostid);
ASYNC_GW(arp2sys_sync);
DIRECT_GW(arp2sys_getpagesize);
DIRECT_GW(arp2sys_getdtablesize);
ASYNC_GW(arp2sys_truncate);
ASYNC_GW(arp2sys_ftruncate);
DIRECT_GW(arp2sys_brk);
DIRECT_GW(arp2sys_sbrk);
ASYNC_GW(arp2sys_lockf);
ASYNC_GW(arp2sys_fdatasync);
ASYNC_GW(arp2sys_getdents);
ASYNC_GW(arp2sys_utime);
ASYNC_GW(arp2sys_setxattr);
ASYNC_GW(arp2sys_lsetxattr);
ASYNC_GW(arp2sys_fsetxattr);
ASYNC_GW(arp2sys_getxattr);
ASYNC_GW(arp2sys_lgetxattr);
ASYNC_GW(arp2sys_fgetxattr);
ASYNC_GW(arp2sys_listxattr);
ASYNC_GW(arp2sys_llistxattr);
ASYNC_GW(arp2sys_flistxattr);
ASYNC_GW(arp2sys_removexattr);
ASYNC_GW(arp2sys_lremovexattr);
ASYNC_GW(arp2sys_fremovexattr);
ASYNC_GW(arp2sys_epoll_create);
ASYNC_GW(arp2sys_epoll_ctl);
ASYNC_GW(arp2sys_epoll_wait);
ASYNC_GW(arp2sys_flock);
DIRECT_GW(arp2sys_setfsuid);
DIRECT_GW(arp2sys_setfsgid);
DIRECT_GW(arp2sys_ioperm);
DIRECT_GW(arp2sys_iopl);
ASYNC_GW(arp2sys_ioctl);
ASYNC_GW(arp2sys_klogctl);
ASYNC_GW(arp2sys_mmap);
ASYNC_GW(arp2sys_munmap);
ASYNC_GW(arp2sys_mprotect);
ASYNC_GW(arp2sys_msync);
ASYNC_GW(arp2sys_madvise);
ASYNC_GW(arp2sys_posix_madvise);
ASYNC_GW(arp2sys_mlock);
ASYNC_GW(arp2sys_munlock);
ASYNC_GW(arp2sys_mlockall);
ASYNC_GW(arp2sys_munlockall);
ASYNC_GW(arp2sys_mincore);
ASYNC_GW(arp2sys_mremap);
ASYNC_GW(arp2sys_remap_file_pages);
ASYNC_GW(arp2sys_shm_open);
ASYNC_GW(arp2sys_shm_unlink);
ASYNC_GW(arp2sys_mount);
ASYNC_GW(arp2sys_umount);
ASYNC_GW(arp2sys_umount2);
DIRECT_GW(arp2sys_getrlimit);
DIRECT_GW(arp2sys_setrlimit);
DIRECT_GW(arp2sys_getrusage);
DIRECT_GW(arp2sys_getpriority);
DIRECT_GW(arp2sys_setpriority);
ASYNC_GW(arp2sys_select);
ASYNC_GW(arp2sys_pselect);
ASYNC_GW(arp2sys_stat);
ASYNC_GW(arp2sys_fstat);
ASYNC_GW(arp2sys_fstatat);
ASYNC_GW(arp2sys_lstat);
ASYNC_GW(arp2sys_chmod);
ASYNC_GW(arp2sys_lchmod);
ASYNC_GW(arp2sys_fchmod);
ASYNC_GW(arp2sys_fchmodat);
ASYNC_GW(arp2sys_umask);
ASYNC_GW(arp2sys_getumask);
ASYNC_GW(arp2sys_mkdir);
ASYNC_GW(arp2sys_mkdirat);
ASYNC_GW(arp2sys_mknod);
ASYNC_GW(arp2sys_mknodat);
ASYNC_GW(arp2sys_mkfifo);
ASYNC_GW(arp2sys_mkfifoat);
ASYNC_GW(arp2sys_statfs);
ASYNC_GW(arp2sys_fstatfs);
ASYNC_GW(arp2sys_statvfs);
ASYNC_GW(arp2sys_fstatvfs);
ASYNC_GW(arp2sys_swapon);
ASYNC_GW(arp2sys_swapoff);
DIRECT_GW(arp2sys_sysinfo);
DIRECT_GW(arp2sys_get_nprocs_conf);
DIRECT_GW(arp2sys_get_nprocs);
DIRECT_GW(arp2sys_get_phys_pages);
DIRECT_GW(arp2sys_get_avphys_pages);
DIRECT_GW(arp2sys_gettimeofday);
DIRECT_GW(arp2sys_settimeofday);
DIRECT_GW(arp2sys_adjtime);
DIRECT_GW(arp2sys_getitimer);
DIRECT_GW(arp2sys_setitimer);
ASYNC_GW(arp2sys_utimes);
ASYNC_GW(arp2sys_lutimes);
ASYNC_GW(arp2sys_futimes);
ASYNC_GW(arp2sys_futimesat);
DIRECT_GW(arp2sys_times);
ASYNC_GW(arp2sys_readv);
ASYNC_GW(arp2sys_writev);
DIRECT_GW(arp2sys_uname);
ASYNC_GW(arp2sys_wait);
ASYNC_GW(arp2sys_waitid);
ASYNC_GW(arp2sys_wait3);
ASYNC_GW(arp2sys_wait4);


/*** A NULL-terminated array of function pointers to the syscall wrappers ****/

void* const arp2sys_functions[260+1] = {
  arp2sys_arp2_inthandler + 1, // signal quick call
  arp2sys_arp2_run,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  gw_arp2sys_readahead,
  gw_arp2sys_sync_file_range,
  gw_arp2sys_vmsplice,
  gw_arp2sys_splice,
  gw_arp2sys_tee,
  gw_arp2sys_fcntl,
  gw_arp2sys_open,
  gw_arp2sys_openat,
  gw_arp2sys_creat,
  gw_arp2sys_posix_fadvise,
  gw_arp2sys_posix_fallocate,
  gw_arp2sys_mq_open,
  gw_arp2sys_mq_close,
  gw_arp2sys_mq_getattr,
  gw_arp2sys_mq_setattr,
  gw_arp2sys_mq_unlink,
  gw_arp2sys_mq_notify,
  gw_arp2sys_mq_receive,
  gw_arp2sys_mq_send,
  gw_arp2sys_mq_timedreceive,
  gw_arp2sys_mq_timedsend,
  gw_arp2sys_poll,
  gw_arp2sys_ppoll,
  gw_arp2sys_clone,
  gw_arp2sys_unshare,
  gw_arp2sys_sched_setparam,
  gw_arp2sys_sched_getparam,
  gw_arp2sys_sched_setscheduler,
  gw_arp2sys_sched_getscheduler,
  gw_arp2sys_sched_yield,
  gw_arp2sys_sched_get_priority_max,
  gw_arp2sys_sched_get_priority_min,
  gw_arp2sys_sched_rr_get_interval,
  gw_arp2sys_sched_setaffinity,
  gw_arp2sys_sched_getaffinity,
  gw_arp2sys_kill,
  gw_arp2sys_killpg,
  gw_arp2sys_raise,
  gw_arp2sys_sigprocmask,
  gw_arp2sys_sigsuspend,
  gw_arp2sys_sigaction,
  gw_arp2sys_sigpending,
  gw_arp2sys_sigwait,
  gw_arp2sys_sigwaitinfo,
  gw_arp2sys_sigtimedwait,
  gw_arp2sys_sigqueue,
  gw_arp2sys_siginterrupt,
  gw_arp2sys_sigaltstack,
  gw_arp2sys_rename,
  gw_arp2sys_renameat,
  gw_arp2sys_clock,
  gw_arp2sys_time,
  gw_arp2sys_nanosleep,
  gw_arp2sys_clock_getres,
  gw_arp2sys_clock_gettime,
  gw_arp2sys_clock_settime,
  gw_arp2sys_clock_nanosleep,
  gw_arp2sys_clock_getcpuclockid,
  gw_arp2sys_timer_create,
  gw_arp2sys_timer_delete,
  gw_arp2sys_timer_settime,
  gw_arp2sys_timer_gettime,
  gw_arp2sys_timer_getoverrun,
  gw_arp2sys_access,
  gw_arp2sys_euidaccess,
  gw_arp2sys_faccessat,
  gw_arp2sys_lseek,
  gw_arp2sys_close,
  gw_arp2sys_read,
  gw_arp2sys_write,
  gw_arp2sys_pread,
  gw_arp2sys_pwrite,
  gw_arp2sys_pipe,
  gw_arp2sys_alarm,
  gw_arp2sys_sleep,
  gw_arp2sys_pause,
  gw_arp2sys_chown,
  gw_arp2sys_fchown,
  gw_arp2sys_lchown,
  gw_arp2sys_fchownat,
  gw_arp2sys_chdir,
  gw_arp2sys_fchdir,
  gw_arp2sys_getcwd,
  gw_arp2sys_dup,
  gw_arp2sys_dup2,
  gw_arp2sys_execve,
  gw_arp2sys_execv,
  gw_arp2sys_execvp,
  gw_arp2sys_nice,
  gw_arp2sys_pathconf,
  gw_arp2sys_fpathconf,
  gw_arp2sys_sysconf,
  gw_arp2sys_confstr,
  gw_arp2sys_getpid,
  gw_arp2sys_getppid,
  gw_arp2sys_getpgrp,
  gw_arp2sys_getpgid,
  gw_arp2sys_setpgid,
  gw_arp2sys_setpgrp,
  gw_arp2sys_setsid,
  gw_arp2sys_getsid,
  gw_arp2sys_getuid,
  gw_arp2sys_geteuid,
  gw_arp2sys_getgid,
  gw_arp2sys_getegid,
  gw_arp2sys_getgroups,
  gw_arp2sys_setgroups,
  gw_arp2sys_group_member,
  gw_arp2sys_setuid,
  gw_arp2sys_setreuid,
  gw_arp2sys_seteuid,
  gw_arp2sys_setgid,
  gw_arp2sys_setregid,
  gw_arp2sys_setegid,
  gw_arp2sys_getresuid,
  gw_arp2sys_getresgid,
  gw_arp2sys_setresuid,
  gw_arp2sys_setresgid,
  gw_arp2sys_fork,
  gw_arp2sys_vfork,
  gw_arp2sys_ttyname_r,
  gw_arp2sys_isatty,
  gw_arp2sys_link,
  gw_arp2sys_linkat,
  gw_arp2sys_symlink,
  gw_arp2sys_readlink,
  gw_arp2sys_symlinkat,
  gw_arp2sys_readlinkat,
  gw_arp2sys_unlink,
  gw_arp2sys_unlinkat,
  gw_arp2sys_rmdir,
  gw_arp2sys_tcgetpgrp,
  gw_arp2sys_tcsetpgrp,
  gw_arp2sys_getlogin_r,
  gw_arp2sys_setlogin,
  gw_arp2sys_gethostname,
  gw_arp2sys_sethostname,
  gw_arp2sys_sethostid,
  gw_arp2sys_getdomainname,
  gw_arp2sys_setdomainname,
  gw_arp2sys_vhangup,
  gw_arp2sys_revoke,
  gw_arp2sys_profil,
  gw_arp2sys_acct,
  gw_arp2sys_daemon,
  gw_arp2sys_chroot,
  gw_arp2sys_fsync,
  gw_arp2sys_gethostid,
  gw_arp2sys_sync,
  gw_arp2sys_getpagesize,
  gw_arp2sys_getdtablesize,
  gw_arp2sys_truncate,
  gw_arp2sys_ftruncate,
  gw_arp2sys_brk,
  gw_arp2sys_sbrk,
  gw_arp2sys_lockf,
  gw_arp2sys_fdatasync,
  gw_arp2sys_getdents,
  gw_arp2sys_utime,
  gw_arp2sys_setxattr,
  gw_arp2sys_lsetxattr,
  gw_arp2sys_fsetxattr,
  gw_arp2sys_getxattr,
  gw_arp2sys_lgetxattr,
  gw_arp2sys_fgetxattr,
  gw_arp2sys_listxattr,
  gw_arp2sys_llistxattr,
  gw_arp2sys_flistxattr,
  gw_arp2sys_removexattr,
  gw_arp2sys_lremovexattr,
  gw_arp2sys_fremovexattr,
  gw_arp2sys_epoll_create,
  gw_arp2sys_epoll_ctl,
  gw_arp2sys_epoll_wait,
  gw_arp2sys_flock,
  gw_arp2sys_setfsuid,
  gw_arp2sys_setfsgid,
  gw_arp2sys_ioperm,
  gw_arp2sys_iopl,
  gw_arp2sys_ioctl,
  gw_arp2sys_klogctl,
  gw_arp2sys_mmap,
  gw_arp2sys_munmap,
  gw_arp2sys_mprotect,
  gw_arp2sys_msync,
  gw_arp2sys_madvise,
  gw_arp2sys_posix_madvise,
  gw_arp2sys_mlock,
  gw_arp2sys_munlock,
  gw_arp2sys_mlockall,
  gw_arp2sys_munlockall,
  gw_arp2sys_mincore,
  gw_arp2sys_mremap,
  gw_arp2sys_remap_file_pages,
  gw_arp2sys_shm_open,
  gw_arp2sys_shm_unlink,
  gw_arp2sys_mount,
  gw_arp2sys_umount,
  gw_arp2sys_umount2,
  gw_arp2sys_getrlimit,
  gw_arp2sys_setrlimit,
  gw_arp2sys_getrusage,
  gw_arp2sys_getpriority,
  gw_arp2sys_setpriority,
  gw_arp2sys_select,
  gw_arp2sys_pselect,
  gw_arp2sys_stat,
  gw_arp2sys_fstat,
  gw_arp2sys_fstatat,
  gw_arp2sys_lstat,
  gw_arp2sys_chmod,
  gw_arp2sys_lchmod,
  gw_arp2sys_fchmod,
  gw_arp2sys_fchmodat,
  gw_arp2sys_umask,
  gw_arp2sys_getumask,
  gw_arp2sys_mkdir,
  gw_arp2sys_mkdirat,
  gw_arp2sys_mknod,
  gw_arp2sys_mknodat,
  gw_arp2sys_mkfifo,
  gw_arp2sys_mkfifoat,
  gw_arp2sys_statfs,
  gw_arp2sys_fstatfs,
  gw_arp2sys_statvfs,
  gw_arp2sys_fstatvfs,
  gw_arp2sys_swapon,
  gw_arp2sys_swapoff,
  gw_arp2sys_sysinfo,
  gw_arp2sys_get_nprocs_conf,
  gw_arp2sys_get_nprocs,
  gw_arp2sys_get_phys_pages,
  gw_arp2sys_get_avphys_pages,
  gw_arp2sys_gettimeofday,
  gw_arp2sys_settimeofday,
  gw_arp2sys_adjtime,
  gw_arp2sys_getitimer,
  gw_arp2sys_setitimer,
  gw_arp2sys_utimes,
  gw_arp2sys_lutimes,
  gw_arp2sys_futimes,
  gw_arp2sys_futimesat,
  gw_arp2sys_times,
  gw_arp2sys_readv,
  gw_arp2sys_writev,
  gw_arp2sys_uname,
  gw_arp2sys_wait,
  gw_arp2sys_waitid,
  gw_arp2sys_wait3,
  gw_arp2sys_wait4,
  NULL
};


/*** Initialization and patching code ****************************************/

int arp2sys_init(void) {
  list_new(&worker_threads);
  list_new(&workload_waiting);
  list_new(&workload_processing);
  list_new(&workload_ready);
  list_new(&workload_done);

  return 1;
}

int arp2sys_reset(uae_u8* arp2rom) {
  uae_u32* rom = (uae_u32*) (uintptr_t) arp2rom;

  if (BE32(rom[1024]) == 0xdeadc0de &&
      BE32(rom[1025]) == ~0U) {
    int i;
    write_log("ARP2 ROM: arp2sys patching ROM image\n");

    // struct arp2_dirent must match struct dirent64 exactly!
    assert (sizeof (struct arp2_dirent) == sizeof (struct dirent64));
    assert (sizeof (arp2_sigset_t) == sizeof (sigset_t));
    assert (sizeof (arp2_cpu_set_t) == sizeof (cpu_set_t));
    assert (sizeof (arp2_fd_set) == sizeof (fd_set));

    for (i = 0; arp2sys_functions[i] != 0; ++i) {
      assert (IS_32BIT(arp2sys_functions[i]));
      
      rom[1024+i] = BE32((uae_u32) (uintptr_t) arp2sys_functions[i]);
    }

    rom[1024+i] = BE32(~0U);
  }

  return 1;
}

void arp2sys_free(void) {
  // Do what?
}

void arp2sys_hsync_handler(void) {
  // Should not require mutex protection
  if (worker_result_ready) {
    // Assert EXTER while "INT6" signal is active
    INTREQ(INTF_SETCLR | INTF_EXTER);
  }
}

#endif
