/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2000 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/* m68k:

LONG	  Samples		 4(sp) long		Samples
LONG	  ScaleLeft		 8(sp) long		ScaleLeft
LONG	  ScaleRight		12(sp) long		ScaleRight
LONG	 *StartPointLeft	16(sp) long		StartPointLeft
LONG	 *StartPointRight	20(sp) long		StartPointRight
void	 *Src			24(sp) long		Src
void	**Dst			28(sp) long		Dst
LONG	  FirstOffsetI		32(sp) long		FirstOffsetI
Fixed64	  Add			36(sp) long long	Add
Fixed64	 *Offset		44(sp) long		Offset
BOOL	  StopAtZero		50(sp) word		StopAtZero

*/

Samples		.equ	11*4 + 4
ScaleLeft	.equ	11*4 + 8
ScaleRight	.equ	11*4 + 12
StartPointLeft	.equ	11*4 + 16
StartPointRight	.equ	11*4 + 20
Src		.equ	11*4 + 24
Dst		.equ	11*4 + 28
FirstOffsetI	.equ	11*4 + 32
AddI		.equ	11*4 + 36
AddF		.equ	11*4 + 40
Offset		.equ	11*4 + 44
StopAtZero	.equ	11*4 + 50

/*

Register usage:

d0	counter
d1	scaleleft
d2	scaleright
d3	int offset
d4.w	fract offset
d5	int add
d6.w	fract offset
a0	src
a1	dst
a2	firstoffset
a5	left lastpoint
a6	right lastpoint

*/

	.text

	.globl	_AddByteMono
	.globl	_AddByteStereo
	.globl	_AddBytesMono
	.globl	_AddBytesStereo
	.globl	_AddWordMono
	.globl	_AddWordStereo
	.globl	_AddWordsMono
	.globl	_AddWordsStereo
	.globl	_AddByteMonoB
	.globl	_AddByteStereoB
	.globl	_AddBytesMonoB
	.globl	_AddBytesStereoB
	.globl	_AddWordMonoB
	.globl	_AddWordStereoB
	.globl	_AddWordsMonoB
	.globl	_AddWordsStereoB

AddSilenceMono:
 .if	CPU < 68060
	move.w	d5,d1			/* AddI<65535 */
	swap.w	d1
	move.w	d6,d1			/* d1=Add<<16  */
	mulu.l	d0,d2:d1
	add.w	d1,d4			/* New OffsetF (X) */
	move.w	d2,d1
	swap.w	d1			/* d1=d2:d1>>16 */
	addx.l	d1,d3			/* New OffsetI */
 .else
	move.l	d5,d1
	mulu.l	d0,d1			/* OffsI*Samples */
	move.l	d6,d2
	add.l	d1,d3			/* New OffsetI (1) */
	mulu.l	d0,d2			/* OffsF*Samples... */
	add.w	d2,d4			/* New OffsetF (X) */
	clr.w	d2
	swap.w	d2			/* ...>>16 */
	addx.l	d2,d3			/* New OffsetI (2) */
 .endif
	lsl.l	#2,d0
	add.l	d0,a1			/* Update dst pointer */
	moveq	#0,d0
	rts

AddSilenceStereo:
 .if	CPU < 68060
	move.w	d5,d1			/* AddI<65535 */
	swap.w	d1
	move.w	d6,d1			/* d1=Add<<16  */
	mulu.l	d0,d2:d1
	add.w	d1,d4			/* New OffsetF (X) */
	move.w	d2,d1
	swap.w	d1			/* d1=d2:d1>>16 */
	addx.l	d1,d3			/* New OffsetI */
 .else
	move.l	d5,d1
	mulu.l	d0,d1			/* OffsI*Samples */
	move.l	d6,d2
	add.l	d1,d3			/* New OffsetI (1) */
	mulu.l	d0,d2			/* OffsF*Samples... */
	add.w	d2,d4			/* New OffsetF (X) */
	clr.w	d2
	swap.w	d2			/* ...>>16 */
	addx.l	d2,d3			/* New OffsetI (2) */
 .endif
	lsl.l	#3,d0
	add.l	d0,a1			/* Update dst pointer */
	moveq	#0,d0
	rts

