/************************************************************

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/********************************************************************
 *
 *  Routines to register and initialize extension input devices.
 *  This also contains ProcessOtherEvent, the routine called from DDX
 *  to route extension events.
 *
 */

#define	 NEED_EVENTS
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/geproto.h>
#include "inputstr.h"
#include "windowstr.h"
#include "miscstruct.h"
#include "region.h"
#include "exevents.h"
#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exglobals.h"
#include "dixevents.h"	/* DeliverFocusedEvent */
#include "dixgrabs.h"	/* CreateGrab() */
#include "scrnintstr.h"
#include "listdev.h" /* for CopySwapXXXClass */

#ifdef XKB
#include <X11/extensions/XKBproto.h>
#include "xkbsrv.h"
extern Bool XkbCopyKeymap(XkbDescPtr src, XkbDescPtr dst, Bool sendNotifies);
#endif

#define WID(w) ((w) ? ((w)->drawable.id) : 0)
#define AllModifiersMask ( \
	ShiftMask | LockMask | ControlMask | Mod1Mask | Mod2Mask | \
	Mod3Mask | Mod4Mask | Mod5Mask )
#define AllButtonsMask ( \
	Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask )
#define Motion_Filter(class) (DevicePointerMotionMask | \
			      (class)->state | (class)->motionMask)

Bool ShouldFreeInputMasks(WindowPtr /* pWin */ ,
				 Bool	/* ignoreSelectedEvents */
    );
static Bool MakeInputMasks(WindowPtr	/* pWin */
    );


void
RegisterOtherDevice(DeviceIntPtr device)
{
    device->public.processInputProc = ProcessOtherEvent;
    device->public.realInputProc = ProcessOtherEvent;
}

/**
 * Copy the device->key into master->key and send a mapping notify to the
 * clients if appropriate.
 * master->key needs to be allocated by the caller.
 *
 * Device is the slave device. If it is attached to a master device, we may
 * need to send a mapping notify to the client because it causes the MD
 * to change state.
 *
 * Mapping notify needs to be sent in the following cases:
 *      - different slave device on same master
 *      - different master
 *
 * XXX: They way how the code is we also send a map notify if the slave device
 * stays the same, but the master changes. This isn't really necessary though.
 *
 * XXX: this gives you funny behaviour with the ClientPointer. When a
 * MappingNotify is sent to the client, the client usually responds with a
 * GetKeyboardMapping. This will retrieve the ClientPointer's keyboard
 * mapping, regardless of which keyboard sent the last mapping notify request.
 * So depending on the CP setting, your keyboard may change layout in each
 * app...
 *
 * This code is basically the old SwitchCoreKeyboard.
 */

static void
CopyKeyClass(DeviceIntPtr device, DeviceIntPtr master)
{
    static DeviceIntPtr lastMapNotifyDevice = NULL;
    KeyClassPtr mk, dk; /* master, device */
    BOOL sendNotify = FALSE;
    int i;

    if (device == master)
        return;

    dk = device->key;
    mk = master->key;

    if (master->devPrivates[CoreDevicePrivatesIndex].ptr != device) {
        memcpy(mk->modifierMap, dk->modifierMap, MAP_LENGTH);

        mk->modifierKeyMap = xcalloc(8, dk->maxKeysPerModifier);
        if (!mk->modifierKeyMap)
            FatalError("[Xi] no memory for class shift.\n");
        memcpy(mk->modifierKeyMap, dk->modifierKeyMap,
                (8 * dk->maxKeysPerModifier));

        mk->maxKeysPerModifier = dk->maxKeysPerModifier;
        mk->curKeySyms.minKeyCode = dk->curKeySyms.minKeyCode;
        mk->curKeySyms.maxKeyCode = dk->curKeySyms.maxKeyCode;
        SetKeySymsMap(&mk->curKeySyms, &dk->curKeySyms);

        /*
         * Copy state from the extended keyboard to core.  If you omit this,
         * holding Ctrl on keyboard one, and pressing Q on keyboard two, will
         * cause your app to quit.  This feels wrong to me, hence the below
         * code.
         *
         * XXX: If you synthesise core modifier events, the state will get
         *      clobbered here.  You'll have to work out something sensible
         *      to fix that.  Good luck.
         */

#define KEYBOARD_MASK (ShiftMask | LockMask | ControlMask | Mod1Mask | \
        Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask)
        mk->state &= ~(KEYBOARD_MASK);
        mk->state |= (dk->state & KEYBOARD_MASK);
#undef KEYBOARD_MASK
        for (i = 0; i < 8; i++)
            mk->modifierKeyCount[i] = dk->modifierKeyCount[i];

#ifdef XKB
        if (!noXkbExtension && dk->xkbInfo && dk->xkbInfo->desc) {
            if (!mk->xkbInfo || !mk->xkbInfo->desc)
                XkbInitDevice(master);
            if (!XkbCopyKeymap(dk->xkbInfo->desc, mk->xkbInfo->desc, True))
                FatalError("Couldn't pivot keymap from device to core!\n");
        }
#endif

        master->devPrivates[CoreDevicePrivatesIndex].ptr = device;
        sendNotify = TRUE;
    } else if (lastMapNotifyDevice != master)
        sendNotify = TRUE;

    if (sendNotify)
    {
        SendMappingNotify(master, MappingKeyboard,
                           mk->curKeySyms.minKeyCode,
                          (mk->curKeySyms.maxKeyCode -
                           mk->curKeySyms.minKeyCode),
                          serverClient);
        lastMapNotifyDevice = master;
    }
}

