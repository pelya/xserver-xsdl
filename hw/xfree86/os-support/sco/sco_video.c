/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sco/sco_video.c,v 3.2.2.1 1997/07/19 04:59:31 dawes Exp $ */
/*
 * Copyright 1993 by David McCullough <davidm@stallion.oz.au>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of David McCullough and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of the 
 * software without specific, written prior permission.  David McCullough and
 * David Wexelblat makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID MCCULLOUGH AND DAVID WEXELBLAT DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL DAVID MCCULLOUGH OR DAVID WEXELBLAT BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: sco_video.c,v 1.3 2000/08/17 19:51:29 cpqbld Exp $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#define _NEED_SYSI86
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

static struct kd_memloc	MapDSC[MAXSCREENS][NUM_REGIONS];
static int		ver_once = 1;

/***************************************************************************/
/*
 * To map the video-memory, we use the MAP_CLASS ioctl.
 * Different drivers may have to do another one of these
 * for their own special registers (ie., ATI).  To find
 * out which strings are valid look in /etc/conf/pack.d/cn/class.h
 *
 * if we fail to find one of these we try for the dmmap driver
 */

struct {
	unsigned long base, size;
	char *class;
} SCO_Mapping[] = {
	{0xA0000, 0x10000, "VGA"},
	{0xA0000, 0x20000, "SVGA"},
	{0xB0000, 0x08000, "HGA"},
	{0x0,     0x0,     ""},
};

/* ARGSUSED */
pointer xf86MapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	int i;
	char *class = (char *)NULL;
	pointer base;

	for (i=0; SCO_Mapping[i].base != 0; i++)
	{
		if (((pointer)SCO_Mapping[i].base == Base) &&
		    (SCO_Mapping[i].size == Size))
		{
			class = SCO_Mapping[i].class;
			break;
		}
	}
	if (class == (char *)NULL)
	{
		int		fd;

#if defined(SVR4) || defined(SCO325)
		if ((fd = open(DEV_MEM, O_RDWR)) < 0)
		{
			FatalError("xf86MapVidMem: failed to open %s (%s)\n",
				   DEV_MEM, strerror(errno));
		}
		base = (pointer)mmap((caddr_t)0, Size, PROT_READ|PROT_WRITE,
			     MAP_SHARED, fd, (off_t)Base);
		close(fd);
		if ((long)base == -1)
		{
			FatalError("%s: Could not mmap framebuffer [s=%x,a=%x] (%s)\n",
				   "xf86MapVidMem", Size, Base, strerror(errno));
		}

		return(base);
#else
		MapDSC[ScreenNum][Region].vaddr    = (char *) NULL;
		MapDSC[ScreenNum][Region].physaddr = (char *) Base;
		MapDSC[ScreenNum][Region].length   = Size;
		MapDSC[ScreenNum][Region].ioflg    = 1;
		if ((fd = open("/dev/dmmap", O_RDWR)) >= 0) {
		    if (ioctl(fd, KDMAPDISP, &MapDSC[ScreenNum][Region]) == -1)
			ErrorF("xf86MapVidMem: dmmap KDMAPDISP failed (%s)\n",
			       strerror(errno));
		    else {
			if (ver_once)
			    ErrorF("Using dmmap version 0x%04x.\n",
				   ioctl(fd, -1));
			ver_once = 0;
			close(fd);
			return(MapDSC[ScreenNum][Region].vaddr);
		    }
		    close(fd);
		}
		FatalError("xf86MapVidMem:No class map defined for (%x,%x)\n",
			   Base, Size);
		/* NOTREACHED */
#endif
	}

	base = (pointer)ioctl(xf86Info.consoleFd, MAP_CLASS, class);
	if ((int)base == -1)
	{
		FatalError("xf86MapVidMem:Failed to map video mem class %s\n",
			   class);
		/* NOTREACHED */
	}
	return(base);
}

/*
 * Nothing to do here if it wasn't mapped using the dmmap driver
 */
/* ARGSUSED */
void xf86UnMapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	int	fd;

#if defined (SVR4) || defined(SCO325)
	munmap(Base, Size);
#else /* SVR4 */
	if (MapDSC[ScreenNum][Region].vaddr) {
	    if ((fd = open("/dev/dmmap", O_RDWR)) < 0) {
		if (ioctl(fd, KDUNMAPDISP, &MapDSC[ScreenNum][Region]) == -1)
		    ErrorF("xf86UnMapVidMem: dmmap KDUNMAPDISP failed (%s)\n",
			   strerror(errno));
		close(fd);
	    }
	    MapDSC[ScreenNum][Region].vaddr    = (char *) NULL;
	    MapDSC[ScreenNum][Region].physaddr = (char *) NULL;
	    MapDSC[ScreenNum][Region].length   = 0;
	    MapDSC[ScreenNum][Region].ioflg    = 0;
	}
#endif
	return;
}

/* ARGSUSED */
Bool xf86LinearVidMem()
{
	int		fd, ver;

#if defined(SVR4) || defined(SCO325)
	return TRUE;
#else
	if ((fd = open("/dev/dmmap", O_RDWR)) >= 0) {
		ver = ioctl(fd, -1);
		close(fd);
		if (ver >= 0) {
			if (ver_once)
				ErrorF("Using dmmap version 0x%04x.\n", ver);
			ver_once = 0;
			return(TRUE);
		}
	}
	return(FALSE);
#endif
}

/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

static Bool ScreenEnabled[MAXSCREENS];
static Bool IOEnabled = FALSE;
static Bool InitDone = FALSE;

void xf86ClearIOPortList(ScreenNum)
int ScreenNum;
{
	int i;

	if (!InitDone)
	{
		for (i = 0; i < MAXSCREENS; i++)
			ScreenEnabled[i] = FALSE;
		InitDone = TRUE;
	}
}

/* ARGSUSED */
void xf86AddIOPorts(ScreenNum, NumPorts, Ports)
int ScreenNum;
int NumPorts;
unsigned *Ports;
{
}

void xf86EnableIOPorts(ScreenNum)
int ScreenNum;
{
	ScreenEnabled[ScreenNum] = TRUE;

	if (IOEnabled)
		return;

	if (sysi86(SI86V86, V86SC_IOPL, PS_IOPL) < 0)
		FatalError("Failed to set IOPL for extended I/O\n");
	IOEnabled = TRUE;
	return;
}

void xf86DisableIOPorts(ScreenNum)
int ScreenNum;
{
	int i;

	ScreenEnabled[ScreenNum] = FALSE;

	if (!IOEnabled)
		return;

	for (i = 0; i < MAXSCREENS; i++)
		if (ScreenEnabled[i])
			return;
	sysi86(SI86V86, V86SC_IOPL, 0);
	IOEnabled = FALSE;
	return;
}

void xf86DisableIOPrivs()
{
	if (IOEnabled)
		sysi86(SI86V86, V86SC_IOPL, 0);
	return;
}

/***************************************************************************/
/* Interrupt Handling section                                              */
/***************************************************************************/

Bool xf86DisableInterrupts()
{
#ifdef __GNUC__
	__asm__ __volatile__("cli");
#else 
	asm("cli");
#endif /* __GNUC__ */
	
	return(TRUE);
}

void xf86EnableInterrupts()
{
#ifdef __GNUC__
	__asm__ __volatile__("sti");
#else 
	asm("sti");
#endif /* __GNUC__ */

	return;
}