AddSilenceMonoB:
 .if	CPU < 68060
	move.w	d5,d1			/* AddI<65535 */
	swap.w	d1
	move.w	d6,d1			/* d1=Add<<16  */
	mulu.l	d0,d2:d1
	sub.w	d1,d4			/* New OffsetF (X) */
	move.w	d2,d1
	swap.w	d1			/* d1=d2:d1>>16 */
	subx.l	d1,d3			/* New OffsetI */
 .else
	move.l	d5,d1
	mulu.l	d0,d1			/* OffsI*Samples */
	move.l	d6,d2
	sub.l	d1,d3			/* New OffsetI (1) */
	mulu.l	d0,d2			/* OffsF*Samples... */
	sub.w	d2,d4			/* New OffsetF (X) */
	clr.w	d2
	swap.w	d2			/* ...>>16 */
	subx.l	d2,d3			/* New OffsetI (2) */
 .endif
	lsl.l	#2,d0
	add.l	d0,a1			/* Update dst pointer */
	moveq	#0,d0
	rts

AddSilenceStereoB:
 .if	CPU < 68060
	move.w	d5,d1			/* AddI<65535 */
	swap.w	d1
	move.w	d6,d1			/* d1=Add<<16  */
	mulu.l	d0,d2:d1
	sub.w	d1,d4			/* New OffsetF (X) */
	move.w	d2,d1
	swap.w	d1			/* d1=d2:d1>>16 */
	subx.l	d1,d3			/* New OffsetI */
 .else
	move.l	d5,d1
	mulu.l	d0,d1			/* OffsI*Samples */
	move.l	d6,d2
	sub.l	d1,d3			/* New OffsetI (1) */
	mulu.l	d0,d2			/* OffsF*Samples... */
	sub.w	d2,d4			/* New OffsetF (X) */
	clr.w	d2
	swap.w	d2			/* ...>>16 */
	subx.l	d2,d3			/* New OffsetI (2) */
 .endif
	lsl.l	#3,d0
	add.l	d0,a1			/* Update dst pointer */
	moveq	#0,d0
	rts

	.macro	prelude

	movem.l	d2-d7/a2-a6,-(sp)
	move.l	(Samples,sp),d0			/* counter */
	move.l	(ScaleLeft,sp),d1
 	move.l	(ScaleRight,sp),d2
	move.l	(Src,sp),a0
	move.l	([Dst,sp]),a1
	move.l	(FirstOffsetI,sp),a2
	move.l	(AddI,sp),d5			/* Integer add */
	moveq	#0,d6
	move.w	(AddF,sp),d6			/* Fraction add (upper 16 bits) */
	move.l	([Offset,sp],0),d3		/* Integer offset */
	moveq	#0,d4
	move.w	([Offset,sp],4),d4		/* Fraction offset (upper 16 bits) */
	suba.l	a5,a5
	suba.l	a6,a6

	.endm


	.macro	postlude

	sub.l	(Samples,sp),d0
	neg.l	d0				/* Return Samples - d0 */
	movem.l	(sp)+,d2-d7/a2-a6

	.endm

###############################################################################