_X_EXPORT void
DeepCopyDeviceClasses(DeviceIntPtr from, DeviceIntPtr to)
{
#define ALLOC_COPY_CLASS_IF(field, type) \
    if (from->field)\
    { \
        to->field = xcalloc(1, sizeof(type)); \
        if (!to->field) \
            FatalError("[Xi] no memory for class shift.\n"); \
        memcpy(to->field, from->field, sizeof(type)); \
    }

    ALLOC_COPY_CLASS_IF(key, KeyClassRec);
    if (to->key)
    {
#ifdef XKB
        to->key->xkbInfo = NULL;
#endif
        CopyKeyClass(from, to);
    }

    if (from->valuator)
    {
        ValuatorClassPtr v;
        to->valuator = xalloc(sizeof(ValuatorClassRec) +
                from->valuator->numAxes * sizeof(AxisInfo) +
                from->valuator->numAxes * sizeof(unsigned int));
        v = to->valuator;
        if (!v)
            FatalError("[Xi] no memory for class shift.\n");
        memcpy(v, from->valuator, sizeof(ValuatorClassRec));
        v->axes = (AxisInfoPtr)&v[1];
        memcpy(v->axes, from->valuator->axes, v->numAxes * sizeof(AxisInfo));
    }

    ALLOC_COPY_CLASS_IF(button, ButtonClassRec);
#ifdef XKB
    if (to->button)
    {
        to->button->xkb_acts = NULL;
        /* XXX: XkbAction needs to be copied */
    }
#endif
    ALLOC_COPY_CLASS_IF(focus, FocusClassRec);
    ALLOC_COPY_CLASS_IF(proximity, ProximityClassRec);
    ALLOC_COPY_CLASS_IF(absolute, AbsoluteClassRec);
    ALLOC_COPY_CLASS_IF(kbdfeed, KbdFeedbackClassRec);
#ifdef XKB
    if (to->kbdfeed)
    {
        to->kbdfeed->xkb_sli = NULL;
        /* XXX: XkbSrvLedInfo needs to be copied*/
    }
#endif
    ALLOC_COPY_CLASS_IF(ptrfeed, PtrFeedbackClassRec);
    ALLOC_COPY_CLASS_IF(intfeed, IntegerFeedbackClassRec);
    ALLOC_COPY_CLASS_IF(stringfeed, StringFeedbackClassRec);
    ALLOC_COPY_CLASS_IF(bell, BellFeedbackClassRec);
    ALLOC_COPY_CLASS_IF(leds, LedFeedbackClassRec);
#ifdef XKB
    if (to->leds)
    {
        to->leds->xkb_sli = NULL;
        /* XXX: XkbSrvLedInfo needs to be copied*/
    }
#endif
}

static void
ChangeMasterDeviceClasses(DeviceIntPtr device,
                          deviceClassesChangedEvent *dcce)
{
    DeviceIntPtr master = device->u.master;
    char* classbuff;

    if (device->isMaster)
        return;

    if (!master) /* if device was set floating between SIGIO and now */
        return;

    dcce->deviceid     = master->id;
    dcce->num_classes  = 0;

    master->public.devicePrivate = device->public.devicePrivate;

    FreeAllDeviceClasses(&master->key);
    DeepCopyDeviceClasses(device, master);

    /* event is already correct size, see comment in GetPointerEvents */
    classbuff = (char*)&dcce[1];

    /* we don't actually swap if there's a NullClient, swapping is done
     * later when event is delivered. */
    CopySwapClasses(NullClient, master, &dcce->num_classes, &classbuff);
    SendEventToAllWindows(master, XI_DeviceClassesChangedMask,
                          (xEvent*)dcce, 1);
}

/**
 * Update the device state according to the data in the event.
 *
 * return values are
 *   DEFAULT ... process as normal
 *   DONT_PROCESS ... return immediately from caller
 *   IS_REPEAT .. event is a repeat event.
 */
#define DEFAULT 0
#define DONT_PROCESS 1
#define IS_REPEAT 2
static int
UpdateDeviceState(DeviceIntPtr device, xEvent* xE, int count)
{
    int i;
    int key = 0,
        bit = 0;

    KeyClassPtr k       = NULL;
    ButtonClassPtr b    = NULL;
    ValuatorClassPtr v  = NULL;
    deviceValuator *xV  = (deviceValuator *) xE;
    BYTE *kptr          = NULL;
    CARD16 modifiers    = 0,
           mask         = 0;

    /* This event is always the first we get, before the actual events with
     * the data. However, the way how the DDX is set up, "device" will
     * actually be the slave device that caused the event.
     */
    if (GEIsType(xE, IReqCode, XI_DeviceClassesChangedNotify))
    {
        ChangeMasterDeviceClasses(device, (deviceClassesChangedEvent*)xE);
        return DONT_PROCESS; /* event has been sent already */
    }

    /* currently no other generic event modifies the device */
    if (xE->u.u.type == GenericEvent)
        return DEFAULT;

    k = device->key;
    v = device->valuator;
    b = device->button;


    if (xE->u.u.type != DeviceValuator)
    {
        key = xE->u.u.detail;
        bit = 1 << (key & 7);
    }

    /* Update device axis */
    for (i = 1; i < count; i++) {
	if ((++xV)->type == DeviceValuator) {
	    int *axisvals;
            int first = xV->first_valuator;

	    if (xV->num_valuators &&
                (!v || (xV->num_valuators &&
                      (first + xV->num_valuators > v->numAxes))))
		FatalError("Bad valuators reported for device %s\n",
			   device->name);
	    if (v && v->axisVal) {
		axisvals = v->axisVal;
		switch (xV->num_valuators) {
		case 6:
		    *(axisvals + first + 5) = xV->valuator5;
		case 5:
		    *(axisvals + first + 4) = xV->valuator4;
		case 4:
		    *(axisvals + first + 3) = xV->valuator3;
		case 3:
		    *(axisvals + first + 2) = xV->valuator2;
		case 2:
		    *(axisvals + first + 1) = xV->valuator1;
		case 1:
		    *(axisvals + first) = xV->valuator0;
		case 0:
		default:
		    break;
		}
	    }
	}
    }

    if (xE->u.u.type == DeviceKeyPress) {
        if (!k)
            return DONT_PROCESS;

	modifiers = k->modifierMap[key];
	kptr = &k->down[key >> 3];
	if (*kptr & bit) {	/* allow ddx to generate multiple downs */
	    return IS_REPEAT;
	}
	if (device->valuator)
	    device->valuator->motionHintWindow = NullWindow;
	*kptr |= bit;
	k->prev_state = k->state;
	for (i = 0, mask = 1; modifiers; i++, mask <<= 1) {
	    if (mask & modifiers) {
		/* This key affects modifier "i" */
		k->modifierKeyCount[i]++;
		k->state |= mask;
		modifiers &= ~mask;
	    }
	}
    } else if (xE->u.u.type == DeviceKeyRelease) {
        if (!k)
            return DONT_PROCESS;

	kptr = &k->down[key >> 3];
	if (!(*kptr & bit))	/* guard against duplicates */
	    return DONT_PROCESS;
	modifiers = k->modifierMap[key];
	if (device->valuator)
	    device->valuator->motionHintWindow = NullWindow;
	*kptr &= ~bit;
	k->prev_state = k->state;
	for (i = 0, mask = 1; modifiers; i++, mask <<= 1) {
	    if (mask & modifiers) {
		/* This key affects modifier "i" */
		if (--k->modifierKeyCount[i] <= 0) {
		    k->modifierKeyCount[i] = 0;
		    k->state &= ~mask;
		}
		modifiers &= ~mask;
	    }
	}
    } else if (xE->u.u.type == DeviceButtonPress) {
        if (!b)
            return DONT_PROCESS;

	kptr = &b->down[key >> 3];
	*kptr |= bit;
	if (device->valuator)
	    device->valuator->motionHintWindow = NullWindow;
        b->buttonsDown++;
	b->motionMask = DeviceButtonMotionMask;
        if (!b->map[key])
            return DONT_PROCESS;
        if (b->map[key] <= 5)
	    b->state |= (Button1Mask >> 1) << b->map[key];
	SetMaskForEvent(Motion_Filter(b), DeviceMotionNotify);
    } else if (xE->u.u.type == DeviceButtonRelease) {
        if (!b)
            return DONT_PROCESS;

	kptr = &b->down[key >> 3];
        if (!(*kptr & bit))
            return DONT_PROCESS;
	*kptr &= ~bit;
	if (device->valuator)
	    device->valuator->motionHintWindow = NullWindow;
        if (b->buttonsDown >= 1 && !--b->buttonsDown)
	    b->motionMask = 0;
        if (!b->map[key])
            return DONT_PROCESS;
	if (b->map[key] <= 5)
	    b->state &= ~((Button1Mask >> 1) << b->map[key]);
	SetMaskForEvent(Motion_Filter(b), DeviceMotionNotify);
    } else if (xE->u.u.type == ProximityIn)
	device->valuator->mode &= ~OutOfProximity;
    else if (xE->u.u.type == ProximityOut)
	device->valuator->mode |= OutOfProximity;

    return DEFAULT;
}

