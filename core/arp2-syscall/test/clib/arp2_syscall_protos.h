/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef CLIB_ARP2_SYSCALL_PROTOS_H
#define CLIB_ARP2_SYSCALL_PROTOS_H

/*
**	$VER: arp2_syscall_protos.h  
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2006-2007 Martin Blom
**	    All Rights Reserved
*/

#include <exec/types.h>
#include <resources/arp2-syscall.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Utility functions */
struct Task* arp2sys_arp2_inthandler(void);
ULONG arp2sys_arp2_run(ULONG* regs, APTR native_func);

/* From <fcntl.h> */
LONG arp2sys_readahead(LONG fd, UQUAD offset, ULONG count);
LONG arp2sys_fcntl(LONG fd, LONG cmd, LONG arg);
LONG arp2sys_open(CONST_STRPTR pathname, LONG flags, ULONG mode);
LONG arp2sys_creat(CONST_STRPTR pathname, ULONG mode);
LONG arp2sys_posix_fadvise(LONG fd, UQUAD offset, UQUAD len, LONG advise);
LONG arp2sys_posix_fallocate(LONG fd, UQUAD offset, UQUAD len);

/* From <mqueue.h> */
arp2_mqd_t arp2sys_mq_open(CONST_STRPTR name, LONG oflag, arp2_mode_t mode, struct arp2_mq_attr* attr);
LONG arp2sys_mq_close(arp2_mqd_t mqdes);
LONG arp2sys_mq_getattr(arp2_mqd_t mqdes, struct arp2_mq_attr* mqstat);
LONG arp2sys_mq_setattr(arp2_mqd_t mqdes, CONST struct arp2_mq_attr* mqstat, struct arp2_mq_attr* omqstat);
LONG arp2sys_mq_unlink(CONST_STRPTR name);
LONG arp2sys_mq_notify(arp2_mqd_t mqdes, CONST struct arp2_sigevent* notification);
LONG arp2sys_mq_receive(arp2_mqd_t mqdes, UBYTE* msg_ptr, ULONG msg_len, ULONG* msg_prio);
LONG arp2sys_mq_send(arp2_mqd_t mqdes, CONST UBYTE* msg_ptr, ULONG msg_len, ULONG msg_prio);
LONG arp2sys_mq_timedreceive(arp2_mqd_t mqdes, UBYTE* msg_ptr, ULONG msg_len, ULONG* msg_prio, CONST struct arp2_timespec* abs_timeout);
LONG arp2sys_mq_timedsend(arp2_mqd_t mqdes, CONST UBYTE* msg_ptr, ULONG msg_len, ULONG msg_prio, CONST struct arp2_timespec* abs_timeout);

/* From <sched.h> */
/* NOTE! clone()'s ptid, tls and ctid are currently ignored! */
LONG arp2sys_clone(LONG (*fn)(APTR arg), APTR child_stack, LONG flags, APTR arg, arp2_pid_t* ptid, struct arp2_user_desc* tls, arp2_pid_t* ctid);
LONG arp2sys_sched_setparam(arp2_pid_t pid, CONST struct arp2_sched_param* param);
LONG arp2sys_sched_getparam(arp2_pid_t pid, struct arp2_sched_param* param);
LONG arp2sys_sched_setscheduler(arp2_pid_t pid, LONG policy, CONST struct arp2_sched_param* param);
LONG arp2sys_sched_getscheduler(arp2_pid_t pid);
LONG arp2sys_sched_yield(void);
LONG arp2sys_sched_get_priority_max(LONG algorithm);
LONG arp2sys_sched_get_priority_min(LONG algorithm);
LONG arp2sys_sched_rr_get_interval(arp2_pid_t pid, struct arp2_timespec* t);
LONG arp2sys_sched_setaffinity(arp2_pid_t pid, ULONG cpusetsize, CONST arp2_cpu_set_t* cpuset);
LONG arp2sys_sched_getaffinity(arp2_pid_t pid, ULONG cpusetsize, arp2_cpu_set_t* cpuset);

