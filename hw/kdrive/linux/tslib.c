/*
 * $RCSId: xc/programs/Xserver/hw/kdrive/linux/tslib.c,v 1.1 2002/11/01 22:27:49 keithp Exp $
 * TSLIB based touchscreen driver for TinyX
 * Derived from ts.c by Keith Packard
 * Derived from ps2.c by Jim Gettys
 *
 * Copyright © 1999 Keith Packard
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 MontaVista Software Inc.
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
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Michael Taht or MontaVista not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Michael Taht and Montavista make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MICHAEL TAHT AND MONTAVISTA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
 * IN NO EVENT SHALL EITHER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xpoll.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include <sys/ioctl.h>
#include <tslib.h>

static long lastx = 0, lasty = 0;
static struct tsdev *tsDev = NULL;

void (*tslib_raw_event_hook)(int x, int y, int pressure, void *closure);
void *tslib_raw_event_closure;

int KdTsPhyScreen = 0;

static void
TsRead (int tsPort, void *closure)
{
    KdMouseInfo	    *mi = closure;
    struct ts_sample event;
    int		    n;
    long	    x, y;
    unsigned long   flags;

    if (tslib_raw_event_hook)
      {
	if (ts_read_raw(tsDev, &event, 1) == 1)
	  {
	    tslib_raw_event_hook (event.x, event.y, event.pressure, tslib_raw_event_closure);
	  }
	return;
      }

    while (ts_read(tsDev, &event, 1) == 1)
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

	KdEnqueueMouseEvent (mi, flags, x, y);
    }
}

static char *TsNames[] = {
  NULL,
  "/dev/ts",	
  "/dev/touchscreen/0",
};

#define NUM_TS_NAMES	(sizeof (TsNames) / sizeof (TsNames[0]))

int TsInputType;

static int
TslibEnable (int not_needed_fd, void *closure)
{
  KdMouseInfo	    *mi = closure;
  int		     fd = 0;

  fprintf(stderr, "%s() called\n", __func__);

  if(!(tsDev = ts_open(mi->name, 0))) {
    fprintf(stderr, "%s() failed to open %s\n", __func__, mi->name );
    return -1; 			/* XXX Not sure what to return here */
  }
  
  if (ts_config(tsDev))
    return -1;
  fd=ts_fd(tsDev);

  return fd;
}

static void
TslibDisable (int fd, void *closure)
{
  ts_close(tsDev);
  tsDev = NULL;
}

static int
TslibInit (void)
{
    int		i, j = 0;
    KdMouseInfo	*mi, *next;
    int		fd= 0;
    int		n = 0;

    if (!TsInputType)
	TsInputType = KdAllocInputType ();

    KdMouseInfoAdd(); /* allocate empty kdMouseInfo entry for ts  */

    for (mi = kdMouseInfo; mi; mi = next)
    {
	next = mi->next;
	if (mi->inputType)
	    continue;

	/* Check for tslib env var device setting */
	if ((TsNames[0] = getenv("TSLIB_TSDEVICE")) == NULL)
	  j++;
	
	if (!mi->name)
	{
	    for (i = j; i < NUM_TS_NAMES; i++)    
	    {

	      /* XXX Should check for  */

		if(!(tsDev = ts_open(TsNames[i],0))) continue;
	        if (ts_config(tsDev)) continue;
	        fd=ts_fd(tsDev);
		if (fd >= 0) 
		{
		    mi->name = KdSaveString (TsNames[i]);
		    break;
		}
	    }
	} else {

	  if(!(tsDev = ts_open(mi->name,0))) 
	    continue;
	  if (ts_config(tsDev)) continue; 
	  fd=ts_fd(tsDev);

	}

	if (fd > 0 && tsDev != 0) 
	  {
	    mi->driver = (void *) fd;
	    mi->inputType = TsInputType;
	    if (KdRegisterFd (TsInputType, fd, TsRead, (void *) mi))
	      n++;

	    /* Set callbacks for vt switches etc */
	    KdRegisterFdEnableDisable (fd, TslibEnable, TslibDisable);

	  } 
	else 
	  {
	    fprintf(stderr, "%s() failed to open tslib\n", __func__);	    
	    if (fd > 0) close(fd);
	  }


	}

    return n;
}

static void
TslibFini (void)
{
    KdMouseInfo	*mi;

    KdUnregisterFds (TsInputType, TRUE);
    for (mi = kdMouseInfo; mi; mi = mi->next)
    {
	if (mi->inputType == TsInputType)
	{
	    if(mi->driver) {
		ts_close(tsDev);
		tsDev = NULL;
	    }
	    mi->driver = 0;
	    mi->inputType = 0;
	}
    }
}

KdMouseFuncs TsFuncs = {
    TslibInit,
    TslibFini
};
