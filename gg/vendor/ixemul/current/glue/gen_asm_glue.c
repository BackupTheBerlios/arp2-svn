#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ixemul_syscall.h>

struct syscall {
  char *name;
  int   vec;
} syscalls[] = {
#define SYSTEM_CALL(func,vec,args) { #func, vec},
#include <sys/ixemul_syscall.def>
#undef SYSTEM_CALL
};

int nsyscall = sizeof(syscalls) / sizeof (syscalls[0]);

static char m68k_amigaos[] =
"
	.globl	_%s
_%s:	movel	_ixemulbase,a0
	jmp	a0@(%d:w)
";

static char m68k_amigaos_baserel[] =
"
	.globl	_%s
_%s:	movel	a4@(_ixemulbase:W),a0
	jmp	a0@(%d:w)
";


static char m68k_amigaos_baserel32[] =
"
	.globl	_%s
_%s:	movel	a4@(_ixemulbase:L),a0
	jmp	a0@(%d:w)
";


static char m68k_amigaos_profiling[] =
"
	.globl	_%s
_%s:
	.data
PROF%s:
	.long	0

	.text
	link	a5,#0
	lea	PROF%s,a0
	jsr	mcount
	unlk	a5
	movel	_ixemulbase,a0
	jmp	a0@(%d:w)
";


static char ix86_amithlon[] =
"
	.globl	%s
%s:	movl	ixemulbase,%%eax
	bswap	%%eax
	addl	%d,%%eax
	push	0
	push	0
	push	0
	push	0
	push	0
	push	%%eax
	jmp	call68k
";


void usage(void)
{
  fprintf(stderr, "Usage: gen_asm_glue baserel | large-baserel | no-baserel | profiling\n");
  exit(1);
}


int main(int argc, char **argv)
{
  FILE *fp;
  struct syscall *sc;
  int i, v;
  int baserel = 0, profiling = 0, lbaserel = 0;
  int amithlon = 0;

  if (argc != 2)
    usage();
  if (!strcmp(argv[1], "amithlon"))
    amithlon = 1;
  else if (!strcmp(argv[1], "baserel"))
    baserel = 1;
  else if (!strcmp(argv[1], "baserel32"))
    lbaserel = 1;
  else if (!strcmp(argv[1], "profiling"))
    profiling = 1;
  else if (strcmp(argv[1], "no-baserel"))
    usage();
  
  for (i = 0, sc = syscalls; i < nsyscall; i++, sc++)
  {
    int namelen = strlen(sc->name);
    char name[namelen + 3];

    if (!memcmp(sc->name, "__obsolete", 10))
      continue;
    if (!memcmp(sc->name, "__must_recompile", 16))
      continue;
    if (!memcmp(sc->name, "__stk", 5))
      continue;
    v = -(sc->vec + 4) * 6;
    sprintf (name, "%s.s", sc->name);

    fp = fopen (name, "w");
      
    if (!fp)
    {
      perror (sc->name);
      exit (20);
    }

    if (profiling)
    {
      fprintf( fp, m68k_amigaos_profiling,
	       sc->name, sc->name, sc->name, sc->name, v );
    }
    else
    {
      if (amithlon)
      {
	  fprintf( fp, ix86_amithlon,
		   sc->name, sc->name, v );
      }
      else
      {
	if (baserel && sc->vec != SYS_ix_geta4)
	{
	  fprintf( fp, m68k_amigaos_baserel,
		   sc->name, sc->name, v );
	}
	else if (lbaserel && sc->vec != SYS_ix_geta4)
	{
	  fprintf( fp, m68k_amigaos_baserel32,
		   sc->name, sc->name, v );
	}
	else
	  fprintf( fp, m68k_amigaos, sc->name, sc->name, v );
      }
    }
    fclose (fp);
  }
  exit(0);
}
