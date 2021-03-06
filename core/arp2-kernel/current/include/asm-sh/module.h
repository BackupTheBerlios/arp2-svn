#ifndef _ASM_SH_MODULE_H
#define _ASM_SH_MODULE_H

/*
 * This file contains the SH architecture specific module code.
 */

struct mod_arch_specific {
	/* Nothing to see here .. */
};

#define MODULES_ARE_ELF32
#define Elf_Shdr		Elf32_Shdr
#define Elf_Sym			Elf32_Sym
#define Elf_Ehdr		Elf32_Ehdr
#define Elf_Rel			Elf32_Rel
#define Elf_Rela		Elf32_Rela
#define ELF_R_TYPE(X)	ELF32_R_TYPE(X)
#define ELF_R_SYM(X)	ELF32_R_SYM(X)

#ifdef CONFIG_CPU_LITTLE_ENDIAN
# ifdef CONFIG_CPU_SH2
#  define MODULE_PROC_FAMILY "SH2LE "
# elif defined  CONFIG_CPU_SH3
#  define MODULE_PROC_FAMILY "SH3LE "
# elif defined  CONFIG_CPU_SH4
#  define MODULE_PROC_FAMILY "SH4LE "
# else
#  error unknown processor family
# endif
#else
# ifdef CONFIG_CPU_SH2
#  define MODULE_PROC_FAMILY "SH2BE "
# elif defined  CONFIG_CPU_SH3
#  define MODULE_PROC_FAMILY "SH3BE "
# elif defined  CONFIG_CPU_SH4
#  define MODULE_PROC_FAMILY "SH4BE "
# else
#  error unknown processor family
# endif
#endif

#define MODULE_ARCH_VERMAGIC MODULE_PROC_FAMILY

#endif /* _ASM_SH_MODULE_H */
