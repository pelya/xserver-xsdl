/*
 * $XFree86: xc/programs/Xserver/hw/kdrive/linux/ts.c,v 1.2 2000/09/26 15:57:04 tsi Exp $
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

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include "Xpoll.h"
#include <sys/ioctl.h>
#include <linux/h3600_ts.h>	/* touch screen events */

static long lastx = 0, lasty = 0;
int TsScreen;
extern int TsFbdev;

void
TsRead (int tsPort)
{
    TS_EVENT	    event;
    long	    buf[3];
    int		    n;
    long	    pressure;
    long	    x, y;
    unsigned long   flags;
    unsigned long   buttons;

    n = Ps2ReadBytes (tsPort, (char *) &event, 
			 sizeof (event), sizeof (event));
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
	    if (TsScreen == TsFbdev) {
	    	flags = KD_BUTTON_1;
	    	x = event.x;
	    	y = event.y;
	    } else {
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
	KdEnqueueMouseEvent (flags, x, y);
    }
}

char	*TsNames[] = {
  "/dev/ts",	
  "/dev/h3600_ts" /* temporary name; note this code can try
			   to open more than one device */
};

#define NUM_TS_NAMES	(sizeof (TsNames) / sizeof (TsNames[0]))

int
TsInit (void)
{
    int	    i;
    int	    TsPort;

    for (i = 0; i < NUM_TS_NAMES; i++)    
    {
	TsPort = open (TsNames[i], 0);
	if (TsPort >= 0) 
	    return TsPort;
    }
    perror("Touch screen not found.\n");
    exit (1);
}

void
TsFini (int tsPort)
{
    if (tsPort >= 0)
	close (tsPort);
}

KdTsFuncs TsFuncs = {
    TsInit,
    TsRead,
    TsFini
};
