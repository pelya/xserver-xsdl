/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_video.c,v 3.13.2.1 1997/05/11 05:04:25 dawes Exp $ */
/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: lnx_video.c,v 1.3 2000/08/17 19:51:23 cpqbld Exp $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#ifdef __alpha__

/*
 * The Jensen lacks dense memory, thus we have to address the bus via
 * the sparse addressing scheme.
 *
 * Martin Ostermann (ost@comnets.rwth-aachen.de) - Apr.-Sep. 1996
 */

#ifdef TEST_JENSEN_CODE /* define to test the Sparse addressing on a non-Jensen */
#define SPARSE (5)
#define isJensen (1)
#else
#define isJensen (!_bus_base())
#define SPARSE (7)
#endif

#define BUS_BASE (isJensen ? _bus_base_sparse() : _bus_base())
#define JENSEN_SHIFT(x) (isJensen ? ((long)x<<SPARSE) : (long)x)
#else
#define BUS_BASE 0
#define JENSEN_SHIFT(x) (x)
#endif

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

/*
 * Unfortunatly mmap without MAP_FIXED only works the first time :-(
 * This is now fixed in pl13 ALPHA, but still seems to have problems.
 */
#undef ONLY_MMAP_FIXED_WORKS

#ifdef ONLY_MMAP_FIXED_WORKS
static pointer AllocAddress[MAXSCREENS][NUM_REGIONS];
#endif

#if 0
static struct xf86memMap {
  int offset;
  int memSize;
} xf86memMaps[MAXSCREENS];
#endif

pointer xf86MapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	pointer base;
      	int fd;

#ifdef ONLY_MMAP_FIXED_WORKS
#ifdef __alpha__
	FatalError("xf86MapVidMem: Unexpected code for Alpha (pagesize=8k!)\n");
#endif 
	AllocAddress[ScreenNum][Region] = (pointer)xalloc(Size + 0x1000);
	if (AllocAddress[ScreenNum][Region] == NULL)
	{
		FatalError("xf86MapVidMem: can't alloc framebuffer space\n");
	}
	base = (pointer)(((unsigned int)AllocAddress[ScreenNum][Region]
			  & ~0xFFF) + 0x1000);
	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	base = (pointer)mmap((caddr_t)base, Size, PROT_READ|PROT_WRITE,
			     MAP_FIXED|MAP_SHARED, fd, (off_t)Base);
#else
	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	/* This requirers linux-0.99.pl10 or above */
	base = (pointer)mmap((caddr_t)0, JENSEN_SHIFT(Size),
			     PROT_READ|PROT_WRITE,
			     MAP_SHARED, fd,
			     (off_t)(JENSEN_SHIFT((off_t)Base) + BUS_BASE));
#endif
	close(fd);
	if ((long)base == -1)
	{
		FatalError("xf86MapVidMem: Could not mmap framebuffer (%s)\n",
			   strerror(errno));
	}
#if 0
	xf86memMaps[ScreenNum].offset = (int) Base;
	xf86memMaps[ScreenNum].memSize = Size;
#endif
	return base;
}

#if 0
void xf86GetVidMemData(ScreenNum, Base, Size)
int ScreenNum;
int *Base;
int *Size;
{
   *Base = xf86memMaps[ScreenNum].offset;
   *Size = xf86memMaps[ScreenNum].memSize;
}
#endif

void xf86UnMapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
    munmap((caddr_t)JENSEN_SHIFT(Base), JENSEN_SHIFT(Size));
#ifdef ONLY_MMAP_FIXED_WORKS
    xfree(AllocAddress[ScreenNum][Region]);
#endif
}

Bool xf86LinearVidMem()
{
	return(TRUE);
}

/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

/*
 * Linux handles regular (<= 0x3ff) ports with the TSS I/O bitmap, and
 * extended ports with the iopl() system call.
 *
 * For testing, it's useful to enable only the ports we need, but for
 * production purposes, it's faster to enable all ports.
 */
#define ALWAYS_USE_EXTENDED

#ifdef ALWAYS_USE_EXTENDED

static Bool ScreenEnabled[MAXSCREENS];
static Bool ExtendedEnabled = FALSE;
static Bool InitDone = FALSE;

void xf86ClearIOPortList(ScreenNum)
int ScreenNum;
{
	if (!InitDone)
	{
		int i;

		for (i = 0; i < MAXSCREENS; i++)
			ScreenEnabled[i] = FALSE;
		InitDone = TRUE;
	}

	return;
}