/**
 * Main device event processing function.
 * Called from when processing the events from the event queue.
 * Generates core events for XI events as needed.
 *
 * Note that these core events are then delivered first. For passive grabs, XI
 * events have preference over core.
 */
void
ProcessOtherEvent(xEventPtr xE, DeviceIntPtr device, int count)
{
    int i;
    CARD16 modifiers;
    GrabPtr grab = device->deviceGrab.grab;
    Bool deactivateDeviceGrab = FALSE;
    int key = 0, rootX, rootY;
    ButtonClassPtr b;
    KeyClassPtr k;
    ValuatorClassPtr v;
    deviceValuator *xV  = (deviceValuator *) xE;
    BOOL sendCore = FALSE;
    xEvent core;
    int coretype = 0;
    int ret = 0;

    ret = UpdateDeviceState(device, xE, count);
    if (ret == DONT_PROCESS)
        return;

    v = device->valuator;
    b = device->button;
    k = device->key;

    coretype = XItoCoreType(xE->u.u.type);
    if (device->isMaster && device->coreEvents && coretype)
        sendCore = TRUE;

    if (device->isMaster)
        CheckMotion(xE, device);

    if (xE->u.u.type != DeviceValuator && xE->u.u.type != GenericEvent) {
        DeviceIntPtr mouse = NULL, kbd = NULL;
	GetSpritePosition(device, &rootX, &rootY);
	xE->u.keyButtonPointer.rootX = rootX;
	xE->u.keyButtonPointer.rootY = rootY;
	NoticeEventTime(xE);

        /* If 'device' is a pointer device, we need to get the paired keyboard
         * for the state. If there is none, the kbd bits of state are 0.
         * If 'device' is a keyboard device, get the paired pointer and use the
         * pointer's state for the button bits.
         */
        if (IsPointerDevice(device))
        {
            kbd = GetPairedDevice(device);
            mouse = device;
        }
        else
        {
            mouse = GetPairedDevice(device);
            kbd = device;
        }
        xE->u.keyButtonPointer.state = (kbd) ? (kbd->key->state) : 0;
        xE->u.keyButtonPointer.state |= (mouse) ? (mouse->button->state) : 0;

        key = xE->u.u.detail;
    }
    if (DeviceEventCallback) {
	DeviceEventInfoRec eventinfo;

	eventinfo.events = (xEventPtr) xE;
	eventinfo.count = count;
	CallCallbacks(&DeviceEventCallback, (pointer) & eventinfo);
    }

    /* Valuator event handling */
    for (i = 1; i < count; i++) {
	if ((++xV)->type == DeviceValuator) {
	    int first = xV->first_valuator;
	    if (xV->num_valuators
		&& (!v
		    || (xV->num_valuators
			&& (first + xV->num_valuators > v->numAxes))))
		FatalError("Bad valuators reported for device %s\n",
			   device->name);
	    xV->device_state = 0;
	    if (k)
		xV->device_state |= k->state;
	    if (b)
		xV->device_state |= b->state;
	}
    }

    if (xE->u.u.type == DeviceKeyPress) {
        if (ret == IS_REPEAT) {	/* allow ddx to generate multiple downs */
            modifiers = k->modifierMap[key];
	    if (!modifiers) {
		xE->u.u.type = DeviceKeyRelease;
		ProcessOtherEvent(xE, device, count);
		xE->u.u.type = DeviceKeyPress;
		/* release can have side effects, don't fall through */
		ProcessOtherEvent(xE, device, count);
	    }
	    return;
	}
        /* XI grabs have priority */
        core = *xE;
        core.u.u.type = coretype;
	if (!grab &&
              (CheckDeviceGrabs(device, xE, 0, count) ||
                 (sendCore && CheckDeviceGrabs(device, &core, 0, 1)))) {
	    device->deviceGrab.activatingKey = key;
	    return;
	}
    } else if (xE->u.u.type == DeviceKeyRelease) {
	if (device->deviceGrab.fromPassiveGrab &&
            (key == device->deviceGrab.activatingKey))
	    deactivateDeviceGrab = TRUE;
    } else if (xE->u.u.type == DeviceButtonPress) {
	xE->u.u.detail = b->map[key];
	if (xE->u.u.detail == 0)
	    return;
        if (!grab)
        {
            core = *xE;
            core.u.u.type = coretype;
            if (CheckDeviceGrabs(device, xE, 0, count) ||
                    (sendCore && CheckDeviceGrabs(device, &core, 0, 1)))
            {
                /* if a passive grab was activated, the event has been sent
                 * already */
                return;
            }
        }

    } else if (xE->u.u.type == DeviceButtonRelease) {
	xE->u.u.detail = b->map[key];
	if (xE->u.u.detail == 0)
	    return;
        if (!b->state && device->deviceGrab.fromPassiveGrab)
            deactivateDeviceGrab = TRUE;
    }

    if (sendCore)
    {
        core = *xE;
        core.u.u.type = coretype;
    }

    if (grab)
    {
        if (sendCore)                      /* never deactivate from core */
            DeliverGrabbedEvent(&core, device, FALSE , 1);
        DeliverGrabbedEvent(xE, device, deactivateDeviceGrab, count);
    }
    else if (device->focus)
    {
        if (sendCore)
            DeliverFocusedEvent(device, &core, GetSpriteWindow(device), 1);
	DeliverFocusedEvent(device, xE, GetSpriteWindow(device), count);
    }
    else
    {
        if (sendCore)
            DeliverDeviceEvents(GetSpriteWindow(device), &core, NullGrab,
                                NullWindow, device, 1);
	DeliverDeviceEvents(GetSpriteWindow(device), xE, NullGrab, NullWindow,
			    device, count);
    }

    if (deactivateDeviceGrab == TRUE)
	(*device->deviceGrab.DeactivateGrab) (device);
}

