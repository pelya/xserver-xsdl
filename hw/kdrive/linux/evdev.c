/*
 * $Id$
 *
 * Copyright © 2004 Keith Packard
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
#define NEED_EVENTS
#include <errno.h>
#include <linux/input.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xpoll.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"

#define NUM_EVENTS  128
#define ABS_UNSET   -65535

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define ISBITSET(x,y) ((x)[LONG(y)] & BIT(y))
#define OFF(x)   ((x)%BITS_PER_LONG)
#define LONG(x)  ((x)/BITS_PER_LONG)
#define BIT(x)	 (1 << OFF(x))
#define SETBIT(x,y) ((x)[LONG(y)] |= BIT(y))
#define CLRBIT(x,y) ((x)[LONG(y)] &= ~BIT(y))
#define ASSIGNBIT(x,y,z)    ((x)[LONG(y)] = ((x)[LONG(y)] & ~BIT(y)) | (z << OFF(y)))

typedef struct _kevdevMouse {
    /* current device state */
    int			    rel[REL_MAX + 1];
    int			    abs[ABS_MAX + 1];
    int			    prevabs[ABS_MAX + 1];
    long		    key[NBITS(KEY_MAX + 1)];
    
    /* supported device info */
    long		    relbits[NBITS(REL_MAX + 1)];
    long		    absbits[NBITS(ABS_MAX + 1)];
    long		    keybits[NBITS(KEY_MAX + 1)];
    struct input_absinfo    absinfo[ABS_MAX + 1];
    int			    max_rel;
    int			    max_abs;
} Kevdev;

static void
EvdevMotion (KdMouseInfo    *mi)
{
    Kevdev		*ke = mi->driver;
    int			i;

    for (i = 0; i <= ke->max_rel; i++)
	if (ke->rel[i])
	{
	    int a;
	    ErrorF ("rel");
	    for (a = 0; a <= ke->max_rel; a++)
	    {
		if (ISBITSET (ke->relbits, a))
		    ErrorF (" %d=%d", a, ke->rel[a]);
		ke->rel[a] = 0;
	    }
	    ErrorF ("\n");
	    break;
	}
    for (i = 0; i < ke->max_abs; i++)
	if (ke->abs[i] != ke->prevabs[i])
	{
	    int a;
	    ErrorF ("abs");
	    for (a = 0; a <= ke->max_abs; a++)
	    {
		if (ISBITSET (ke->absbits, a))
		    ErrorF (" %d=%d", a, ke->abs[a]);
		ke->prevabs[a] = ke->abs[a];
	    }
	    ErrorF ("\n");
	    break;
	}
}

static void
EvdevRead (int evdevPort, void *closure)
{
    KdMouseInfo		*mi = closure;
    Kevdev		*ke = mi->driver;
    int			i;
    struct input_event	events[NUM_EVENTS];
    int			n;

    n = read (evdevPort, &events, NUM_EVENTS * sizeof (struct input_event));
    if (n <= 0)
	return;
    n /= sizeof (struct input_event);
    for (i = 0; i < n; i++)
    {
	switch (events[i].type) {
	case EV_SYN:
	    break;
	case EV_KEY:
	    EvdevMotion (mi);
	    ASSIGNBIT(ke->key,events[i].code, events[i].value);
	    if (events[i].code < 0x100)
		ErrorF ("key %d %d\n", events[i].code, events[i].value);
	    else
		ErrorF ("key 0x%x %d\n", events[i].code, events[i].value);
	    break;
	case EV_REL:
	    ke->rel[events[i].code] += events[i].value;
	    break;
	case EV_ABS:
	    ke->abs[events[i].code] = events[i].value;
	    break;
	}
    }
    EvdevMotion (mi);
}

int EvdevInputType;

char *kdefaultEvdev[] =  {
    "/dev/input/event0",
    "/dev/input/event1",
    "/dev/input/event2",
    "/dev/input/event3",
};

#define NUM_DEFAULT_EVDEV    (sizeof (kdefaultEvdev) / sizeof (kdefaultEvdev[0]))

