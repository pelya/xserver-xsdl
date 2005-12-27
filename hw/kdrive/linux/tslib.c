/*
 * $RCSId: xc/programs/Xserver/hw/kdrive/linux/tslib.c,v 1.1 2002/11/01 22:27:49 keithp Exp $
 * TSLIB based touchscreen driver for TinyX
 * Derived from ts.c by Keith Packard
 * Derived from ps2.c by Jim Gettys
 *
 * Copyright © 1999 Keith Packard
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 MontaVista Software Inc.
 * Copyright © 2005 OpenedHand Ltd.
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
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Matthew Allum or OpenedHand not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Matthew Allum and OpenedHand make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MATTHEW ALLUM AND OPENEDHAND DISCLAIM ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
 * IN NO EVENT SHALL EITHER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
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
#include <tslib.h>

static struct tsdev *tsDev = NULL;

static char *TsNames[] = {
  NULL, 			/* set via TSLIB_TSDEVICE */
  "/dev/ts",	
  "/dev/touchscreen/0",
};

#define NUM_TS_NAMES (sizeof (TsNames) / sizeof (TsNames[0]))

/* For XCalibrate extension */
void (*tslib_raw_event_hook)(int x, int y, int pressure, void *closure);
void *tslib_raw_event_closure;

int TsInputType = 0;
int KdTsPhyScreen = 0; 		/* XXX Togo .. */

static void
TsRead (int tsPort, void *closure)
{
    KdMouseInfo	    *mi = closure;
    struct ts_sample event;
    long	    x, y;
    unsigned long   flags;

    if (tslib_raw_event_hook)
      {
	/* XCalibrate Ext */
	if (ts_read_raw(tsDev, &event, 1) == 1)
	  {
	    tslib_raw_event_hook (event.x, 
				  event.y, 
				  event.pressure, 
				  tslib_raw_event_closure);
	  }
	return;
      }

    while (ts_read(tsDev, &event, 1) == 1)
      {
	flags = (event.pressure) ? KD_BUTTON_1 : 0;
	x = event.x;
	y = event.y;
	
	KdEnqueueMouseEvent (mi, flags, x, y);
      }
}

static int
TsLibOpen(char *dev)
{
  if(!(tsDev = ts_open(dev, 0)))
    return -1;

  if (ts_config(tsDev))
    return -1;

  return ts_fd(tsDev);
}

static int
TslibEnable (int not_needed_fd, void *closure)
{
  KdMouseInfo	    *mi = closure;
  int		     fd = 0;

  if ((fd = TsLibOpen(mi->name)) == -1)
    ErrorF ("Unable to re-enable TSLib ( on %s )", mi->name);

  return fd;
}

static void
TslibDisable (int fd, void *closure)
{
  if (tsDev)
    ts_close(tsDev);
  tsDev = NULL;
}

static int
TslibInit (void)
{
  int		i, j = 0;
  KdMouseInfo	*mi, *next;
  int		fd = 0;
  int           req_type;

  if (!TsInputType)
    {
      TsInputType = KdAllocInputType ();
      KdParseMouse(0); /* allocate safe slot in kdMouseInfo */
      req_type = 0;
    }
  else req_type = TsInputType; 	/* is being re-inited */
  
  for (mi = kdMouseInfo; mi; mi = next)
    {
      next = mi->next;
      
      /* find a usuable slot */
      if (mi->inputType != req_type) 
	continue;
      
      /* Check for tslib env var device setting */
      if ((TsNames[0] = getenv("TSLIB_TSDEVICE")) == NULL)
	j++;
      
      if (!mi->name)
	{
	  for (i = j; i < NUM_TS_NAMES; i++)    
	    {
	      fd = TsLibOpen(TsNames[i]);
	      
	      if (fd >= 0) 
		{
		  mi->name = KdSaveString (TsNames[i]);
		  break;
		}
	    }
	} 
      else 
	fd = TsLibOpen(mi->name);
      
      if (fd >= 0 && tsDev != NULL) 
	{
	  mi->driver    = (void *) fd;
	  mi->inputType = TsInputType;
	  
	  KdRegisterFd (TsInputType, fd, TsRead, (void *) mi);
	  
	  /* Set callbacks for vt switches etc */
	  KdRegisterFdEnableDisable (fd, TslibEnable, TslibDisable);
	  
	  return TRUE;
	} 
    }
  
  ErrorF ("Failed to open TSLib device, tried ");
  for (i = j; i < NUM_TS_NAMES; i++)    
    ErrorF ("%s ", TsNames[i]);
  ErrorF (".\n");
  if (!TsNames[0]) 
    ErrorF ("Try setting TSLIB_TSDEVICE to valid /dev entry?\n");
  
  if (fd > 0) 
    close(fd);
  
  return FALSE;
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
	    if(mi->driver) 
	      {
		ts_close(tsDev);
		tsDev = NULL;
	      }
	    mi->driver    = 0;

	    /* If below is set to 0, then MouseInit() will trash it,
	     * setting to 'mouse type' ( via server reset). Therefore 
             * Leave it alone and work around in TslibInit()  ( see
             * req_type ).
	    */
	    /* mi->inputType = 0; */
	}
    }
}

KdMouseFuncs TsFuncs = {
    TslibInit,
    TslibFini
};
