#define HAVE_SYS_MMAN_H
#define HAVE_SYS_SYSCALL_H
#define HAVE_UNISTD_H
#define HAVE_ELF_H
#define HAVE_LINK_H
#undef HAVE_SYS_LINK_H
#define __ASM_FUNC(name) ".type " __ASM_NAME(name) ",@function"
#define __ASM_NAME(name) name
