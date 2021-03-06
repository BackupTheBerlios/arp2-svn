/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2003 Martin Blom <martin@blom.org>
     
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

	.global	_EchoMono16
	.type	_EchoMono16,@func
	.global	_EchoStereo16
	.type	_EchoStereo16,@func
	.global	_EchoMono32
	.type	_EchoMono32,@func
	.global	_EchoStereo32
	.type	_EchoStereo32,@func


ahiecho_Delay		.equ	0
ahiecho_Code		.equ	4
ahiecho_FeedbackDS	.equ	8
ahiecho_FeedbackDO	.equ	12
ahiecho_FeedbackNS	.equ	16
ahiecho_FeedbackNO	.equ	20
ahiecho_MixN		.equ	24
ahiecho_MixD		.equ	28
ahiecho_Offset		.equ	32
ahiecho_SrcPtr		.equ	36
ahiecho_DstPtr		.equ	40
ahiecho_EndPtr		.equ	44
ahiecho_BufferLength	.equ	48
ahiecho_BufferSize	.equ	52
ahiecho_Buffer		.equ	56
	
/**
*** DSPECHO
**/

/*
void
EchoXX( LONG          loops,
        struct Echo  *es,
        void        **buffer,
        void        **srcptr,
        void        **dstptr)
*/

	.MACRO	DSPECHO_PRE
	movem.l	d0-a6,-(sp)
	move.l	16*4+0(sp),d7
	move.l	16*4+4(sp),a0
	move.l	16*4+8(sp),a2
	move.l	(a2),a1
	move.l	16*4+12(sp),a4
	move.l	(a4),a3
	move.l	16*4+16(sp),a5
	move.l	(a5),a6

	subq.l	#1,d7
	bmi	2f
	.ENDM



	.MACRO	DSPECHO_POST
2:
	move.l	a1,(a2)
	move.l	a3,(a4)
	move.l	a6,(a5)
	movem.l	(sp)+,d0-a6
	rts
	.ENDM


_EchoMono16:
	DSPECHO_PRE

/* in:
* a0	struct Echo*
* a1	& x[n]
* a3	& d[n-N]
* a6	& d[n]
*/

1:
	move.w	(a1),d0				/* Get sample x[n] */
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			/* Get delayed sample d[n-N] */
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	swap.w	d3
	move.w	d3,(a1)+			/* x[n]=MixN*x[n]+MixD*d[n-N] */

	addq.l	#1,d2				/* Fix for -1 */
	muls.l	ahiecho_FeedbackDS(a0),d2	/* d2=FeedbackDS*d[n-N] */

	muls.l	ahiecho_FeedbackNS(a0),d1
	add.l	d1,d2				/* d2=...+FeedbackNS*x[n] */
	swap.w	d2

	move.w	d2,(a6)+			/* store d2 */
	dbf	d7,1b
	DSPECHO_POST


_EchoStereo16:
	DSPECHO_PRE

/* in:
* a0	struct Echo*
* a1	& x[n]
* a3	& d[n-N]
* a6	& d[n]
*/

1:	
	move.w	(a1),d0				/* Get left sample x[n] */
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			/* Get left delayed sample d[n-N]*/
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	swap.w	d3
	move.w	d3,(a1)+			/* x[n]=MixN*x[n]+MixD*d[n-N] */

	move.w	(a1),d0				/* Get right sample x[n] */
	ext.l	d0
	move.l	d0,d4
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			/*Get right delayed sample d[n-N]*/
	ext.l	d3
	move.l	d3,d5
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	swap.w	d3
	move.w	d3,(a1)+			/* x[n]=MixN*x[n]+MixD*d[n-N] */

	addq.l	#1,d2				/* Fix for -1 */
	move.l	d2,d0
	muls.l	ahiecho_FeedbackDS(a0),d2	/* d2=FeedbackDS*d[n-N] (left) */
	muls.l	ahiecho_FeedbackDO(a0),d0

	addq.l	#1,d5				/* Fix for -1 */
	move.l	d5,d3
	muls.l	ahiecho_FeedbackDS(a0),d5	/* d5=FeedbackDS*d[n-N] (right) */
	muls.l	ahiecho_FeedbackDO(a0),d3

	add.l	d3,d2				/* d2=...+FeedbackDO*d[n-N] */
	add.l	d0,d5				/* d5=...+FeedbackDO*d[n-N] */

	move.l	d1,d0
	muls.l	ahiecho_FeedbackNS(a0),d0
	add.l	d0,d2				/* d2=...+FeedbackNS*x[n] */
	muls.l	ahiecho_FeedbackNO(a0),d1
	add.l	d1,d5				/* d5=...+FeedbackNO*x[n] */

	move.l	d4,d0
	muls.l	ahiecho_FeedbackNO(a0),d0
	add.l	d0,d2				/* d2=...+FeedbackNO*x[n] */
	muls.l	ahiecho_FeedbackNS(a0),d4
	add.l	d4,d5				/* d5=...+FeedbackNS*x[n] */

	swap.w	d5
	move.w	d5,d2
	move.l	d2,(a6)+			/* store d2 and d5 */
	dbf	d7,1b
	DSPECHO_POST
		

