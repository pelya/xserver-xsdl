/*
 * Id: kmap.c,v 1.1 1999/11/02 03:54:46 keithp Exp $
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/kdrive/kmap.c,v 1.9 2001/06/29 13:57:45 keithp Exp $ */

#include "kdrive.h"

#ifdef linux
#ifdef __i386__
#define HAS_MTRR
#endif
#endif

#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#ifdef HAS_MTRR
#include <asm/mtrr.h>
#endif

#include <sys/ioctl.h>

void *
KdMapDevice (CARD32 addr, CARD32 size)
{
#ifdef WINDOWS
    void    *a;
    void    *d;

    d = VirtualAlloc (NULL, size, MEM_RESERVE, PAGE_NOACCESS);
    if (!d)
	return NULL;
    DRAW_DEBUG ((DEBUG_S3INIT, "Virtual address of 0x%x is 0x%x", addr, d));
    a = VirtualCopyAddr (addr);
    DRAW_DEBUG ((DEBUG_S3INIT, "Translated address is 0x%x", a));
    if (!VirtualCopy (d, a, size, 
		      PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL))
    {
	DRAW_DEBUG ((DEBUG_FAILURE, "VirtualCopy failed %d",
		    GetLastError ()));
	return NULL;
    }
    DRAW_DEBUG ((DEBUG_S3INIT, "Device mapped successfully"));
    return d;
#endif
#ifdef linux
    void    *a;
    int	    fd;

#ifdef __arm__
    fd = open ("/dev/mem", O_RDWR|O_SYNC);
#else
    fd = open ("/dev/mem", O_RDWR);
#endif
    if (fd < 0)
	FatalError ("KdMapDevice: failed to open /dev/mem (%s)\n",
		    strerror (errno));
    
    a = mmap ((caddr_t) 0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, addr);
    close (fd);
    if ((long) a == -1)
	FatalError ("KdMapDevice: failed to map frame buffer (%s)\n",
		    strerror (errno));
    return a;
#endif
#ifdef VXWORKS
    return (void *) addr;
#endif
}

void
KdUnmapDevice (void *addr, CARD32 size)
{
#ifdef WINDOWS
    VirtualFree (addr, size, MEM_DECOMMIT);
    VirtualFree (addr, 0, MEM_RELEASE);
#endif
#ifdef linux
    munmap (addr, size);
#endif
#ifdef VXWORKS
    ;
#endif
}

#ifdef HAS_MTRR
static int  mtrr;
#endif

void
KdSetMappedMode (CARD32 addr, CARD32 size, int mode)
{
#ifdef HAS_MTRR
    struct mtrr_sentry  sentry;
    unsigned long    	base, bound;
    unsigned int	type;

    if (addr < 0x100000)
	return;
    if (!mtrr)
	mtrr = open ("/proc/mtrr", 2);
    if (mtrr > 0)
    {
	base = addr & ~((1<<22)-1);
	bound = ((addr + size) + ((1<<22) - 1)) & ~((1<<22) - 1);
	switch (mode) {
	case KD_MAPPED_MODE_REGISTERS:
	    type = MTRR_TYPE_UNCACHABLE;
	    break;
	case KD_MAPPED_MODE_FRAMEBUFFER:
	    type = MTRR_TYPE_WRCOMB;
	    break;
	}
	sentry.base = base;
	sentry.size = bound - base;
	sentry.type = type;
	
	if (ioctl (mtrr, MTRRIOC_ADD_ENTRY, &sentry) < 0)
	    ErrorF ("MTRRIOC_ADD_ENTRY failed 0x%x 0x%x %d (errno %d)\n",
		    base, bound - base, type, errno);
    }
#endif
}

void
KdResetMappedMode (CARD32 addr, CARD32 size, int mode)
{
#ifdef HAS_MTRR
    struct mtrr_sentry  sentry;
    unsigned long    	base, bound;
    unsigned int	type;

    if (addr < 0x100000)
	return;
    if (!mtrr)
	mtrr = open ("/proc/mtrr", 2);
    if (mtrr > 0)
    {
	base = addr & ~((1<<22)-1);
	bound = ((addr + size) + ((1<<22) - 1)) & ~((1<<22) - 1);
	switch (mode) {
	case KD_MAPPED_MODE_REGISTERS:
	    type = MTRR_TYPE_UNCACHABLE;
	    break;
	case KD_MAPPED_MODE_FRAMEBUFFER:
	    type = MTRR_TYPE_WRCOMB;
	    break;
	}
	sentry.base = base;
	sentry.size = bound - base;
	sentry.type = type;
	
	if (ioctl (mtrr, MTRRIOC_DEL_ENTRY, &sentry) < 0)
	    ErrorF ("MTRRIOC_DEL_ENTRY failed 0x%x 0x%x %d (errno %d)\n",
		    base, bound - base, type, errno);
    }
#endif
}
