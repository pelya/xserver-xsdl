/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/bios_devmem.c,v 3.7 2000/09/19 12:46:22 eich Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: bios_devmem.c /main/5 1996/10/19 18:07:41 kaleb $ */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include <string.h>

/*
 * Read BIOS via /dev/mem.
 */

#ifndef DEV_MEM
# define DEV_MEM "/dev/mem"
#endif

int
xf86ReadBIOS(unsigned long Base, unsigned long Offset, unsigned char *Buf,
		int Len)
{
 	int fd;

#ifdef __ia64__
	if ((fd = open(DEV_MEM, O_RDONLY | O_SYNC)) < 0)
#else
 	if ((fd = open(DEV_MEM, O_RDONLY)) < 0)
#endif
	{
		xf86Msg(X_WARNING, "xf86ReadBIOS: Failed to open %s (%s)\n",
			DEV_MEM, strerror(errno));
		return(-1);
	}

	if (lseek(fd, (Base+Offset), SEEK_SET) < 0)
	{
		xf86Msg(X_WARNING, "xf86ReadBIOS: %s seek failed (%s)\n",
			DEV_MEM, strerror(errno));
		close(fd);
		return(-1);
	}
	if (read(fd, Buf, Len) != Len)
	{
		xf86Msg(X_WARNING, "xf86ReadBIOS: %s read failed (%s)\n",
			DEV_MEM, strerror(errno));
		close(fd);
		return(-1);
	}
	close(fd);
	return(Len);
}
