;/*
* $Id$
* $Log$
* Revision 4.4  1998/01/12 20:05:03  lcs
* More restruction, mixer in C added. (Just about to make fraction 32 bit!)
*
* Revision 4.3  1997/12/21 17:41:50  lcs
* Major source cleanup, moved some functions to separate files.
*
* Revision 4.2  1997/04/14 01:50:39  lcs
* Spellchecked
*
* Revision 4.1  1997/04/02 22:29:53  lcs
* Bumped to version 4
*
* Revision 1.4  1997/03/24 12:41:51  lcs
* Echo rewritten
*
* Revision 1.3  1997/02/18 22:26:49  lcs
* Fixed a bug in CMD_READ?
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
	IF	0

*******************************************************************************
** C function prototypes ******************************************************
*******************************************************************************

*/

#include <CompilerSpecific.h>
#include <devices/ahi.h>

#define RecArgs REG(d0, ULONG size),    \
                REG(d1, ULONG add),     \
                REG(a0, APTR src),      \
                REG(a2, ULONG *offset), \
                REG(a3, void **dest)

void ASMCALL  RecM8S( RecArgs ) {}
void ASMCALL  RecS8S( RecArgs ) {}
void ASMCALL RecM16S( RecArgs ) {}
void ASMCALL RecS16S( RecArgs ) {}
void ASMCALL RecM32S( RecArgs ) {}
void ASMCALL RecS32S( RecArgs ) {}

ULONG ASMCALL MultFixed ( REG(d0, ULONG a),
                          REG(d1, Fixed b) ) {}
void ASMCALL asmRecordFunc ( REG(d0, ULONG samples),
                             REG(a0, void *data),
                             REG(a1, void *buffer) ) {}

;/*     Comment terminated at the end of the file!

	ENDC

*******************************************************************************
** Assembly code **************************************************************
*******************************************************************************

	include exec/types.i
	include	macros.i

	XDEF	_RecM8S
	XDEF	_RecS8S
	XDEF	_RecM16S
	XDEF	_RecS16S
	XDEF	_RecM32S
	XDEF	_RecS32S

	XDEF	_MultFixed
	XDEF	_asmRecordFunc

* Not the best routines (fraction does not get saved between calls,
* loads of byte writes, no interpolation etc), but who cares? 

;in:
* d0	Number of SAMPLES to fill. (Max 131071)
* d1	Add interger.fraction in samples (2�16 bit)
* a0	Source (AHIST_S16S)
* a2	Pointer to Source Offset in bytes (will be updated)
* a3	Pointer to Destination (will be updated)
;out

Cnt	EQUR	d0
Tmp	EQUR	d1
OffsI	EQUR	d2
OffsF	EQUR	d3
AddI	EQUR	d4
AddF	EQUR	d5
Src	EQUR	a0
Dst	EQUR	a1

*******************************************************************************
** RecM8S *********************************************************************
*******************************************************************************

_RecM8S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecS8S *********************************************************************
*******************************************************************************

_RecS8S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
	move.b	2(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
	move.b	2(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
	move.b	2(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
	move.b	2(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecM16S ********************************************************************
*******************************************************************************

_RecM16S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecS16S ********************************************************************
*******************************************************************************

_RecS16S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.l	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.l	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.l	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.l	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecM32S ********************************************************************
*******************************************************************************

_RecM32S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	clr.w	(Dst)+
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	clr.w	(Dst)+
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecS32S ********************************************************************
*******************************************************************************

_RecS32S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** MultFixed ******************************************************************
*******************************************************************************

;in:
* d0	unsigned long
* d1	Fixed
;out:
* d0	d0*d1
_MultFixed:
	push	d2
	move.l	d0,d2
	move.l	d1,d0
	beq	.exit			;Sanity check
	swap.w	d2
	moveq	#0,d1
	move.w	d2,d1
	clr.w	d2
* d2:d0 is now d0<<16
 IFGE	__CPU-68020
	divu.l	d0,d1:d2
	move.l	d2,d0
 ELSE
	XREF	_UDivMod64
	bsr	_UDivMod64		;d0 = (d1:d2)/d0
 ENDC
;	addq.l	#1,d0
.exit
	pop	d2
	rts

*******************************************************************************
** RecordFunc *****************************************************************
*******************************************************************************

* This function is not used, but can be used to fill a large record buffer

;in:
* d0	Sample frames in buffer
* a0	Pointer to struct RD
* a1	The samples themselves

	STRUCTURE RD,0
	APTR	buffer1				;Address of first buffer
	APTR	buffer2				;Address of second buffer
	ULONG	bufferlen			;Length in samples
	APTR	task				;Task to signal when buffer is full
	ULONG	signalmask			;Signal mask
	ULONG	offs				;My variable
	ULONG	count				;My variable

;	struct {
;	 APTR			 buffer1;
;	 APTR			 buffer2;
;	 ULONG			 bufferlen;
;	 APTR			 task;
;	 ULONG			 signalmask;
;	 ULONG			 offs;
;	 ULONG			 count;
;	}			 ahidu_RecordData;

_asmRecordFunc:

	IF	0

	move.l	a2,-(sp)

	move.l	buffer1(a0),a2			; Our own buffer
	add.l	offs(a0),a2

	cmp.l	count(a0),d0			; Will all samples fit in our buffer?
	bls.b	3$				; Branch if yes.

; Fill buffer until it's full
	move.l	count(a0),d1			; Number of samples left
	beq.b	2$				; Buffer already full? Skip 1st pass.
	sub.l	d1,d0				; d0 = # of samples in second pass.
1$
	move.l	(a1)+,(a2)+			; Move both left & right word
	subq.l	#1,d1				; We can't use dbf/dbra here.
	bne.b	1$
2$
	move.l	buffer2(a0),a2			; Swap pointers to 1st and 2nd buffer
	move.l	buffer1(a0),buffer2(a0)
	move.l	a2,buffer1(a0)
	move.l	bufferlen(a0),count(a0)		; Init count
	clr.l	offs(a0)
	movem.l	d0/a0-a1/a6,-(sp)
	move.l	4.w,a6
	move.l	task(a0),a1
	move.l	signalflag(a0),d0
	jsr	_LVOSignal(a6)			; Tell main task buffer is ready
	movem.l	(sp)+,d0/a0-a1/a6

* Now fill the rest of the new buffer and exit.

3$
	sub.l	d0,count(a0)			; Update count
	move.l	d0,d1
	lsl.l	#2,d1
	add.l	d1,offs(a0)
4$
	move.l	(a1)+,(a2)+			; Move both left & right word
	subq.l	#1,d0
	bne.b	4$

	move.l	(sp)+,a2

	ENDC

	rts

;	C comment terminating here... */


