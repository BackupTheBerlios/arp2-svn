; $Id$

_LVORawDoFmt	EQU     -522
_LVORawPutChar	EQU	-516

	XDEF	kprint_macro
	XDEF	_KPrintF
	XDEF	_kprintf
	XDEF	_VKPrintF
	XDEF	_vkprintf
	XDEF	_KPutFmt
	XDEF	KPrintF
	XDEF	kprintf
	XDEF	KPutFmt

kprint_macro:
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	KPrintF
	movem.l	(sp)+,d0-d1/a0-a1
	rts

_KPrintF:
_kprintf:
	move.l	4(sp),a0
	lea	8(sp),a1
	bra	KPrintF

_VKPrintF:
_vkprintf:
_KPutFmt:
	move.l	4(sp),a0
	move.l	8(sp),a1

KPrintF:
kprintf:
KPutFmt:
	movem.l	a2-a3/a6,-(sp)
	move.l	4.w,a6
	lea.l	RawPutChar(pc),a2
	move.l	a6,a3
	jsr	_LVORawDoFmt(a6)
	movem.l	(sp)+,a2-a3/a6
	rts

RawPutChar:
	move.l	a3,a6
	jsr	_LVORawPutChar(a6)
	rts
