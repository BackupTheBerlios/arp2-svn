//#include <syscalls.h>
#include <powerup/ppcproto/exec.h>

asm("	.section \".text\"
	nop
	lis	11,__SaveSP@ha
	stw	1,__SaveSP@l(11)
	lis	11,__SaveLR@ha
	mflr	0
	stw	0,__SaveLR@l(11)
	b	__start
	");

char *__commandline;
unsigned long __commandlen;
struct ExecBase *SysBase;
void *__SaveSP;
void *__SaveLR;
int __argc;
char **__argv;
struct Library *DOSBase;

struct func
{
	void (*f)(void);
	unsigned int pri;
};

void exit(int error) __attribute__ ((noreturn));
void _exit(int error) __attribute__ ((noreturn));
void callfuncs(struct func *list,unsigned int flag);
extern struct func __initlist[];
extern struct func __exitlist[];
extern unsigned long __ctrslist[];
extern unsigned long __dtrslist[];
int main(int argc,char *argv[]);

void __start(char *cline)
{
	int i=0;
	char *cptr=cline;
	if(cline!=0)
		while(*cptr++)
			i++;
	__commandlen=i;
	__commandline=cline;
	SysBase=*(void **)4L;
	DOSBase=OpenLibrary("dos.library",37);
	callfuncs(__initlist,-1);
	i=0;
	while(__ctrslist[i]!=0)
	{
		void (*fp)()=__ctrslist[i++];
		fp();
	}
	exit(main(__argc,__argv));
}

int __Saveerror;

void exit(int error)
{
	int i;
	__Saveerror=error;
	i=0;
	while(__dtrslist[i]!=0)
	{
		void (*fp)()=__dtrslist[i++];
		fp();
	}
	callfuncs(__exitlist,0);
	CloseLibrary(DOSBase);
	asm("
	lis	11,__Saveerror@ha
	lwz	3,__Saveerror@l(11)
	lis	11,__SaveSP@ha
	lwz	1,__SaveSP@l(11)
	lis	11,__SaveLR@ha
	lwz	0,__SaveLR@l(11)
	mtlr	0
	blr
	");
}

void _exit(int error)
{
	int i;
	__Saveerror=error;
	i=0;
	while(__dtrslist[i]!=0)
	{
		void (*fp)()=__dtrslist[i++];
		fp();
	}
	callfuncs(__exitlist,0);
	CloseLibrary(DOSBase);
	asm("
	lis	11,__Saveerror@ha
	lwz	3,__Saveerror@l(11)
	lis	11,__SaveSP@ha
	lwz	1,__SaveSP@l(11)
	lis	11,__SaveLR@ha
	lwz	0,__SaveLR@l(11)
	mtlr	0
	blr
	");
}

unsigned int cleanupflag=0;

void callfuncs(struct func *a2,unsigned int d2)
{
	struct func *a3;
	unsigned int d3,d4,d5;
	void (*f)();
	do
	{
		a3=a2;
		d3=0;
		while((f=a3->f)!=0L)
		{
			d4=a3++->pri;
			d5=cleanupflag;
			if(d4==d5)
				f();
			d4^=d2;
			d5^=d2;
			if((d4<d5)&&(d4>d3))
				d3=d4;
		}
	}while(d3^=d2,cleanupflag=d3,d2!=d3);
}

asm("
	.long __nocommandline
	");

asm("
	.section \".init\",\"a\"
__initlist:
	");
asm("
	.section \".fini\",\"a\"
__exitlist:
	");
asm("
	.section \".ctors\",\"a\"
__ctrslist:
	");
asm("
	.section \".dtors\",\"a\"
__dtrslist:
	");
