| residentable baserelative startup code for gcc v2.3.3+
| (c) by M.Fleischer and G.Nikl Wed Apr 13 17:46 1994
| No bugs known
|
| Problem: geta4()
| I really don´t know how to implement it with multiple starts
|
| You should use the non-resident startup-code if you need it !

| some feature macros

#define REL(x) a4@(x/**/:W)
#define SYS(x) jsr a6@(_LVO/**/x/**/:W)

| some specific defines

_LVOForbid	=	-132
_LVOAllocMem	=	-198
_LVOFreeMem	=	-210
_LVOFindTask	=	-294
_LVOGetMsg	=	-372
_LVOReplyMsg	=	-378
_LVOWaitPort	=	-384
_LVOCopyMemQuick =	-630

pr_MsgPort	=	  92
pr_CLI		=	 172

ERROR_NO_FREE_STORE =	 103

| public symbols

		.globl	__exit

		.text

| first entry - check for cli or wb start

start:		movel	a0,d3
		movel	d0,d4

		movel	4:W,a6

		subal	a1,a1
		SYS	(FindTask)
		movel	d0,a3
		tstl	a3@(pr_CLI:W)
		subal	a2,a2
		bne	fromCLI

| wb start - get our message

fromWB:		lea	a3@(pr_MsgPort:W),a0
		SYS	(WaitPort)
		lea	a3@(pr_MsgPort:W),a0
		SYS	(GetMsg)
		movel	d0,a2

| resident setup - allocate memory and relocate

fromCLI:	movql	#ERROR_NO_FREE_STORE,d7
		movel	#___data_size,d5
		movel	#___bss_size,d2
		addl	d5,d2
		movel	d2,d0
		movql	#1,d1
		swap	d1
		SYS	(AllocMem)
		movel	d0,a4
		movel	a4,d0
		beq	cleanup
		lea	___a4_init-0x7ffe,a5
		movel	d5,d0
		movel	a5,a0
		movel	a4,a1
		SYS	(CopyMemQuick)
		lea	___datadata_relocs,a0
		movel	a0@+,d0
		beq	no_reloc
		movel	a5,d1
		subl	a4,d1
do_reloc:	movel	a0@+,a1
		subl	d1,a4@(a1:l)
		subql	#1,d0
		bne	do_reloc
no_reloc:	lea	a4@(0x7ffe:W),a4

		movel	sp,REL(___SaveSP)
		movel	a6,REL(_SysBase)
		movel	a2,REL(__WBenchMsg)
		movel	d3,REL(___commandline)
		movel	d4,REL(___commandlen)
		movel	d2,REL(___memlen)

| call main() - push args onto stack

		lea	REL(___INIT_LIST__),a2
		moveql	#-1,d2
		jbsr	callfuncs

		movel	REL(___env),sp@-
		movel	REL(___argv),sp@-
		movel	REL(___argc),sp@-
		jbsr	_main
		movel	d0,sp@(4:W)

| exit() entry - get return val, restore stackptr

__exit:		lea	REL(___EXIT_LIST__),a2
		moveql	#0,d2
		jbsr	callfuncs

		movel	sp@(4:W),d7
		movel	REL(___SaveSP),sp

| prepare cleanup - setup regs, free resident memory

		movel	REL(_SysBase),a6

		movel	REL(__WBenchMsg),a2
		movel	REL(___memlen),d0
		lea	a4@(-0x7ffe:W),a1
		SYS	(FreeMem)

| cleanup - reply wbmsg and exit

cleanup:	movel	a2,d0
		beqs	todos
		SYS	(Forbid)
		movel	a2,a1
		SYS	(ReplyMsg)
todos:		movel	d7,d0
		rts

| call all functions in the NULL terminated list pointed to by a2
| d2 ascending or descending priority mode

callfuncs:	addql	#4,a2
callloop:	lea	REL(cleanupflag),a5
		movel	a2,a3
		moveql	#0,d3
		jra	oldpri
stabloop:	movel	a3@+,d4
		movel	a5@,d5
		cmpl	d4,d5
		jne	notnow
		movel	d0,a0
		jsr	a0@
notnow:		eorl	d2,d4
		eorl	d2,d5
		cmpl	d5,d4
		jcc	oldpri
		cmpl	d3,d4
		jls	oldpri
		movel	d4,d3
oldpri:		movel	a3@+,d0
		jne	stabloop
		eorl	d2,d3
		movel	d3,a5@
		cmpl	d2,d3
		jne	callloop
		rts

| redirection of _exit

		.stabs	"_exit",11,0,0,0
		.stabs	"__exit",1,0,0,0

| data area

		.data

		.long ___nocommandline
		.long ___initlibraries
		.long ___cpucheck

.comm		_SysBase,4
.comm		___SaveSP,4
.comm		__WBenchMsg,4
.comm		___commandline,4
.comm		___commandlen,4
.comm		___argc,4
.comm		___argv,4
.comm		___env,4
.lcomm		___memlen,4
.lcomm		cleanupflag,4
