/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sysv/xqueue.c,v 3.8.2.1 1997/07/13 14:45:04 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: xqueue.c,v 1.3 2000/08/17 19:51:33 cpqbld Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

#ifdef XQUEUE

static xqEventQueue      *XqueQaddr;
static int xqueFd = -1;
#ifndef XQUEUE_ASYNC
static int xquePipe[2];
#endif

#ifdef XKB
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
extern Bool noXkbExtension;
#endif

#ifdef XINPUT
#include "xf86_Config.h"
#include "xf86Xinput.h"
#endif
extern int miPointerGetMotionEvents(DeviceIntPtr pPtr, xTimecoord *coords,
				unsigned long start, unsigned long stop,
				ScreenPtr pScreen);

#ifndef XQUEUE_ASYNC
/*
 * xf86XqueSignal --
 *	Trap the signal from xqueue and let it be known that events are
 *	ready for collection
 */

static void
xf86XqueSignal(int signum)
{
  xf86Info.mouseDev->xquePending = 1;
  /*
   * This is a hack, but it is the only reliable way I can find of letting
   * the main select() loop know that there is more input waiting.  Receiving
   * a signal will interrupt select(), but there is no way I can find of
   * dealing with events that come in between the end of processing the
   * last set and when select() gets called.
   *
   * Suggestions for better ways of dealing with this without going back to
   * asynchronous event processing are welcome.
   */
  write(xquePipe[1], "X", 1);
  signal(SIGUSR2, xf86XqueSignal);
}
#endif
  

/*
 * xf86XqueRequest --
 *      Notice an i/o request from the xqueue.
 */

void
xf86XqueRequest()
{
  xqEvent  *XqueEvents = XqueQaddr->xq_events;
  int      XqueHead = XqueQaddr->xq_head;
  char buf[100];

  while (XqueHead != XqueQaddr->xq_tail)
    {

      switch(XqueEvents[XqueHead].xq_type) {
	
      case XQ_BUTTON:
	xf86PostMseEvent(xf86Info.pMouse,
			~(XqueEvents[XqueHead].xq_code) & 0x07, 0, 0);
	break;

      case XQ_MOTION:
	xf86PostMseEvent(xf86Info.pMouse,
			~(XqueEvents[XqueHead].xq_code) & 0x07,
		       XqueEvents[XqueHead].xq_x,
		       XqueEvents[XqueHead].xq_y);
	break;

      case XQ_KEY:
	xf86PostKbdEvent(XqueEvents[XqueHead].xq_code);
	break;
	
      default:
	ErrorF("Unknown Xque Event: 0x%02x\n", XqueEvents[XqueHead].xq_type);
      }
      
      if ((++XqueHead) == XqueQaddr->xq_size) XqueHead = 0;
    }

  /* reenable the signal-processing */
  xf86Info.inputPending = TRUE;
#ifdef XQUEUE_ASYNC
  signal(SIGUSR2, (void (*)()) xf86XqueRequest);
#else
#if 0
  signal(SIGUSR2, (void (*)()) xf86XqueSignal);
#endif
#endif

#ifndef XQUEUE_ASYNC
  {
    int rval;

    while ((rval = read(xquePipe[0], buf, sizeof(buf))) > 0)
#ifdef DEBUG
      ErrorF("Read %d bytes from xquePipe[0]\n", rval);
#else
      ;
#endif
  }
#endif

  XqueQaddr->xq_head = XqueQaddr->xq_tail;
  xf86Info.mouseDev->xquePending = 0;
  XqueQaddr->xq_sigenable = 1; /* UNLOCK */
}



/*
 * xf86XqueEnable --
 *      Enable the handling of the Xque
 */

static int
xf86XqueEnable()
{
  static struct kd_quemode xqueMode;
  static Bool              was_here = FALSE;

  if (!was_here) {
    if ((xqueFd = open("/dev/mouse", O_RDONLY|O_NDELAY)) < 0)
      {
	if (xf86AllowMouseOpenFail) {
	  ErrorF("Cannot open /dev/mouse (%s) - Continuing...\n",
		strerror(errno));
	  return (Success);
	} else {
	  Error ("Cannot open /dev/mouse");
	  return (!Success);
	}
      }
#ifndef XQUEUE_ASYNC
    pipe(xquePipe);
    fcntl(xquePipe[0],F_SETFL,fcntl(xquePipe[0],F_GETFL,0)|O_NDELAY);
    fcntl(xquePipe[1],F_SETFL,fcntl(xquePipe[1],F_GETFL,0)|O_NDELAY);
#endif
    was_here = TRUE;
  }

  if (xf86Info.mouseDev->xqueSema++ == 0) 
    {
#ifdef XQUEUE_ASYNC
      (void) signal(SIGUSR2, (void (*)()) xf86XqueRequest);
#else
      (void) signal(SIGUSR2, (void (*)()) xf86XqueSignal);
#endif
      xqueMode.qsize = 64;    /* max events */
      xqueMode.signo = SIGUSR2;
      ioctl(xf86Info.consoleFd, KDQUEMODE, NULL);
      
      if (ioctl(xf86Info.consoleFd, KDQUEMODE, &xqueMode) < 0) {
	Error ("Cannot set KDQUEMODE");
	/* CONSTCOND */
	return (!Success);
      }
      
      XqueQaddr = (xqEventQueue *)xqueMode.qaddr;
      XqueQaddr->xq_sigenable = 1; /* UNLOCK */
    }

  return(Success);
}



