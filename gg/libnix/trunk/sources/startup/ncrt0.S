| normal and baserelative startup code for gcc v2.3.3+
| (c) by M.Fleischer and G.Nikl Wed Apr 13 17:45 1994
| No bugs known

| some feature macros

#if !defined(SMALL_DATA)
#define REL(x) x
#else
#define REL(x) a4@(x/**/:W)
#endif
#define SYS(x) jsr a6@(_LVO/**/x/**/:W)

| some specific defines

_LVOForbid	=	-132
_LVOFindTask	=	-294
_LVOGetMsg	=	-372
_LVOReplyMsg	=	-378
_LVOWaitPort	=	-384

pr_MsgPort	=	  92
pr_CLI		=	 172

| public symbols

		.globl	__exit
		.globl	_geta4

		.text

| first entry - init some vars, check for cli or wb start

start:

#if defined(SMALL_DATA)

		movel	#___bss_size,d1
		jbsr	_geta4
		beqs	nobss
		lea	REL(__edata),a1
clrbss:		clrl	a1@+
		subql	#4,d1
		bnes	clrbss
nobss:

#endif

		movel	a0,REL(___commandline)
		movel	d0,REL(___commandlen)

		movel	sp,REL(___SaveSP)
		movel	4:W,a6
		movel	a6,REL(_SysBase)

		subal	a1,a1
		SYS	(FindTask)
		movel	d0,a3
		tstl	a3@(pr_CLI:W)
		bne	fromCLI

| wb start - get wbmsg

fromWB:		lea	a3@(pr_MsgPort:W),a0
		SYS	(WaitPort)
		lea	a3@(pr_MsgPort:W),a0
		SYS	(GetMsg)
		movel	d0,REL(__WBenchMsg)

| execute all init functions then call main

fromCLI:	lea	REL(___INIT_LIST__),a2
		moveql	#-1,d2
		jbsr	callfuncs

		movel	REL(___env),sp@-
		movel	REL(___argv),sp@-
		movel	REL(___argc),sp@-
		jbsr	_main
		movel	d0,sp@(4:W)

| exit() entry - execute all exit functions, reply wbmsg

__exit:		lea	REL(___EXIT_LIST__),a2
		moveql	#0,d2
		jbsr	callfuncs

		movel	REL(_SysBase),a6

		movel	REL(__WBenchMsg),d2
		beq	todos
		SYS	(Forbid)
		movel	d2,a1
		SYS	(ReplyMsg)

| leave - get return val, restore stackptr

todos:		movel	sp@(4:W),d0
		movel	REL(___SaveSP),sp
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

#if defined(SMALL_DATA)

		rts

| get baseptr

_geta4:		lea	___a4_init,a4
		rts

#else

| geta4() doesn´t do anything, but enables you to use
| one source for both code models

_geta4:		rts

#endif

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
.lcomm		cleanupflag,4
