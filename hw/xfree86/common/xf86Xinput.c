/*
 * Copyright 1995-1999 by Frederic Lepied, France. <Lepied@XFree86.org>
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 2000-2002 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */
/* $XConsortium: xf86Xinput.c /main/14 1996/10/27 11:05:25 kaleb $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#ifdef XINPUT
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#endif
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Xinput.h"
#ifdef XINPUT
#include "XIstubs.h"
#endif
#include "mipointer.h"
#include "xf86InPriv.h"

#ifdef DPMSExtension
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#include "dpmsproc.h"
#endif

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#include "exevents.h"	/* AddInputDevice */
#include "exglobals.h"

#define EXTENSION_PROC_ARGS void *
#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */

#include "windowstr.h"	/* screenIsSaved */

#include <stdarg.h>

#include "osdep.h"		/* EnabledDevices */
#include <X11/Xpoll.h>
#include "xf86_OSproc.h"	/* sigio stuff */

/******************************************************************************
 * debugging macro
 *****************************************************************************/
#ifdef DBG
#undef DBG
#endif
#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 0

#if DEBUG
static int      debug_level = 0;

#define DBG(lvl, f) {if ((lvl) <= debug_level) f;}
#else
#define DBG(lvl, f)
#endif

/******************************************************************************
 * macros
 *****************************************************************************/
#define ENQUEUE(e) xf86eqEnqueue((e))

/***********************************************************************
 *
 * xf86AlwaysCoreControl --
 *	
 *	Control proc for the integer feedback that controls the always
 * core feature.
 *
 ***********************************************************************
 */
static void
xf86AlwaysCoreControl(DeviceIntPtr	device,
		      IntegerCtrl	*control)
{
}

/***********************************************************************
 *
 * Core devices functions --
 *	
 *	Test if device is the core device by checking the
 * value of always core feedback and the inputInfo struct.
 *
 ***********************************************************************
 */
_X_EXPORT int
xf86IsCorePointer(DeviceIntPtr	device)
{
    return(device == inputInfo.pointer);
}

static int
xf86ShareCorePointer(DeviceIntPtr	device)
{
    LocalDevicePtr	local = (LocalDevicePtr) device->public.devicePrivate;
    
    return((local->always_core_feedback &&
	    local->always_core_feedback->ctrl.integer_displayed));
}

static Bool
xf86SendDragEvents(DeviceIntPtr	device)
{
    LocalDevicePtr local = (LocalDevicePtr) device->public.devicePrivate;
    
    if (inputInfo.pointer->button->buttonsDown > 0)
	return (local->flags & XI86_SEND_DRAG_EVENTS);
    else
	return (TRUE);
}

int
xf86IsCoreKeyboard(DeviceIntPtr	device)
{
    LocalDevicePtr	local = (LocalDevicePtr) device->public.devicePrivate;
    
    return((local->flags & XI86_ALWAYS_CORE) ||
	   (device == inputInfo.keyboard));
}

_X_EXPORT void
xf86XInputSetSendCoreEvents(LocalDevicePtr local, Bool always)
{
    if (always) {
	local->flags |= XI86_ALWAYS_CORE;
    } else {
	local->flags &= ~XI86_ALWAYS_CORE;
    }
}

static int xf86CoreButtonState;

/***********************************************************************
 *
 * xf86CheckButton --
 *	
 *	Test if the core pointer button state is coherent with
 * the button event to send.
 *
 ***********************************************************************
 */
Bool
xf86CheckButton(int	button,
		int	down)
{
    int	check;
    int bit = (1 << (button - 1));

    check = xf86CoreButtonState & bit;
    
    DBG(5, ErrorF("xf86CheckButton "
		  "button=%d down=%d state=%d check=%d returns ",
		   button, down, xf86CoreButtonState, check));
    if ((check && down) || (!check && !down)) {
	DBG(5, ErrorF("FALSE\n"));
	return FALSE;
    }
    xf86CoreButtonState ^= bit;

    DBG(5, ErrorF("TRUE\n"));
    return TRUE;
}

/***********************************************************************
 *
 * xf86ProcessCommonOptions --
 * 
 *	Process global options.
 *
 ***********************************************************************
 */
_X_EXPORT void
xf86ProcessCommonOptions(LocalDevicePtr local,
			 pointer	list)
{
    if (xf86SetBoolOption(list, "AlwaysCore", 0) ||
	xf86SetBoolOption(list, "SendCoreEvents", 0)) {
	local->flags |= XI86_ALWAYS_CORE;
	xf86Msg(X_CONFIG, "%s: always reports core events\n", local->name);
    }

    if (xf86SetBoolOption(list, "CorePointer", 0)) {
	local->flags |= XI86_CORE_POINTER;
	xf86Msg(X_CONFIG, "%s: Core Pointer\n", local->name);
    }

    if (xf86SetBoolOption(list, "CoreKeyboard", 0)) {
	local->flags |= XI86_CORE_KEYBOARD;
	xf86Msg(X_CONFIG, "%s: Core Keyboard\n", local->name);
    }

    if (xf86SetBoolOption(list, "SendDragEvents", 1)) {
	local->flags |= XI86_SEND_DRAG_EVENTS;
    } else {
	xf86Msg(X_CONFIG, "%s: doesn't report drag events\n", local->name);
    }
    
    local->history_size = xf86SetIntOption(list, "HistorySize", 0);

    if (local->history_size > 0) {
	xf86Msg(X_CONFIG, "%s: has a history of %d motions\n", local->name,
		local->history_size);
    }
}

/***********************************************************************
 *
 * xf86XinputFinalizeInit --
 * 
 *	Create and initialize an integer feedback to control the always
 * core feature.
 *
 ***********************************************************************
 */
