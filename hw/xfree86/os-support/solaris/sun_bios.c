/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sunos/sun_bios.c,v 1.4 2004/03/08 15:37:12 tsi Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * Copyright 1999 by David Holland <davidh@iquest.net>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the names of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef i386
#define _NEED_SYSI86
#endif
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

extern char *apertureDevName;

/*
 * Read BIOS via mmap()ing physical memory.
 */
int
xf86ReadBIOS(unsigned long Base, unsigned long Offset, unsigned char *Buf,
	     int Len)
{
	int fd;
	unsigned char *ptr;
	int psize;
	int mlen;

	psize = xf86getpagesize();
	Offset += Base & (psize - 1);
	Base &= ~(psize - 1);
	mlen = (Offset + Len + psize - 1) & ~(psize - 1);

	if (!xf86LinearVidMem())
		FatalError("xf86ReadBIOS: Could not mmap BIOS [a=%lx]\n", Base);

	if ((fd = open(apertureDevName, O_RDONLY)) < 0)
	{
		xf86Msg(X_WARNING, "xf86ReadBIOS: Failed to open %s (%s)\n",
			apertureDevName, strerror(errno));
		return(-1);
	}

	ptr = (unsigned char *)mmap((caddr_t)0, mlen, PROT_READ,
					MAP_SHARED, fd, (off_t)Base);
	if (ptr == MAP_FAILED)
	{
		xf86Msg(X_WARNING, "xf86ReadBIOS: %s mmap failed "
			"[0x%08lx, 0x%04x]\n",
			apertureDevName, Base, mlen);
		close(fd);
		return -1;
	}

	(void)memcpy(Buf, (void *)(ptr + Offset), Len);
	(void)munmap((caddr_t)ptr, mlen);
	(void)close(fd);

	return Len;
}