/* From <signal.h> */
LONG arp2sys_kill(arp2_pid_t pid, LONG sig);
LONG arp2sys_killpg(arp2_pid_t pgrp, LONG sig);
LONG arp2sys_raise(LONG sig);
LONG arp2sys_sigprocmask(LONG how, CONST arp2_sigset_t* set, arp2_sigset_t* oldset);
LONG arp2sys_sigsuspend(CONST arp2_sigset_t* set);
LONG arp2sys_sigpending(arp2_sigset_t* set);
LONG arp2sys_sigwait(CONST arp2_sigset_t* set, LONG* sig);
LONG arp2sys_sigwaitinfo(CONST arp2_sigset_t* set, arp2_siginfo_t* info);
LONG arp2sys_sigtimedwait(CONST arp2_sigset_t* set, arp2_siginfo_t* info, CONST struct arp2_timespec* timeout);
LONG arp2sys_sigqueue(arp2_pid_t pid, LONG sig, CONST union arp2_sigval val);
LONG arp2sys_siginterrupt(LONG sig, LONG interrupt);

/* From <stdio.h> */
LONG arp2sys_rename(CONST_STRPTR old, CONST_STRPTR new);

/* From <time.h> */
arp2_clock_t arp2sys_clock(void);
arp2_time_t arp2sys_time(arp2_time_t* time);
LONG arp2sys_nanosleep(CONST struct arp2_timespec* requested_time, struct arp2_timespec* remaining);
LONG arp2sys_clock_getres(arp2_clockid_t clock_id, struct arp2_timespec* res);
LONG arp2sys_clock_gettime(arp2_clockid_t clock_id, struct arp2_timespec* tp);
LONG arp2sys_clock_settime(arp2_clockid_t clock_id, CONST struct arp2_timespec* tp);
LONG arp2sys_clock_nanosleep(arp2_clockid_t clock_id, LONG flags, CONST struct arp2_timespec* req, struct arp2_timespec* rem);
LONG arp2sys_clock_getcpuclockid(arp2_pid_t pid, arp2_clockid_t* clock_id);

/* From <unistd.h> */
LONG arp2sys_access(CONST_STRPTR name, LONG type);
LONG arp2sys_euidaccess(CONST_STRPTR name, LONG type);
LONG arp2sys_lseek(LONG fd, QUAD offset, LONG whence);
LONG arp2sys_close(LONG fd);
LONG arp2sys_read(LONG fd, APTR buf, ULONG count);
LONG arp2sys_write(LONG fd, CONST_APTR buf, ULONG count);
LONG arp2sys_pread(LONG fd, APTR buf, ULONG nbytes, UQUAD offset);
LONG arp2sys_pwrite(LONG fd, CONST_APTR buf, ULONG n, UQUAD offset);
LONG arp2sys_pipe(LONG* pipedes);
ULONG arp2sys_alarm(ULONG seconds);
ULONG arp2sys_sleep(ULONG seconds);
LONG arp2sys_pause(void);
LONG arp2sys_chown(CONST_STRPTR file, arp2_uid_t owner, arp2_gid_t group);
LONG arp2sys_fchown(LONG fd, arp2_uid_t owner, arp2_gid_t group);
LONG arp2sys_lchown(CONST_STRPTR file, arp2_uid_t owner, arp2_gid_t group);
LONG arp2sys_chdir(CONST_STRPTR path);
LONG arp2sys_fchdir(LONG fd);
STRPTR arp2sys_getcwd(STRPTR buf, ULONG size);
LONG arp2sys_dup(LONG fd);
LONG arp2sys_dup2(LONG fd, LONG fd2);
LONG arp2sys_execve(CONST_STRPTR path, STRPTR CONST* argv, STRPTR CONST* envp);
LONG arp2sys_execv(CONST_STRPTR path, STRPTR CONST* argv);

/* LONG arp2sys_execle(CONST_STRPTR path, CONST_STRPTR arg, ...) */
/* LONG arp2sys_execl(CONST_STRPTR path, CONST_STRPTR arg, ...) */
LONG arp2sys_execvp(CONST_STRPTR file, STRPTR CONST* argv);

/* LONG arp2sys_execlp(CONST_STRPTR file, CONST_STRPTR arg, ...) */
LONG arp2sys_nice(LONG inc);
LONG arp2sys_pathconf(CONST_STRPTR path, LONG name);
LONG arp2sys_fpathconf(LONG fd, LONG name);
LONG arp2sys_sysconf(LONG name);
ULONG arp2sys_confstr(LONG name, STRPTR buf, ULONG len);
arp2_pid_t arp2sys_getpid(void);
arp2_pid_t arp2sys_getppid(void);
arp2_pid_t arp2sys_getpgrp(void);
arp2_pid_t arp2sys_getpgid(arp2_pid_t pid);
LONG arp2sys_setpgid(arp2_pid_t pid, arp2_pid_t pgid);
LONG arp2sys_setpgrp(void);
arp2_pid_t arp2sys_setsid(void);
arp2_pid_t arp2sys_getsid(arp2_pid_t pid);
arp2_uid_t arp2sys_getuid(void);
arp2_uid_t arp2sys_geteuid(void);
arp2_gid_t arp2sys_getgid(void);
arp2_gid_t arp2sys_getegid(void);
LONG arp2sys_getgroups(LONG size, arp2_gid_t* list);