void
xf86XinputFinalizeInit(DeviceIntPtr	dev)
{
    LocalDevicePtr        local = (LocalDevicePtr)dev->public.devicePrivate;

    local->dxremaind = 0.0;
    local->dyremaind = 0.0;
    
    if (InitIntegerFeedbackClassDeviceStruct(dev, xf86AlwaysCoreControl) == FALSE) {
	ErrorF("Unable to init integer feedback for always core feature\n");
    } else {
	local->always_core_feedback = dev->intfeed;
	dev->intfeed->ctrl.integer_displayed = (local->flags & XI86_ALWAYS_CORE) ? 1 : 0;
    }
}

/***********************************************************************
 *
 * xf86ActivateDevice --
 * 
 *	Initialize an input device.
 *
 ***********************************************************************
 */
_X_EXPORT void
xf86ActivateDevice(LocalDevicePtr local)
{
    DeviceIntPtr	dev;

    if (local->flags & XI86_CONFIGURED) {
	int	open_on_init;
	
	open_on_init = local->flags &
		(XI86_OPEN_ON_INIT |
		 XI86_ALWAYS_CORE | XI86_CORE_POINTER | XI86_CORE_KEYBOARD);
	
	dev = AddInputDevice(local->device_control,
			     open_on_init);
	if (dev == NULL)
	    FatalError("Too many input devices");
	
	local->atom = MakeAtom(local->type_name,
			       strlen(local->type_name),
			       TRUE);
	AssignTypeAndName (dev, local->atom, local->name);
	dev->public.devicePrivate = (pointer) local;
	local->dev = dev;      
	
	xf86XinputFinalizeInit(dev);

	/*
	 * XXX Can a single device instance be both core keyboard and
	 * core pointer?  If so, this should be changed.
	 */
	if (local->flags & XI86_CORE_POINTER)
	    RegisterPointerDevice(dev);
	else if (local->flags & XI86_CORE_KEYBOARD)
	    RegisterKeyboardDevice(dev);
#ifdef XINPUT
	else
	    RegisterOtherDevice(dev);
#endif

	if (serverGeneration == 1) 
	    xf86Msg(X_INFO, "XINPUT: Adding extended input device \"%s\" (type: %s)\n",
		    local->name, local->type_name);
    }
}


#ifdef XINPUT
/***********************************************************************
 *
 * Caller:	ProcXOpenDevice
 *
 * This is the implementation-dependent routine to open an input device.
 * Some implementations open all input devices when the server is first
 * initialized, and never close them.  Other implementations open only
 * the X pointer and keyboard devices during server initialization,
 * and only open other input devices when some client makes an
 * XOpenDevice request.  This entry point is for the latter type of 
 * implementation.
 *
 * If the physical device is not already open, do it here.  In this case,
 * you need to keep track of the fact that one or more clients has the
 * device open, and physically close it when the last client that has
 * it open does an XCloseDevice.
 *
 * The default implementation is to do nothing (assume all input devices
 * are opened during X server initialization and kept open).
 *
 ***********************************************************************
 */

void
OpenInputDevice(DeviceIntPtr	dev,
		ClientPtr	client,
		int		*status)
{
    if (!dev->inited) {
	*status = BadDevice;
    } else {
	if (!dev->public.on) {
	    if (!EnableDevice(dev)) {
		*status = BadDevice;
	    } else {
		/* to prevent ProcXOpenDevice to call EnableDevice again */
		dev->startup = FALSE;
	    }
	}
    }
}


/***********************************************************************
 *
 * Caller:	ProcXChangeKeyboardDevice
 *
 * This procedure does the implementation-dependent portion of the work
 * needed to change the keyboard device.
 *
 * The X keyboard device has a FocusRec.  If the device that has been 
 * made into the new X keyboard did not have a FocusRec, 
 * ProcXChangeKeyboardDevice will allocate one for it.
 *
 * If you do not want clients to be able to focus the old X keyboard
 * device, call DeleteFocusClassDeviceStruct to free the FocusRec.
 *
 * If you support input devices with keys that you do not want to be 
 * used as the X keyboard, you need to check for them here and return 
 * a BadDevice error.
 *
 * The default implementation is to do nothing (assume you do want
 * clients to be able to focus the old X keyboard).  The commented-out
 * sample code shows what you might do if you don't want the default.
 *
 ***********************************************************************
 */

int
ChangeKeyboardDevice (DeviceIntPtr old_dev, DeviceIntPtr new_dev)
{
  /**********************************************************************
   * DeleteFocusClassDeviceStruct(old_dev);	 * defined in xchgptr.c *
   **********************************************************************/
  return !Success;
}


/***********************************************************************
 *
 * Caller:	ProcXChangePointerDevice
 *
 * This procedure does the implementation-dependent portion of the work
 * needed to change the pointer device.
 *
 * The X pointer device does not have a FocusRec.  If the device that
 * has been made into the new X pointer had a FocusRec, 
 * ProcXChangePointerDevice will free it.
 *
 * If you want clients to be able to focus the old pointer device that
 * has now become accessible through the input extension, you need to 
 * add a FocusRec to it here.
 *
 * The XChangePointerDevice protocol request also allows the client
 * to choose which axes of the new pointer device are used to move 
 * the X cursor in the X- and Y- directions.  If the axes are different
 * than the default ones, you need to keep track of that here.
 *
 * If you support input devices with valuators that you do not want to be 
 * used as the X pointer, you need to check for them here and return a 
 * BadDevice error.
 *
 * The default implementation is to do nothing (assume you don't want
 * clients to be able to focus the old X pointer).  The commented-out
 * sample code shows what you might do if you don't want the default.
 *
 ***********************************************************************
 */