_X_EXPORT int
InitProximityClassDeviceStruct(DeviceIntPtr dev)
{
    ProximityClassPtr proxc;

    proxc = (ProximityClassPtr) xalloc(sizeof(ProximityClassRec));
    if (!proxc)
	return FALSE;
    dev->proximity = proxc;
    return TRUE;
}

_X_EXPORT void
InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, int minval, int maxval,
		       int resolution, int min_res, int max_res)
{
    AxisInfoPtr ax;
  
    if (!dev || !dev->valuator)
        return;

    ax = dev->valuator->axes + axnum;

    ax->min_value = minval;
    ax->max_value = maxval;
    ax->resolution = resolution;
    ax->min_resolution = min_res;
    ax->max_resolution = max_res;
}

static void
FixDeviceStateNotify(DeviceIntPtr dev, deviceStateNotify * ev, KeyClassPtr k,
		     ButtonClassPtr b, ValuatorClassPtr v, int first)
{
    ev->type = DeviceStateNotify;
    ev->deviceid = dev->id;
    ev->time = currentTime.milliseconds;
    ev->classes_reported = 0;
    ev->num_keys = 0;
    ev->num_buttons = 0;
    ev->num_valuators = 0;

    if (b) {
	ev->classes_reported |= (1 << ButtonClass);
	ev->num_buttons = b->numButtons;
	memmove((char *)&ev->buttons[0], (char *)b->down, 4);
    } else if (k) {
	ev->classes_reported |= (1 << KeyClass);
	ev->num_keys = k->curKeySyms.maxKeyCode - k->curKeySyms.minKeyCode;
	memmove((char *)&ev->keys[0], (char *)k->down, 4);
    }
    if (v) {
	int nval = v->numAxes - first;

	ev->classes_reported |= (1 << ValuatorClass);
	ev->classes_reported |= (dev->valuator->mode << ModeBitsShift);
	ev->num_valuators = nval < 3 ? nval : 3;
	switch (ev->num_valuators) {
	case 3:
	    ev->valuator2 = v->axisVal[first + 2];
	case 2:
	    ev->valuator1 = v->axisVal[first + 1];
	case 1:
	    ev->valuator0 = v->axisVal[first];
	    break;
	}
    }
}

static void
FixDeviceValuator(DeviceIntPtr dev, deviceValuator * ev, ValuatorClassPtr v,
		  int first)
{
    int nval = v->numAxes - first;

    ev->type = DeviceValuator;
    ev->deviceid = dev->id;
    ev->num_valuators = nval < 3 ? nval : 3;
    ev->first_valuator = first;
    switch (ev->num_valuators) {
    case 3:
	ev->valuator2 = v->axisVal[first + 2];
    case 2:
	ev->valuator1 = v->axisVal[first + 1];
    case 1:
	ev->valuator0 = v->axisVal[first];
	break;
    }
    first += ev->num_valuators;
}

void
DeviceFocusEvent(DeviceIntPtr dev, int type, int mode, int detail,
		 WindowPtr pWin)
{
    deviceFocus event;

    if (type == FocusIn)
	type = DeviceFocusIn;
    else
	type = DeviceFocusOut;

    event.deviceid = dev->id;
    event.mode = mode;
    event.type = type;
    event.detail = detail;
    event.window = pWin->drawable.id;
    event.time = currentTime.milliseconds;

    (void)DeliverEventsToWindow(dev, pWin, (xEvent *) & event, 1,
				DeviceFocusChangeMask, NullGrab, dev->id);

    if ((type == DeviceFocusIn) &&
	(wOtherInputMasks(pWin)) &&
	(wOtherInputMasks(pWin)->inputEvents[dev->id] & DeviceStateNotifyMask))
    {
	int evcount = 1;
	deviceStateNotify *ev, *sev;
	deviceKeyStateNotify *kev;
	deviceButtonStateNotify *bev;

	KeyClassPtr k;
	ButtonClassPtr b;
	ValuatorClassPtr v;
	int nval = 0, nkeys = 0, nbuttons = 0, first = 0;

	if ((b = dev->button) != NULL) {
	    nbuttons = b->numButtons;
	    if (nbuttons > 32)
		evcount++;
	}
	if ((k = dev->key) != NULL) {
	    nkeys = k->curKeySyms.maxKeyCode - k->curKeySyms.minKeyCode;
	    if (nkeys > 32)
		evcount++;
	    if (nbuttons > 0) {
		evcount++;
	    }
	}
	if ((v = dev->valuator) != NULL) {
	    nval = v->numAxes;

	    if (nval > 3)
		evcount++;
	    if (nval > 6) {
		if (!(k && b))
		    evcount++;
		if (nval > 9)
		    evcount += ((nval - 7) / 3);
	    }
	}

	sev = ev = (deviceStateNotify *) xalloc(evcount * sizeof(xEvent));
	FixDeviceStateNotify(dev, ev, NULL, NULL, NULL, first);

	if (b != NULL) {
	    FixDeviceStateNotify(dev, ev++, NULL, b, v, first);
	    first += 3;
	    nval -= 3;
	    if (nbuttons > 32) {
		(ev - 1)->deviceid |= MORE_EVENTS;
		bev = (deviceButtonStateNotify *) ev++;
		bev->type = DeviceButtonStateNotify;
		bev->deviceid = dev->id;
		memmove((char *)&bev->buttons[0], (char *)&b->down[4], 28);
	    }
	    if (nval > 0) {
		(ev - 1)->deviceid |= MORE_EVENTS;
		FixDeviceValuator(dev, (deviceValuator *) ev++, v, first);
		first += 3;
		nval -= 3;
	    }
	}

	if (k != NULL) {
	    FixDeviceStateNotify(dev, ev++, k, NULL, v, first);
	    first += 3;
	    nval -= 3;
	    if (nkeys > 32) {
		(ev - 1)->deviceid |= MORE_EVENTS;
		kev = (deviceKeyStateNotify *) ev++;
		kev->type = DeviceKeyStateNotify;
		kev->deviceid = dev->id;
		memmove((char *)&kev->keys[0], (char *)&k->down[4], 28);
	    }
	    if (nval > 0) {
		(ev - 1)->deviceid |= MORE_EVENTS;
		FixDeviceValuator(dev, (deviceValuator *) ev++, v, first);
		first += 3;
		nval -= 3;
	    }
	}

	while (nval > 0) {
	    FixDeviceStateNotify(dev, ev++, NULL, NULL, v, first);
	    first += 3;
	    nval -= 3;
	    if (nval > 0) {
		(ev - 1)->deviceid |= MORE_EVENTS;
		FixDeviceValuator(dev, (deviceValuator *) ev++, v, first);
		first += 3;
		nval -= 3;
	    }
	}

	(void)DeliverEventsToWindow(dev, pWin, (xEvent *) sev, evcount,
				    DeviceStateNotifyMask, NullGrab, dev->id);
	xfree(sev);
    }
}

