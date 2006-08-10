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

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
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
#include "Keyboard.h"
#include "mipointer.h"

CARD32 lastEventTime = 0;

void
ProcessInputEvents()
{
  mieqProcessInputEvents();
  miPointerUpdate();
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
xnestQueueKeyEvent(int type, unsigned int keycode)
{
  xEvent x;
  x.u.u.type = type;
  x.u.u.detail = keycode;
  x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
  mieqEnqueue(&x);
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
      xnestUpdateModifierState(X.xkey.state);
      xnestQueueKeyEvent(KeyPress, X.xkey.keycode);
      break;
      
    case KeyRelease:
      xnestUpdateModifierState(X.xkey.state);
      xnestQueueKeyEvent(KeyRelease, X.xkey.keycode);
      break;
      
    case ButtonPress:
      xnestUpdateModifierState(X.xkey.state);
      x.u.u.type = ButtonPress;
      x.u.u.detail = X.xbutton.button;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case ButtonRelease:
      xnestUpdateModifierState(X.xkey.state);
      x.u.u.type = ButtonRelease;
      x.u.u.detail = X.xbutton.button;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
      break;
      
    case MotionNotify:
#if 0
      x.u.u.type = MotionNotify;
      x.u.keyButtonPointer.rootX = X.xmotion.x;
      x.u.keyButtonPointer.rootY = X.xmotion.y;
      x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
      mieqEnqueue(&x);
#endif 
      miPointerAbsoluteCursor (X.xmotion.x, X.xmotion.y, 
			       lastEventTime = GetTimeInMillis());
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
#if 0
	  x.u.u.type = MotionNotify;
	  x.u.keyButtonPointer.rootX = X.xcrossing.x;
	  x.u.keyButtonPointer.rootY = X.xcrossing.y;
	  x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis();
	  mieqEnqueue(&x);
#endif
	  miPointerAbsoluteCursor (X.xcrossing.x, X.xcrossing.y, 
				   lastEventTime = GetTimeInMillis());
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

    case CirculateNotify:
    case ConfigureNotify:
    case GravityNotify:
    case MapNotify:
    case ReparentNotify:
    case UnmapNotify:
      break;
      
    default:
      ErrorF("xnest warning: unhandled event\n");
      break;
    }
  }
}