int
ChangePointerDevice (
     DeviceIntPtr	old_dev,
     DeviceIntPtr	new_dev,
     unsigned char	x,
     unsigned char	y)
{
  /************************************************************************
    InitFocusClassDeviceStruct(old_dev);	* allow focusing old ptr*
    
    x_axis = x;					* keep track of new x-axis*
    y_axis = y;					* keep track of new y-axis*
    if (x_axis != 0 || y_axis != 1)
    axes_changed = TRUE;			* remember axes have changed*
    else
    axes_changed = FALSE;
   *************************************************************************/

  /*
   * We don't allow axis swap or other exotic features.
   */
  if (x == 0 && y == 1) {
      LocalDevicePtr	old_local = (LocalDevicePtr)old_dev->public.devicePrivate;
      LocalDevicePtr	new_local = (LocalDevicePtr)new_dev->public.devicePrivate;
      
      InitFocusClassDeviceStruct(old_dev);
    
      /* Restore Extended motion history information */
      old_dev->valuator->GetMotionProc   = old_local->motion_history_proc;
      old_dev->valuator->numMotionEvents = old_local->history_size;

      /* Save Extended motion history information */
      new_local->motion_history_proc = new_dev->valuator->GetMotionProc;
      new_local->history_size	     = new_dev->valuator->numMotionEvents;
      
      /* Set Core motion history information */
      new_dev->valuator->GetMotionProc   = miPointerGetMotionEvents;
      new_dev->valuator->numMotionEvents = miPointerGetMotionBufferSize();
      
    return Success;
  }
  else
    return !Success;
}


/***********************************************************************
 *
 * Caller:	ProcXCloseDevice
 *
 * Take care of implementation-dependent details of closing a device.
 * Some implementations may actually close the device, others may just
 * remove this clients interest in that device.
 *
 * The default implementation is to do nothing (assume all input devices
 * are initialized during X server initialization and kept open).
 *
 ***********************************************************************
 */

void
CloseInputDevice (DeviceIntPtr d, ClientPtr client)
{
  ErrorF("ProcXCloseDevice to close or not ?\n");
}


/***********************************************************************
 *
 * Caller:	ProcXListInputDevices
 *
 * This is the implementation-dependent routine to initialize an input 
 * device to the point that information about it can be listed.
 * Some implementations open all input devices when the server is first
 * initialized, and never close them.  Other implementations open only
 * the X pointer and keyboard devices during server initialization,
 * and only open other input devices when some client makes an
 * XOpenDevice request.  If some other process has the device open, the
 * server may not be able to get information about the device to list it.
 *
 * This procedure should be used by implementations that do not initialize
 * all input devices at server startup.  It should do device-dependent
 * initialization for any devices not previously initialized, and call
 * AddInputDevice for each of those devices so that a DeviceIntRec will be 
 * created for them.
 *
 * The default implementation is to do nothing (assume all input devices
 * are initialized during X server initialization and kept open).
 * The commented-out sample code shows what you might do if you don't want 
 * the default.
 *
 ***********************************************************************
 */

void
AddOtherInputDevices ()
{
}


/****************************************************************************
 *
 * Caller:	ProcXSetDeviceMode
 *
 * Change the mode of an extension device.
 * This function is used to change the mode of a device from reporting
 * relative motion to reporting absolute positional information, and
 * vice versa.
 * The default implementation below is that no such devices are supported.
 *
 ***********************************************************************
 */

int
SetDeviceMode (ClientPtr client, DeviceIntPtr dev, int mode)
{
  LocalDevicePtr        local = (LocalDevicePtr)dev->public.devicePrivate;

  if (local->switch_mode) {
    return (*local->switch_mode)(client, dev, mode);
  }
  else
    return BadMatch;
}


/***********************************************************************
 *
 * Caller:	ProcXSetDeviceValuators
 *
 * Set the value of valuators on an extension input device.
 * This function is used to set the initial value of valuators on
 * those input devices that are capable of reporting either relative
 * motion or an absolute position, and allow an initial position to be set.
 * The default implementation below is that no such devices are supported.
 *
 ***********************************************************************
 */

int
SetDeviceValuators (ClientPtr client, DeviceIntPtr dev, int *valuators,
		    int first_valuator, int num_valuators)
{
  return BadMatch;
}


/***********************************************************************
 *
 * Caller:	ProcXChangeDeviceControl
 *
 * Change the specified device controls on an extension input device.
 *
 ***********************************************************************
 */

int
ChangeDeviceControl (ClientPtr client, DeviceIntPtr dev, xDeviceCtl *control)
{
  LocalDevicePtr        local = (LocalDevicePtr)dev->public.devicePrivate;

  if (!local->control_proc) {
      return (BadMatch);
  }
  else {
      return (*local->control_proc)(local, control);
  }
}
#endif

/*
 * adapted from mieq.c to support extended events
 *
 */
#define QUEUE_SIZE  256

typedef struct _Event {
    xEvent	event;
#ifdef XINPUT
  deviceValuator val;
#endif
    ScreenPtr	pScreen;
} EventRec, *EventPtr;

typedef struct _EventQueue {
    HWEventQueueType head, tail;
    CARD32	lastEventTime;	    /* to avoid time running backwards */
    Bool	lastMotion;
    EventRec	events[QUEUE_SIZE]; /* static allocation for signals */
    DevicePtr	pKbd, pPtr;	    /* device pointer, to get funcs */
    ScreenPtr	pEnqueueScreen;	    /* screen events are being delivered to */
    ScreenPtr	pDequeueScreen;	    /* screen events are being dispatched to */
} EventQueueRec, *EventQueuePtr;

static EventQueueRec xf86EventQueue;

