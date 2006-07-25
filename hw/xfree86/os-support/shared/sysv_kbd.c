/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@XFree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Thomas Roell and
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: sysv_kbd.c /main/3 1996/02/21 17:53:59 kaleb $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

int
xf86GetKbdLeds()
{
	int leds;

	ioctl(xf86Info.consoleFd, KDGETLED, &leds);
	return(leds);
}

void
xf86SetKbdRepeat(char rad)
{
#ifdef KDSETRAD
	ioctl(xf86Info.consoleFd, KDSETRAD, rad);
#endif
}

static int kbdtrans;
static struct termio kbdtty;
static char *kbdemap = NULL;

void
xf86KbdInit()
{
#ifdef KDGKBMODE
	ioctl (xf86Info.consoleFd, KDGKBMODE, &kbdtrans);
#endif
	ioctl (xf86Info.consoleFd, TCGETA, &kbdtty);
#if defined(E_TABSZ)
	kbdemap = xalloc(E_TABSZ);
	if (ioctl(xf86Info.consoleFd, LDGMAP, kbdemap) < 0)
	{
		xfree(kbdemap);
		kbdemap = NULL;
	}
#endif
}

int
xf86KbdOn()
{
	struct termio nTty;

	ioctl(xf86Info.consoleFd, KDSKBMODE, K_RAW);
	ioctl(xf86Info.consoleFd, LDNMAP, 0); /* disable mapping completely */
	nTty = kbdtty;
	nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
	nTty.c_oflag = 0;
	nTty.c_cflag = CREAD | CS8 | B9600;
	nTty.c_lflag = 0;
	nTty.c_cc[VTIME]=0;
	nTty.c_cc[VMIN]=1;
	ioctl(xf86Info.consoleFd, TCSETA, &nTty);
	return(xf86Info.consoleFd);
}

int
xf86KbdOff()
{
	if (kbdemap)
	{
		ioctl(xf86Info.consoleFd, LDSMAP, kbdemap);
	}
	ioctl(xf86Info.consoleFd, KDSKBMODE, kbdtrans);
	ioctl(xf86Info.consoleFd, TCSETA, &kbdtty);
	return(xf86Info.consoleFd);
}
