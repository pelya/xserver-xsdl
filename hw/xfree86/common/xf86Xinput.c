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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/Xatom.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Config.h"
#include "xf86Xinput.h"
#include "xf86Optrec.h"
#include "mipointer.h"
#include "extinit.h"

#include "exevents.h"	/* AddInputDevice */
#include "exglobals.h"
#include "eventstr.h"

#include <string.h>     /* InputClassMatches */
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include <stdarg.h>
#include <stdint.h>          /* for int64_t */

#include "mi.h"

#include <ptrveloc.h>          /* dix pointer acceleration */
#include <xserver-properties.h>

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#include "xkbsrv.h"


EventListPtr xf86Events = NULL;

static int
xf86InputDevicePostInit(DeviceIntPtr dev);

/**
 * Eval config and modify DeviceVelocityRec accordingly
 */
static void
ProcessVelocityConfiguration(DeviceIntPtr pDev, char* devname, pointer list,
                             DeviceVelocityPtr s)
{
    int tempi;
    float tempf;
    Atom float_prop = XIGetKnownProperty(XATOM_FLOAT);
    Atom prop;

    if(!s)
        return;

    /* common settings (available via device properties) */
    tempf = xf86SetRealOption(list, "ConstantDeceleration", 1.0);
    if (tempf > 1.0) {
        xf86Msg(X_CONFIG, "%s: (accel) constant deceleration by %.1f\n",
                devname, tempf);
        prop = XIGetKnownProperty(ACCEL_PROP_CONSTANT_DECELERATION);
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }

    tempf = xf86SetRealOption(list, "AdaptiveDeceleration", 1.0);
    if (tempf > 1.0) {
        xf86Msg(X_CONFIG, "%s: (accel) adaptive deceleration by %.1f\n",
                devname, tempf);
        prop = XIGetKnownProperty(ACCEL_PROP_ADAPTIVE_DECELERATION);
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }

    /* select profile by number */
    tempi = xf86SetIntOption(list, "AccelerationProfile",
            s->statistics.profile_number);

    prop = XIGetKnownProperty(ACCEL_PROP_PROFILE_NUMBER);
    if (XIChangeDeviceProperty(pDev, prop, XA_INTEGER, 32,
                               PropModeReplace, 1, &tempi, FALSE) == Success) {
        xf86Msg(X_CONFIG, "%s: (accel) acceleration profile %i\n", devname,
                tempi);
    } else {
        xf86Msg(X_CONFIG, "%s: (accel) acceleration profile %i is unknown\n",
                devname, tempi);
    }

    /* set scaling */
    tempf = xf86SetRealOption(list, "ExpectedRate", 0);
    prop = XIGetKnownProperty(ACCEL_PROP_VELOCITY_SCALING);
    if (tempf > 0) {
        tempf = 1000.0 / tempf;
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    } else {
        tempf = xf86SetRealOption(list, "VelocityScale", s->corr_mul);
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }

    tempi = xf86SetIntOption(list, "VelocityTrackerCount", -1);
    if (tempi > 1)
	InitTrackers(s, tempi);

    s->initial_range = xf86SetIntOption(list, "VelocityInitialRange",
                                        s->initial_range);

    s->max_diff = xf86SetRealOption(list, "VelocityAbsDiff", s->max_diff);

    tempf = xf86SetRealOption(list, "VelocityRelDiff", -1);
    if (tempf >= 0) {
	xf86Msg(X_CONFIG, "%s: (accel) max rel. velocity difference: %.1f%%\n",
	        devname, tempf*100.0);
	s->max_rel_diff = tempf;
    }

    /*  Configure softening. If const deceleration is used, this is expected
     *  to provide better subpixel information so we enable
     *  softening by default only if ConstantDeceleration is not used
     */
    s->use_softening = xf86SetBoolOption(list, "Softening",
                                         s->const_acceleration == 1.0);

    s->average_accel = xf86SetBoolOption(list, "AccelerationProfileAveraging",
                                         s->average_accel);

    s->reset_time = xf86SetIntOption(list, "VelocityReset", s->reset_time);
}