Bool
xf86eqInit (DevicePtr pKbd, DevicePtr pPtr)
{
    xf86EventQueue.head = xf86EventQueue.tail = 0;
    xf86EventQueue.lastEventTime = GetTimeInMillis ();
    xf86EventQueue.pKbd = pKbd;
    xf86EventQueue.pPtr = pPtr;
    xf86EventQueue.lastMotion = FALSE;
    xf86EventQueue.pEnqueueScreen = screenInfo.screens[0];
    xf86EventQueue.pDequeueScreen = xf86EventQueue.pEnqueueScreen;
    SetInputCheck (&xf86EventQueue.head, &xf86EventQueue.tail);
    return TRUE;
}

/*
 * Must be reentrant with ProcessInputEvents.  Assumption: xf86eqEnqueue
 * will never be interrupted.  If this is called from both signal
 * handlers and regular code, make sure the signal is suspended when
 * called from regular code.
 */

_X_EXPORT void
xf86eqEnqueue (xEvent *e)
{
    int		oldtail, newtail;
    Bool	isMotion;
#ifdef XINPUT
    int		count;
    
    xf86AssertBlockedSIGIO ("xf86eqEnqueue");
    switch (e->u.u.type) {
    case KeyPress:
    case KeyRelease:
#ifdef XFreeXDGA
	/* we do this here, because nobody seems to be calling
	   xf86PostKeyEvent().  We can't steal MotionNotify events here
	   because the motion-relative information has been lost already. */
	if(DGAStealKeyEvent(xf86EventQueue.pEnqueueScreen->myNum, e))
	    return;
#endif
	/* fall through */
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
        count = 1;
        break;
    default:
#ifdef XFreeXDGA
	if (DGAIsDgaEvent (e))
	{
	    count = 1;
	    break;
	}
#endif
	if (!((deviceKeyButtonPointer *) e)->deviceid & MORE_EVENTS) {
            count = 1;
	}
        else {
	    count = 2;
	}
        break;
    }
#endif

    oldtail = xf86EventQueue.tail;
    isMotion = e->u.u.type == MotionNotify;
    if (isMotion && xf86EventQueue.lastMotion && oldtail != xf86EventQueue.head) {
	if (oldtail == 0)
	    oldtail = QUEUE_SIZE;
	oldtail = oldtail - 1;
    }
    else {
    	newtail = oldtail + 1;
    	if (newtail == QUEUE_SIZE)
	    newtail = 0;
    	/* Toss events which come in late */
    	if (newtail == xf86EventQueue.head)
	    return;
	xf86EventQueue.tail = newtail;
    }
    
    xf86EventQueue.lastMotion = isMotion;
    xf86EventQueue.events[oldtail].event = *e;
#ifdef XINPUT
    if (count == 2) {
	xf86EventQueue.events[oldtail].val = *((deviceValuator *) (((deviceKeyButtonPointer *) e)+1));
    }
#endif
    /*
     * Make sure that event times don't go backwards - this
     * is "unnecessary", but very useful
     */
    if (e->u.keyButtonPointer.time < xf86EventQueue.lastEventTime &&
	xf86EventQueue.lastEventTime - e->u.keyButtonPointer.time < 10000) {
	
	xf86EventQueue.events[oldtail].event.u.keyButtonPointer.time =
	    xf86EventQueue.lastEventTime;
    }
    xf86EventQueue.events[oldtail].pScreen = xf86EventQueue.pEnqueueScreen;
}

/*
 * Call this from ProcessInputEvents()
 */
void
xf86eqProcessInputEvents ()
{
    EventRec	*e;
    int		x, y;
    struct {
	xEvent	event;
#ifdef XINPUT
	deviceValuator	val;
#endif
    }		xe;
#ifdef XINPUT
    DeviceIntPtr                dev;
    int                         id, count;
    deviceKeyButtonPointer      *dev_xe;
#endif

    while (xf86EventQueue.head != xf86EventQueue.tail) {
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens (SCREEN_SAVER_OFF, ScreenSaverReset);
#ifdef DPMSExtension
	else if (DPMSPowerLevel != DPMSModeOn)
	    SetScreenSaverTimer();

        if (DPMSPowerLevel != DPMSModeOn)
            DPMSSet(DPMSModeOn);
#endif

	e = &xf86EventQueue.events[xf86EventQueue.head];
	/*
	 * Assumption - screen switching can only occur on motion events
	 */
	if (e->pScreen != xf86EventQueue.pDequeueScreen) {
	    xf86EventQueue.pDequeueScreen = e->pScreen;
	    x = e->event.u.keyButtonPointer.rootX;
	    y = e->event.u.keyButtonPointer.rootY;
	    if (xf86EventQueue.head == QUEUE_SIZE - 1)
	    	xf86EventQueue.head = 0;
	    else
	    	++xf86EventQueue.head;
	    NewCurrentScreen (xf86EventQueue.pDequeueScreen, x, y);
	}
	else {
	    xe.event = e->event;
	    xe.val = e->val;
	    if (xf86EventQueue.head == QUEUE_SIZE - 1)
	    	xf86EventQueue.head = 0;
	    else
	    	++xf86EventQueue.head;
	    switch (xe.event.u.u.type) {
	    case KeyPress:
	    case KeyRelease:
	    	(*xf86EventQueue.pKbd->processInputProc)
		    (&xe.event, (DeviceIntPtr)xf86EventQueue.pKbd, 1);
	    	break;
#ifdef XINPUT
            case ButtonPress:
            case ButtonRelease:
            case MotionNotify:
	    	(*(inputInfo.pointer->public.processInputProc))
		    (&xe.event, (DeviceIntPtr)inputInfo.pointer, 1);
		break;

	    default:
#ifdef XFreeXDGA
		if (DGADeliverEvent (xf86EventQueue.pDequeueScreen, &xe.event))
		    break;
#endif
		dev_xe = (deviceKeyButtonPointer *) &xe.event;
		id = dev_xe->deviceid & DEVICE_BITS;
		if (!(dev_xe->deviceid & MORE_EVENTS)) {
		    count = 1;
		}
		else {
		    count = 2;
		}
		dev = LookupDeviceIntRec(id);
		if (dev == NULL) {
		    ErrorF("LookupDeviceIntRec id=0x%x not found\n", id);
/*                   FatalError("xf86eqProcessInputEvents : device not found.\n");
 */
		    break;
		}
		if (!dev->public.processInputProc) {
		    FatalError("xf86eqProcessInputEvents : device has no input proc.\n");
		    break;
		}
		(*dev->public.processInputProc)(&xe.event, dev, count);
#else
	    default:
	    	(*xf86EventQueue.pPtr->processInputProc)
		    (&xe.event, (DeviceIntPtr)xf86EventQueue.pPtr, 1);
#endif
	    	break;
	    }
	}
    }
}