/*
 * xf86XqueDisable --
 *      disable the handling of the Xque
 */

static int
xf86XqueDisable()
{
  if (xf86Info.mouseDev->xqueSema-- == 1)
    {
      
      XqueQaddr->xq_sigenable = 0; /* LOCK */
      
      if (ioctl(xf86Info.consoleFd, KDQUEMODE, NULL) < 0) {
	Error ("Cannot unset KDQUEMODE");
	/* CONSTCOND */
	return (!Success);
      }
    }

  return(Success);
}



/*
 * xf86XqueMseProc --
 *      Handle the initialization, etc. of a mouse
 */

int
xf86XqueMseProc(pPointer, what)
     DeviceIntPtr	pPointer;
     int        what;
{
  MouseDevPtr	mouse = MOUSE_DEV(pPointer);
  unchar        map[4];
  int ret;
 
  mouse->device = pPointer;

  switch (what)
    {
    case DEVICE_INIT: 
      
      pPointer->public.on = FALSE;
      
      map[1] = 1;
      map[2] = 2;
      map[3] = 3;
      InitPointerDeviceStruct((DevicePtr)pPointer, 
			      map, 
			      3, 
			      miPointerGetMotionEvents, 
			      (PtrCtrlProcPtr)xf86MseCtrl, 
			      miPointerGetMotionBufferSize());
      break;
      
    case DEVICE_ON:
      mouse->lastButtons = 0;
      mouse->emulateState = 0;
      pPointer->public.on = TRUE;
      ret = xf86XqueEnable();
#ifndef XQUEUE_ASYNC
      if (xquePipe[0] != -1)
	AddEnabledDevice(xquePipe[0]);
#endif
      return(ret);
      
    case DEVICE_CLOSE:
    case DEVICE_OFF:
      pPointer->public.on = FALSE;
      ret = xf86XqueDisable();
#ifndef XQUEUE_ASYNC
      if (xquePipe[0] != -1)
	RemoveEnabledDevice(xquePipe[0]);
#endif
      return(ret);
    }
  
  return Success;
}



/*
 * xf86XqueKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 */

int
xf86XqueKbdProc (pKeyboard, what)
     DeviceIntPtr pKeyboard;	/* Keyboard to manipulate */
     int       what;	    	/* What to do to it */
{
  KeySymsRec  keySyms;
  CARD8       modMap[MAP_LENGTH];

  switch (what) {
      
  case DEVICE_INIT:
    
    xf86KbdGetMapping(&keySyms, modMap);
    
    /*
     * Get also the initial led settings
     */
    ioctl(xf86Info.consoleFd, KDGETLED, &xf86Info.leds);

    /*
     * Perform final initialization of the system private keyboard
     * structure and fill in various slots in the device record
     * itself which couldn't be filled in before.
     */
    pKeyboard->public.on = FALSE;

#ifdef XKB
    if (noXkbExtension) {
#endif
    InitKeyboardDeviceStruct((DevicePtr)xf86Info.pKeyboard,
			     &keySyms,
			     modMap,
			     xf86KbdBell,
			     (KbdCtrlProcPtr)xf86KbdCtrl);
#ifdef XKB
    } else {
	XkbComponentNamesRec names;
	if (XkbInitialMap) {
	    if ((xf86Info.xkbkeymap = strchr(XkbInitialMap, '/')) != NULL)
		xf86Info.xkbkeymap++;
	    else
		xf86Info.xkbkeymap = XkbInitialMap;
	}
	if (xf86Info.xkbkeymap) {
	    names.keymap = xf86Info.xkbkeymap;
	    names.keycodes = NULL;
	    names.types = NULL;
	    names.compat = NULL;
	    names.symbols = NULL;
	    names.geometry = NULL;
	} else {
	    names.keymap = NULL;
	    names.keycodes = xf86Info.xkbkeycodes;
	    names.types = xf86Info.xkbtypes;
	    names.compat = xf86Info.xkbcompat;
	    names.symbols = xf86Info.xkbsymbols;
	    names.geometry = xf86Info.xkbgeometry;
	}
	if ((xf86Info.xkbkeymap || xf86Info.xkbcomponents_specified)
	   && (xf86Info.xkbmodel == NULL || xf86Info.xkblayout == NULL)) {
		xf86Info.xkbrules = NULL;
	}
	XkbSetRulesDflts(xf86Info.xkbrules, xf86Info.xkbmodel,
			 xf86Info.xkblayout, xf86Info.xkbvariant,
			 xf86Info.xkboptions);
	XkbInitKeyboardDeviceStruct(pKeyboard, 
				    &names,
				    &keySyms, 
				    modMap, 
				    xf86KbdBell,
				    (KbdCtrlProcPtr)xf86KbdCtrl);
    }
#endif

    xf86InitKBD(TRUE);
    break;
    
  case DEVICE_ON:
    pKeyboard->public.on = TRUE;
    xf86InitKBD(FALSE);
    return(xf86XqueEnable());
    
  case DEVICE_CLOSE:
  case DEVICE_OFF:
    pKeyboard->public.on = FALSE;
    return(xf86XqueDisable());
  }
  
  return (Success);
}


/*
 * xf86XqueEvents --
 *      Get some events from our queue. Nothing to do here ...
 */

void
xf86XqueEvents()
{
}

#endif /* XQUEUE */