int
GrabButton(ClientPtr client, DeviceIntPtr dev, BYTE this_device_mode,
	   BYTE other_devices_mode, CARD16 modifiers,
	   DeviceIntPtr modifier_device, CARD8 button, Window grabWindow,
	   BOOL ownerEvents, Cursor rcursor, Window rconfineTo, Mask eventMask)
{
    WindowPtr pWin, confineTo;
    CursorPtr cursor;
    GrabPtr grab;
    int rc;

    if ((this_device_mode != GrabModeSync) &&
	(this_device_mode != GrabModeAsync)) {
	client->errorValue = this_device_mode;
	return BadValue;
    }
    if ((other_devices_mode != GrabModeSync) &&
	(other_devices_mode != GrabModeAsync)) {
	client->errorValue = other_devices_mode;
	return BadValue;
    }
    if ((modifiers != AnyModifier) && (modifiers & ~AllModifiersMask)) {
	client->errorValue = modifiers;
	return BadValue;
    }
    if ((ownerEvents != xFalse) && (ownerEvents != xTrue)) {
	client->errorValue = ownerEvents;
	return BadValue;
    }
    rc = dixLookupWindow(&pWin, grabWindow, client, DixUnknownAccess);
    if (rc != Success)
	return rc;
    if (rconfineTo == None)
	confineTo = NullWindow;
    else {
	rc = dixLookupWindow(&confineTo, rconfineTo, client, DixUnknownAccess);
	if (rc != Success)
	    return rc;
    }
    if (rcursor == None)
	cursor = NullCursor;
    else {
	cursor = (CursorPtr) LookupIDByType(rcursor, RT_CURSOR);
	if (!cursor) {
	    client->errorValue = rcursor;
	    return BadCursor;
	}
    }

    grab = CreateGrab(client->index, dev, pWin, eventMask,
		      (Bool) ownerEvents, (Bool) this_device_mode,
		      (Bool) other_devices_mode, modifier_device, modifiers,
		      DeviceButtonPress, button, confineTo, cursor);
    if (!grab)
	return BadAlloc;
    return AddPassiveGrabToList(grab);
}

int
GrabKey(ClientPtr client, DeviceIntPtr dev, BYTE this_device_mode,
	BYTE other_devices_mode, CARD16 modifiers,
	DeviceIntPtr modifier_device, CARD8 key, Window grabWindow,
	BOOL ownerEvents, Mask mask)
{
    WindowPtr pWin;
    GrabPtr grab;
    KeyClassPtr k = dev->key;
    int rc;

    if (k == NULL)
	return BadMatch;
    if ((other_devices_mode != GrabModeSync) &&
	(other_devices_mode != GrabModeAsync)) {
	client->errorValue = other_devices_mode;
	return BadValue;
    }
    if ((this_device_mode != GrabModeSync) &&
	(this_device_mode != GrabModeAsync)) {
	client->errorValue = this_device_mode;
	return BadValue;
    }
    if (((key > k->curKeySyms.maxKeyCode) || (key < k->curKeySyms.minKeyCode))
	&& (key != AnyKey)) {
	client->errorValue = key;
	return BadValue;
    }
    if ((modifiers != AnyModifier) && (modifiers & ~AllModifiersMask)) {
	client->errorValue = modifiers;
	return BadValue;
    }
    if ((ownerEvents != xTrue) && (ownerEvents != xFalse)) {
	client->errorValue = ownerEvents;
	return BadValue;
    }
    rc = dixLookupWindow(&pWin, grabWindow, client, DixUnknownAccess);
    if (rc != Success)
	return rc;

    grab = CreateGrab(client->index, dev, pWin,
		      mask, ownerEvents, this_device_mode, other_devices_mode,
		      modifier_device, modifiers, DeviceKeyPress, key,
		      NullWindow, NullCursor);
    if (!grab)
	return BadAlloc;
    return AddPassiveGrabToList(grab);
}

int
SelectForWindow(DeviceIntPtr dev, WindowPtr pWin, ClientPtr client,
		Mask mask, Mask exclusivemasks, Mask validmasks)
{
    int mskidx = dev->id;
    int i, ret;
    Mask check;
    InputClientsPtr others;

    if (mask & ~validmasks) {
	client->errorValue = mask;
	return BadValue;
    }
    check = (mask & exclusivemasks);
    if (wOtherInputMasks(pWin)) {
	if (check & wOtherInputMasks(pWin)->inputEvents[mskidx]) {	/* It is illegal for two different
									 * clients to select on any of the
									 * events for maskcheck. However,
									 * it is OK, for some client to
									 * continue selecting on one of those
									 * events.  */
	    for (others = wOtherInputMasks(pWin)->inputClients; others;
		 others = others->next) {
		if (!SameClient(others, client) && (check &
						    others->mask[mskidx]))
		    return BadAccess;
	    }
	}
	for (others = wOtherInputMasks(pWin)->inputClients; others;
	     others = others->next) {
	    if (SameClient(others, client)) {
		check = others->mask[mskidx];
		others->mask[mskidx] = mask;
		if (mask == 0) {
		    for (i = 0; i < EMASKSIZE; i++)
			if (i != mskidx && others->mask[i] != 0)
			    break;
		    if (i == EMASKSIZE) {
			RecalculateDeviceDeliverableEvents(pWin);
			if (ShouldFreeInputMasks(pWin, FALSE))
			    FreeResource(others->resource, RT_NONE);
			return Success;
		    }
		}
		goto maskSet;
	    }
	}
    }
    check = 0;
    if ((ret = AddExtensionClient(pWin, client, mask, mskidx)) != Success)
	return ret;
  maskSet:
    if (dev->valuator)
	if ((dev->valuator->motionHintWindow == pWin) &&
	    (mask & DevicePointerMotionHintMask) &&
	    !(check & DevicePointerMotionHintMask) && !dev->deviceGrab.grab)
	    dev->valuator->motionHintWindow = NullWindow;
    RecalculateDeviceDeliverableEvents(pWin);
    return Success;
}