void
xf86eqSwitchScreen(ScreenPtr	pScreen,
		   Bool		fromDIX)
{
    xf86EventQueue.pEnqueueScreen = pScreen;
  
    if (fromDIX)
	xf86EventQueue.pDequeueScreen = pScreen;
}

/* 
 * convenient functions to post events
 */

_X_EXPORT void
xf86PostMotionEvent(DeviceIntPtr	device,
		    int			is_absolute,
		    int			first_valuator,
		    int			num_valuators,
		    ...)
{
    va_list			var;
    int				loop;
    xEvent			xE[2];
    deviceKeyButtonPointer	*xev  = (deviceKeyButtonPointer*) xE;
    deviceValuator		*xv   = (deviceValuator*) xev+1;
    LocalDevicePtr		local = (LocalDevicePtr) device->public.devicePrivate;
    char			*buff = 0;
    Time			current;
    Bool			is_core = xf86IsCorePointer(device);
    Bool			is_shared = xf86ShareCorePointer(device);
    Bool			drag = xf86SendDragEvents(device);
    ValuatorClassPtr		val = device->valuator;
    int				valuator[6];
    int				oldaxis[6];
    int				*axisvals;
    int				dx = 0, dy = 0;
    float			mult;
    int				x, y;
    int				loop_start;
    int				i;
    int				num;
    
    DBG(5, ErrorF("xf86PostMotionEvent BEGIN 0x%x(%s) is_core=%s is_shared=%s is_absolute=%s\n",
		  device, device->name,
		  is_core ? "True" : "False",
		  is_shared ? "True" : "False",
		  is_absolute ? "True" : "False"));
    
    xf86Info.lastEventTime = xev->time = current = GetTimeInMillis();
    
    if (!is_core) {
      if (HAS_MOTION_HISTORY(local)) {
	buff = ((char *)local->motion_history +
		(sizeof(INT32) * local->dev->valuator->numAxes + sizeof(Time)) * local->last);
      }
    }

    if (num_valuators && (!val || (first_valuator + num_valuators > val->numAxes))) {
	ErrorF("Bad valuators reported for device \"%s\"\n", device->name);
	return;
    }

    axisvals = val->axisVal;
    
    va_start(var, num_valuators);

    loop_start = first_valuator;
    for(loop=0; loop<num_valuators; loop++) {
	
	valuator[loop%6] = va_arg(var,int);
	
	if (loop % 6 == 5 || loop == num_valuators - 1)	{
	    num = loop % 6 + 1;
	    /*
	     * Adjust first two relative valuators
	     */
	    if (!is_absolute && num_valuators >= 2 && loop_start == 0) {
		
		dx = valuator[0];
		dy = valuator[1];

		/*
		 * Accelerate
		 */
		if (device->ptrfeed && device->ptrfeed->ctrl.num) {
		    /* modeled from xf86Events.c */
		    if (device->ptrfeed->ctrl.threshold) {
			if ((abs(dx) + abs(dy)) >= device->ptrfeed->ctrl.threshold) {
			    local->dxremaind = ((float)dx * (float)(device->ptrfeed->ctrl.num)) /
			        (float)(device->ptrfeed->ctrl.den) + local->dxremaind;
			    valuator[0] = (int)local->dxremaind;
			    local->dxremaind = local->dxremaind - (float)valuator[0];
			    
			    local->dyremaind = ((float)dy * (float)(device->ptrfeed->ctrl.num)) /
			        (float)(device->ptrfeed->ctrl.den) + local->dyremaind;
			    valuator[1] = (int)local->dyremaind;
			    local->dyremaind = local->dyremaind - (float)valuator[1];
			}
		    }
		    else if (dx || dy) {
			mult = pow((float)(dx*dx+dy*dy),
				   ((float)(device->ptrfeed->ctrl.num) /
				    (float)(device->ptrfeed->ctrl.den) - 1.0) / 
				   2.0) / 2.0;
			if (dx) {
			    local->dxremaind = mult * (float)dx + local->dxremaind;
			    valuator[0] = (int)local->dxremaind;
			    local->dxremaind = local->dxremaind - (float)valuator[0];
			}
			if (dy) {
			    local->dyremaind = mult * (float)dy + local->dyremaind;
			    valuator[1] = (int)local->dyremaind;
			    local->dyremaind = local->dyremaind - (float)valuator[1];
			}
		    }
		    DBG(6, ErrorF("xf86PostMotionEvent acceleration v0=%d v1=%d\n",
				  valuator[0], valuator[1]));
		}
		
		/*
		 * Map current position back to device space in case
		 * the cursor was warped
		 */
		if (is_core || is_shared)
		{
		    miPointerPosition (&x, &y);
		    if (local->reverse_conversion_proc)
			(*local->reverse_conversion_proc)(local, x, y, axisvals);
		    else
		    {
			axisvals[0] = x;
			axisvals[1] = y;
		    }
		}
	    }
		
	    /*
	     * Update axes
	     */
	    for (i = 0; i < num; i++)
	    {
		oldaxis[i] = axisvals[loop_start + i];
	        if (is_absolute)
		    axisvals[loop_start + i] = valuator[i];
		else
		    axisvals[loop_start + i] += valuator[i];
	    }
		
	    /*
	     * Deliver extension event
	     */
	    if (!is_core) {
		xev->type = DeviceMotionNotify;
		xev->detail = 0;
		xev->deviceid = device->id | MORE_EVENTS;
            
		xv->type = DeviceValuator;
		xv->deviceid = device->id;
	    
		xv->device_state = 0;
		xv->num_valuators = num;
		xv->first_valuator = loop_start;
		memcpy (&xv->valuator0, &axisvals[loop_start],
			sizeof(INT32)*xv->num_valuators);
		
		if (HAS_MOTION_HISTORY(local)) {
		    *(Time*)buff = current;
		    memcpy(buff+sizeof(Time)+sizeof(INT32)*xv->first_valuator,
			   &axisvals[loop_start],
			   sizeof(INT32)*xv->num_valuators);
		}
		ENQUEUE(xE);
	    }
	    
	    /*
	     * Deliver core event
	     */
	    if (is_core ||
		(is_shared && num_valuators >= 2 && loop_start == 0)) {
#ifdef XFreeXDGA
		/*
		 * Let DGA peek at the event and steal it
		 */
		xev->type = MotionNotify;
		xev->detail = 0;
		if (is_absolute)
		{
		    dx = axisvals[0] - oldaxis[0];
		    dy = axisvals[1] - oldaxis[1];
		}
		if (DGAStealMouseEvent(xf86EventQueue.pEnqueueScreen->myNum,
				       xE, dx, dy))
		    continue;
#endif
		if (!(*local->conversion_proc)(local, loop_start, num,
					       axisvals[0], axisvals[1],
					       axisvals[2], axisvals[3],
					       axisvals[4], axisvals[5],
					       &x, &y))
		    continue;

		if (drag)
		    miPointerAbsoluteCursor (x, y, current);
		/*
		 * Retrieve the position
		 */
		miPointerPosition (&x, &y);
		if (local->reverse_conversion_proc)
		    (*local->reverse_conversion_proc)(local, x, y, axisvals);
		else
		{
		    axisvals[0] = x;
		    axisvals[1] = y;
		}
	    }
	    loop_start += 6;
	}
    }
    va_end(var);
    if (HAS_MOTION_HISTORY(local)) {
	local->last = (local->last + 1) % device->valuator->numMotionEvents;
	if (local->last == local->first)
	    local->first = (local->first + 1) % device->valuator->numMotionEvents;
    }
    DBG(5, ErrorF("xf86PostMotionEvent END   0x%x(%s) is_core=%s is_shared=%s\n",
		  device, device->name,
		  is_core ? "True" : "False",
		  is_shared ? "True" : "False"));
}

