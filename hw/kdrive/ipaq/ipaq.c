/*
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
 *
 * Adapted from ts300.c by Alan Hourihane <alanh@fairlite.demon.co.uk>
 * For the Compaq IPAQ handheld, with the HP VGA Out Card (F1252A).
 */
/* $XFree86: xc/programs/Xserver/hw/kdrive/ipaq/ipaq.c,v 1.1 2001/05/23 17:28:39 alanh Exp $ */

#include "pcmcia.h"

extern KdCardFuncs  pcmciaFuncs;

void
InitCard (char *name)
{
    KdCardAttr attr;
    if (name && !strcmp(name, "pcmcia"))
    	KdCardInfoAdd (&pcmciaFuncs, &attr, 0);
    else
    	KdCardInfoAdd (&fbdevFuncs, &attr, 0);
}

void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
#ifdef __powerpc__
    KdInitInput (&BusMouseFuncs, &LinuxKeyboardFuncs);
#else
    KdInitInput (&Ps2MouseFuncs, &LinuxKeyboardFuncs);
#endif
#ifdef TOUCHSCREEN
    KdInitTouchScreen (&TsFuncs);
#endif
}

extern pcmciaDisplayModeRec pcmciaDefaultModes[];

int
ddxProcessArgument (int argc, char **argv, int i)
{
    int	ret;
    
    if (!strcmp (argv[i], "-listmodes"))
    {
	int j = 0, bpp = 0;
	ErrorF("Valid modes are....\n\n");
	
	for (bpp = 8; bpp < 24; bpp += 8) {
    	  while (pcmciaDefaultModes[j].Width != 0) {
	    if ((pcmciaDefaultModes[j].Width *
		 pcmciaDefaultModes[j].Height * bpp/8) <= 512 * 1024) {
		ErrorF("%dx%dx%dx%d\n",
			pcmciaDefaultModes[j].Width,
			pcmciaDefaultModes[j].Height,
			bpp,
			pcmciaDefaultModes[j].Refresh);
	    }
	    j++;
	  }
	  j = 0;
	} 
	exit(1);
    }
    return KdProcessArgument (argc, argv, i);
}