static void
ApplyAccelerationSettings(DeviceIntPtr dev){
    int scheme, i;
    DeviceVelocityPtr pVel;
    InputInfoPtr pInfo = (InputInfoPtr)dev->public.devicePrivate;
    char* schemeStr;

    if (dev->valuator && dev->ptrfeed) {
	schemeStr = xf86SetStrOption(pInfo->options, "AccelerationScheme", "");

	scheme = dev->valuator->accelScheme.number;

	if (!xf86NameCmp(schemeStr, "predictable"))
	    scheme = PtrAccelPredictable;

	if (!xf86NameCmp(schemeStr, "lightweight"))
	    scheme = PtrAccelLightweight;

	if (!xf86NameCmp(schemeStr, "none"))
	    scheme = PtrAccelNoOp;

        /* reinit scheme if needed */
        if (dev->valuator->accelScheme.number != scheme) {
            if (dev->valuator->accelScheme.AccelCleanupProc) {
                dev->valuator->accelScheme.AccelCleanupProc(dev);
            }

            if (InitPointerAccelerationScheme(dev, scheme)) {
		xf86Msg(X_CONFIG, "%s: (accel) selected scheme %s/%i\n",
		        pInfo->name, schemeStr, scheme);
	    } else {
        	xf86Msg(X_CONFIG, "%s: (accel) could not init scheme %s\n",
		        pInfo->name, schemeStr);
        	scheme = dev->valuator->accelScheme.number;
            }
        } else {
            xf86Msg(X_CONFIG, "%s: (accel) keeping acceleration scheme %i\n",
                    pInfo->name, scheme);
        }

        free(schemeStr);

        /* process special configuration */
        switch (scheme) {
            case PtrAccelPredictable:
                pVel = GetDevicePredictableAccelData(dev);
                ProcessVelocityConfiguration (dev, pInfo->name, pInfo->options,
                                              pVel);
                break;
        }

        i = xf86SetIntOption(pInfo->options, "AccelerationNumerator",
                             dev->ptrfeed->ctrl.num);
        if (i >= 0)
            dev->ptrfeed->ctrl.num = i;

        i = xf86SetIntOption(pInfo->options, "AccelerationDenominator",
                             dev->ptrfeed->ctrl.den);
        if (i > 0)
            dev->ptrfeed->ctrl.den = i;

        i = xf86SetIntOption(pInfo->options, "AccelerationThreshold",
                             dev->ptrfeed->ctrl.threshold);
        if (i >= 0)
            dev->ptrfeed->ctrl.threshold = i;

        xf86Msg(X_CONFIG, "%s: (accel) acceleration factor: %.3f\n",
                            pInfo->name, ((float)dev->ptrfeed->ctrl.num)/
                                         ((float)dev->ptrfeed->ctrl.den));
        xf86Msg(X_CONFIG, "%s: (accel) acceleration threshold: %i\n",
                pInfo->name, dev->ptrfeed->ctrl.threshold);
    }
}

/***********************************************************************
 *
 * xf86ProcessCommonOptions --
 * 
 *	Process global options.
 *
 ***********************************************************************
 */
void
xf86ProcessCommonOptions(InputInfoPtr pInfo,
                         pointer	list)
{
    if (!xf86SetBoolOption(list, "AlwaysCore", 1) ||
        !xf86SetBoolOption(list, "SendCoreEvents", 1) ||
        !xf86SetBoolOption(list, "CorePointer", 1) ||
        !xf86SetBoolOption(list, "CoreKeyboard", 1)) {
        xf86Msg(X_CONFIG, "%s: doesn't report core events\n", pInfo->name);
    } else {
        pInfo->flags |= XI86_ALWAYS_CORE;
        xf86Msg(X_CONFIG, "%s: always reports core events\n", pInfo->name);
    }
}

/***********************************************************************
 *
 * xf86ActivateDevice --
 *
 *	Initialize an input device.
 *
 * Returns TRUE on success, or FALSE otherwise.
 ***********************************************************************
 */
