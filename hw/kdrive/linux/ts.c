/*
 * $RCSId: xc/programs/Xserver/hw/kdrive/linux/ts.c,v 1.9 2002/08/15 18:07:48 keithp Exp $
 *
 * Derived from ps2.c by Jim Gettys
 *
 * Copyright © 1999 Keith Packard
 * Copyright © 2000 Compaq Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard or Compaq not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard and Compaq makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD AND COMPAQ DISCLAIM ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
 * IN NO EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xpoll.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include <sys/ioctl.h>
#include <linux/h3600_ts.h>	/* touch screen events */

static long lastx = 0, lasty = 0;

int KdTsPhyScreen = 0;

static int
TsReadBytes (int fd, char *buf, int len, int min)
{
    int		    n, tot;
    fd_set	    set;
    struct timeval  tv;

    tot = 0;
    while (len)
    {
	n = read (fd, buf, len);
	if (n > 0)
	{
	    tot += n;
	    buf += n;
	    len -= n;
	}
	if (tot % min == 0)
	    break;
	FD_ZERO (&set);
	FD_SET (fd, &set);
	tv.tv_sec = 0;
	tv.tv_usec = 100 * 1000;
	n = select (fd + 1, &set, 0, 0, &tv);
	if (n <= 0)
	    break;
    }
    return tot;
}

static void
TsRead (int tsPort, void *closure)
{
    KdMouseInfo	    *mi = closure;
    TS_EVENT	    event;
    int		    n;
    long	    x, y;
    unsigned long   flags;

    n = TsReadBytes (tsPort, (char *) &event, sizeof (event), sizeof (event));
    if (n == sizeof (event))  
    {
	if (event.pressure) 
	{
	    /* 
	     * HACK ATTACK.  (static global variables used !)
	     * Here we test for the touch screen driver actually being on the
	     * touch screen, if it is we send absolute coordinates. If not,
	     * then we send delta's so that we can track the entire vga screen.
	     */
	    if (KdCurScreen == KdTsPhyScreen) {
	    	flags = KD_BUTTON_1;
	    	x = event.x;
	    	y = event.y;
	    }
	    else
	      {
	    	flags = /* KD_BUTTON_1 |*/ KD_MOUSE_DELTA;
	    	if ((lastx == 0) || (lasty == 0)) {
	    	    x = 0;
	    	    y = 0;
	    	} else {
	    	    x = event.x - lastx;
	    	    y = event.y - lasty;
	    	}
	    	lastx = event.x;
	    	lasty = event.y;
	      }
	} else {
	    flags = KD_MOUSE_DELTA;
	    x = 0;
	    y = 0;
	    lastx = 0;
	    lasty = 0;
	}
	KdEnqueueMouseEvent (mi, flags, x, y);
    }
}

char	*TsNames[] = {
  "/dev/ts",	
  "/dev/h3600_ts" /* temporary name; note this code can try
			   to open more than one device */
};

#define NUM_TS_NAMES	(sizeof (TsNames) / sizeof (TsNames[0]))

int TsInputType;

static int
TsEnable (int fd, void *closure)
{
    KdMouseInfo *mi = (KdMouseInfo *)closure;

    return open (mi->name, 0);
}

static void
TsDisable (int fd, void *closure)
{
    close (fd);
}

static int
TsInit (void)
{
    int		i;
    int		fd;
    KdMouseInfo	*mi, *next;
    int		n = 0;

    if (!TsInputType)
	TsInputType = KdAllocInputType ();
    
    for (mi = kdMouseInfo; mi; mi = next)
    {
	next = mi->next;
	if (mi->inputType)
	    continue;
	if (!mi->name)
	{
	    for (i = 0; i < NUM_TS_NAMES; i++)    
	    {
		fd = open (TsNames[i], 0);
		if (fd >= 0) 
		{
		    mi->name = KdSaveString (TsNames[i]);
		    break;
		}
	    }
	}
	else
	    fd = open (mi->name, 0);
	if (fd >= 0)
	{
	    struct h3600_ts_calibration cal;
	    /*
	     * Check to see if this is a touch screen
	     */
	    if (ioctl (fd, TS_GET_CAL, &cal) != -1)
	    {
		mi->driver = (void *) fd;
		mi->inputType = TsInputType;
		if (KdRegisterFd (TsInputType, fd, TsRead, (void *) mi))
		{
		    /* Set callbacks for vt switches etc */
		    KdRegisterFdEnableDisable (fd, TsEnable, TsDisable);

		    n++;
		}
	    }
	    else
		close (fd);
	}
    }

    return 0;
}

static void
TsFini (void)
{
    KdMouseInfo	*mi;

    KdUnregisterFds (TsInputType, TRUE);
    for (mi = kdMouseInfo; mi; mi = mi->next)
    {
	if (mi->inputType == TsInputType)
	{
	    mi->driver = 0;
	    mi->inputType = 0;
	}
    }
}

KdMouseFuncs TsFuncs = {
    TsInit,
    TsFini
};