/* setgroups() is actually in <grp.h> */
LONG arp2sys_setgroups(ULONG n, CONST arp2_gid_t* groups);
LONG arp2sys_group_member(arp2_gid_t gid);
LONG arp2sys_setuid(arp2_uid_t uid);
LONG arp2sys_setreuid(arp2_uid_t ruid, arp2_uid_t euid);
LONG arp2sys_seteuid(arp2_uid_t uid);
LONG arp2sys_setgid(arp2_gid_t gid);
LONG arp2sys_setregid(arp2_gid_t rgid, arp2_gid_t egid);
LONG arp2sys_setegid(arp2_gid_t gid);
LONG arp2sys_getresuid(arp2_uid_t* ruid, arp2_uid_t* euid, arp2_uid_t* suid);
LONG arp2sys_getresgid(arp2_gid_t* rgid, arp2_gid_t* egid, arp2_gid_t* sgid);
LONG arp2sys_setresuid(arp2_uid_t ruid, arp2_uid_t euid, arp2_uid_t suid);
LONG arp2sys_setresgid(arp2_gid_t rgid, arp2_gid_t egid, arp2_gid_t sgid);
arp2_pid_t arp2sys_fork(void);
arp2_pid_t arp2sys_vfork(void);
LONG arp2sys_ttyname_r(LONG fd, STRPTR buf, ULONG buflen);
LONG arp2sys_isatty(LONG fd);
LONG arp2sys_link(CONST_STRPTR from, CONST_STRPTR to);
LONG arp2sys_symlink(CONST_STRPTR from, CONST_STRPTR to);
LONG arp2sys_readlink(CONST_STRPTR path, STRPTR buf, ULONG len);
LONG arp2sys_unlink(CONST_STRPTR name);
LONG arp2sys_rmdir(CONST_STRPTR path);
arp2_pid_t arp2sys_tcgetpgrp(LONG fd);
LONG arp2sys_tcsetpgrp(LONG fd, arp2_pid_t pgrp_id);
LONG arp2sys_getlogin_r(STRPTR name, ULONG name_len);
LONG arp2sys_setlogin(CONST_STRPTR name);
LONG arp2sys_gethostname(STRPTR name, ULONG len);
LONG arp2sys_sethostname(CONST_STRPTR name, ULONG len);
LONG arp2sys_sethostid(LONG id);
LONG arp2sys_getdomainname(STRPTR name, ULONG len);
LONG arp2sys_setdomainname(CONST_STRPTR name, ULONG len);
LONG arp2sys_vhangup(void);
LONG arp2sys_revoke(CONST_STRPTR file);
LONG arp2sys_profil(UWORD* sample_buffer, ULONG size, ULONG offset, ULONG scale);
LONG arp2sys_acct(CONST_STRPTR name);
LONG arp2sys_daemon(LONG nochdir, LONG noclose);
LONG arp2sys_chroot(CONST_STRPTR path);
LONG arp2sys_fsync(LONG fd);
LONG arp2sys_gethostid(void);
VOID arp2sys_sync(void);
LONG arp2sys_getpagesize(void);
LONG arp2sys_getdtablesize(void);
LONG arp2sys_truncate(CONST_STRPTR file, UQUAD length);
LONG arp2sys_ftruncate(LONG fd, UQUAD length);
LONG arp2sys_brk(APTR end_data_segment);
APTR arp2sys_sbrk(LONG increment);
LONG arp2sys_lockf(LONG fd, LONG cmd, UQUAD len);
LONG arp2sys_fdatasync(LONG fildes);

/* getdents() is not really in <unistd.h> */
LONG arp2sys_getdents(ULONG fd, struct arp2_dirent* dirp, ULONG count);

/* From <utime.h> */
LONG arp2sys_utime(CONST_STRPTR file, CONST struct arp2_utimbuf* file_times);