_X_EXPORT void
xf86PostProximityEvent(DeviceIntPtr	device,
		       int		is_in,
		       int		first_valuator,
		       int		num_valuators,
		       ...)
{
    va_list			var;
    int				loop;
    xEvent			xE[2];
    deviceKeyButtonPointer	*xev = (deviceKeyButtonPointer*) xE;
    deviceValuator		*xv = (deviceValuator*) xev+1;
    ValuatorClassPtr		val = device->valuator;
    Bool			is_core = xf86IsCorePointer(device);
    Bool			is_absolute = val && ((val->mode & 1) == Relative);
    
    DBG(5, ErrorF("xf86PostProximityEvent BEGIN 0x%x(%s) prox=%s is_core=%s is_absolute=%s\n",
		  device, device->name, is_in ? "true" : "false",
		  is_core ? "True" : "False",
		  is_absolute ? "True" : "False"));
    
    if (is_core) {
	return;
    }
  
    if (num_valuators && (!val || (first_valuator + num_valuators > val->numAxes))) {
	ErrorF("Bad valuators reported for device \"%s\"\n", device->name);
	return;
    }

    xev->type = is_in ? ProximityIn : ProximityOut;
    xev->detail = 0;
    xev->deviceid = device->id | MORE_EVENTS;
	
    xv->type = DeviceValuator;
    xv->deviceid = device->id;
    xv->device_state = 0;

    if ((device->valuator->mode & 1) == Relative) {
	num_valuators = 0;
    }
  
    if (num_valuators != 0) {
	int	*axisvals = val->axisVal;
	    
	va_start(var, num_valuators);

	for(loop=0; loop<num_valuators; loop++) {
	    switch (loop % 6) {
	    case 0:
		xv->valuator0 = is_absolute ? va_arg(var, int) : axisvals[loop]; 
		break;
	    case 1:
		xv->valuator1 = is_absolute ? va_arg(var, int) : axisvals[loop];
		break;
	    case 2:
		xv->valuator2 = is_absolute ? va_arg(var, int) : axisvals[loop];
		break;
	    case 3:
		xv->valuator3 = is_absolute ? va_arg(var, int) : axisvals[loop];
		break;
	    case 4:
		xv->valuator4 = is_absolute ? va_arg(var, int) : axisvals[loop];
		break;
	    case 5:
		xv->valuator5 = is_absolute ? va_arg(var, int) : axisvals[loop];
		break;
	    }
	    if ((loop % 6 == 5) || (loop == num_valuators - 1)) {
		xf86Info.lastEventTime = xev->time = GetTimeInMillis();

		xv->num_valuators = (loop % 6) + 1;
		xv->first_valuator = first_valuator + (loop / 6) * 6;
		ENQUEUE(xE);
	    }
	}
	va_end(var);
    }
    else {
	/* no valuator */
	xf86Info.lastEventTime = xev->time = GetTimeInMillis();

	xv->num_valuators = 0;
	xv->first_valuator = 0;
	ENQUEUE(xE);
    }
    DBG(5, ErrorF("xf86PostProximityEvent END   0x%x(%s) prox=%s is_core=%s is_absolute=%s\n",
		  device, device->name, is_in ? "true" : "false",
		  is_core ? "True" : "False",
		  is_absolute ? "True" : "False"));
    
}

