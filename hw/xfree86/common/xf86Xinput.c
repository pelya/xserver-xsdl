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
#include "xf86Optrec.h"
#endif
#include "mipointer.h"
#include "xf86InPriv.h"

#ifdef DPMSExtension
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#include "dpmsproc.h"
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

#include "mi.h"

/******************************************************************************
 * debugging macro
 *****************************************************************************/
#ifdef DEBUG
static int      debug_level = 0;
#define DBG(lvl, f) {if ((lvl) <= debug_level) f;}
#else
#define DBG(lvl, f)
#endif

xEvent *xf86Events = NULL;

static Bool
xf86SendDragEvents(DeviceIntPtr	device)
{
    LocalDevicePtr local = (LocalDevicePtr) device->public.devicePrivate;
    
    if (device->button->buttonsDown > 0)
        return (local->flags & XI86_SEND_DRAG_EVENTS);
    else
        return (TRUE);
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
    if (!xf86SetBoolOption(list, "AlwaysCore", 0) ||
        xf86SetBoolOption(list, "SendCoreEvents", 0) ||
        xf86SetBoolOption(list, "CorePointer", 0) ||
        xf86SetBoolOption(list, "CoreKeyboard", 0)) {
        local->flags |= XI86_ALWAYS_CORE;
        xf86Msg(X_CONFIG, "%s: always reports core events\n", local->name);
    }

    if (xf86SetBoolOption(list, "SendDragEvents", 1)) {
        local->flags |= XI86_SEND_DRAG_EVENTS;
    } else {
        xf86Msg(X_CONFIG, "%s: doesn't report drag events\n", local->name);
    }
}

void
xf86AlwaysCoreControl(DeviceIntPtr pDev, IntegerCtrl *control)
{
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
        dev = AddInputDevice(local->device_control, TRUE);

        if (dev == NULL)
            FatalError("Too many input devices");
        
        local->atom = MakeAtom(local->type_name,
                               strlen(local->type_name),
                               TRUE);
        AssignTypeAndName(dev, local->atom, local->name);
        dev->public.devicePrivate = (pointer) local;
        local->dev = dev;      
        
        xf86XinputFinalizeInit(dev);

        dev->coreEvents = local->flags & XI86_ALWAYS_CORE;
        RegisterOtherDevice(dev);

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
    if (!dev->inited)
        ActivateDevice(dev);

    if (!dev->public.on) {
        if (EnableDevice(dev)) {
            dev->startup = FALSE;
        }
        else {
            ErrorF("couldn't enable device %s\n", dev->name);
            *status = BadDevice;
            return;
        }
    }

    *status = Success;
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
    LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;

    if (local->set_device_valuators)
	return (*local->set_device_valuators)(local, valuators, first_valuator,
					      num_valuators);

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
      switch (control->control) {
      case DEVICE_CORE:
      case DEVICE_RESOLUTION:
      case DEVICE_ABS_CALIB:
      case DEVICE_ABS_AREA:
        return Success;
      default:
        return BadMatch;
      }
  }
  else {
      return (*local->control_proc)(local, control);
  }
}
#endif

int
NewInputDeviceRequest (InputOption *options)
{
    IDevRec *idev = NULL;
    InputDriverPtr drv = NULL;
    InputInfoPtr pInfo = NULL;
    InputOption *option = NULL;
    DeviceIntPtr dev = NULL;

    idev = xcalloc(sizeof(*idev), 1);
    if (!idev)
        return BadAlloc;

    for (option = options; option; option = option->next) {
        if (strcmp(option->key, "driver") == 0) {
            if (!xf86LoadOneModule(option->value, NULL))
                return BadName;
            drv = xf86LookupInputDriver(option->value);
            if (!drv) {
                xf86Msg(X_ERROR, "No input driver matching `%s'\n",
                        option->value);
                return BadName;
            }
            idev->driver = xstrdup(option->value);
            if (!idev->driver) {
                xfree(idev);
                return BadAlloc;
            }
        }
        if (strcmp(option->key, "name") == 0 ||
            strcmp(option->key, "identifier") == 0) {
            idev->identifier = xstrdup(option->value);
            if (!idev->identifier) {
                xfree(idev);
                return BadAlloc;
            }
        }
    }

    if (!drv->PreInit) {
        xf86Msg(X_ERROR,
                "Input driver `%s' has no PreInit function (ignoring)\n",
                drv->driverName);
        return BadImplementation;
    }

    idev->commonOptions = NULL;
    for (option = options; option; option = option->next)
        idev->commonOptions = xf86addNewOption(idev->commonOptions,
                                               option->key, option->value);
    idev->extraOptions = NULL;

    pInfo = drv->PreInit(drv, idev, 0);

    if (!pInfo) {
        xf86Msg(X_ERROR, "PreInit returned NULL for \"%s\"\n", idev->identifier);
        return BadMatch;
    }
    else if (!(pInfo->flags & XI86_CONFIGURED)) {
        xf86Msg(X_ERROR, "PreInit failed for input device \"%s\"\n",
                idev->identifier);
        xf86DeleteInput(pInfo, 0);
        return BadMatch;
    }

    xf86ActivateDevice(pInfo);

    dev = pInfo->dev;
    dev->inited = ((*dev->deviceProc)(dev, DEVICE_INIT) == Success);
    if (dev->inited && dev->startup)
        EnableDevice(dev);

    return Success;
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
    va_list var;
    int i = 0, nevents = 0;
    Bool drag = xf86SendDragEvents(device);
    LocalDevicePtr local = (LocalDevicePtr) device->public.devicePrivate;
    int *valuators = NULL;
    int flags = 0;

    if (is_absolute)
        flags = POINTER_ABSOLUTE;
    else
        flags = POINTER_RELATIVE | POINTER_ACCELERATE;
    
    valuators = xcalloc(sizeof(int), num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuators[i] = va_arg(var, int);
    va_end(var);

    if (!xf86Events)
        xf86Events = (xEvent *)xcalloc(sizeof(xEvent), GetMaximumEventsNum());
    if (!xf86Events)
        FatalError("Couldn't allocate event store\n");

    nevents = GetPointerEvents(xf86Events, device, MotionNotify, 0,
                               flags, first_valuator, num_valuators,
                               valuators);

    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, xf86Events + i);

    xfree(valuators);
}