/* From <attr/xattr.h> */
LONG arp2sys_setxattr(CONST_STRPTR path, CONST_STRPTR name, CONST_APTR value, ULONG size, LONG flags);
LONG arp2sys_lsetxattr(CONST_STRPTR path, CONST_STRPTR name, CONST_APTR value, ULONG size, LONG flags);
LONG arp2sys_fsetxattr(LONG filedes, CONST_STRPTR name, CONST_APTR value, ULONG size, LONG flags);
LONG arp2sys_getxattr(CONST_STRPTR path, CONST_STRPTR name, APTR value, ULONG size);
LONG arp2sys_lgetxattr(CONST_STRPTR path, CONST_STRPTR name, APTR value, ULONG size);
LONG arp2sys_fgetxattr(LONG filedes, CONST_STRPTR name, APTR value, ULONG size);
LONG arp2sys_listxattr(CONST_STRPTR path, STRPTR list, ULONG size);
LONG arp2sys_llistxattr(CONST_STRPTR path, STRPTR list, ULONG size);
LONG arp2sys_flistxattr(LONG filedes, STRPTR list, ULONG size);
LONG arp2sys_removexattr(CONST_STRPTR path, CONST_STRPTR name);
LONG arp2sys_lremovexattr(CONST_STRPTR path, CONST_STRPTR name);
LONG arp2sys_fremovexattr(LONG filedes, CONST_STRPTR name);

/* From <sys/file.h> */
LONG arp2sys_flock(LONG fd, LONG operation);

/* From <sys/fsuid.h> */
LONG arp2sys_setfsuid(arp2_uid_t uid);
LONG arp2sys_setfsgid(arp2_gid_t gid);

/* From <sys/io.h> */
LONG arp2sys_ioperm(ULONG from, ULONG num, LONG turn_on);
LONG arp2sys_iopl(LONG level);

/* From <sys/ioctl.h> */
LONG arp2sys_ioctl(LONG fd, ULONG request, APTR arg);

/* From <sys/klog.h> */
LONG arp2sys_klogctl(LONG type, STRPTR bufp, LONG len);

/* From <sys/mman.h> */
APTR arp2sys_mmap(APTR start, ULONG length, LONG prot, LONG flags, LONG fd, UQUAD offset);
LONG arp2sys_munmap(APTR start, ULONG length);
LONG arp2sys_mprotect(CONST_APTR addr, ULONG len, LONG prot);
LONG arp2sys_msync(APTR start, ULONG length, LONG flags);
LONG arp2sys_madvise(APTR start, ULONG length, LONG advice);
LONG arp2sys_posix_madvise(APTR start, ULONG length, LONG advice);
LONG arp2sys_mlock(CONST_APTR addr, ULONG len);
LONG arp2sys_munlock(CONST_APTR addr, ULONG len);
LONG arp2sys_mlockall(LONG flags);
LONG arp2sys_munlockall(void);
LONG arp2sys_mincore(APTR start, ULONG length, UBYTE* vec);
APTR arp2sys_mremap(APTR old_address, ULONG old_size, ULONG new_size, LONG flags);
LONG arp2sys_remap_file_pages(APTR start, ULONG size, LONG prot, ULONG pgoff, LONG flags);
LONG arp2sys_shm_open(CONST_STRPTR name, LONG oflag, arp2_mode_t mode);
LONG arp2sys_shm_unlink(CONST_STRPTR name);

/* From <sys/mount.h> */
LONG arp2sys_mount(CONST_STRPTR special_file, CONST_STRPTR dir, CONST_STRPTR fstype, ULONG rwflag, CONST_APTR data);
LONG arp2sys_umount(CONST_STRPTR special_file);
LONG arp2sys_umount2(CONST_STRPTR special_file, LONG flags);

/* From <sys/resource.h> */
LONG arp2sys_getrlimit(arp2_rlimit_resource_t resource, struct arp2_rlimit* rlimits);
LONG arp2sys_setrlimit(arp2_rlimit_resource_t resource, CONST struct arp2_rlimit* rlimits);
LONG arp2sys_getrusage(arp2_rusage_who_t who, struct arp2_rusage* usage);
LONG arp2sys_getpriority(arp2_priority_which_t which, arp2_id_t who);
LONG arp2sys_setpriority(arp2_priority_which_t which, arp2_id_t who, LONG prio);

