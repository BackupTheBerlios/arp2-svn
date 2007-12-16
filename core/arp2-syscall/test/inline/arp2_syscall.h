/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_ARP2_SYSCALL_H
#define _INLINE_ARP2_SYSCALL_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef ARP2_SYSCALL_BASE_NAME
#define ARP2_SYSCALL_BASE_NAME ARP2_SysCallBase
#endif /* !ARP2_SYSCALL_BASE_NAME */

#define arp2sys_arp2_inthandler() \
	LP0(0x6, struct Task*, arp2sys_arp2_inthandler ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_arp2_run(___regs, ___native_func) \
	LP2(0xc, ULONG, arp2sys_arp2_run , ULONG*, ___regs, a0, APTR, ___native_func, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_readahead(___fd, ___offset, ___count) \
	LP3(0x42, LONG, arp2sys_readahead , LONG, ___fd, d0, UQUAD, ___offset, d1, ULONG, ___count, d3,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fcntl(___fd, ___cmd, ___arg) \
	LP3(0x60, LONG, arp2sys_fcntl , LONG, ___fd, d0, LONG, ___cmd, d1, LONG, ___arg, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_open(___pathname, ___flags, ___mode) \
	LP3(0x66, LONG, arp2sys_open , CONST_STRPTR, ___pathname, a0, LONG, ___flags, d0, ULONG, ___mode, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_creat(___pathname, ___mode) \
	LP2(0x72, LONG, arp2sys_creat , CONST_STRPTR, ___pathname, a0, ULONG, ___mode, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_posix_fadvise(___fd, ___offset, ___len, ___advise) \
	LP4(0x78, LONG, arp2sys_posix_fadvise , LONG, ___fd, d0, UQUAD, ___offset, d1, UQUAD, ___len, d3, LONG, ___advise, d5,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_posix_fallocate(___fd, ___offset, ___len) \
	LP3(0x7e, LONG, arp2sys_posix_fallocate , LONG, ___fd, d0, UQUAD, ___offset, d1, UQUAD, ___len, d3,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_open(___name, ___oflag, ___mode, ___attr) \
	LP4(0x84, arp2_mqd_t, arp2sys_mq_open , CONST_STRPTR, ___name, a0, LONG, ___oflag, d0, arp2_mode_t, ___mode, d1, struct arp2_mq_attr*, ___attr, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_close(___mqdes) \
	LP1(0x8a, LONG, arp2sys_mq_close , arp2_mqd_t, ___mqdes, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_getattr(___mqdes, ___mqstat) \
	LP2(0x90, LONG, arp2sys_mq_getattr , arp2_mqd_t, ___mqdes, d0, struct arp2_mq_attr*, ___mqstat, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_setattr(___mqdes, ___mqstat, ___omqstat) \
	LP3(0x96, LONG, arp2sys_mq_setattr , arp2_mqd_t, ___mqdes, d0, CONST struct arp2_mq_attr*, ___mqstat, a0, struct arp2_mq_attr*, ___omqstat, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_unlink(___name) \
	LP1(0x9c, LONG, arp2sys_mq_unlink , CONST_STRPTR, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_notify(___mqdes, ___notification) \
	LP2(0xa2, LONG, arp2sys_mq_notify , arp2_mqd_t, ___mqdes, d0, CONST struct arp2_sigevent*, ___notification, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_receive(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio) \
	LP4(0xa8, LONG, arp2sys_mq_receive , arp2_mqd_t, ___mqdes, d0, UBYTE*, ___msg_ptr, a0, ULONG, ___msg_len, d1, ULONG*, ___msg_prio, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_send(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio) \
	LP4(0xae, LONG, arp2sys_mq_send , arp2_mqd_t, ___mqdes, d0, CONST UBYTE*, ___msg_ptr, a0, ULONG, ___msg_len, d1, ULONG, ___msg_prio, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_timedreceive(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio, ___abs_timeout) \
	LP5(0xb4, LONG, arp2sys_mq_timedreceive , arp2_mqd_t, ___mqdes, d0, UBYTE*, ___msg_ptr, a0, ULONG, ___msg_len, d1, ULONG*, ___msg_prio, a1, CONST struct arp2_timespec*, ___abs_timeout, a2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mq_timedsend(___mqdes, ___msg_ptr, ___msg_len, ___msg_prio, ___abs_timeout) \
	LP5(0xba, LONG, arp2sys_mq_timedsend , arp2_mqd_t, ___mqdes, d0, CONST UBYTE*, ___msg_ptr, a0, ULONG, ___msg_len, d1, ULONG, ___msg_prio, d2, CONST struct arp2_timespec*, ___abs_timeout, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clone(___fn, ___child_stack, ___flags, ___arg, ___ptid, ___tls, ___ctid) \
	LP7FP(0xcc, LONG, arp2sys_clone , __fpt, ___fn, a0, APTR, ___child_stack, a1, LONG, ___flags, d0, APTR, ___arg, a2, arp2_pid_t*, ___ptid, d1, struct arp2_user_desc*, ___tls, d2, arp2_pid_t*, ___ctid, d3,\
	, ARP2_SYSCALL_BASE_NAME, LONG (*__fpt)(APTR arg))

#define arp2sys_sched_setparam(___pid, ___param) \
	LP2(0xd8, LONG, arp2sys_sched_setparam , arp2_pid_t, ___pid, d0, CONST struct arp2_sched_param*, ___param, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_getparam(___pid, ___param) \
	LP2(0xde, LONG, arp2sys_sched_getparam , arp2_pid_t, ___pid, d0, struct arp2_sched_param*, ___param, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_setscheduler(___pid, ___policy, ___param) \
	LP3(0xe4, LONG, arp2sys_sched_setscheduler , arp2_pid_t, ___pid, d0, LONG, ___policy, d1, CONST struct arp2_sched_param*, ___param, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_getscheduler(___pid) \
	LP1(0xea, LONG, arp2sys_sched_getscheduler , arp2_pid_t, ___pid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_yield() \
	LP0(0xf0, LONG, arp2sys_sched_yield ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_get_priority_max(___algorithm) \
	LP1(0xf6, LONG, arp2sys_sched_get_priority_max , LONG, ___algorithm, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_get_priority_min(___algorithm) \
	LP1(0xfc, LONG, arp2sys_sched_get_priority_min , LONG, ___algorithm, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_rr_get_interval(___pid, ___t) \
	LP2(0x102, LONG, arp2sys_sched_rr_get_interval , arp2_pid_t, ___pid, d0, struct arp2_timespec*, ___t, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_setaffinity(___pid, ___cpusetsize, ___cpuset) \
	LP3(0x108, LONG, arp2sys_sched_setaffinity , arp2_pid_t, ___pid, d0, ULONG, ___cpusetsize, d1, CONST arp2_cpu_set_t*, ___cpuset, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sched_getaffinity(___pid, ___cpusetsize, ___cpuset) \
	LP3(0x10e, LONG, arp2sys_sched_getaffinity , arp2_pid_t, ___pid, d0, ULONG, ___cpusetsize, d1, arp2_cpu_set_t*, ___cpuset, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_kill(___pid, ___sig) \
	LP2(0x114, LONG, arp2sys_kill , arp2_pid_t, ___pid, d0, LONG, ___sig, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_killpg(___pgrp, ___sig) \
	LP2(0x11a, LONG, arp2sys_killpg , arp2_pid_t, ___pgrp, d0, LONG, ___sig, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_raise(___sig) \
	LP1(0x120, LONG, arp2sys_raise , LONG, ___sig, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigprocmask(___how, ___set, ___oldset) \
	LP3(0x126, LONG, arp2sys_sigprocmask , LONG, ___how, d0, CONST arp2_sigset_t*, ___set, a0, arp2_sigset_t*, ___oldset, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigsuspend(___set) \
	LP1(0x12c, LONG, arp2sys_sigsuspend , CONST arp2_sigset_t*, ___set, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigpending(___set) \
	LP1(0x138, LONG, arp2sys_sigpending , arp2_sigset_t*, ___set, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigwait(___set, ___sig) \
	LP2(0x13e, LONG, arp2sys_sigwait , CONST arp2_sigset_t*, ___set, a0, LONG*, ___sig, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigwaitinfo(___set, ___info) \
	LP2(0x144, LONG, arp2sys_sigwaitinfo , CONST arp2_sigset_t*, ___set, a0, arp2_siginfo_t*, ___info, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigtimedwait(___set, ___info, ___timeout) \
	LP3(0x14a, LONG, arp2sys_sigtimedwait , CONST arp2_sigset_t*, ___set, a0, arp2_siginfo_t*, ___info, a1, CONST struct arp2_timespec*, ___timeout, a2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sigqueue(___pid, ___sig, ___val) \
	LP3(0x150, LONG, arp2sys_sigqueue , arp2_pid_t, ___pid, d0, LONG, ___sig, d1, CONST union arp2_sigval, ___val, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_siginterrupt(___sig, ___interrupt) \
	LP2(0x156, LONG, arp2sys_siginterrupt , LONG, ___sig, d0, LONG, ___interrupt, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_rename(___old, ___new) \
	LP2(0x162, LONG, arp2sys_rename , CONST_STRPTR, ___old, a0, CONST_STRPTR, ___new, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clock() \
	LP0(0x16e, arp2_clock_t, arp2sys_clock ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_time(___time) \
	LP1(0x174, arp2_time_t, arp2sys_time , arp2_time_t*, ___time, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_nanosleep(___requested_time, ___remaining) \
	LP2(0x17a, LONG, arp2sys_nanosleep , CONST struct arp2_timespec*, ___requested_time, a0, struct arp2_timespec*, ___remaining, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clock_getres(___clock_id, ___res) \
	LP2(0x180, LONG, arp2sys_clock_getres , arp2_clockid_t, ___clock_id, d0, struct arp2_timespec*, ___res, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clock_gettime(___clock_id, ___tp) \
	LP2(0x186, LONG, arp2sys_clock_gettime , arp2_clockid_t, ___clock_id, d0, struct arp2_timespec*, ___tp, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clock_settime(___clock_id, ___tp) \
	LP2(0x18c, LONG, arp2sys_clock_settime , arp2_clockid_t, ___clock_id, d0, CONST struct arp2_timespec*, ___tp, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clock_nanosleep(___clock_id, ___flags, ___req, ___rem) \
	LP4(0x192, LONG, arp2sys_clock_nanosleep , arp2_clockid_t, ___clock_id, d0, LONG, ___flags, d1, CONST struct arp2_timespec*, ___req, a0, struct arp2_timespec*, ___rem, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_clock_getcpuclockid(___pid, ___clock_id) \
	LP2(0x198, LONG, arp2sys_clock_getcpuclockid , arp2_pid_t, ___pid, d0, arp2_clockid_t*, ___clock_id, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_access(___name, ___type) \
	LP2(0x1bc, LONG, arp2sys_access , CONST_STRPTR, ___name, a0, LONG, ___type, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_euidaccess(___name, ___type) \
	LP2(0x1c2, LONG, arp2sys_euidaccess , CONST_STRPTR, ___name, a0, LONG, ___type, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lseek(___fd, ___offset, ___whence) \
	LP3(0x1ce, LONG, arp2sys_lseek , LONG, ___fd, d0, QUAD, ___offset, d1, LONG, ___whence, d3,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_close(___fd) \
	LP1(0x1d4, LONG, arp2sys_close , LONG, ___fd, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_read(___fd, ___buf, ___count) \
	LP3(0x1da, LONG, arp2sys_read , LONG, ___fd, d0, APTR, ___buf, a0, ULONG, ___count, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_write(___fd, ___buf, ___count) \
	LP3(0x1e0, LONG, arp2sys_write , LONG, ___fd, d0, CONST_APTR, ___buf, a0, ULONG, ___count, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_pread(___fd, ___buf, ___nbytes, ___offset) \
	LP4(0x1e6, LONG, arp2sys_pread , LONG, ___fd, d0, APTR, ___buf, a0, ULONG, ___nbytes, d1, UQUAD, ___offset, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_pwrite(___fd, ___buf, ___n, ___offset) \
	LP4(0x1ec, LONG, arp2sys_pwrite , LONG, ___fd, d0, CONST_APTR, ___buf, a0, ULONG, ___n, d1, UQUAD, ___offset, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_pipe(___pipedes) \
	LP1(0x1f2, LONG, arp2sys_pipe , LONG*, ___pipedes, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_alarm(___seconds) \
	LP1(0x1f8, ULONG, arp2sys_alarm , ULONG, ___seconds, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sleep(___seconds) \
	LP1(0x1fe, ULONG, arp2sys_sleep , ULONG, ___seconds, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_pause() \
	LP0(0x204, LONG, arp2sys_pause ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_chown(___file, ___owner, ___group) \
	LP3(0x20a, LONG, arp2sys_chown , CONST_STRPTR, ___file, a0, arp2_uid_t, ___owner, d0, arp2_gid_t, ___group, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fchown(___fd, ___owner, ___group) \
	LP3(0x210, LONG, arp2sys_fchown , LONG, ___fd, d0, arp2_uid_t, ___owner, d1, arp2_gid_t, ___group, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lchown(___file, ___owner, ___group) \
	LP3(0x216, LONG, arp2sys_lchown , CONST_STRPTR, ___file, a0, arp2_uid_t, ___owner, d0, arp2_gid_t, ___group, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_chdir(___path) \
	LP1(0x222, LONG, arp2sys_chdir , CONST_STRPTR, ___path, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fchdir(___fd) \
	LP1(0x228, LONG, arp2sys_fchdir , LONG, ___fd, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getcwd(___buf, ___size) \
	LP2(0x22e, STRPTR, arp2sys_getcwd , STRPTR, ___buf, a0, ULONG, ___size, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_dup(___fd) \
	LP1(0x234, LONG, arp2sys_dup , LONG, ___fd, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_dup2(___fd, ___fd2) \
	LP2(0x23a, LONG, arp2sys_dup2 , LONG, ___fd, d0, LONG, ___fd2, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_execve(___path, ___argv, ___envp) \
	LP3(0x240, LONG, arp2sys_execve , CONST_STRPTR, ___path, a0, STRPTR CONST*, ___argv, a1, STRPTR CONST*, ___envp, a2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_execv(___path, ___argv) \
	LP2(0x246, LONG, arp2sys_execv , CONST_STRPTR, ___path, a0, STRPTR CONST*, ___argv, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_execvp(___file, ___argv) \
	LP2(0x24c, LONG, arp2sys_execvp , CONST_STRPTR, ___file, a0, STRPTR CONST*, ___argv, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_nice(___inc) \
	LP1(0x252, LONG, arp2sys_nice , LONG, ___inc, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_pathconf(___path, ___name) \
	LP2(0x258, LONG, arp2sys_pathconf , CONST_STRPTR, ___path, a0, LONG, ___name, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fpathconf(___fd, ___name) \
	LP2(0x25e, LONG, arp2sys_fpathconf , LONG, ___fd, d0, LONG, ___name, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sysconf(___name) \
	LP1(0x264, LONG, arp2sys_sysconf , LONG, ___name, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_confstr(___name, ___buf, ___len) \
	LP3(0x26a, ULONG, arp2sys_confstr , LONG, ___name, d0, STRPTR, ___buf, a0, ULONG, ___len, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getpid() \
	LP0(0x270, arp2_pid_t, arp2sys_getpid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getppid() \
	LP0(0x276, arp2_pid_t, arp2sys_getppid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getpgrp() \
	LP0(0x27c, arp2_pid_t, arp2sys_getpgrp ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getpgid(___pid) \
	LP1(0x282, arp2_pid_t, arp2sys_getpgid , arp2_pid_t, ___pid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setpgid(___pid, ___pgid) \
	LP2(0x288, LONG, arp2sys_setpgid , arp2_pid_t, ___pid, d0, arp2_pid_t, ___pgid, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setpgrp() \
	LP0(0x28e, LONG, arp2sys_setpgrp ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setsid() \
	LP0(0x294, arp2_pid_t, arp2sys_setsid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getsid(___pid) \
	LP1(0x29a, arp2_pid_t, arp2sys_getsid , arp2_pid_t, ___pid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getuid() \
	LP0(0x2a0, arp2_uid_t, arp2sys_getuid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_geteuid() \
	LP0(0x2a6, arp2_uid_t, arp2sys_geteuid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getgid() \
	LP0(0x2ac, arp2_gid_t, arp2sys_getgid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getegid() \
	LP0(0x2b2, arp2_gid_t, arp2sys_getegid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getgroups(___size, ___list) \
	LP2(0x2b8, LONG, arp2sys_getgroups , LONG, ___size, d0, arp2_gid_t*, ___list, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setgroups(___n, ___groups) \
	LP2(0x2be, LONG, arp2sys_setgroups , ULONG, ___n, d0, CONST arp2_gid_t*, ___groups, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_group_member(___gid) \
	LP1(0x2c4, LONG, arp2sys_group_member , arp2_gid_t, ___gid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setuid(___uid) \
	LP1(0x2ca, LONG, arp2sys_setuid , arp2_uid_t, ___uid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setreuid(___ruid, ___euid) \
	LP2(0x2d0, LONG, arp2sys_setreuid , arp2_uid_t, ___ruid, d0, arp2_uid_t, ___euid, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_seteuid(___uid) \
	LP1(0x2d6, LONG, arp2sys_seteuid , arp2_uid_t, ___uid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setgid(___gid) \
	LP1(0x2dc, LONG, arp2sys_setgid , arp2_gid_t, ___gid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setregid(___rgid, ___egid) \
	LP2(0x2e2, LONG, arp2sys_setregid , arp2_gid_t, ___rgid, d0, arp2_gid_t, ___egid, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setegid(___gid) \
	LP1(0x2e8, LONG, arp2sys_setegid , arp2_gid_t, ___gid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getresuid(___ruid, ___euid, ___suid) \
	LP3(0x2ee, LONG, arp2sys_getresuid , arp2_uid_t*, ___ruid, a0, arp2_uid_t*, ___euid, a1, arp2_uid_t*, ___suid, a2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getresgid(___rgid, ___egid, ___sgid) \
	LP3(0x2f4, LONG, arp2sys_getresgid , arp2_gid_t*, ___rgid, a0, arp2_gid_t*, ___egid, a1, arp2_gid_t*, ___sgid, a2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setresuid(___ruid, ___euid, ___suid) \
	LP3(0x2fa, LONG, arp2sys_setresuid , arp2_uid_t, ___ruid, d0, arp2_uid_t, ___euid, d1, arp2_uid_t, ___suid, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setresgid(___rgid, ___egid, ___sgid) \
	LP3(0x300, LONG, arp2sys_setresgid , arp2_gid_t, ___rgid, d0, arp2_gid_t, ___egid, d1, arp2_gid_t, ___sgid, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fork() \
	LP0(0x306, arp2_pid_t, arp2sys_fork ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_vfork() \
	LP0(0x30c, arp2_pid_t, arp2sys_vfork ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_ttyname_r(___fd, ___buf, ___buflen) \
	LP3(0x312, LONG, arp2sys_ttyname_r , LONG, ___fd, d0, STRPTR, ___buf, a0, ULONG, ___buflen, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_isatty(___fd) \
	LP1(0x318, LONG, arp2sys_isatty , LONG, ___fd, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_link(___from, ___to) \
	LP2(0x31e, LONG, arp2sys_link , CONST_STRPTR, ___from, a0, CONST_STRPTR, ___to, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_symlink(___from, ___to) \
	LP2(0x32a, LONG, arp2sys_symlink , CONST_STRPTR, ___from, a0, CONST_STRPTR, ___to, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_readlink(___path, ___buf, ___len) \
	LP3(0x330, LONG, arp2sys_readlink , CONST_STRPTR, ___path, a0, STRPTR, ___buf, a1, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_unlink(___name) \
	LP1(0x342, LONG, arp2sys_unlink , CONST_STRPTR, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_rmdir(___path) \
	LP1(0x34e, LONG, arp2sys_rmdir , CONST_STRPTR, ___path, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_tcgetpgrp(___fd) \
	LP1(0x354, arp2_pid_t, arp2sys_tcgetpgrp , LONG, ___fd, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_tcsetpgrp(___fd, ___pgrp_id) \
	LP2(0x35a, LONG, arp2sys_tcsetpgrp , LONG, ___fd, d0, arp2_pid_t, ___pgrp_id, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getlogin_r(___name, ___name_len) \
	LP2(0x360, LONG, arp2sys_getlogin_r , STRPTR, ___name, a0, ULONG, ___name_len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setlogin(___name) \
	LP1(0x366, LONG, arp2sys_setlogin , CONST_STRPTR, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_gethostname(___name, ___len) \
	LP2(0x36c, LONG, arp2sys_gethostname , STRPTR, ___name, a0, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sethostname(___name, ___len) \
	LP2(0x372, LONG, arp2sys_sethostname , CONST_STRPTR, ___name, a0, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sethostid(___id) \
	LP1(0x378, LONG, arp2sys_sethostid , LONG, ___id, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getdomainname(___name, ___len) \
	LP2(0x37e, LONG, arp2sys_getdomainname , STRPTR, ___name, a0, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setdomainname(___name, ___len) \
	LP2(0x384, LONG, arp2sys_setdomainname , CONST_STRPTR, ___name, a0, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_vhangup() \
	LP0(0x38a, LONG, arp2sys_vhangup ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_revoke(___file) \
	LP1(0x390, LONG, arp2sys_revoke , CONST_STRPTR, ___file, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_profil(___sample_buffer, ___size, ___offset, ___scale) \
	LP4(0x396, LONG, arp2sys_profil , UWORD*, ___sample_buffer, a0, ULONG, ___size, d0, ULONG, ___offset, d1, ULONG, ___scale, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_acct(___name) \
	LP1(0x39c, LONG, arp2sys_acct , CONST_STRPTR, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_daemon(___nochdir, ___noclose) \
	LP2(0x3a2, LONG, arp2sys_daemon , LONG, ___nochdir, d0, LONG, ___noclose, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_chroot(___path) \
	LP1(0x3a8, LONG, arp2sys_chroot , CONST_STRPTR, ___path, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fsync(___fd) \
	LP1(0x3ae, LONG, arp2sys_fsync , LONG, ___fd, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_gethostid() \
	LP0(0x3b4, LONG, arp2sys_gethostid ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sync() \
	LP0NR(0x3ba, arp2sys_sync ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getpagesize() \
	LP0(0x3c0, LONG, arp2sys_getpagesize ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getdtablesize() \
	LP0(0x3c6, LONG, arp2sys_getdtablesize ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_truncate(___file, ___length) \
	LP2(0x3cc, LONG, arp2sys_truncate , CONST_STRPTR, ___file, a0, UQUAD, ___length, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_ftruncate(___fd, ___length) \
	LP2(0x3d2, LONG, arp2sys_ftruncate , LONG, ___fd, d0, UQUAD, ___length, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_brk(___end_data_segment) \
	LP1(0x3d8, LONG, arp2sys_brk , APTR, ___end_data_segment, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sbrk(___increment) \
	LP1(0x3de, APTR, arp2sys_sbrk , LONG, ___increment, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lockf(___fd, ___cmd, ___len) \
	LP3(0x3e4, LONG, arp2sys_lockf , LONG, ___fd, d0, LONG, ___cmd, d1, UQUAD, ___len, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fdatasync(___fildes) \
	LP1(0x3ea, LONG, arp2sys_fdatasync , LONG, ___fildes, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getdents(___fd, ___dirp, ___count) \
	LP3(0x3f0, LONG, arp2sys_getdents , ULONG, ___fd, d0, struct arp2_dirent*, ___dirp, a0, ULONG, ___count, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_utime(___file, ___file_times) \
	LP2(0x3f6, LONG, arp2sys_utime , CONST_STRPTR, ___file, a0, CONST struct arp2_utimbuf*, ___file_times, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setxattr(___path, ___name, ___value, ___size, ___flags) \
	LP5(0x3fc, LONG, arp2sys_setxattr , CONST_STRPTR, ___path, a0, CONST_STRPTR, ___name, a1, CONST_APTR, ___value, a2, ULONG, ___size, d0, LONG, ___flags, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lsetxattr(___path, ___name, ___value, ___size, ___flags) \
	LP5(0x402, LONG, arp2sys_lsetxattr , CONST_STRPTR, ___path, a0, CONST_STRPTR, ___name, a1, CONST_APTR, ___value, a2, ULONG, ___size, d0, LONG, ___flags, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fsetxattr(___filedes, ___name, ___value, ___size, ___flags) \
	LP5(0x408, LONG, arp2sys_fsetxattr , LONG, ___filedes, d0, CONST_STRPTR, ___name, a0, CONST_APTR, ___value, a1, ULONG, ___size, d1, LONG, ___flags, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getxattr(___path, ___name, ___value, ___size) \
	LP4(0x40e, LONG, arp2sys_getxattr , CONST_STRPTR, ___path, a0, CONST_STRPTR, ___name, a1, APTR, ___value, a2, ULONG, ___size, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lgetxattr(___path, ___name, ___value, ___size) \
	LP4(0x414, LONG, arp2sys_lgetxattr , CONST_STRPTR, ___path, a0, CONST_STRPTR, ___name, a1, APTR, ___value, a2, ULONG, ___size, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fgetxattr(___filedes, ___name, ___value, ___size) \
	LP4(0x41a, LONG, arp2sys_fgetxattr , LONG, ___filedes, d0, CONST_STRPTR, ___name, a0, APTR, ___value, a1, ULONG, ___size, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_listxattr(___path, ___list, ___size) \
	LP3(0x420, LONG, arp2sys_listxattr , CONST_STRPTR, ___path, a0, STRPTR, ___list, a1, ULONG, ___size, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_llistxattr(___path, ___list, ___size) \
	LP3(0x426, LONG, arp2sys_llistxattr , CONST_STRPTR, ___path, a0, STRPTR, ___list, a1, ULONG, ___size, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_flistxattr(___filedes, ___list, ___size) \
	LP3(0x42c, LONG, arp2sys_flistxattr , LONG, ___filedes, d0, STRPTR, ___list, a0, ULONG, ___size, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_removexattr(___path, ___name) \
	LP2(0x432, LONG, arp2sys_removexattr , CONST_STRPTR, ___path, a0, CONST_STRPTR, ___name, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lremovexattr(___path, ___name) \
	LP2(0x438, LONG, arp2sys_lremovexattr , CONST_STRPTR, ___path, a0, CONST_STRPTR, ___name, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fremovexattr(___filedes, ___name) \
	LP2(0x43e, LONG, arp2sys_fremovexattr , LONG, ___filedes, d0, CONST_STRPTR, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_flock(___fd, ___operation) \
	LP2(0x456, LONG, arp2sys_flock , LONG, ___fd, d0, LONG, ___operation, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setfsuid(___uid) \
	LP1(0x45c, LONG, arp2sys_setfsuid , arp2_uid_t, ___uid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setfsgid(___gid) \
	LP1(0x462, LONG, arp2sys_setfsgid , arp2_gid_t, ___gid, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_ioperm(___from, ___num, ___turn_on) \
	LP3(0x468, LONG, arp2sys_ioperm , ULONG, ___from, d0, ULONG, ___num, d1, LONG, ___turn_on, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_iopl(___level) \
	LP1(0x46e, LONG, arp2sys_iopl , LONG, ___level, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_ioctl(___fd, ___request, ___arg) \
	LP3(0x474, LONG, arp2sys_ioctl , LONG, ___fd, d0, ULONG, ___request, d1, APTR, ___arg, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_klogctl(___type, ___bufp, ___len) \
	LP3(0x47a, LONG, arp2sys_klogctl , LONG, ___type, d0, STRPTR, ___bufp, a0, LONG, ___len, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mmap(___start, ___length, ___prot, ___flags, ___fd, ___offset) \
	LP6(0x480, APTR, arp2sys_mmap , APTR, ___start, a0, ULONG, ___length, d0, LONG, ___prot, d1, LONG, ___flags, d2, LONG, ___fd, d3, UQUAD, ___offset, d4,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_munmap(___start, ___length) \
	LP2(0x486, LONG, arp2sys_munmap , APTR, ___start, a0, ULONG, ___length, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mprotect(___addr, ___len, ___prot) \
	LP3(0x48c, LONG, arp2sys_mprotect , CONST_APTR, ___addr, a0, ULONG, ___len, d0, LONG, ___prot, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_msync(___start, ___length, ___flags) \
	LP3(0x492, LONG, arp2sys_msync , APTR, ___start, a0, ULONG, ___length, d0, LONG, ___flags, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_madvise(___start, ___length, ___advice) \
	LP3(0x498, LONG, arp2sys_madvise , APTR, ___start, a0, ULONG, ___length, d0, LONG, ___advice, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_posix_madvise(___start, ___length, ___advice) \
	LP3(0x49e, LONG, arp2sys_posix_madvise , APTR, ___start, a0, ULONG, ___length, d0, LONG, ___advice, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mlock(___addr, ___len) \
	LP2(0x4a4, LONG, arp2sys_mlock , CONST_APTR, ___addr, a0, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_munlock(___addr, ___len) \
	LP2(0x4aa, LONG, arp2sys_munlock , CONST_APTR, ___addr, a0, ULONG, ___len, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mlockall(___flags) \
	LP1(0x4b0, LONG, arp2sys_mlockall , LONG, ___flags, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_munlockall() \
	LP0(0x4b6, LONG, arp2sys_munlockall ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mincore(___start, ___length, ___vec) \
	LP3(0x4bc, LONG, arp2sys_mincore , APTR, ___start, a0, ULONG, ___length, d0, UBYTE*, ___vec, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mremap(___old_address, ___old_size, ___new_size, ___flags) \
	LP4(0x4c2, APTR, arp2sys_mremap , APTR, ___old_address, a0, ULONG, ___old_size, d0, ULONG, ___new_size, d1, LONG, ___flags, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_remap_file_pages(___start, ___size, ___prot, ___pgoff, ___flags) \
	LP5(0x4c8, LONG, arp2sys_remap_file_pages , APTR, ___start, a0, ULONG, ___size, d0, LONG, ___prot, d1, ULONG, ___pgoff, d2, LONG, ___flags, d3,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_shm_open(___name, ___oflag, ___mode) \
	LP3(0x4ce, LONG, arp2sys_shm_open , CONST_STRPTR, ___name, a0, LONG, ___oflag, d0, arp2_mode_t, ___mode, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_shm_unlink(___name) \
	LP1(0x4d4, LONG, arp2sys_shm_unlink , CONST_STRPTR, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mount(___special_file, ___dir, ___fstype, ___rwflag, ___data) \
	LP5(0x4da, LONG, arp2sys_mount , CONST_STRPTR, ___special_file, a0, CONST_STRPTR, ___dir, a1, CONST_STRPTR, ___fstype, a2, ULONG, ___rwflag, d0, CONST_APTR, ___data, a3,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_umount(___special_file) \
	LP1(0x4e0, LONG, arp2sys_umount , CONST_STRPTR, ___special_file, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_umount2(___special_file, ___flags) \
	LP2(0x4e6, LONG, arp2sys_umount2 , CONST_STRPTR, ___special_file, a0, LONG, ___flags, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getrlimit(___resource, ___rlimits) \
	LP2(0x4ec, LONG, arp2sys_getrlimit , arp2_rlimit_resource_t, ___resource, d0, struct arp2_rlimit*, ___rlimits, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setrlimit(___resource, ___rlimits) \
	LP2(0x4f2, LONG, arp2sys_setrlimit , arp2_rlimit_resource_t, ___resource, d0, CONST struct arp2_rlimit*, ___rlimits, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getrusage(___who, ___usage) \
	LP2(0x4f8, LONG, arp2sys_getrusage , arp2_rusage_who_t, ___who, d0, struct arp2_rusage*, ___usage, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getpriority(___which, ___who) \
	LP2(0x4fe, LONG, arp2sys_getpriority , arp2_priority_which_t, ___which, d0, arp2_id_t, ___who, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setpriority(___which, ___who, ___prio) \
	LP3(0x504, LONG, arp2sys_setpriority , arp2_priority_which_t, ___which, d0, arp2_id_t, ___who, d1, LONG, ___prio, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_select(___nfds, ___readfds, ___writefds, ___exceptfds, ___timeout) \
	LP5(0x50a, LONG, arp2sys_select , LONG, ___nfds, d0, arp2_fd_set*, ___readfds, a0, arp2_fd_set*, ___writefds, a1, arp2_fd_set*, ___exceptfds, a2, struct arp2_timeval*, ___timeout, a3,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_pselect(___nfds, ___readfds, ___writefds, ___exceptfds, ___timeout, ___sigmask) \
	LP6A4(0x510, LONG, arp2sys_pselect , LONG, ___nfds, d0, arp2_fd_set*, ___readfds, a0, arp2_fd_set*, ___writefds, a1, arp2_fd_set*, ___exceptfds, a2, CONST struct arp2_timespec*, ___timeout, a3, CONST arp2_sigset_t*, ___sigmask, d7,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_stat(___path, ___buf) \
	LP2(0x516, LONG, arp2sys_stat , CONST_STRPTR, ___path, a0, struct arp2_stat*, ___buf, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fstat(___fd, ___buf) \
	LP2(0x51c, LONG, arp2sys_fstat , LONG, ___fd, d0, struct arp2_stat*, ___buf, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lstat(___path, ___buf) \
	LP2(0x528, LONG, arp2sys_lstat , CONST_STRPTR, ___path, a0, struct arp2_stat*, ___buf, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_chmod(___file, ___mode) \
	LP2(0x52e, LONG, arp2sys_chmod , CONST_STRPTR, ___file, a0, arp2_mode_t, ___mode, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lchmod(___file, ___mode) \
	LP2(0x534, LONG, arp2sys_lchmod , CONST_STRPTR, ___file, a0, arp2_mode_t, ___mode, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fchmod(___fd, ___mode) \
	LP2(0x53a, LONG, arp2sys_fchmod , LONG, ___fd, d0, arp2_mode_t, ___mode, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_umask(___mask) \
	LP1(0x546, arp2_mode_t, arp2sys_umask , arp2_mode_t, ___mask, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getumask() \
	LP0(0x54c, arp2_mode_t, arp2sys_getumask ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mkdir(___path, ___mode) \
	LP2(0x552, LONG, arp2sys_mkdir , CONST_STRPTR, ___path, a0, arp2_mode_t, ___mode, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mknod(___path, ___mode, ___dev) \
	LP3(0x55e, LONG, arp2sys_mknod , CONST_STRPTR, ___path, a0, arp2_mode_t, ___mode, d0, arp2_dev_t, ___dev, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_mkfifo(___path, ___mode) \
	LP2(0x56a, LONG, arp2sys_mkfifo , CONST_STRPTR, ___path, a0, arp2_mode_t, ___mode, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_statfs(___file, ___buf) \
	LP2(0x576, LONG, arp2sys_statfs , CONST_STRPTR, ___file, a0, struct arp2_statfs*, ___buf, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fstatfs(___fildes, ___buf) \
	LP2(0x57c, LONG, arp2sys_fstatfs , LONG, ___fildes, d0, struct arp2_statfs*, ___buf, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_statvfs(___file, ___buf) \
	LP2(0x582, LONG, arp2sys_statvfs , CONST_STRPTR, ___file, a0, struct arp2_statvfs*, ___buf, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_fstatvfs(___fildes, ___buf) \
	LP2(0x588, LONG, arp2sys_fstatvfs , LONG, ___fildes, d0, struct arp2_statvfs*, ___buf, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_swapon(___path, ___flags) \
	LP2(0x58e, LONG, arp2sys_swapon , CONST_STRPTR, ___path, a0, LONG, ___flags, d0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_swapoff(___path) \
	LP1(0x594, LONG, arp2sys_swapoff , CONST_STRPTR, ___path, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_sysinfo(___info) \
	LP1(0x59a, LONG, arp2sys_sysinfo , struct arp2_sysinfo*, ___info, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_get_nprocs_conf() \
	LP0(0x5a0, LONG, arp2sys_get_nprocs_conf ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_get_nprocs() \
	LP0(0x5a6, LONG, arp2sys_get_nprocs ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_get_phys_pages() \
	LP0(0x5ac, LONG, arp2sys_get_phys_pages ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_get_avphys_pages() \
	LP0(0x5b2, LONG, arp2sys_get_avphys_pages ,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_gettimeofday(___tv, ___tz) \
	LP2(0x5b8, LONG, arp2sys_gettimeofday , struct arp2_timeval*, ___tv, a0, struct arp2_timezone*, ___tz, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_settimeofday(___tv, ___tz) \
	LP2(0x5be, LONG, arp2sys_settimeofday , CONST struct arp2_timeval*, ___tv, a0, CONST struct arp2_timezone*, ___tz, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_adjtime(___delta, ___olddelta) \
	LP2(0x5c4, LONG, arp2sys_adjtime , CONST struct arp2_timeval*, ___delta, a0, struct arp2_timeval*, ___olddelta, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_getitimer(___which, ___value) \
	LP2(0x5ca, LONG, arp2sys_getitimer , arp2_itimer_which_t, ___which, d0, struct arp2_itimerval*, ___value, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_setitimer(___which, ___new, ___old) \
	LP3(0x5d0, LONG, arp2sys_setitimer , arp2_itimer_which_t, ___which, d0, CONST struct arp2_itimerval*, ___new, a0, struct arp2_itimerval*, ___old, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_utimes(___file, ___tvp) \
	LP2(0x5d6, LONG, arp2sys_utimes , CONST_STRPTR, ___file, a0, CONST struct arp2_timeval*, ___tvp, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_lutimes(___file, ___tvp) \
	LP2(0x5dc, LONG, arp2sys_lutimes , CONST_STRPTR, ___file, a0, CONST struct arp2_timeval*, ___tvp, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_futimes(___fd, ___tvp) \
	LP2(0x5e2, LONG, arp2sys_futimes , LONG, ___fd, d0, CONST struct arp2_timeval*, ___tvp, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_times(___buffer) \
	LP1(0x5ee, arp2_clock_t, arp2sys_times , struct arp2_tms*, ___buffer, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_readv(___fd, ___iovec, ___count) \
	LP3(0x5f4, LONG, arp2sys_readv , LONG, ___fd, d0, CONST struct arp2_iovec*, ___iovec, a0, LONG, ___count, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_writev(___fd, ___iovec, ___count) \
	LP3(0x5fa, LONG, arp2sys_writev , LONG, ___fd, d0, CONST struct arp2_iovec*, ___iovec, a0, LONG, ___count, d1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_uname(___name) \
	LP1(0x600, LONG, arp2sys_uname , struct arp2_utsname*, ___name, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_wait(___stat_loc) \
	LP1(0x606, arp2_pid_t, arp2sys_wait , LONG*, ___stat_loc, a0,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_waitid(___idtype, ___id, ___infop, ___options) \
	LP4(0x60c, LONG, arp2sys_waitid , arp2_idtype_t, ___idtype, d0, arp2_id_t, ___id, d1, arp2_siginfo_t*, ___infop, a0, LONG, ___options, d2,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_wait3(___stat_loc, ___options, ___usage) \
	LP3(0x612, arp2_pid_t, arp2sys_wait3 , LONG*, ___stat_loc, a0, LONG, ___options, d0, struct arp2_rusage*, ___usage, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#define arp2sys_wait4(___pid, ___stat_loc, ___options, ___usage) \
	LP4(0x618, arp2_pid_t, arp2sys_wait4 , arp2_pid_t, ___pid, d0, LONG*, ___stat_loc, a0, LONG, ___options, d1, struct arp2_rusage*, ___usage, a1,\
	, ARP2_SYSCALL_BASE_NAME)

#endif /* !_INLINE_ARP2_SYSCALL_H */