_EchoMono32:
	DSPECHO_PRE

/* in:
* a0	struct Echo*
* a1	& x[n]
* a3	& d[n-N]
* a6	& d[n]
*/

/** The delay buffer is only 16 bit, and only the upper word of the 32 bit
*** input data is used! */

1:
	move.w	(a1),d0				/* Get sample x[n] (high word) */
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			/* Get delayed sample d[n-N] */
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			/* x[n]=MixN*x[n]+MixD*d[n-N] */

	addq.l	#1,d2				/* Fix for -1 */
	muls.l	ahiecho_FeedbackDS(a0),d2	/* d2=FeedbackDS*d[n-N] */

	muls.l	ahiecho_FeedbackNS(a0),d1
	add.l	d1,d2				/* d2=...+FeedbackNS*x[n] */

	swap.w	d2
	move.w	d2,(a6)+			/* store d2 */
	dbf	d7,1b
	DSPECHO_POST


_EchoStereo32:
	DSPECHO_PRE

/* in:
* a0	struct Echo*
* a1	& x[n]
* a3	& d[n-N]
* a6	& d[n]
*/

/** The delay buffer is only 16 bit, and only the upper word of the 32 bit
*** input data is used! */

1:
	move.w	(a1),d0				/*Get left sample x[n] (high word)*/
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			/* Get left delayed sample d[n-N]*/
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			/* x[n]=MixN*x[n]+MixD*d[n-N] */

	move.w	(a1),d0				/* Get right sample x[n] (high word) */
	ext.l	d0
	move.l	d0,d4
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			/*Get right delayed sample d[n-N]*/
	ext.l	d3
	move.l	d3,d5
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			/* x[n]=MixN*x[n]+MixD*d[n-N] */

	addq.l	#1,d2				/* Fix for -1 */
	move.l	d2,d0
	muls.l	ahiecho_FeedbackDS(a0),d2	/* d2=FeedbackDS*d[n-N] (left) */
	muls.l	ahiecho_FeedbackDO(a0),d0

	addq.l	#1,d5				/* Fix for -1 */
	move.l	d5,d3
	muls.l	ahiecho_FeedbackDS(a0),d5	/* d5=FeedbackDS*d[n-N] (right) */
	muls.l	ahiecho_FeedbackDO(a0),d3

	add.l	d3,d2				/* d2=...+FeedbackDO*d[n-N] */
	add.l	d0,d5				/* d5=...+FeedbackDO*d[n-N] */

	move.l	d1,d0
	muls.l	ahiecho_FeedbackNS(a0),d0
	add.l	d0,d2				/* d2=...+FeedbackNS*x[n] */
	muls.l	ahiecho_FeedbackNO(a0),d1
	add.l	d1,d5				/* d5=...+FeedbackNO*x[n] */

	move.l	d4,d0
	muls.l	ahiecho_FeedbackNO(a0),d0
	add.l	d0,d2				/* d2=...+FeedbackNO*x[n] */
	muls.l	ahiecho_FeedbackNS(a0),d4
	add.l	d4,d5				/* d5=...+FeedbackNS*x[n] */

	swap.w	d5
	move.w	d5,d2
	move.l	d2,(a6)+			/* store d1 and d4 */
	dbf	d7,1b
	DSPECHO_POST

	.end