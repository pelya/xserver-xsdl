/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/misc/PortIO.S,v 1.2 2004/03/05 04:02:14 dawes Exp $ */

/* Port I/O functions for platforms with no inlining. */

#if defined(i386)

#include "assyntax.h"

	FILE("PortIO.s")

	AS_BEGIN

	GLOBL	GLNAME(outb)
	GLOBL	GLNAME(outw)
	GLOBL	GLNAME(outl)
	GLOBL	GLNAME(inb)
	GLOBL	GLNAME(inw)
	GLOBL	GLNAME(inl)

	SEG_TEXT
	ALIGNTEXT4
GLNAME(outb):
	MOV_L	(REGOFF(4,ESP), EDX)
	MOV_L	(REGOFF(8,ESP), EAX)
	OUT_B
	RET

	ALIGNTEXT4
GLNAME(outw):
	MOV_L	(REGOFF(4,ESP), EDX)
	MOV_L	(REGOFF(8,ESP), EAX)
	OUT_W
	RET

	ALIGNTEXT4
GLNAME(outl):
	MOV_L	(REGOFF(4,ESP), EDX)
	MOV_L	(REGOFF(8,ESP), EAX)
	OUT_L
	RET

	ALIGNTEXT4
GLNAME(inb):
	MOV_L	(REGOFF(4,ESP), EDX)
	IN_B
	RET

	ALIGNTEXT4
GLNAME(inw):
	MOV_L	(REGOFF(4,ESP), EDX)
	IN_L
	RET

	ALIGNTEXT4
GLNAME(inl):
	MOV_L	(REGOFF(4,ESP), EDX)
	IN_L
	RET

#elif defined(sparc)

#ifndef ASI_PL
#define ASI_PL	136
#endif

	.file "PortIO.s"

	.global outb
	.global outw
	.global outl
	.global inb
	.global inw
	.global inl

	.section	".text"

	.align 4
outb:
	stba %o1, [%o0] ASI_PL
	membar 10
	retl
	nop

	.align 4
outw:
	stha %o1, [%o0] ASI_PL
	membar 10
	retl
	nop

	.align 4
outl:
	sta %o1, [%o0] ASI_PL
	membar 10
	retl
	nop

	.align 4
inb:
	lduba [%o0] ASI_PL, %o0
	retl
	nop

	.align 4
inw:
	lduha [%o0] ASI_PL, %o0
	retl
	nop

	.align 4
inl:
	lda [%o0] ASI_PL, %o0
	retl
	nop

#endif