int
AddExtensionClient(WindowPtr pWin, ClientPtr client, Mask mask, int mskidx)
{
    InputClientsPtr others;

    if (!pWin->optional && !MakeWindowOptional(pWin))
	return BadAlloc;
    others = (InputClients *) xalloc(sizeof(InputClients));
    if (!others)
	return BadAlloc;
    if (!pWin->optional->inputMasks && !MakeInputMasks(pWin))
	return BadAlloc;
    bzero((char *)&others->mask[0], sizeof(Mask) * EMASKSIZE);
    others->mask[mskidx] = mask;
    others->resource = FakeClientID(client->index);
    others->next = pWin->optional->inputMasks->inputClients;
    pWin->optional->inputMasks->inputClients = others;
    if (!AddResource(others->resource, RT_INPUTCLIENT, (pointer) pWin))
	return BadAlloc;
    return Success;
}

static Bool
MakeInputMasks(WindowPtr pWin)
{
    struct _OtherInputMasks *imasks;

    imasks = (struct _OtherInputMasks *)
	xalloc(sizeof(struct _OtherInputMasks));
    if (!imasks)
	return FALSE;
    bzero((char *)imasks, sizeof(struct _OtherInputMasks));
    pWin->optional->inputMasks = imasks;
    return TRUE;
}

void
RecalculateDeviceDeliverableEvents(WindowPtr pWin)
{
    InputClientsPtr others;
    struct _OtherInputMasks *inputMasks;	/* default: NULL */
    WindowPtr pChild, tmp;
    int i;

    pChild = pWin;
    while (1) {
	if ((inputMasks = wOtherInputMasks(pChild)) != 0) {
	    for (others = inputMasks->inputClients; others;
		 others = others->next) {
		for (i = 0; i < EMASKSIZE; i++)
		    inputMasks->inputEvents[i] |= others->mask[i];
	    }
	    for (i = 0; i < EMASKSIZE; i++)
		inputMasks->deliverableEvents[i] = inputMasks->inputEvents[i];
	    for (tmp = pChild->parent; tmp; tmp = tmp->parent)
		if (wOtherInputMasks(tmp))
		    for (i = 0; i < EMASKSIZE; i++)
			inputMasks->deliverableEvents[i] |=
			    (wOtherInputMasks(tmp)->deliverableEvents[i]
			     & ~inputMasks->
			     dontPropagateMask[i] & PropagateMask[i]);
	}
	if (pChild->firstChild) {
	    pChild = pChild->firstChild;
	    continue;
	}
	while (!pChild->nextSib && (pChild != pWin))
	    pChild = pChild->parent;
	if (pChild == pWin)
	    break;
	pChild = pChild->nextSib;
    }
}

int
InputClientGone(WindowPtr pWin, XID id)
{
    InputClientsPtr other, prev;

    if (!wOtherInputMasks(pWin))
	return (Success);
    prev = 0;
    for (other = wOtherInputMasks(pWin)->inputClients; other;
	 other = other->next) {
	if (other->resource == id) {
	    if (prev) {
		prev->next = other->next;
		xfree(other);
	    } else if (!(other->next)) {
		if (ShouldFreeInputMasks(pWin, TRUE)) {
		    wOtherInputMasks(pWin)->inputClients = other->next;
		    xfree(wOtherInputMasks(pWin));
		    pWin->optional->inputMasks = (OtherInputMasks *) NULL;
		    CheckWindowOptionalNeed(pWin);
		    xfree(other);
		} else {
		    other->resource = FakeClientID(0);
		    if (!AddResource(other->resource, RT_INPUTCLIENT,
				     (pointer) pWin))
			return BadAlloc;
		}
	    } else {
		wOtherInputMasks(pWin)->inputClients = other->next;
		xfree(other);
	    }
	    RecalculateDeviceDeliverableEvents(pWin);
	    return (Success);
	}
	prev = other;
    }
    FatalError("client not on device event list");
}

int
SendEvent(ClientPtr client, DeviceIntPtr d, Window dest, Bool propagate,
	  xEvent * ev, Mask mask, int count)
{
    WindowPtr pWin;
    WindowPtr effectiveFocus = NullWindow;	/* only set if dest==InputFocus */
    WindowPtr spriteWin = GetSpriteWindow(d);

    if (dest == PointerWindow)
	pWin = spriteWin;
    else if (dest == InputFocus) {
	WindowPtr inputFocus;

	if (!d->focus)
	    inputFocus = spriteWin;
	else
	    inputFocus = d->focus->win;

	if (inputFocus == FollowKeyboardWin)
	    inputFocus = inputInfo.keyboard->focus->win;

	if (inputFocus == NoneWin)
	    return Success;

	/* If the input focus is PointerRootWin, send the event to where
	 * the pointer is if possible, then perhaps propogate up to root. */
	if (inputFocus == PointerRootWin)
	    inputFocus = GetCurrentRootWindow(d);

	if (IsParent(inputFocus, spriteWin)) {
	    effectiveFocus = inputFocus;
	    pWin = spriteWin;
	} else
	    effectiveFocus = pWin = inputFocus;
    } else
	dixLookupWindow(&pWin, dest, client, DixUnknownAccess);
    if (!pWin)
	return BadWindow;
    if ((propagate != xFalse) && (propagate != xTrue)) {
	client->errorValue = propagate;
	return BadValue;
    }
    ev->u.u.type |= 0x80;
    if (propagate) {
	for (; pWin; pWin = pWin->parent) {
	    if (DeliverEventsToWindow(d, pWin, ev, count, mask, NullGrab, d->id))
		return Success;
	    if (pWin == effectiveFocus)
		return Success;
	    if (wOtherInputMasks(pWin))
		mask &= ~wOtherInputMasks(pWin)->dontPropagateMask[d->id];
	    if (!mask)
		break;
	}
    } else
	(void)(DeliverEventsToWindow(d, pWin, ev, count, mask, NullGrab, d->id));
    return Success;
}

int
SetButtonMapping(ClientPtr client, DeviceIntPtr dev, int nElts, BYTE * map)
{
    int i;
    ButtonClassPtr b = dev->button;

    if (b == NULL)
	return BadMatch;

    if (nElts != b->numButtons) {
	client->errorValue = nElts;
	return BadValue;
    }
    if (BadDeviceMap(&map[0], nElts, 1, 255, &client->errorValue))
	return BadValue;
    for (i = 0; i < nElts; i++)
	if ((b->map[i + 1] != map[i]) && BitIsOn(b->down, i + 1))
	    return MappingBusy;
    for (i = 0; i < nElts; i++)
	b->map[i + 1] = map[i];
    return Success;
}

