/* $Xorg: Events.c,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
/* $XFree86: xc/programs/Xserver/hw/xnest/Events.c,v 1.3 2003/11/16 05:05:20 dawes Exp $ */

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"

#include "mi.h"

#include "Xnest.h"

#include "Args.h"
#include "Color.h"
#include "Display.h"
#include "Screen.h"
#include "XNWindow.h"
#include "Events.h"

CARD32 lastEventTime = 0;

void
ProcessInputEvents()
{
  mieqProcessInputEvents();
}

int
TimeSinceLastInputEvent()
{
    if (lastEventTime == 0)
        lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

void
SetTimeSinceLastInputEvent()
{
  lastEventTime = GetTimeInMillis();
}

static Bool
xnestExposurePredicate(Display *display, XEvent *event, char *args)
{
  return (event->type == Expose || event->type == ProcessedExpose);
}

static Bool
xnestNotExposurePredicate(Display *display, XEvent *event, char *args)
{
  return !xnestExposurePredicate(display, event, args);
}

void
xnestCollectExposures()
{
  XEvent X;
  WindowPtr pWin;
  RegionRec Rgn;
  BoxRec Box;
  
  while (XCheckIfEvent(xnestDisplay, &X, xnestExposurePredicate, NULL)) {
    pWin = xnestWindowPtr(X.xexpose.window);
    
    if (pWin) {
      Box.x1 = pWin->drawable.x + wBorderWidth(pWin) + X.xexpose.x;
      Box.y1 = pWin->drawable.y + wBorderWidth(pWin) + X.xexpose.y;
      Box.x2 = Box.x1 + X.xexpose.width;
      Box.y2 = Box.y1 + X.xexpose.height;
      
      REGION_INIT(pWin->drawable.pScreen, &Rgn, &Box, 1);
      
      miWindowExposures(pWin, &Rgn, NullRegion); 
    }
  }
}

void
xnestCollectEvents()
{
  XEvent X;
  xEvent x;
  ScreenPtr pScreen;

  while (XCheckIfEvent(xnestDisplay, &X, xnestNotExposurePredicate, NULL)) {
    switch (X.type) {
    case KeyPress:
      x.u.u.type = KeyPress;
      x.u.u.detail = X.xkey.keycode;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case KeyRelease:
      x.u.u.type = KeyRelease;
      x.u.u.detail = X.xkey.keycode;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case ButtonPress:
      x.u.u.type = ButtonPress;
      x.u.u.detail = X.xbutton.button;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case ButtonRelease:
      x.u.u.type = ButtonRelease;
      x.u.u.detail = X.xbutton.button;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case MotionNotify:
      x.u.u.type = MotionNotify;
      x.u.keyButtonPointer.rootX = X.xmotion.x;
      x.u.keyButtonPointer.rootY = X.xmotion.y;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case FocusIn:
      if (X.xfocus.detail != NotifyInferior) {
	pScreen = xnestScreen(X.xfocus.window);
	if (pScreen)
	  xnestDirectInstallColormaps(pScreen);
      }
      break;
   
    case FocusOut:
      if (X.xfocus.detail != NotifyInferior) {
	pScreen = xnestScreen(X.xfocus.window);
	if (pScreen)
	  xnestDirectUninstallColormaps(pScreen);
      }
      break;

    case KeymapNotify:
      break;

    case EnterNotify:
      if (X.xcrossing.detail != NotifyInferior) {
	pScreen = xnestScreen(X.xcrossing.window);
	if (pScreen) {
	  NewCurrentScreen(pScreen, X.xcrossing.x, X.xcrossing.y);
	  x.u.u.type = MotionNotify;
	  x.u.keyButtonPointer.rootX = X.xcrossing.x;
	  x.u.keyButtonPointer.rootY = X.xcrossing.y;
	  x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
	  mieqEnqueue(&x);
	  xnestDirectInstallColormaps(pScreen);
	}
      }
      break;
      
    case LeaveNotify:
      if (X.xcrossing.detail != NotifyInferior) {
	pScreen = xnestScreen(X.xcrossing.window);
	if (pScreen) {
	  xnestDirectUninstallColormaps(pScreen);
	}	
      }
      break;
      
    case DestroyNotify:
      if (xnestParentWindow != (Window) 0 &&
	  X.xdestroywindow.window == xnestParentWindow)
	exit (0);
      break;
      
    default:
      ErrorF("xnest warning: unhandled event\n");
      break;
    }
  }
}
