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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
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
    KdInitInput (&LinuxMouseFuncs, &LinuxKeyboardFuncs);
#ifdef TOUCHSCREEN
    KdAddMouseDriver (&TsFuncs);
#endif
}

extern pcmciaDisplayModeRec pcmciaDefaultModes[];

void
ddxUseMsg (void)
{
    KdUseMsg();
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    return KdProcessArgument (argc, argv, i);
}