static DeviceIntPtr
xf86ActivateDevice(InputInfoPtr pInfo)
{
    DeviceIntPtr	dev;
    Atom		atom;

    dev = AddInputDevice(serverClient, pInfo->device_control, TRUE);

    if (dev == NULL)
    {
        xf86Msg(X_ERROR, "Too many input devices. Ignoring %s\n",
                pInfo->name);
        pInfo->dev = NULL;
        return NULL;
    }

    atom = MakeAtom(pInfo->type_name, strlen(pInfo->type_name), TRUE);
    AssignTypeAndName(dev, atom, pInfo->name);
    dev->public.devicePrivate = pInfo;
    pInfo->dev = dev;

    dev->coreEvents = pInfo->flags & XI86_ALWAYS_CORE;
    dev->type = SLAVE;
    dev->spriteInfo->spriteOwner = FALSE;

    dev->config_info = xf86SetStrOption(pInfo->options, "config_info", NULL);

    if (serverGeneration == 1)
        xf86Msg(X_INFO, "XINPUT: Adding extended input device \"%s\" (type: %s)\n",
                pInfo->name, pInfo->type_name);

    return dev;
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
  InputInfoPtr        pInfo = (InputInfoPtr)dev->public.devicePrivate;

  if (pInfo->switch_mode) {
    return (*pInfo->switch_mode)(client, dev, mode);
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
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;

    if (pInfo->set_device_valuators)
	return (*pInfo->set_device_valuators)(pInfo, valuators, first_valuator,
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
  InputInfoPtr        pInfo = (InputInfoPtr)dev->public.devicePrivate;

  if (!pInfo->control_proc) {
      switch (control->control) {
      case DEVICE_CORE:
          return BadMatch;
      case DEVICE_RESOLUTION:
      case DEVICE_ABS_CALIB:
      case DEVICE_ABS_AREA:
      case DEVICE_ENABLE:
        return Success;
      default:
        return BadMatch;
      }
  }
  else {
      return (*pInfo->control_proc)(pInfo, control);
  }
}

/*
 * Get the operating system name from uname and store it statically to avoid
 * repeating the system call each time MatchOS is checked.
 */
static const char *
HostOS(void)
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname name;
    static char host_os[sizeof(name.sysname)] = "";

    if (*host_os == '\0') {
        if (uname(&name) >= 0)
            strcpy(host_os, name.sysname);
        else {
            strncpy(host_os, "unknown", sizeof(host_os));
            host_os[sizeof(host_os)-1] = '\0';
        }
    }
    return host_os;
#else
    return "";
#endif
}

static int
match_substring(const char *attr, const char *pattern)
{
    return (strstr(attr, pattern)) ? 0 : -1;
}

#ifdef HAVE_FNMATCH_H
static int
match_pattern(const char *attr, const char *pattern)
{
    return fnmatch(pattern, attr, 0);
}
#else
#define match_pattern match_substring
#endif

#ifdef HAVE_FNMATCH_H
static int
match_path_pattern(const char *attr, const char *pattern)
{
    return fnmatch(pattern, attr, FNM_PATHNAME);
}
#else
#define match_path_pattern match_substring
#endif

/*
 * Match an attribute against a list of NULL terminated arrays of patterns.
 * If a pattern in each list entry is matched, return TRUE.
 */
static Bool
MatchAttrToken(const char *attr, struct list *patterns,
               int (*compare)(const char *attr, const char *pattern))
{
    const xf86MatchGroup *group;

    /* If there are no patterns, accept the match */
    if (list_is_empty(patterns))
        return TRUE;

    /* If there are patterns but no attribute, reject the match */
    if (!attr)
        return FALSE;

    /*
     * Otherwise, iterate the list of patterns ensuring each entry has a
     * match. Each list entry is a separate Match line of the same type.
     */
    list_for_each_entry(group, patterns, entry) {
        char * const *cur;
        Bool match = FALSE;

        for (cur = group->values; *cur; cur++)
            if ((*compare)(attr, *cur) == 0) {
                match = TRUE;
                break;
            }
        if (!match)
            return FALSE;
    }

    /* All the entries in the list matched the attribute */
    return TRUE;
}

/*
 * Classes without any Match statements match all devices. Otherwise, all
 * statements must match.
 */
static Bool
InputClassMatches(const XF86ConfInputClassPtr iclass, const IDevPtr idev,
                  const InputAttributes *attrs)
{
    /* MatchProduct substring */
    if (!MatchAttrToken(attrs->product, &iclass->match_product, match_substring))
        return FALSE;

    /* MatchVendor substring */
    if (!MatchAttrToken(attrs->vendor, &iclass->match_vendor, match_substring))
        return FALSE;

    /* MatchDevicePath pattern */
    if (!MatchAttrToken(attrs->device, &iclass->match_device, match_path_pattern))
        return FALSE;

    /* MatchOS case-insensitive string */
    if (!MatchAttrToken(HostOS(), &iclass->match_os, strcasecmp))
        return FALSE;

    /* MatchPnPID pattern */
    if (!MatchAttrToken(attrs->pnp_id, &iclass->match_pnpid, match_pattern))
        return FALSE;

    /* MatchUSBID pattern */
    if (!MatchAttrToken(attrs->usb_id, &iclass->match_usbid, match_pattern))
        return FALSE;

    /* MatchDriver string */
    if (!MatchAttrToken(idev->driver, &iclass->match_driver, strcmp))
        return FALSE;

    /*
     * MatchTag string
     * See if any of the device's tags match any of the MatchTag tokens.
     */
    if (!list_is_empty(&iclass->match_tag)) {
        char * const *tag;
        Bool match;

        if (!attrs->tags)
            return FALSE;
        for (tag = attrs->tags, match = FALSE; *tag; tag++) {
            if (MatchAttrToken(*tag, &iclass->match_tag, strcmp)) {
                match = TRUE;
                break;
            }
        }
        if (!match)
            return FALSE;
    }

    /* MatchIs* booleans */
    if (iclass->is_keyboard.set &&
        iclass->is_keyboard.val != !!(attrs->flags & ATTR_KEYBOARD))
        return FALSE;
    if (iclass->is_pointer.set &&
        iclass->is_pointer.val != !!(attrs->flags & ATTR_POINTER))
        return FALSE;
    if (iclass->is_joystick.set &&
        iclass->is_joystick.val != !!(attrs->flags & ATTR_JOYSTICK))
        return FALSE;
    if (iclass->is_tablet.set &&
        iclass->is_tablet.val != !!(attrs->flags & ATTR_TABLET))
        return FALSE;
    if (iclass->is_touchpad.set &&
        iclass->is_touchpad.val != !!(attrs->flags & ATTR_TOUCHPAD))
        return FALSE;
    if (iclass->is_touchscreen.set &&
        iclass->is_touchscreen.val != !!(attrs->flags & ATTR_TOUCHSCREEN))
        return FALSE;

    return TRUE;
}

/*
 * Merge in any InputClass configurations. Options in each InputClass
 * section have more priority than the original device configuration as
 * well as any previous InputClass sections.
 */
static int
MergeInputClasses(const IDevPtr idev, const InputAttributes *attrs)
{
    XF86ConfInputClassPtr cl;
    XF86OptionPtr classopts;

    for (cl = xf86configptr->conf_inputclass_lst; cl; cl = cl->list.next) {
        if (!InputClassMatches(cl, idev, attrs))
            continue;

        /* Collect class options and driver settings */
        classopts = xf86optionListDup(cl->option_lst);
        if (cl->driver) {
            free(idev->driver);
            idev->driver = xstrdup(cl->driver);
            if (!idev->driver) {
                xf86Msg(X_ERROR, "Failed to allocate memory while merging "
                        "InputClass configuration");
                return BadAlloc;
            }
            classopts = xf86ReplaceStrOption(classopts, "driver",
                                             idev->driver);
        }

        /* Apply options to device with InputClass settings preferred. */
        xf86Msg(X_CONFIG, "%s: Applying InputClass \"%s\"\n",
                idev->identifier, cl->identifier);
        idev->commonOptions = xf86optionListMerge(idev->commonOptions,
                                                  classopts);
    }

    return Success;
}

/*
 * Iterate the list of classes and look for Option "Ignore". Return the
 * value of the last matching class and holler when returning TRUE.
 */
static Bool
IgnoreInputClass(const IDevPtr idev, const InputAttributes *attrs)
{
    XF86ConfInputClassPtr cl;
    Bool ignore = FALSE;
    const char *ignore_class;

    for (cl = xf86configptr->conf_inputclass_lst; cl; cl = cl->list.next) {
        if (!InputClassMatches(cl, idev, attrs))
            continue;
        if (xf86findOption(cl->option_lst, "Ignore")) {
            ignore = xf86CheckBoolOption(cl->option_lst, "Ignore", FALSE);
            ignore_class = cl->identifier;
        }
    }

    if (ignore)
        xf86Msg(X_CONFIG, "%s: Ignoring device from InputClass \"%s\"\n",
                idev->identifier, ignore_class);
    return ignore;
}

/* Allocate a new InputInfoRec and append it to the tail of xf86InputDevs. */
static InputInfoPtr
xf86AllocateInput(InputDriverPtr drv, IDevPtr idev)
{
    InputInfoPtr new, *prev = NULL;

    if (!(new = calloc(sizeof(InputInfoRec), 1)))
	return NULL;

    new->drv = drv;
    drv->refCount++;
    new->module = DuplicateModule(drv->module, NULL);

    for (prev = &xf86InputDevs; *prev; prev = &(*prev)->next)
        ;

    *prev = new;
    new->next = NULL;

    new->fd = -1;
    new->name = idev->identifier;
    new->type_name = "UNKNOWN";
    new->device_control = NULL;
    new->read_input = NULL;
    new->control_proc = NULL;
    new->switch_mode = NULL;
    new->dev = NULL;
    new->private = NULL;
    new->conf_idev = idev;

    xf86CollectInputOptions(new, (const char**)drv->default_options, NULL);
    xf86ProcessCommonOptions(new, new->options);

    return new;
}

/*
 * Remove an entry from xf86InputDevs.  Ideally it should free all allocated
 * data.  To do this properly may require a driver hook.
 */

void
xf86DeleteInput(InputInfoPtr pInp, int flags)
{
    InputInfoPtr p;

    /* First check if the inputdev is valid. */
    if (pInp == NULL)
	return;

#if 0
    /* If a free function is defined, call it here. */
    if (pInp->free)
	pInp->free(pInp, 0);
#endif

    if (pInp->module)
	UnloadModule(pInp->module);

    if (pInp->drv)
	pInp->drv->refCount--;

    /* This should *really* be handled in drv->UnInit(dev) call instead, but
     * if the driver forgets about it make sure we free it or at least crash
     * with flying colors */
    free(pInp->private);

    FreeInputAttributes(pInp->attrs);

    /* Remove the entry from the list. */
    if (pInp == xf86InputDevs)
	xf86InputDevs = pInp->next;
    else {
	p = xf86InputDevs;
	while (p && p->next != pInp)
	    p = p->next;
	if (p)
	    p->next = pInp->next;
	/* Else the entry wasn't in the xf86InputDevs list (ignore this). */
    }
    free(pInp);
}

/*
 * Apply backend-specific initialization. Invoked after ActiveteDevice(),
 * i.e. after the driver successfully completed DEVICE_INIT and the device
 * is advertised.
 * @param dev the device
 * @return Success or an error code
 */
static int
xf86InputDevicePostInit(DeviceIntPtr dev) {
    ApplyAccelerationSettings(dev);
    return Success;
}

/**
 * Create a new input device, activate and enable it.
 *
 * Possible return codes:
 *    BadName .. a bad driver name was supplied.
 *    BadImplementation ... The driver does not have a PreInit function. This
 *                          is a driver bug.
 *    BadMatch .. device initialization failed.
 *    BadAlloc .. too many input devices
 *
 * @param idev The device, already set up with identifier, driver, and the
 * options.
 * @param pdev Pointer to the new device, if Success was reported.
 * @param enable Enable the device after activating it.
 *
 * @return Success or an error code
 */
_X_INTERNAL int
xf86NewInputDevice(IDevPtr idev, DeviceIntPtr *pdev, BOOL enable)
{
    InputDriverPtr drv = NULL;
    InputInfoPtr pInfo = NULL;
    DeviceIntPtr dev = NULL;
    int rval;

    /* Memory leak for every attached device if we don't
     * test if the module is already loaded first */
    drv = xf86LookupInputDriver(idev->driver);
    if (!drv)
        if (xf86LoadOneModule(idev->driver, NULL))
            drv = xf86LookupInputDriver(idev->driver);
    if (!drv) {
        xf86Msg(X_ERROR, "No input driver matching `%s'\n", idev->driver);
        rval = BadName;
        goto unwind;
    }

    if (!drv->PreInit) {
        xf86Msg(X_ERROR,
                "Input driver `%s' has no PreInit function (ignoring)\n",
                drv->driverName);
        rval = BadImplementation;
        goto unwind;
    }

    if (!(pInfo = xf86AllocateInput(drv, idev)))
	goto unwind;

    rval = drv->PreInit(drv, pInfo, 0);

    if (rval != Success) {
        xf86Msg(X_ERROR, "PreInit returned %d for \"%s\"\n", rval, idev->identifier);
        goto unwind;
    }

    if (!(dev = xf86ActivateDevice(pInfo)))
    {
        rval = BadAlloc;
        goto unwind;
    }

    rval = ActivateDevice(dev, TRUE);
    if (rval != Success)
    {
        xf86Msg(X_ERROR, "Couldn't init device \"%s\"\n", idev->identifier);
        RemoveDevice(dev, TRUE);
        goto unwind;
    }

    rval = xf86InputDevicePostInit(dev);
    if (rval != Success)
    {
	xf86Msg(X_ERROR, "Couldn't post-init device \"%s\"\n", idev->identifier);
	RemoveDevice(dev, TRUE);
	goto unwind;
    }

    /* Enable it if it's properly initialised and we're currently in the VT */
    if (enable && dev->inited && dev->startup && xf86Screens[0]->vtSema)
    {
        EnableDevice(dev, TRUE);
        if (!dev->enabled)
        {
            xf86Msg(X_ERROR, "Couldn't init device \"%s\"\n", idev->identifier);
            rval = BadMatch;
            goto unwind;
        }
        /* send enter/leave event, update sprite window */
        CheckMotion(NULL, dev);
    }

    *pdev = dev;
    return Success;

unwind:
    if(pInfo) {
        if(drv->UnInit)
            drv->UnInit(drv, pInfo, 0);
        else
            xf86DeleteInput(pInfo, 0);
    }
    return rval;
}

int
NewInputDeviceRequest (InputOption *options, InputAttributes *attrs,
                       DeviceIntPtr *pdev)
{
    IDevRec *idev = NULL;
    InputOption *option = NULL;
    int rval = Success;
    int is_auto = 0;

    idev = calloc(sizeof(*idev), 1);
    if (!idev)
        return BadAlloc;

    for (option = options; option; option = option->next) {
        if (strcasecmp(option->key, "driver") == 0) {
            if (idev->driver) {
                rval = BadRequest;
                goto unwind;
            }
            idev->driver = xstrdup(option->value);
            if (!idev->driver) {
                rval = BadAlloc;
                goto unwind;
            }
        }

        if (strcasecmp(option->key, "name") == 0 ||
            strcasecmp(option->key, "identifier") == 0) {
            if (idev->identifier) {
                rval = BadRequest;
                goto unwind;
            }
            idev->identifier = xstrdup(option->value);
            if (!idev->identifier) {
                rval = BadAlloc;
                goto unwind;
            }
        }

        if (strcmp(option->key, "_source") == 0 &&
            (strcmp(option->value, "server/hal") == 0 ||
             strcmp(option->value, "server/udev") == 0)) {
            is_auto = 1;
            if (!xf86Info.autoAddDevices) {
                rval = BadMatch;
                goto unwind;
            }
        }
    }

    for (option = options; option; option = option->next) {
        /* Steal option key/value strings from the provided list.
         * We need those strings, the InputOption list doesn't. */
        idev->commonOptions = xf86addNewOption(idev->commonOptions,
                                               option->key, option->value);
        option->key = NULL;
        option->value = NULL;
    }

    /* Apply InputClass settings */
    if (attrs) {
        if (IgnoreInputClass(idev, attrs)) {
            rval = BadIDChoice;
            goto unwind;
        }

        rval = MergeInputClasses(idev, attrs);
        if (rval != Success)
            goto unwind;

        idev->attrs = DuplicateInputAttributes(attrs);
    }

    if (!idev->driver || !idev->identifier) {
        xf86Msg(X_INFO, "No input driver/identifier specified (ignoring)\n");
        rval = BadRequest;
        goto unwind;
    }

    if (!idev->identifier) {
        xf86Msg(X_ERROR, "No device identifier specified (ignoring)\n");
        return BadMatch;
    }

    rval = xf86NewInputDevice(idev, pdev,
                (!is_auto || (is_auto && xf86Info.autoEnableDevices)));
    if (rval == Success)
        return Success;

unwind:
    if (is_auto && !xf86Info.autoAddDevices)
        xf86Msg(X_INFO, "AutoAddDevices is off - not adding device.\n");
    free(idev->driver);
    free(idev->identifier);
    xf86optionListFree(idev->commonOptions);
    free(idev);
    return rval;
}

void
DeleteInputDeviceRequest(DeviceIntPtr pDev)
{
    InputInfoPtr pInfo = (InputInfoPtr) pDev->public.devicePrivate;
    InputDriverPtr drv = NULL;
    IDevRec *idev = NULL;
    IDevPtr *it;
    Bool isMaster = IsMaster(pDev);

    if (pInfo) /* need to get these before RemoveDevice */
    {
        drv = pInfo->drv;
        idev = pInfo->conf_idev;
    }

    OsBlockSignals();
    RemoveDevice(pDev, TRUE);

    if (!isMaster && pInfo != NULL)
    {
        if(drv->UnInit)
            drv->UnInit(drv, pInfo, 0);
        else
            xf86DeleteInput(pInfo, 0);

        /* devices added through HAL aren't in the config layout */
        it = xf86ConfigLayout.inputs;
        while(*it && *it != idev)
            it++;

        if (!(*it)) /* end of list, not in the layout */
        {
            free(idev->driver);
            free(idev->identifier);
            xf86optionListFree(idev->commonOptions);
            free(idev);
        }
    }
    OsReleaseSignals();
}

/* 
 * convenient functions to post events
 */

void
xf86PostMotionEvent(DeviceIntPtr	device,
                    int			is_absolute,
                    int			first_valuator,
                    int			num_valuators,
                    ...)
{
    va_list var;
    int i = 0;
    static int valuators[MAX_VALUATORS];

    XI_VERIFY_VALUATORS(num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuators[i] = va_arg(var, int);
    va_end(var);

    xf86PostMotionEventP(device, is_absolute, first_valuator, num_valuators, valuators);
}

void
xf86PostMotionEventP(DeviceIntPtr	device,
                    int			is_absolute,
                    int			first_valuator,
                    int			num_valuators,
                    const int		*valuators)
{
    int i = 0, nevents = 0;
    DeviceEvent *event;
    int flags = 0;

#if XFreeXDGA
    int index;
    int dx = 0, dy = 0;
#endif

    XI_VERIFY_VALUATORS(num_valuators);

    if (is_absolute)
        flags = POINTER_ABSOLUTE;
    else
        flags = POINTER_RELATIVE | POINTER_ACCELERATE;

#if XFreeXDGA
    /* The evdev driver may not always send all axes across. */
    if (num_valuators >= 1 && first_valuator <= 1) {
        if (miPointerGetScreen(device)) {
            index = miPointerGetScreen(device)->myNum;
            if (first_valuator == 0)
            {
                dx = valuators[0];
                if (is_absolute)
                    dx -= device->last.valuators[0];
            }

            if (first_valuator == 1 || num_valuators >= 2)
            {
                dy = valuators[1 - first_valuator];
                if (is_absolute)
                    dy -= device->last.valuators[1];
            }

            if (DGAStealMotionEvent(device, index, dx, dy))
                return;
        }
    }
#endif

    nevents = GetPointerEvents(xf86Events, device, MotionNotify, 0,
                               flags, first_valuator, num_valuators,
                               valuators);

    for (i = 0; i < nevents; i++) {
        event = (DeviceEvent*)((xf86Events + i)->event);
        mieqEnqueue(device, (InternalEvent*)((xf86Events + i)->event));
    }
}

void
xf86PostProximityEvent(DeviceIntPtr	device,
                       int		is_in,
                       int		first_valuator,
                       int		num_valuators,
                       ...)
{
    va_list var;
    int i;
    int valuators[MAX_VALUATORS];

    XI_VERIFY_VALUATORS(num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuators[i] = va_arg(var, int);
    va_end(var);

    xf86PostProximityEventP(device, is_in, first_valuator, num_valuators,
			    valuators);

}

void
xf86PostProximityEventP(DeviceIntPtr	device,
                        int		is_in,
                        int		first_valuator,
                        int		num_valuators,
                        const int	*valuators)
{
    int i, nevents;

    XI_VERIFY_VALUATORS(num_valuators);

    nevents = GetProximityEvents(xf86Events, device,
                                 is_in ? ProximityIn : ProximityOut, 
                                 first_valuator, num_valuators, valuators);
    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, (InternalEvent*)((xf86Events + i)->event));

}

void
xf86PostButtonEvent(DeviceIntPtr	device,
                    int			is_absolute,
                    int			button,
                    int			is_down,
                    int			first_valuator,
                    int			num_valuators,
                    ...)
{
    va_list var;
    int valuators[MAX_VALUATORS];
    int i = 0;

    XI_VERIFY_VALUATORS(num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuators[i] = va_arg(var, int);
    va_end(var);

    xf86PostButtonEventP(device, is_absolute, button, is_down, first_valuator,
			 num_valuators, valuators);

}

void
xf86PostButtonEventP(DeviceIntPtr	device,
                     int		is_absolute,
                     int		button,
                     int		is_down,
                     int		first_valuator,
                     int		num_valuators,
                     const int		*valuators)
{
    int i = 0, nevents = 0;
    int flags = 0;

#if XFreeXDGA
    int index;
#endif

    XI_VERIFY_VALUATORS(num_valuators);

    if (is_absolute)
        flags = POINTER_ABSOLUTE;
    else
        flags = POINTER_RELATIVE | POINTER_ACCELERATE;

#if XFreeXDGA
    if (miPointerGetScreen(device)) {
        index = miPointerGetScreen(device)->myNum;
        if (DGAStealButtonEvent(device, index, button, is_down))
            return;
    }
#endif

    nevents = GetPointerEvents(xf86Events, device,
                               is_down ? ButtonPress : ButtonRelease, button,
                               flags, first_valuator, num_valuators, valuators);

    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, (InternalEvent*)((xf86Events + i)->event));

}

void
xf86PostKeyEvent(DeviceIntPtr	device,
                 unsigned int	key_code,
                 int		is_down,
                 int		is_absolute,
                 int		first_valuator,
                 int		num_valuators,
                 ...)
{
    va_list var;
    int i = 0;
    static int valuators[MAX_VALUATORS];

    XI_VERIFY_VALUATORS(num_valuators);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
      valuators[i] = va_arg(var, int);
    va_end(var);

    xf86PostKeyEventP(device, key_code, is_down, is_absolute, first_valuator,
		      num_valuators, valuators);

}

void
xf86PostKeyEventP(DeviceIntPtr	device,
                  unsigned int	key_code,
                  int		is_down,
                  int		is_absolute,
                  int		first_valuator,
                  int		num_valuators,
                  const int	*valuators)
{
    int i = 0, nevents = 0;

    XI_VERIFY_VALUATORS(num_valuators);

    if (is_absolute) {
        nevents = GetKeyboardValuatorEvents(xf86Events, device,
                                            is_down ? KeyPress : KeyRelease,
                                            key_code, first_valuator,
                                            num_valuators, valuators);
    }
    else {
        nevents = GetKeyboardEvents(xf86Events, device,
                                    is_down ? KeyPress : KeyRelease,
                                    key_code);
    }

    for (i = 0; i < nevents; i++)
        mieqEnqueue(device, (InternalEvent*)((xf86Events + i)->event));
}

void
xf86PostKeyboardEvent(DeviceIntPtr      device,
                      unsigned int      key_code,
                      int               is_down)
{
    xf86PostKeyEventP(device, key_code, is_down, 0, 0, 0, NULL);
}

InputInfoPtr
xf86FirstLocalDevice(void)
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

int
xf86ScaleAxis(int	Cx,
              int	Sxhigh,
              int	Sxlow,
              int	Rxhigh,
              int	Rxlow )
{
    int X;
    int64_t dSx = Sxhigh - Sxlow;
    int64_t dRx = Rxhigh - Rxlow;

    if (dRx) {
	X = (int)(((dSx * (Cx - Rxlow)) / dRx) + Sxlow);
    }
    else {
	X = 0;
	ErrorF ("Divide by Zero in xf86ScaleAxis");
    }
    
    if (X > Sxhigh)
	X = Sxhigh;
    if (X < Sxlow)
	X = Sxlow;
    
    return X;
}

/*
 * This function checks the given screen against the current screen and
 * makes changes if appropriate. It should be called from an XInput driver's
 * ReadInput function before any events are posted, if the device is screen
 * specific like a touch screen.
 */
void
xf86XInputSetScreen(InputInfoPtr	pInfo,
		    int			screen_number,
		    int			x,
		    int			y)
{
    if (miPointerGetScreen(pInfo->dev) !=
          screenInfo.screens[screen_number]) {
	miPointerSetScreen(pInfo->dev, screen_number, x, y);
    }
}


void
xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, Atom label, int minval, int maxval,
			   int resolution, int min_res, int max_res)
{
    if (!dev || !dev->valuator)
        return;

    InitValuatorAxisStruct(dev, axnum, label, minval, maxval, resolution, min_res,
			   max_res);
}

/*
 * Set the valuator values to be in synch with dix/event.c
 * DefineInitialRootWindow().
 */
void
xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum)
{
    if (axnum == 0) {
	dev->valuator->axisVal[0] = screenInfo.screens[0]->width / 2;
        dev->last.valuators[0] = dev->valuator->axisVal[0];
    }
    else if (axnum == 1) {
	dev->valuator->axisVal[1] = screenInfo.screens[0]->height / 2;
        dev->last.valuators[1] = dev->valuator->axisVal[1];
    }
}


/**
 * Deactivate a device. Call this function from the driver if you receive a
 * read error or something else that spoils your day.
 * Device will be moved to the off_devices list, but it will still be there
 * until you really clean up after it.
 * Notifies the client about an inactive device.
 * 
 * @param panic True if device is unrecoverable and needs to be removed.
 */
void
xf86DisableDevice(DeviceIntPtr dev, Bool panic)
{
    if(!panic)
    {
        DisableDevice(dev, TRUE);
    } else
    {
        SendDevicePresenceEvent(dev->id, DeviceUnrecoverable);
        DeleteInputDeviceRequest(dev);
    }
}

/**
 * Reactivate a device. Call this function from the driver if you just found
 * out that the read error wasn't quite that bad after all.
 * Device will be re-activated, and an event sent to the client. 
 */
void
xf86EnableDevice(DeviceIntPtr dev)
{
    EnableDevice(dev, TRUE);
}

/* end of xf86Xinput.c */
