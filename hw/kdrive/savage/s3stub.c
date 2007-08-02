/*
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "s3.h"

void
InitCard (char *name)
{
    KdCardAttr	attr;
#ifdef VXWORKS
    attr.naddr = 2;
    attr.io = 0;
    attr.address[0] = 0xbc000000;		/* registers */
    attr.address[1] = 0xba000000;		/* frame buffer */
    KdCardInfoAdd (&s3Funcs, &attr, 0);
#else
    CARD32	count;

    count = 0;
    while (LinuxFindPci (0x5333, 0x8a22, count, &attr))
    {
	KdCardInfoAdd (&s3Funcs, &attr, 0);
	count++;
    }
#endif
}

void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
    KdOsAddInputDrivers ();
    KdInitInput ();
}

extern int	s3CpuTimeout;
extern int	s3AccelTimeout;

void
ddxUseMsg (void)
{
    ErrorF("\nSavage Driver Options:\n");
    ErrorF("-cpu    Sets CPU timout\n");
    ErrorF("-accel  Sets acceleration timout\n");

    KdUseMsg();
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    int	ret;
    
    if (!strcmp (argv[i], "-cpu"))
    {
	s3CpuTimeout = strtol(argv[i+1], NULL, 0);
	return 2;
    }
    if (!strcmp (argv[i], "-accel"))
    {
	s3AccelTimeout = strtol (argv[i+1], NULL, 0);
	return 2;
    }
    return KdProcessArgument (argc, argv, i);
}
