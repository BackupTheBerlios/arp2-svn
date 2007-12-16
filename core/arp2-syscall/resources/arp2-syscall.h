#ifndef RESOURCES_ARP2_SYSCALL_H
#define RESOURCES_ARP2_SYSCALL_H

#include <sys/types.h>

#define ARP2_SYSCALL_NAME "arp2-syscall.resource"

typedef long long QUAD;
typedef unsigned long long UQUAD;

typedef LONG  arp2_clock_t;
typedef LONG  arp2_clockid_t;
typedef LONG  arp2_mqd_t;
typedef LONG  arp2_pid_t;
typedef LONG  arp2_time_t;
typedef ULONG arp2_gid_t;
typedef ULONG arp2_id_t;
typedef ULONG arp2_idtype_t;			// Is actually an enum
typedef ULONG arp2_itimer_which_t;		// Is actually an enum
typedef ULONG arp2_mode_t;
typedef ULONG arp2_priority_which_t;		// Is actually an enum
typedef ULONG arp2_rlimit_resource_t;		// Is actually an enum
typedef ULONG arp2_rusage_who_t;		// Is actually an enum
typedef ULONG arp2_uid_t;
typedef UQUAD arp2_dev_t;
typedef UQUAD arp2_rlim_t;
typedef UQUAD arp2_timer_t;

//typedef VOID (*arp2_sighandler_t) (LONG,);

struct arp2_user_desc;

struct arp2_sched_param {
    LONG sched_priority;
};


typedef struct arp2_sigset {
    UBYTE __val[1024 / (8 * sizeof (UBYTE))];
} arp2_sigset_t;

typedef struct arp2_cpu_set {
    UBYTE __val[1024 / (8 * sizeof (UBYTE))];
} arp2_cpu_set_t;

typedef struct {
    UBYTE fds_bits[1024 / (8 * sizeof (UBYTE))];
} arp2_fd_set;

typedef union arp2_sigval {
    LONG sival_int;
    APTR sival_ptr;
} arp2_sigval_t;


typedef struct arp2_siginfo {
    LONG si_signo;
    LONG si_errno;

    LONG si_code;

    union {
	LONG _pad[((128 / sizeof (LONG)) - 4)];

	struct {
	    arp2_pid_t si_pid;
	    arp2_uid_t si_uid;
	} _kill;

	struct {
	    LONG si_tid;
	    LONG si_overrun;
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
	    LONG si_status;
	    arp2_clock_t si_utime;
	    arp2_clock_t si_stime;
	} _sigchld;

	struct {
	    APTR si_addr;
	} _sigfault;

	struct {
	    LONG  si_band;
	    LONG si_fd;
	} _sigpoll;
    } _sifields;
} arp2_siginfo_t;


/* struct arp2_sigaction { */
/*     union { */
/* 	arp2_sighandler_t sa_handler; */
/* 	VOID (*sa_sigaction) (LONG, arp2_siginfo_t*, APTR); */
/*     } __sigaction_handler; */

/*     arp2_sigset_t sa_mask; */

/*     LONG sa_flags; */

/*     VOID (*sa_restorer) (VOID); */
/* }; */


typedef struct arp2_sigevent {
    arp2_sigval_t sigev_value;
    LONG sigev_signo;
    LONG sigev_notify;

    union {
	LONG _pad[((64 / sizeof (LONG)) - 4)];

	arp2_pid_t _tid;

	struct {
	    VOID (*_function) (arp2_sigval_t);
	    APTR _attribute;
	} _sigev_thread;
    } _sigev_un;
} arp2_sigevent_t;


/* struct arp2_sigaltstack { */
/*     APTR  ss_sp; */
/*     LONG  ss_flags; */
/*     ULONG ss_size; */
/* }; */




struct arp2_dirent {
    UQUAD  d_ino;
    QUAD   d_off;
    UWORD  d_reclen;
    UBYTE  d_type;
    TEXT   d_name[256];
};


struct arp2_pollfd {
    LONG fd;
    WORD events;
    WORD revents;
};