void xf86AddIOPorts(ScreenNum, NumPorts, Ports)
int ScreenNum;
int NumPorts;
unsigned *Ports;
{
	return;
}

void xf86EnableIOPorts(ScreenNum)
int ScreenNum;
{
	int i;

	ScreenEnabled[ScreenNum] = TRUE;

	if (ExtendedEnabled)
		return;

#ifndef __mc68000__
	if (iopl(3))
		FatalError("%s: Failed to set IOPL for I/O\n",
			   "xf86EnableIOPorts");
#endif
	ExtendedEnabled = TRUE;

	return;
}

void xf86DisableIOPorts(ScreenNum)
int ScreenNum;
{
	int i;

	ScreenEnabled[ScreenNum] = FALSE;

	if (!ExtendedEnabled)
		return;

	for (i = 0; i < MAXSCREENS; i++)
		if (ScreenEnabled[i])
			return;

#ifndef __mc68000__
	iopl(0);
#endif
	ExtendedEnabled = FALSE;

	return;
}

#else /* !ALWAYS_USE_EXTENDED */

static unsigned *EnabledPorts[MAXSCREENS];
static int NumEnabledPorts[MAXSCREENS];
static Bool ScreenEnabled[MAXSCREENS];
static Bool ExtendedPorts[MAXSCREENS];
static Bool ExtendedEnabled = FALSE;
static Bool InitDone = FALSE;

void xf86ClearIOPortList(ScreenNum)
int ScreenNum;
{
	if (!InitDone)
	{
		xf86InitPortLists(EnabledPorts, NumEnabledPorts,
				  ScreenEnabled, ExtendedPorts, MAXSCREENS);
		InitDone = TRUE;
		return;
	}
	ExtendedPorts[ScreenNum] = FALSE;
	if (EnabledPorts[ScreenNum] != (unsigned *)NULL)
		xfree(EnabledPorts[ScreenNum]);
	EnabledPorts[ScreenNum] = (unsigned *)NULL;
	NumEnabledPorts[ScreenNum] = 0;
}

void xf86AddIOPorts(ScreenNum, NumPorts, Ports)
int ScreenNum;
int NumPorts;
unsigned *Ports;
{
	int i;

	if (!InitDone)
	{
	    FatalError("xf86AddIOPorts: I/O control lists not initialised\n");
	}
	EnabledPorts[ScreenNum] = (unsigned *)xrealloc(EnabledPorts[ScreenNum],
			(NumEnabledPorts[ScreenNum]+NumPorts)*sizeof(unsigned));
	for (i = 0; i < NumPorts; i++)
	{
		EnabledPorts[ScreenNum][NumEnabledPorts[ScreenNum] + i] =
								Ports[i];
		if (Ports[i] > 0x3FF)
			ExtendedPorts[ScreenNum] = TRUE;
	}
	NumEnabledPorts[ScreenNum] += NumPorts;
}

void xf86EnableIOPorts(ScreenNum)
int ScreenNum;
{
	int i;

	if (ScreenEnabled[ScreenNum])
		return;

	for (i = 0; i < MAXSCREENS; i++)
	{
		if (ExtendedPorts[i] && (ScreenEnabled[i] || i == ScreenNum))
		{
#ifndef __mc68000__
		    if (iopl(3))
		    {
			FatalError("%s: Failed to set IOPL for extended I/O\n",
				   "xf86EnableIOPorts");
		    }
#endif
		    ExtendedEnabled = TRUE;
		    break;
		}
	}
	/* Extended I/O was used, but not any more */
	if (ExtendedEnabled && i == MAXSCREENS)
	{
#ifndef __mc68000__
		iopl(0);
#endif
		ExtendedEnabled = FALSE;
	}
	/*
	 * Turn on non-extended ports even when using extended I/O
	 * so they are there if extended I/O gets turned off when it's no
	 * longer needed.
	 */
	for (i = 0; i < NumEnabledPorts[ScreenNum]; i++)
	{
		unsigned port = EnabledPorts[ScreenNum][i];

		if (port > 0x3FF)
			continue;

		if (xf86CheckPorts(port, EnabledPorts, NumEnabledPorts,
				   ScreenEnabled, MAXSCREENS))
		{
		    if (ioperm(port, 1, TRUE) < 0)
		    {
			FatalError("%s: Failed to enable I/O port 0x%x (%s)\n",
				   "xf86EnableIOPorts", port, strerror(errno));
		    }
		}
	}
	ScreenEnabled[ScreenNum] = TRUE;
	return;
}