int
SetModifierMapping(ClientPtr client, DeviceIntPtr dev, int len, int rlen,
		   int numKeyPerModifier, KeyCode * inputMap, KeyClassPtr * k)
{
    KeyCode *map = NULL;
    int inputMapLen;
    int i;

    *k = dev->key;
    if (*k == NULL)
	return BadMatch;
    if (len != ((numKeyPerModifier << 1) + rlen))
	return BadLength;

    inputMapLen = 8 * numKeyPerModifier;

    /*
     *  Now enforce the restriction that "all of the non-zero keycodes must be
     *  in the range specified by min-keycode and max-keycode in the
     *  connection setup (else a Value error)"
     */
    i = inputMapLen;
    while (i--) {
	if (inputMap[i]
	    && (inputMap[i] < (*k)->curKeySyms.minKeyCode
		|| inputMap[i] > (*k)->curKeySyms.maxKeyCode)) {
	    client->errorValue = inputMap[i];
	    return -1;	/* BadValue collides with MappingFailed */
	}
    }

    /*
     *  Now enforce the restriction that none of the old or new
     *  modifier keys may be down while we change the mapping,  and
     *  that the DDX layer likes the choice.
     */
    if (!AllModifierKeysAreUp(dev, (*k)->modifierKeyMap,
			      (int)(*k)->maxKeysPerModifier, inputMap,
			      (int)numKeyPerModifier)
	|| !AllModifierKeysAreUp(dev, inputMap, (int)numKeyPerModifier,
				 (*k)->modifierKeyMap,
				 (int)(*k)->maxKeysPerModifier)) {
	return MappingBusy;
    } else {
	for (i = 0; i < inputMapLen; i++) {
	    if (inputMap[i] && !LegalModifier(inputMap[i], dev)) {
		return MappingFailed;
	    }
	}
    }

    /*
     *  Now build the keyboard's modifier bitmap from the
     *  list of keycodes.
     */
    if (inputMapLen) {
	map = (KeyCode *) xalloc(inputMapLen);
	if (!map)
	    return BadAlloc;
    }
    if ((*k)->modifierKeyMap)
	xfree((*k)->modifierKeyMap);
    if (inputMapLen) {
	(*k)->modifierKeyMap = map;
	memmove((char *)(*k)->modifierKeyMap, (char *)inputMap, inputMapLen);
    } else
	(*k)->modifierKeyMap = NULL;

    (*k)->maxKeysPerModifier = numKeyPerModifier;
    for (i = 0; i < MAP_LENGTH; i++)
	(*k)->modifierMap[i] = 0;
    for (i = 0; i < inputMapLen; i++)
	if (inputMap[i]) {
	    (*k)->modifierMap[inputMap[i]]
		|= (1 << (i / (*k)->maxKeysPerModifier));
	}

    return (MappingSuccess);
}

void
SendDeviceMappingNotify(ClientPtr client, CARD8 request,
			KeyCode firstKeyCode, CARD8 count, DeviceIntPtr dev)
{
    xEvent event;
    deviceMappingNotify *ev = (deviceMappingNotify *) & event;

    ev->type = DeviceMappingNotify;
    ev->request = request;
    ev->deviceid = dev->id;
    ev->time = currentTime.milliseconds;
    if (request == MappingKeyboard) {
	ev->firstKeyCode = firstKeyCode;
	ev->count = count;
    }

#ifdef XKB
    if (request == MappingKeyboard || request == MappingModifier)
        XkbApplyMappingChange(dev, request, firstKeyCode, count, client);
#endif

    SendEventToAllWindows(dev, DeviceMappingNotifyMask, (xEvent *) ev, 1);
}

int
ChangeKeyMapping(ClientPtr client,
		 DeviceIntPtr dev,
		 unsigned len,
		 int type,
		 KeyCode firstKeyCode,
		 CARD8 keyCodes, CARD8 keySymsPerKeyCode, KeySym * map)
{
    KeySymsRec keysyms;
    KeyClassPtr k = dev->key;

    if (k == NULL)
	return (BadMatch);

    if (len != (keyCodes * keySymsPerKeyCode))
	return BadLength;

    if ((firstKeyCode < k->curKeySyms.minKeyCode) ||
	(firstKeyCode + keyCodes - 1 > k->curKeySyms.maxKeyCode)) {
	client->errorValue = firstKeyCode;
	return BadValue;
    }
    if (keySymsPerKeyCode == 0) {
	client->errorValue = 0;
	return BadValue;
    }
    keysyms.minKeyCode = firstKeyCode;
    keysyms.maxKeyCode = firstKeyCode + keyCodes - 1;
    keysyms.mapWidth = keySymsPerKeyCode;
    keysyms.map = map;
    if (!SetKeySymsMap(&k->curKeySyms, &keysyms))
	return BadAlloc;
    SendDeviceMappingNotify(client, MappingKeyboard, firstKeyCode, keyCodes, dev);
    return client->noClientException;
}

static void
DeleteDeviceFromAnyExtEvents(WindowPtr pWin, DeviceIntPtr dev)
{
    WindowPtr parent;

    /* Deactivate any grabs performed on this window, before making
     * any input focus changes.
     * Deactivating a device grab should cause focus events. */

    if (dev->deviceGrab.grab && (dev->deviceGrab.grab->window == pWin))
	(*dev->deviceGrab.DeactivateGrab) (dev);

    /* If the focus window is a root window (ie. has no parent)
     * then don't delete the focus from it. */

    if (dev->focus && (pWin == dev->focus->win) && (pWin->parent != NullWindow)) {
	int focusEventMode = NotifyNormal;

	/* If a grab is in progress, then alter the mode of focus events. */

	if (dev->deviceGrab.grab)
	    focusEventMode = NotifyWhileGrabbed;

	switch (dev->focus->revert) {
	case RevertToNone:
	    DoFocusEvents(dev, pWin, NoneWin, focusEventMode);
	    dev->focus->win = NoneWin;
	    dev->focus->traceGood = 0;
	    break;
	case RevertToParent:
	    parent = pWin;
	    do {
		parent = parent->parent;
		dev->focus->traceGood--;
	    }
	    while (!parent->realized);
	    DoFocusEvents(dev, pWin, parent, focusEventMode);
	    dev->focus->win = parent;
	    dev->focus->revert = RevertToNone;
	    break;
	case RevertToPointerRoot:
	    DoFocusEvents(dev, pWin, PointerRootWin, focusEventMode);
	    dev->focus->win = PointerRootWin;
	    dev->focus->traceGood = 0;
	    break;
	case RevertToFollowKeyboard:
	    if (inputInfo.keyboard->focus->win) {
		DoFocusEvents(dev, pWin, inputInfo.keyboard->focus->win,
			      focusEventMode);
		dev->focus->win = FollowKeyboardWin;
		dev->focus->traceGood = 0;
	    } else {
		DoFocusEvents(dev, pWin, NoneWin, focusEventMode);
		dev->focus->win = NoneWin;
		dev->focus->traceGood = 0;
	    }
	    break;
	}
    }

    if (dev->valuator)
	if (dev->valuator->motionHintWindow == pWin)
	    dev->valuator->motionHintWindow = NullWindow;
}

