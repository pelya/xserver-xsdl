/*
 * $Id$
 *
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */
/* $XFree86: xc/programs/Xserver/hw/kdrive/trident/tridentstub.c,v 1.1 1999/11/19 13:54:02 hohndel Exp $ */

#include "trident.h"

void
InitCard (char *name)
{
    KdCardAttr	attr;

    if (LinuxFindPci (0x1023, 0x9525, 0, &attr))
	KdCardInfoAdd (&tridentFuncs, &attr, 0);
}

void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
    KdInitInput (&Ps2MouseFuncs, &LinuxKeyboardFuncs);
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    return KdProcessArgument (argc, argv, i);
}