typedef union arp2_epoll_data {
  APTR  ptr;
  LONG  fd;
  ULONG u32;
  UQUAD u64;
} arp2_epoll_data_t;

struct arp2_epoll_event {
    arp2_epoll_data_t data;
    ULONG events;
};


struct arp2_sysinfo {
    LONG  uptime;
    ULONG loads[3];
    UQUAD totalram;
    UQUAD freeram;
    UQUAD sharedram;
    UQUAD bufferram;
    UQUAD totalswap;
    UQUAD freeswap;
    UQUAD totalhigh;
    UQUAD freehigh;
    ULONG procs;
    ULONG __extra[3];
};


struct arp2_rlimit {
    arp2_rlim_t rlim_cur;
    arp2_rlim_t rlim_max;
 };

struct arp2_utsname {
    TEXT sysname[65];
    TEXT nodename[65];
    TEXT release[65];
    TEXT version[65];
    TEXT machine[65];
    TEXT domainname[65];
};


struct arp2_tms{
    arp2_clock_t tms_utime;
    arp2_clock_t tms_stime;

    arp2_clock_t tms_cutime;
    arp2_clock_t tms_cstime;
};


struct arp2_timespec {
    arp2_time_t tv_sec;
    LONG        tv_nsec;
};


struct arp2_timeval {
    arp2_time_t tv_sec;
    LONG        tv_usec;
};


struct arp2_itimerspec {
    struct arp2_timespec it_interval;
    struct arp2_timespec it_value;
};


struct arp2_itimerval {
    struct arp2_timeval it_interval;
    struct arp2_timeval it_value;
};


struct arp2_timezone {
    LONG tz_minuteswest;
    LONG tz_dsttime;
};

struct arp2_utimbuf {
    arp2_time_t actime;
    arp2_time_t modtime;
};


struct arp2_rusage {
    struct arp2_timeval ru_utime;
    struct arp2_timeval ru_stime;
    LONG                ru_maxrss;
    LONG                ru_ixrss;
    LONG                ru_idrss;
    LONG                ru_isrss;
    LONG                ru_minflt;
    LONG                ru_majflt;
    LONG                ru_nswap;
    LONG                ru_inblock;
    LONG                ru_oublock;
    LONG                ru_msgsnd;
    LONG                ru_msgrcv;
    LONG                ru_nsignals;
    LONG                ru_nvcsw;
    LONG                ru_nivcsw;
};


struct arp2_stat {
    arp2_dev_t           st_dev;
    UQUAD                st_ino;
    arp2_mode_t          st_mode;
    ULONG                st_nlink;
    arp2_uid_t           st_uid;
    arp2_gid_t           st_gid;
    arp2_dev_t           st_rdev;
    UQUAD                st_size;
    UQUAD                st_blocks;
    ULONG                st_blksize;
    struct arp2_timespec st_atim;
    struct arp2_timespec st_mtim;
    struct arp2_timespec st_ctim;
};


struct arp2_statfs {
    LONG  f_type;
    LONG  f_bsize;
    UQUAD f_blocks;
    UQUAD f_bfree;
    UQUAD f_bavail;
    UQUAD f_files;
    UQUAD f_ffree;
    LONG  f_fsid[2];
    LONG  f_namelen;
    LONG  f_frsize;
    LONG  f_spare[5];
};


struct arp2_statvfs {
    ULONG  f_bsize;
    ULONG  f_frsize;
    UQUAD  f_blocks;
    UQUAD  f_bfree;
    UQUAD  f_bavail;
    UQUAD  f_files;
    UQUAD  f_ffree;
    UQUAD  f_favail;
    ULONG  f_fsid;
    LONG   __f_unused;
    ULONG  f_flag;
    ULONG  f_namemax;
    LONG   __f_spare[6];
};


struct arp2_iovec {
    APTR iov_base;
    ULONG iov_len;
};


struct arp2_mq_attr {
    LONG mq_flags;
    LONG mq_maxmsg;
    LONG mq_msgsize;
    LONG mq_curmsgs;
    LONG __pad[4];
};


#endif /* RESOURCES_ARP2_SYSCALL_H */