_X_EXPORT void
xf86PostProximityEvent(DeviceIntPtr	device,
                       int		is_in,
                       int		first_valuator,
                       int		num_valuators,
                       ...)
{
    va_list var;
    int i, nevents, *valuators = NULL;

    valuators = xcalloc(sizeof(int), num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuators[i] = va_arg(var, int);
    va_end(var);

    if (!xf86Events)
        xf86Events = (xEvent *)xcalloc(sizeof(xEvent), GetMaximumEventsNum());
    if (!xf86Events)
        FatalError("Couldn't allocate event store\n");

    nevents = GetProximityEvents(xf86Events, device,
                                 is_in ? ProximityIn : ProximityOut, 
                                 first_valuator, num_valuators, valuators);
    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, xf86Events + i);

    xfree(valuators);
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
    va_list var;
    int *valuators = NULL;
    int i = 0, nevents = 0;
    
    valuators = xcalloc(sizeof(int), num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuators[i] = va_arg(var, int);
    va_end(var);

    if (!xf86Events)
        xf86Events = (xEvent *)xcalloc(sizeof(xEvent), GetMaximumEventsNum());
    if (!xf86Events)
        FatalError("Couldn't allocate event store\n");

    nevents = GetPointerEvents(xf86Events, device,
                               is_down ? ButtonPress : ButtonRelease, button,
                               is_absolute ? POINTER_ABSOLUTE :
                                             POINTER_RELATIVE,
                               first_valuator, num_valuators, valuators);

    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, xf86Events + i);

    xfree(valuators);
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
    va_list var;
    int i = 0, nevents = 0, *valuators = NULL;

    /* instil confidence in the user */
    DebugF("this function has never been tested properly.  if things go quite "
           "badly south after this message, then xf86PostKeyEvent is "
           "broken.\n");

    if (!xf86Events)
        xf86Events = (xEvent *)xcalloc(sizeof(xEvent), GetMaximumEventsNum());
    if (!xf86Events)
        FatalError("Couldn't allocate event store\n");

    if (is_absolute) {
        valuators = xcalloc(sizeof(int), num_valuators);
        va_start(var, num_valuators);
        for (i = 0; i < num_valuators; i++)
            valuators[i] = va_arg(var, int);
        va_end(var);

        nevents = GetKeyboardValuatorEvents(xf86Events, device,
                                            is_down ? KeyPress : KeyRelease,
                                            key_code, first_valuator,
                                            num_valuators, valuators);
        xfree(valuators);
    }
    else {
        nevents = GetKeyboardEvents(xf86Events, device,
                                    is_down ? KeyPress : KeyRelease,
                                    key_code);
    }

    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, xf86Events + i);
}

_X_EXPORT void
xf86PostKeyboardEvent(DeviceIntPtr      device,
                      unsigned int      key_code,
                      int               is_down)
{
    int nevents = 0, i = 0;

    if (!xf86Events)
        xf86Events = (xEvent *)xcalloc(sizeof(xEvent), GetMaximumEventsNum());
    if (!xf86Events)
        FatalError("Couldn't allocate event store\n");

    nevents = GetKeyboardEvents(xf86Events, device,
                                is_down ? KeyPress : KeyRelease, key_code);

    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, xf86Events + i);
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
    if (miPointerGetScreen(local->dev) !=
          screenInfo.screens[screen_number]) {
	miPointerSetScreen(local->dev, screen_number, x, y);
    }
}


_X_EXPORT void
xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, int minval, int maxval,
			   int resolution, int min_res, int max_res)
{
    if (!dev || !dev->valuator)
        return;

    InitValuatorAxisStruct(dev, axnum, minval, maxval, resolution, min_res,
			   max_res);
}

/*
 * Set the valuator values to be in synch with dix/event.c
 * DefineInitialRootWindow().
 */
_X_EXPORT void
xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum)
{
    if (axnum == 0) {
	dev->valuator->axisVal[0] = screenInfo.screens[0]->width / 2;
        dev->valuator->lastx = dev->valuator->axisVal[0];
    }
    else if (axnum == 1) {
	dev->valuator->axisVal[1] = screenInfo.screens[0]->height / 2;
        dev->valuator->lasty = dev->valuator->axisVal[1];
    }
}

/* end of xf86Xinput.c */
