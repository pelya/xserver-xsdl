/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/lynxos/lynx_noinline.c,v 3.6 2002/01/25 21:56:20 tsi Exp $ */
/*
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#if /* NO_INLINE && */ defined(__powerpc__)

#include "xf86Pci.h"

extern volatile unsigned char *ioBase;

void
eieio()
{
	__asm__ __volatile__ ("eieio");
}

unsigned long
ldl_brx(volatile unsigned char *base, int ndx)
{
	register unsigned long tmp = *(volatile unsigned long *)(base+ndx);
	return( ((tmp & 0x000000ff) << 24) |
		((tmp & 0x0000ff00) << 8) |
		((tmp & 0x00ff0000) >> 8) |
		((tmp & 0xff000000) >> 24) );
}

unsigned short
ldw_brx(volatile unsigned char *base, int ndx)
{
	register unsigned short tmp = *(volatile unsigned short *)(base+ndx);
	return((tmp << 8) | (tmp >> 8));
}

void
stl_brx(unsigned long val, volatile unsigned char *base, int ndx)
{
   unsigned char *p = (unsigned char *)&val;
   unsigned long tmp = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | (p[0] << 0);
   *(volatile unsigned long *)(base+ndx) = tmp;
}

void
stw_brx(unsigned short val, volatile unsigned char *base, int ndx)
{
  unsigned char *p = (unsigned char *)&val;
  unsigned short tmp = (p[1] << 8) | p[0];
  *(volatile unsigned short *)(base+ndx) = tmp;
}

void
outb(IOADDRESS port, unsigned char value)
{
	*((volatile unsigned char *)(ioBase + port)) = value; eieio();
}

void
outw(IOADDRESS port, unsigned short value)
{
	stw_brx(value, ioBase, port); eieio();
}

void
outl(IOADDRESS port, unsigned int value)
{
	stl_brx(value, ioBase, port); eieio();
}

unsigned char
inb(IOADDRESS port)
{
	unsigned char val;

	val = *((volatile unsigned char *)(ioBase + port)); eieio();
	return(val);
}

unsigned short
inw(IOADDRESS port)
{
	unsigned short val;

	val = ldw_brx(ioBase, port); eieio();
	return(val);
}

unsigned int
inl(IOADDRESS port)
{
	unsigned int val;

	val = ldl_brx(ioBase, port); eieio();
	return(val);
}

unsigned long 
ldl_u(void *p)
{
	return (((*(unsigned char *)(p)) |
	 	 (*((unsigned char *)(p)+1)<<8)	|
		 (*((unsigned char *)(p)+2)<<16) |
		 (*((unsigned char *)(p)+3)<<24)));
}

unsigned long 
ldq_u(void *p)
{
	return ldl_u(p);
}

unsigned short
ldw_u(void *p)
{
	return(((*(unsigned char *)(p)) |
	       (*((unsigned char *)(p)+1)<<8)));
}

void
stl_u(unsigned long v, void *p)
{

	(*(unsigned char *)(p)) = (v);
	(*((unsigned char *)(p)+1)) = ((v) >> 8);
	(*((unsigned char *)(p)+2)) = ((v) >> 16);
	(*((unsigned char *)(p)+3)) = ((v) >> 24);
}

void
stq_u(unsigned long v, void *p)
{
	stl_u(v,p);
}

void
stw_u(unsigned short v, void *p)
{
	(*(unsigned char *)(p)) = (v);
	(*((unsigned char *)(p)+1)) = ((v) >> 8);
}


void
mem_barrier(void)
{
   __asm__ __volatile__("eieio");
}

void
write_mem_barrier(void)
{
   __asm__ __volatile__("eieio");
}

#endif /* NO_INLINE && __powerpc__ */