void xf86DisableIOPorts(ScreenNum)
int ScreenNum;
{
	int i;

	if (!ScreenEnabled[ScreenNum])
		return;

	ScreenEnabled[ScreenNum] = FALSE;
	for (i = 0; i < MAXSCREENS; i++)
	{
		if (ScreenEnabled[i] && ExtendedPorts[i])
			break;
	}
	if (ExtendedEnabled && i == MAXSCREENS)
	{
#ifndef __mc68000__
		iopl(0);
#endif
		ExtendedEnabled = FALSE;
	}
	for (i = 0; i < NumEnabledPorts[ScreenNum]; i++)
	{
		unsigned port = EnabledPorts[ScreenNum][i];

		if (port > 0x3FF)
			continue;

		if (xf86CheckPorts(port, EnabledPorts, NumEnabledPorts,
				   ScreenEnabled, MAXSCREENS))
		{
			ioperm(port, 1, FALSE);
		}
	}
	return;
}

#endif /* ALWAYS_USE_EXTENDED */

void xf86DisableIOPrivs()
{
#ifndef __mc68000__
	if (ExtendedEnabled)
		iopl(0);
#endif
	return;
}

/***************************************************************************/
/* Interrupt Handling section                                              */
/***************************************************************************/

Bool xf86DisableInterrupts()
{
	if (!ExtendedEnabled)
#ifndef __mc68000__
		if (iopl(3))
			return (FALSE);
#endif
#if defined(__alpha__) || defined(__mc68000__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("cli");
#else
	asm("cli");
#endif
#endif
#ifndef __mc68000__
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return (TRUE);
}

void xf86EnableInterrupts()
{
	if (!ExtendedEnabled)
#ifndef __mc68000__
		if (iopl(3))
			return;
#endif
#if defined(__alpha__) || defined(__mc68000__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("sti");
#else
	asm("sti");
#endif
#endif
#ifndef __mc68000__
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return;
}

#if defined(__alpha__)

static int xf86SparseShift = 5; /* default to all but JENSEN */

pointer xf86MapVidMemSparse(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	pointer base;
      	int fd;

	if (!_bus_base()) xf86SparseShift = 7; /* Uh, oh, JENSEN... */

	Size <<= xf86SparseShift;
	Base = (pointer)((unsigned long)Base << xf86SparseShift);

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	/* This requirers linux-0.99.pl10 or above */
	base = (pointer)mmap((caddr_t)0, Size,
			     PROT_READ | PROT_WRITE,
			     MAP_SHARED, fd,
			     (off_t)Base + _bus_base_sparse());
	close(fd);
	if ((long)base == -1)
	{
		FatalError("xf86MapVidMem: Could not mmap framebuffer (%s)\n",
			   strerror(errno));
	}
	return base;
}

void xf86UnMapVidMemSparse(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	Size <<= xf86SparseShift;

	munmap((caddr_t)Base, Size);
}

#define vuip    volatile unsigned int *

extern void sethae(unsigned long hae);

int xf86ReadSparse8(Base, Offset)
pointer Base;
unsigned long Offset;
{
    unsigned long result, shift;
    unsigned long msb = 0;

    shift = (Offset & 0x3) * 8;
    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    result = *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift));
    if (msb)
      sethae(0);
    result >>= shift;
    return 0xffUL & result;
}

int xf86ReadSparse16(Base, Offset)
pointer Base;
unsigned long Offset;
{
    unsigned long result, shift;
    unsigned long msb = 0;

    shift = (Offset & 0x2) * 8;
    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    result = *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2)));
    if (msb)
      sethae(0);
    result >>= shift;
    return 0xffffUL & result;
}

int xf86ReadSparse32(Base, Offset)
pointer Base;
unsigned long Offset;
{
    unsigned long result;
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    result = *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2)));
    if (msb)
      sethae(0);
    return result;
}

void xf86WriteSparse8(Value, Base, Offset)
int Value;
pointer Base;
unsigned long Offset;
{
    unsigned long msb = 0;
    unsigned int b = Value & 0xffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift)) = b * 0x01010101;
    if (msb)
      sethae(0);
}

void xf86WriteSparse16(Value, Base, Offset)
int Value;
pointer Base;
unsigned long Offset;
{
    unsigned long msb = 0;
    unsigned int w = Value & 0xffffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2))) =
      w * 0x00010001;
    if (msb)
      sethae(0);
}

void xf86WriteSparse32(Value, Base, Offset)
int Value;
pointer Base;
unsigned long Offset;
{
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2))) = Value;
    if (msb)
      sethae(0);
}
#endif /* __alpha__ */