_X_EXPORT void
xf86PostButtonEvent(DeviceIntPtr	device,
		    int			is_absolute,
		    int			button,
		    int			is_down,
		    int			first_valuator,
		    int			num_valuators,
		    ...)
{
    va_list			var;
    int				loop;
    xEvent			xE[2];
    deviceKeyButtonPointer	*xev	        = (deviceKeyButtonPointer*) xE;
    deviceValuator		*xv	        = (deviceValuator*) xev+1;
    ValuatorClassPtr		val		= device->valuator;
    Bool			is_core		= xf86IsCorePointer(device);
    Bool			is_shared       = xf86ShareCorePointer(device);
    
    DBG(5, ErrorF("xf86PostButtonEvent BEGIN 0x%x(%s) button=%d down=%s is_core=%s is_shared=%s is_absolute=%s\n",
		  device, device->name, button,
		  is_down ? "True" : "False",
		  is_core ? "True" : "False",
		  is_shared ? "True" : "False",
		  is_absolute ? "True" : "False"));
    
    /* Check the core pointer button state not to send an inconsistent
     * event. This can happen with the AlwaysCore feature.
     */
    if ((is_core || is_shared) && 
	!xf86CheckButton(device->button->map[button], is_down)) 
    {
	return;
    }
    
    if (num_valuators && (!val || (first_valuator + num_valuators > val->numAxes))) {
	ErrorF("Bad valuators reported for device \"%s\"\n", device->name);
	return;
    }

    if (!is_core) {
	xev->type = is_down ? DeviceButtonPress : DeviceButtonRelease;
	xev->detail = button;
	xev->deviceid = device->id | MORE_EVENTS;
	    
	xv->type = DeviceValuator;
	xv->deviceid = device->id;
	xv->device_state = 0;

	if (num_valuators != 0) {
	    int			*axisvals = val->axisVal;
	    
	    va_start(var, num_valuators);
      
	    for(loop=0; loop<num_valuators; loop++) {
		switch (loop % 6) {
		case 0:
		    xv->valuator0 = is_absolute ? va_arg(var, int) : axisvals[loop];
		    break;
		case 1:
		    xv->valuator1 = is_absolute ? va_arg(var, int) : axisvals[loop];
		    break;
		case 2:
		    xv->valuator2 = is_absolute ? va_arg(var, int) : axisvals[loop];
		    break;
		case 3:
		    xv->valuator3 = is_absolute ? va_arg(var, int) : axisvals[loop];
		    break;
		case 4:
		    xv->valuator4 = is_absolute ? va_arg(var, int) : axisvals[loop];
		    break;
		case 5:
		    xv->valuator5 = is_absolute ? va_arg(var, int) : axisvals[loop];
		    break;
		}
		if ((loop % 6 == 5) || (loop == num_valuators - 1)) {
		    xf86Info.lastEventTime = xev->time = GetTimeInMillis();
		    xv->num_valuators = (loop % 6) + 1;
		    xv->first_valuator = first_valuator + (loop / 6) * 6;
		    ENQUEUE(xE);
		    
		}
	    }
	    va_end(var);
	}
	else {
	    /* no valuator */
	    xf86Info.lastEventTime = xev->time = GetTimeInMillis();
	    xv->num_valuators = 0;
	    xv->first_valuator = 0;
	    ENQUEUE(xE);
	}
    }

    /* removed rootX/rootY as DIX sets these fields */
    if (is_core || is_shared) {
	xE->u.u.type = is_down ? ButtonPress : ButtonRelease;
	xE->u.u.detail =  device->button->map[button];
	xf86Info.lastEventTime = xE->u.keyButtonPointer.time = GetTimeInMillis();
	
#ifdef XFreeXDGA
	if (!DGAStealMouseEvent(xf86EventQueue.pEnqueueScreen->myNum, xE, 0, 0))
#endif
	    ENQUEUE(xE);
    }
    DBG(5, ErrorF("xf86PostButtonEvent END\n"));
}

_X_EXPORT void
xf86PostKeyEvent(DeviceIntPtr	device,
		 unsigned int	key_code,
		 int		is_down,
		 int		is_absolute,
		 int		first_valuator,
		 int		num_valuators,
		 ...)
{
    va_list			var;
    int				loop;
    xEvent			xE[2];
    deviceKeyButtonPointer	*xev = (deviceKeyButtonPointer*) xE;
    deviceValuator		*xv = (deviceValuator*) xev+1;
    
    va_start(var, num_valuators);

    for(loop=0; loop<num_valuators; loop++) {
	switch (loop % 6) {
	case 0:
	    xv->valuator0 = va_arg(var, int);
	    break;
	case 1:
	    xv->valuator1 = va_arg(var, int);
	    break;
	case 2:
	    xv->valuator2 = va_arg(var, int);
	    break;
	case 3:
	    xv->valuator3 = va_arg(var, int);
	    break;
	case 4:
	    xv->valuator4 = va_arg(var, int);
	    break;
	case 5:
	    xv->valuator5 = va_arg(var, int);
	    break;
	}
	if (((loop % 6 == 5) || (loop == num_valuators - 1))) {
	    xev->type = is_down ? DeviceKeyPress : DeviceKeyRelease;
	    xev->detail = key_code;
	    
	    xf86Info.lastEventTime = xev->time = GetTimeInMillis();
	    xev->deviceid = device->id | MORE_EVENTS;
	    
	    xv->type = DeviceValuator;
	    xv->deviceid = device->id;
	    xv->device_state = 0;
	    /* if the device is in the relative mode we don't have to send valuators */
	    xv->num_valuators = is_absolute ? (loop % 6) + 1 : 0;
	    xv->first_valuator = first_valuator + (loop / 6) * 6;
	    
	    ENQUEUE(xE);
	    /* if the device is in the relative mode only one event is needed */
	    if (!is_absolute) break;
	}
    }
    va_end(var);
}