static Bool
EvdevInit (void)
{
    int		i;
    int		fd;
    KdMouseInfo	*mi, *next;
    int		n = 0;
    char	*prot;

    if (!EvdevInputType)
	EvdevInputType = KdAllocInputType ();

    for (mi = kdMouseInfo; mi; mi = next)
    {
	next = mi->next;
	prot = mi->prot;
	if (mi->inputType)
	    continue;
	if (!mi->name)
	{
	    for (i = 0; i < NUM_DEFAULT_EVDEV; i++)
	    {
		fd = open (kdefaultEvdev[i], 2);
		if (fd >= 0)
		{
		    mi->name = KdSaveString (kdefaultEvdev[i]);
		    break;
		}
	    }
	}
	else
	    fd = open (mi->name, 2);
	    
	if (fd >= 0)
	{
	    unsigned long   ev[NBITS(EV_MAX)];
	    Kevdev	    *ke;
	    
	    if (ioctl (fd, EVIOCGBIT(0 /*EV*/, sizeof (ev)), ev) < 0)
	    {
		perror ("EVIOCGBIT 0");
		close (fd);
		continue;
	    }
	    ke = xalloc (sizeof (Kevdev));
	    if (!ke)
	    {
		close (fd);
		continue;
	    }
	    memset (ke, '\0', sizeof (Kevdev));
	    if (ISBITSET (ev, EV_KEY))
	    {
		if (ioctl (fd, EVIOCGBIT (EV_KEY, sizeof (ke->keybits)),
			   ke->keybits) < 0)
		{
		    perror ("EVIOCGBIT EV_KEY");
		    xfree (ke);
		    close (fd);
		    continue;
		}
	    }
	    if (ISBITSET (ev, EV_REL))
	    {
		if (ioctl (fd, EVIOCGBIT (EV_REL, sizeof (ke->relbits)),
			   ke->relbits) < 0)
		{
		    perror ("EVIOCGBIT EV_REL");
		    xfree (ke);
		    close (fd);
		    continue;
		}
		for (ke->max_rel = REL_MAX; ke->max_rel >= 0; ke->max_rel--)
		    if (ISBITSET(ke->relbits, ke->max_rel))
			break;
	    }
	    if (ISBITSET (ev, EV_ABS))
	    {
		int i;

		if (ioctl (fd, EVIOCGBIT (EV_ABS, sizeof (ke->absbits)),
			   ke->absbits) < 0)
		{
		    perror ("EVIOCGBIT EV_ABS");
		    xfree (ke);
		    close (fd);
		    continue;
		}
		for (ke->max_abs = ABS_MAX; ke->max_abs >= 0; ke->max_abs--)
		    if (ISBITSET(ke->absbits, ke->max_abs))
			break;
		for (i = 0; i <= ke->max_abs; i++)
		{
		    if (ISBITSET (ke->absbits, i))
			if (ioctl (fd, EVIOCGABS(i), &ke->absinfo[i]) < 0)
			{
			    perror ("EVIOCGABS");
			    break;
			}
		    ke->prevabs[i] = ABS_UNSET;
		}
		if (i <= ke->max_abs)
		{
		    xfree (ke);
		    close (fd);
		    continue;
		}
	    }
	    mi->driver = ke;
	    mi->inputType = EvdevInputType;
	    if (KdRegisterFd (EvdevInputType, fd, EvdevRead, (void *) mi))
		n++;
	}
    }
    return TRUE;
}

static void
EvdevFini (void)
{
    KdMouseInfo	*mi;

    KdUnregisterFds (EvdevInputType, TRUE);
    for (mi = kdMouseInfo; mi; mi = mi->next)
    {
	if (mi->inputType == EvdevInputType)
	{
	    xfree (mi->driver);
	    mi->driver = 0;
	    mi->inputType = 0;
	}
    }
}

KdMouseFuncs LinuxEvdevMouseFuncs = {
    EvdevInit,
    EvdevFini,
};

#if 0
KdKeyboardFuncs LinuxEvdevKeyboardFuncs = {
    EvdevKbdLoad,
    EvdevKbdInit,
    EvdevKbdLeds,
    EvdevKbdBell,
    EvdevKbdFini,
    0,
};
#endif