/* From <sys/select.h> */
LONG arp2sys_select(LONG nfds, arp2_fd_set* readfds, arp2_fd_set* writefds, arp2_fd_set* exceptfds, struct arp2_timeval* timeout);
LONG arp2sys_pselect(LONG nfds, arp2_fd_set* readfds, arp2_fd_set* writefds, arp2_fd_set* exceptfds, CONST struct arp2_timespec* timeout, CONST arp2_sigset_t* sigmask);

/* From <sys/stat.h> */
LONG arp2sys_stat(CONST_STRPTR path, struct arp2_stat* buf);
LONG arp2sys_fstat(LONG fd, struct arp2_stat* buf);
LONG arp2sys_lstat(CONST_STRPTR path, struct arp2_stat* buf);
LONG arp2sys_chmod(CONST_STRPTR file, arp2_mode_t mode);
LONG arp2sys_lchmod(CONST_STRPTR file, arp2_mode_t mode);
LONG arp2sys_fchmod(LONG fd, arp2_mode_t mode);
arp2_mode_t arp2sys_umask(arp2_mode_t mask);
arp2_mode_t arp2sys_getumask(void);
LONG arp2sys_mkdir(CONST_STRPTR path, arp2_mode_t mode);
LONG arp2sys_mknod(CONST_STRPTR path, arp2_mode_t mode, arp2_dev_t dev);
LONG arp2sys_mkfifo(CONST_STRPTR path, arp2_mode_t mode);

/* From <sys/statfs.h> */
LONG arp2sys_statfs(CONST_STRPTR file, struct arp2_statfs* buf);
LONG arp2sys_fstatfs(LONG fildes, struct arp2_statfs* buf);

/* From <sys/statvfs.h> */
LONG arp2sys_statvfs(CONST_STRPTR file, struct arp2_statvfs* buf);
LONG arp2sys_fstatvfs(LONG fildes, struct arp2_statvfs* buf);

/* From <sys/swap.h> */
LONG arp2sys_swapon(CONST_STRPTR path, LONG flags);
LONG arp2sys_swapoff(CONST_STRPTR path);

/* From <sys/sysinfo.h> */
LONG arp2sys_sysinfo(struct arp2_sysinfo* info);
LONG arp2sys_get_nprocs_conf(void);
LONG arp2sys_get_nprocs(void);
LONG arp2sys_get_phys_pages(void);
LONG arp2sys_get_avphys_pages(void);

/* From <sys/time.h> */
LONG arp2sys_gettimeofday(struct arp2_timeval* tv, struct arp2_timezone* tz);
LONG arp2sys_settimeofday(CONST struct arp2_timeval* tv, CONST struct arp2_timezone* tz);
LONG arp2sys_adjtime(CONST struct arp2_timeval* delta, struct arp2_timeval* olddelta);
LONG arp2sys_getitimer(arp2_itimer_which_t which, struct arp2_itimerval* value);
LONG arp2sys_setitimer(arp2_itimer_which_t which, CONST struct arp2_itimerval* new, struct arp2_itimerval* old);
LONG arp2sys_utimes(CONST_STRPTR file, CONST struct arp2_timeval* tvp);
LONG arp2sys_lutimes(CONST_STRPTR file, CONST struct arp2_timeval* tvp);
LONG arp2sys_futimes(LONG fd, CONST struct arp2_timeval* tvp);

/* From <sys/times.h> */
arp2_clock_t arp2sys_times(struct arp2_tms* buffer);

/* From <sys/uio.h> */
LONG arp2sys_readv(LONG fd, CONST struct arp2_iovec* iovec, LONG count);
LONG arp2sys_writev(LONG fd, CONST struct arp2_iovec* iovec, LONG count);

/* From <sys/utsname.h> */
LONG arp2sys_uname(struct arp2_utsname* name);

/* From <sys/wait.h> */
arp2_pid_t arp2sys_wait(LONG* stat_loc);
LONG arp2sys_waitid(arp2_idtype_t idtype, arp2_id_t id, arp2_siginfo_t* infop, LONG options);
arp2_pid_t arp2sys_wait3(LONG* stat_loc, LONG options, struct arp2_rusage* usage);
arp2_pid_t arp2sys_wait4(arp2_pid_t pid, LONG* stat_loc, LONG options, struct arp2_rusage* usage);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_ARP2_SYSCALL_PROTOS_H */