void
DeleteWindowFromAnyExtEvents(WindowPtr pWin, Bool freeResources)
{
    int i;
    DeviceIntPtr dev;
    InputClientsPtr ic;
    struct _OtherInputMasks *inputMasks;

    for (dev = inputInfo.devices; dev; dev = dev->next) {
	if (dev == inputInfo.pointer || dev == inputInfo.keyboard)
	    continue;
	DeleteDeviceFromAnyExtEvents(pWin, dev);
    }

    for (dev = inputInfo.off_devices; dev; dev = dev->next)
	DeleteDeviceFromAnyExtEvents(pWin, dev);

    if (freeResources)
	while ((inputMasks = wOtherInputMasks(pWin)) != 0) {
	    ic = inputMasks->inputClients;
	    for (i = 0; i < EMASKSIZE; i++)
		inputMasks->dontPropagateMask[i] = 0;
	    FreeResource(ic->resource, RT_NONE);
	}
}

int
MaybeSendDeviceMotionNotifyHint(deviceKeyButtonPointer * pEvents, Mask mask)
{
    DeviceIntPtr dev;

    dev = LookupDeviceIntRec(pEvents->deviceid & DEVICE_BITS);
    if (!dev)
        return 0;

    if (pEvents->type == DeviceMotionNotify) {
	if (mask & DevicePointerMotionHintMask) {
	    if (WID(dev->valuator->motionHintWindow) == pEvents->event) {
		return 1;	/* don't send, but pretend we did */
	    }
	    pEvents->detail = NotifyHint;
	} else {
	    pEvents->detail = NotifyNormal;
	}
    }
    return (0);
}

void
CheckDeviceGrabAndHintWindow(WindowPtr pWin, int type,
			     deviceKeyButtonPointer * xE, GrabPtr grab,
			     ClientPtr client, Mask deliveryMask)
{
    DeviceIntPtr dev;

    dev = LookupDeviceIntRec(xE->deviceid & DEVICE_BITS);
    if (!dev)
        return;

    if (type == DeviceMotionNotify)
	dev->valuator->motionHintWindow = pWin;
    else if ((type == DeviceButtonPress) && (!grab) &&
	     (deliveryMask & DeviceButtonGrabMask)) {
	GrabRec tempGrab;

	tempGrab.device = dev;
	tempGrab.resource = client->clientAsMask;
	tempGrab.window = pWin;
	tempGrab.ownerEvents =
	    (deliveryMask & DeviceOwnerGrabButtonMask) ? TRUE : FALSE;
	tempGrab.eventMask = deliveryMask;
	tempGrab.keyboardMode = GrabModeAsync;
	tempGrab.pointerMode = GrabModeAsync;
	tempGrab.confineTo = NullWindow;
	tempGrab.cursor = NullCursor;
        tempGrab.genericMasks = NULL;
        tempGrab.next = NULL;
	(*dev->deviceGrab.ActivateGrab) (dev, &tempGrab, currentTime, TRUE);
    }
}

static Mask
DeviceEventMaskForClient(DeviceIntPtr dev, WindowPtr pWin, ClientPtr client)
{
    InputClientsPtr other;

    if (!wOtherInputMasks(pWin))
	return 0;
    for (other = wOtherInputMasks(pWin)->inputClients; other;
	 other = other->next) {
	if (SameClient(other, client))
	    return other->mask[dev->id];
    }
    return 0;
}

void
MaybeStopDeviceHint(DeviceIntPtr dev, ClientPtr client)
{
    WindowPtr pWin;
    GrabPtr grab = dev->deviceGrab.grab;

    pWin = dev->valuator->motionHintWindow;

    if ((grab && SameClient(grab, client) &&
	 ((grab->eventMask & DevicePointerMotionHintMask) ||
	  (grab->ownerEvents &&
	   (DeviceEventMaskForClient(dev, pWin, client) &
	    DevicePointerMotionHintMask)))) ||
	(!grab &&
	 (DeviceEventMaskForClient(dev, pWin, client) &
	  DevicePointerMotionHintMask)))
	dev->valuator->motionHintWindow = NullWindow;
}

int
DeviceEventSuppressForWindow(WindowPtr pWin, ClientPtr client, Mask mask,
			     int maskndx)
{
    struct _OtherInputMasks *inputMasks = wOtherInputMasks(pWin);

    if (mask & ~PropagateMask[maskndx]) {
	client->errorValue = mask;
	return BadValue;
    }

    if (mask == 0) {
	if (inputMasks)
	    inputMasks->dontPropagateMask[maskndx] = mask;
    } else {
	if (!inputMasks)
	    AddExtensionClient(pWin, client, 0, 0);
	inputMasks = wOtherInputMasks(pWin);
	inputMasks->dontPropagateMask[maskndx] = mask;
    }
    RecalculateDeviceDeliverableEvents(pWin);
    if (ShouldFreeInputMasks(pWin, FALSE))
	FreeResource(inputMasks->inputClients->resource, RT_NONE);
    return Success;
}

Bool
ShouldFreeInputMasks(WindowPtr pWin, Bool ignoreSelectedEvents)
{
    int i;
    Mask allInputEventMasks = 0;
    struct _OtherInputMasks *inputMasks = wOtherInputMasks(pWin);

    for (i = 0; i < EMASKSIZE; i++)
	allInputEventMasks |= inputMasks->dontPropagateMask[i];
    if (!ignoreSelectedEvents)
	for (i = 0; i < EMASKSIZE; i++)
	    allInputEventMasks |= inputMasks->inputEvents[i];
    if (allInputEventMasks == 0)
	return TRUE;
    else
	return FALSE;
}

/***********************************************************************
 *
 * Walk through the window tree, finding all clients that want to know
 * about the Event.
 *
 */

static void
FindInterestedChildren(DeviceIntPtr dev, WindowPtr p1, Mask mask,
                       xEvent * ev, int count)
{
    WindowPtr p2;

    while (p1) {
        p2 = p1->firstChild;
        (void)DeliverEventsToWindow(dev, p1, ev, count, mask, NullGrab, dev->id);
        FindInterestedChildren(dev, p2, mask, ev, count);
        p1 = p1->nextSib;
    }
}

/***********************************************************************
 *
 * Send an event to interested clients in all windows on all screens.
 *
 */

void
SendEventToAllWindows(DeviceIntPtr dev, Mask mask, xEvent * ev, int count)
{
    int i;
    WindowPtr pWin, p1;

    for (i = 0; i < screenInfo.numScreens; i++) {
        pWin = WindowTable[i];
        (void)DeliverEventsToWindow(dev, pWin, ev, count, mask, NullGrab, dev->id);
        p1 = pWin->firstChild;
        FindInterestedChildren(dev, p1, mask, ev, count);
    }
}
