/*
 * $Xorg: stip68kgnu.h,v 1.4 2001/02/09 02:04:39 xorgcvs Exp $
 *
Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * Stipple stack macro for 68k GCC
 *
 * Note - this macro can nott describe the full extent of the
 * modifications made to the arguments (GCC does not allow enough
 * arguments to __asm statements).  Therefore, it is possible
 * (though unlikely) that future magic versions of GCC may
 * miscompile this somehow.  In particular, (stipple) is modified
 * by the macro, yet not listed as an output value.
 */

#define STIPPLE(addr,stipple,value,width,count,shift) \
    __asm volatile ( \
       "subqw   #1,%1\n\
	lea	0f,a1\n\
	movew	#28,d2\n\
	addl	%7,d2\n\
	movew	#28,d3\n\
	subql	#4,%7\n\
	negl	%7\n\
1:\n\
	movel	%0,a0\n\
	addl	%5,%0\n\
	movel	%3@+,d1\n\
	beq	3f\n\
	movel	d1,d0\n\
	lsrl	d2,d0\n\
	lsll	#5,d0\n\
	lsll	%7,d1\n\
	jmp	a1@(d0:l)\n\
2:\n\
	addl	#4,a0\n\
	movel	d1,d0\n\
	lsrl	d3,d0\n\
	lsll	#5,d0\n\
	lsll	#4,d1\n\
	jmp	a1@(d0:l)\n\
0:\n\
	bne 2b ; dbra %1,1b ; bra 4f\n\
	. = 0b + 0x20\n\
	moveb	%4,a0@(3)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f\n\
	. = 0b + 0x40\n\
	moveb	%4,a0@(2)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f\n\
	. = 0b + 0x60\n\
	movew	%4,a0@(2)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f\n\
	. = 0b + 0x80\n\
	moveb	%4,a0@(1)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0xa0\n\
	moveb	%4,a0@(3) ; moveb	%4,a0@(1)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0xc0\n\
	movew	%4,a0@(1)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0xe0\n\
	movew	%4,a0@(2) ; moveb	%4,a0@(1)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x100\n\
	moveb	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x120\n\
	moveb	%4,a0@(3) ; moveb	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x140\n\
	moveb	%4,a0@(2) ; moveb	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x160\n\
	movew	%4,a0@(2) ; moveb	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x180\n\
	movew	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x1a0\n\
	moveb	%4,a0@(3) ; movew	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x1c0\n\
	moveb	%4,a0@(2) ; movew	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; dbra %1,1b ; bra 4f ;\n\
	. = 0b + 0x1e0\n\
	movel	%4,a0@(0)\n\
	andl	d1,d1 ; bne 2b ; \n\
3: 	dbra %1,1b ; \n\
4:\n"\
	    : "=a" (addr),	    /* %0 */ \
	      "=d" (count)	    /* %1 */ \
	    : "0" (addr),	    /* %2 */ \
	      "a" (stipple),	    /* %3 */ \
	      "d" (value),	    /* %4 */ \
	      "a" (width),	    /* %5 */ \
	      "1" (count),	    /* %6 */ \
	      "d" (shift)	    /* %7 */ \
	    : /* ctemp */	    "d0", \
 	      /* c */		    "d1", \
	      /* lshift */	    "d2", \
	      /* rshift */	    "d3", \
 	      /* atemp */	    "a0", \
 	      /* case */	    "a1")
