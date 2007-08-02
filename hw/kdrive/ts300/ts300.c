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
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "kdrive.h"

extern KdCardFuncs  sisFuncs;
extern KdCardFuncs  s3Funcs;

/*
 * Kludgy code to parse the ascii /proc/pci file as the TS300
 * is running a 2.0 kernel
 */
BOOL
HasPCI (char *name, KdCardAttr *attr)
{
    FILE    *f;
    char    line[1024];
    BOOL    waiting;
    BOOL    found = FALSE;
    char    *mem;

    f = fopen ("/proc/pci", "r");
    if (!f)
	return FALSE;
    waiting = FALSE;
    attr->naddr = 0;
    while (fgets (line, sizeof (line), f))
    {
	if (waiting)
	{
	    
	    if (mem = strstr (line, "memory at "))
	    {
		mem += strlen ("memory at ");
		attr->address[attr->naddr++] = strtoul (mem, NULL, 0);
		found = TRUE;
	    }
	    else if (mem = strstr (line, "I/O at "))
	    {
		mem += strlen ("I/O at ");
		attr->io = strtoul (mem, NULL, 0);
		found = TRUE;
	    }
	    else if (strstr (line, "Bus") && strstr (line, "device") &&
		strstr (line, "function"))
		break;
	}
	else if (strstr (line, "VGA compatible controller"))
	{
	    if (strstr (line, name))
		waiting = TRUE;
	}
    }
    fclose (f);
    return found;
}

typedef struct _PCICard {
    char	*user;
    char	*name;
    KdCardFuncs	*funcs;
} PCICard;

PCICard	PCICards[] = {
    "sis",  "Silicon Integrated Systems",   &sisFuncs,
    "s3",   "S3 Inc.",			    &s3Funcs,
};

#define NUM_PCI_CARDS	(sizeof (PCICards) / sizeof (PCICards[0]))
    
void
InitCard (char *name)
{
    KdCardInfo	*card;
    CARD32	fb;
    int		i;
    KdCardAttr	attr;
    
    for (i = 0; i < NUM_PCI_CARDS; i++)
    {
	if (!name || !strcmp (name, PCICards[i].user))
	{
	    if (HasPCI (PCICards[i].name, &attr))
	    {
		KdCardInfoAdd (PCICards[i].funcs, &attr, 0);
		return;
	    }
	}
    }
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
}

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
