#objdump: -dr
#name: ld/lr

# Test the ld/lr insn.

.*: +file format elf32-.*arc

Disassembly of section .text:
00000000 08008000	ld r0,\[r1\]
00000004 00418800	ld r2,\[r3,r4\]
00000008 08a30001	ld r5,\[r6,1\]
0000000c 08e401ff	ld r7,\[r8,-1\]
00000010 092500ff	ld r9,\[r10,255\]
00000014 09660100	ld r11,\[r12,-256\]
00000018 01a77c00	ld r13,\[r14,256\]
00000020 01e87c00	ld r15,\[r16,-257\]
00000028 023f3800	ld r17,\[305419896,sp\]
00000030 0a7f0000	ld r19,\[0\]
		RELOC: 00000034 R_ARC_32 foo
00000038 0a9f0000	ld r20,\[4\]
		RELOC: 0000003c R_ARC_32 foo
00000040 081f8400	ldb r0,\[0\]
00000044 081f8800	ldw r0,\[0\]
00000048 081f8200	ld.x r0,\[0\]
0000004c 081f9000	ld.a r0,\[0\]
00000050 081fc000	ld.di r0,\[0\]
00000054 08005600	ldb.x.a.di r0,\[r0\]
00000058 0800a000	lr r0,\[r1\]
0000005c 085fa000	lr r2,\[status\]
00000060 087f2000	lr r3,\[305419896\]