_X_EXPORT void
xf86PostKeyboardEvent(DeviceIntPtr      device,
                      unsigned int      key_code,
                      int               is_down)
{
    xEvent                      xE[2];
    deviceKeyButtonPointer      *xev = (deviceKeyButtonPointer*) xE;

    if (xf86IsCoreKeyboard(device)) {
        xev->type = is_down ? KeyPress : KeyRelease;
    } else {
        xev->type = is_down ? DeviceKeyPress : DeviceKeyRelease;
    }
    xev->detail = key_code;
    xf86Info.lastEventTime = xev->time = GetTimeInMillis();

#ifdef XFreeXDGA
    /* if(!DGAStealKeyEvent(xf86EventQueue.pEnqueueScreen->myNum, xE)) */
#endif
    ENQUEUE(xE);
}

/* 
 * Motion history management.
 */

_X_EXPORT void
xf86MotionHistoryAllocate(LocalDevicePtr	local)
{
    ValuatorClassPtr	valuator = local->dev->valuator;
    
    if (!HAS_MOTION_HISTORY(local))
	return;
    if (local->motion_history) xfree(local->motion_history);
    local->motion_history = xalloc((sizeof(INT32) * valuator->numAxes + sizeof(Time))
				   * valuator->numMotionEvents);
    local->first = 0;
    local->last	 = 0;
}

_X_EXPORT int
xf86GetMotionEvents(DeviceIntPtr	dev,
		    xTimecoord		*buff,
		    unsigned long	start,
		    unsigned long	stop,
		    ScreenPtr		pScreen)
{
    LocalDevicePtr	local	 = (LocalDevicePtr)dev->public.devicePrivate;
    ValuatorClassPtr	valuator = dev->valuator;
    int			num  	 = 0;
    int			loop	 = local->first;
    int			size;
    Time		current;
    
    if (!HAS_MOTION_HISTORY(local))
	return 0;

    size = (sizeof(INT32) * valuator->numAxes + sizeof(Time));

    while (loop != local->last) {
	current = *(Time*)(((char *)local->motion_history)+loop*size);
	if (current > stop)
	    return num;
	if (current >= start) {
	    memcpy(((char *)buff)+size*num,
		   ((char *)local->motion_history)+loop*size, size);
	    num++;
	}
	loop = (loop + 1) % valuator->numMotionEvents;
    }
    return num;
}

_X_EXPORT LocalDevicePtr
xf86FirstLocalDevice()
{
    return xf86InputDevs;
}

/* 
 * Cx     - raw data from touch screen
 * Sxhigh - scaled highest dimension
 *          (remember, this is of rows - 1 because of 0 origin)
 * Sxlow  - scaled lowest dimension
 * Rxhigh - highest raw value from touch screen calibration
 * Rxlow  - lowest raw value from touch screen calibration
 *
 * This function is the same for X or Y coordinates.
 * You may have to reverse the high and low values to compensate for
 * different orgins on the touch screen vs X.
 */

_X_EXPORT int
xf86ScaleAxis(int	Cx,
	      int	Sxhigh,
	      int	Sxlow,
	      int	Rxhigh,
	      int	Rxlow )
{
    int X;
    int dSx = Sxhigh - Sxlow;
    int dRx = Rxhigh - Rxlow;

    dSx = Sxhigh - Sxlow;
    if (dRx) {
	X = ((dSx * (Cx - Rxlow)) / dRx) + Sxlow;
    }
    else {
	X = 0;
	ErrorF ("Divide by Zero in xf86ScaleAxis");
    }
    
    if (X > Sxlow)
	X = Sxlow;
    if (X < Sxhigh)
	X = Sxhigh;
    
    return (X);
}

/*
 * This function checks the given screen against the current screen and
 * makes changes if appropriate. It should be called from an XInput driver's
 * ReadInput function before any events are posted, if the device is screen
 * specific like a touch screen.
 */
_X_EXPORT void
xf86XInputSetScreen(LocalDevicePtr	local,
		    int			screen_number,
		    int			x,
		    int			y)
{
    if ((xf86IsCorePointer(local->dev) || xf86ShareCorePointer(local->dev)) &&
	(miPointerCurrentScreen() != screenInfo.screens[screen_number])) {
	miPointerSetNewScreen (screen_number, x, y);
    }
}


_X_EXPORT void
xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, int minval, int maxval,
			   int resolution, int min_res, int max_res)
{
#ifdef XINPUT
    if (maxval == -1) {
	if (axnum == 0)
	    maxval = screenInfo.screens[0]->width - 1;
	else if (axnum == 1)
	    maxval = screenInfo.screens[0]->height - 1;
	/* else? */
    }
    InitValuatorAxisStruct(dev, axnum, minval, maxval, resolution, min_res,
			   max_res);
#endif
}

/*
 * Set the valuator values to be in synch with dix/event.c
 * DefineInitialRootWindow().
 */
_X_EXPORT void
xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum)
{
#ifdef XINPUT
    if (axnum == 0)
	dev->valuator->axisVal[0] = screenInfo.screens[0]->width / 2;
    else if (axnum == 1)
	dev->valuator->axisVal[1] = screenInfo.screens[0]->height / 2;
#endif
}

/* end of xf86Xinput.c */