_AddByteMono:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceMono
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(-1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.b	(-1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l),d7			/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddBytesMono:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceMono
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(-2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(-1,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.b	(-2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(-1,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.b	(1,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddByteStereo:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceStereo
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(-1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.b	(-1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l),d7			/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddBytesStereo:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceStereo
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(-2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(-1,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.b	(-2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(-1,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.b	(1,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordMono:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceMono
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(-2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.w	(-2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordsMono:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceMono
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(-4,a0,d3.l*4),a3		/* sign extend */
	move.w	(-2,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.w	(-4,a0,d3.l*4),a3		/* sign extend */
	move.w	(-2,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.w	(2,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordStereo:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceStereo
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(-2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.w	(-2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordsStereo:
	prelude

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceStereo
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	addx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(-4,a0,d3.l*4),a3		/* sign extend */
	move.w	(-2,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	addx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.w	(-4,a0,d3.l*4),a3		/* sign extend */
	move.w	(-2,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.w	(2,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddByteMonoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceMonoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.b	(1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l),d7			/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddBytesMonoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceMonoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(3,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.b	(2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(3,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.b	(1,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddByteStereoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceStereoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.b	(1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l),d7			/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddBytesStereoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceStereoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.b	(2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(3,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sampleZ */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.b	(2,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
	move.b	(3,a0,d3.l*2),d7
	lsl.w	#8,d7
	move.w	d7,a4				/* sign extend */
4:	/* .got_sample */
	move.b	(0,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.b	(1,a0,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.b	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.b	(1,a0,d3.l*2),d7		/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordMonoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceMonoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.w	(2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordsMonoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceMonoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(4,a0,d3.l*4),a3		/* sign extend */
	move.w	(6,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.w	(4,a0,d3.l*4),a3		/* sign extend */
	move.w	(6,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.w	(2,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordStereoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne.b	2f
	bsr	AddSilenceStereoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge.b	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble.b	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	bra.b	4f
3:	/* .not_first */
	move.w	(2,a0,d3.l*2),a3		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*2),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a3,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*2),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

###############################################################################

_AddWordsStereoB:
	prelude
	neg.w	d4

	tst.w	(StopAtZero,sp)
	bne.b	1f
	tst.l	d1
	bne	2f
	bsr	AddSilenceStereoB
	bra	7f

0:	/* .next_sampleZ */
	add.w	d6,d4
	subx.l	d5,d3
1:	/* .first_sampleZ */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_firstZ */
	move.w	(4,a0,d3.l*4),a3		/* sign extend */
	move.w	(6,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sampleZ */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	tst.l	a5
	bgt.b	5f
	beq.b	6f
	tst.l	a3
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a3
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a3,a5				/* update lastsample */

	tst.l	a6
	bgt.b	5f
	beq.b	6f
	tst.l	a4
	bge	7f
	bra.b	6f
5:	/* .lastpoint_gtZ */
	tst.l	a4
	ble	7f
6:	/* .lastpoint_checkedZ */
	move.l	a4,a6				/* update lastsample */

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne	0b
	bra.b	8f

0:	/* .next_sample */
	add.w	d6,d4
	subx.l	d5,d3
2:	/* .first_sample */
	cmp.l	a2,d3
	bne.b	3f
	move.l	([StartPointLeft,sp]),a3
	move.l	([StartPointRight,sp]),a4
	bra.b	4f
3:	/* .not_first */
	move.w	(4,a0,d3.l*4),a3		/* sign extend */
	move.w	(6,a0,d3.l*4),a4		/* sign extend */
4:	/* .got_sample */
	move.w	(0,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a3

	move.w	(2,a0,d3.l*4),d7
	ext.l	d7
	sub.l	a4,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a4

	move.l	a3,d7
	muls.l	d1,d7
	add.l	d7,(a1)+
	move.l	a4,d7
	muls.l	d2,d7
	add.l	d7,(a1)+
	
	subq.l	#1,d0
	bne.b	0b
	bra.b	8f

7:	/* .abort */
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
8:	/* .exit */
	move.w	(0,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	move.w	(2,a0,d3.l*4),d7		/* Fetch last endpoint */
	ext.l	d7
	move.l	d7,([StartPointRight,sp])

	add.w	d6,d4
	subx.l	d5,d3

	neg.w	d4
	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	postlude
	rts

	.end
