/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/bios_devmem.c,v 3.3 1996/12/23 06:50:58 dawes Exp $ */
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
/* $Xorg: bios_devmem.c,v 1.3 2000/08/17 19:51:30 cpqbld Exp $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

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

int xf86ReadBIOS(Base, Offset, Buf, Len)
unsigned long Base;
unsigned long Offset;
unsigned char *Buf;
int Len;
{
#ifdef __alpha__
  /*
   * The Alpha version uses "mmap" instead of "lseek/read",
   *  because these (currently) don't work for BUS memory.
   * We trick "mmap" into mapping BUS memory for us via BUS_BASE,
   *  which is the KSEG address of the start of the DENSE memory
   *  area.
   */

  /*
   * NOTE: there prolly ought to be more validity checks and all
   *  re: boundaries and sizes and such...
   */

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

extern unsigned long _bus_base(void);
extern unsigned long _bus_base_sparse(void);
#define BUS_BASE (isJensen ? _bus_base_sparse() : _bus_base())
#define JENSEN_SHIFT(x) (isJensen ? ((long)x<<SPARSE) : (long)x)

#define SIZE (64*1024)

	caddr_t base;
 	int fd;

	if ((fd = open(DEV_MEM, O_RDONLY)) < 0)
	{
		ErrorF("xf86ReadBios: Failed to open %s (%s)\n", DEV_MEM,
		       strerror(errno));
		return(-1);
	}

	base = mmap((caddr_t)0, JENSEN_SHIFT(SIZE), PROT_READ,
		    MAP_SHARED, fd, (off_t)(JENSEN_SHIFT(Base) + BUS_BASE));

	if (base == (caddr_t)-1UL)
	{
		ErrorF("xf86ReadBios: Failed to mmap %s (%s)\n", DEV_MEM,
		       strerror(errno));
		return(-1);
	}

	SlowBCopyFromBus(base+JENSEN_SHIFT(Offset), Buf, Len);

	munmap((caddr_t)JENSEN_SHIFT(base), JENSEN_SHIFT(SIZE));
	close(fd);
	return(Len);

#else /* __alpha__ */

 	int fd;

	if ((fd = open(DEV_MEM, O_RDONLY)) < 0)
	{
		ErrorF("xf86ReadBios: Failed to open %s (%s)\n", DEV_MEM,
		       strerror(errno));
		return(-1);
	}

	if (lseek(fd, (Base+Offset), SEEK_SET) < 0)
	{
		ErrorF("xf86ReadBios: %s seek failed (%s)\n", DEV_MEM,
		       strerror(errno));
		close(fd);
		return(-1);
	}
	if (read(fd, Buf, Len) != Len)
	{
		ErrorF("xf86ReadBios: %s read failed (%s)\n", DEV_MEM,
		       strerror(errno));
		close(fd);
		return(-1);
	}
	close(fd);
	return(Len);
#endif /* __alpha__ */
}
