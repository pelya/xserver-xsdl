/*
 * $Id$
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
/* $XFree86: xc/programs/Xserver/hw/kdrive/kmap.c,v 1.1 1999/11/19 13:53:50 hohndel Exp $ */

#include "kdrive.h"

#ifdef linux
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

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

    fd = open ("/dev/mem", O_RDWR);
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

