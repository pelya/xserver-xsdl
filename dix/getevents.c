/*
 * Copyright © 2006 Nokia Corporation
 * Copyright © 2006 Daniel Stone
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that this copyright notice and this permission notice appear in
 * supporting electronic documentation.
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */
 /* 
  * MPX additions:
  * Copyright © 2006 Peter Hutterer
  * Author: Peter Hutterer <peter@cs.unisa.edu.au>
  *
  */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/keysym.h>
#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xproto.h>

#include "misc.h"
#include "resource.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "globals.h"
#include "dixevents.h"
#include "mipointer.h"

#ifdef XKB
#include <X11/extensions/XKBproto.h>
#include <xkbsrv.h>
extern Bool XkbCopyKeymap(XkbDescPtr src, XkbDescPtr dst, Bool sendNotifies);
#endif

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"
#include "exevents.h"
#include "exglobals.h"
#include "extnsionst.h"

/* Maximum number of valuators, divided by six, rounded up, to get number
 * of events. */
#define MAX_VALUATOR_EVENTS 6

/* Number of motion history events to store. */
#define MOTION_HISTORY_SIZE 256


/**
 * Pick some arbitrary size for Xi motion history.
 */
_X_EXPORT int
GetMotionHistorySize(void)
{
    return MOTION_HISTORY_SIZE;
}


/**
 * Allocate the motion history buffer.
 */
_X_EXPORT void
AllocateMotionHistory(DeviceIntPtr pDev)
{
    if (pDev->valuator->motion)
        xfree(pDev->valuator->motion);

    if (pDev->valuator->numMotionEvents < 1)
        return;

    pDev->valuator->motion = xalloc(((sizeof(INT32) * pDev->valuator->numAxes) +
                                     sizeof(Time)) *
                                    pDev->valuator->numMotionEvents);
    pDev->valuator->first_motion = 0;
    pDev->valuator->last_motion = 0;
}


/**
 * Dump the motion history between start and stop into the supplied buffer.
 * Only records the event for a given screen in theory, but in practice, we
 * sort of ignore this.
 */
_X_EXPORT int
GetMotionHistory(DeviceIntPtr pDev, xTimecoord *buff, unsigned long start,
                 unsigned long stop, ScreenPtr pScreen)
{
    char *ibuff = NULL, *obuff = (char *) buff;
    int i = 0, ret = 0;
    Time current;
    /* The size of a single motion event. */
    int size = (sizeof(INT32) * pDev->valuator->numAxes) + sizeof(Time);

    if (!pDev->valuator || !pDev->valuator->numMotionEvents)
        return 0;

    for (i = pDev->valuator->first_motion;
         i != pDev->valuator->last_motion;
         i = (i + 1) % pDev->valuator->numMotionEvents) {
        /* We index the input buffer by which element we're accessing, which
         * is not monotonic, and the output buffer by how many events we've
         * written so far. */
        ibuff = (char *) pDev->valuator->motion + (i * size);
        memcpy(&current, ibuff, sizeof(Time));

        if (current > stop) {
            return ret;
        }
        else if (current >= start) {
            memcpy(obuff, ibuff, size);
            obuff += size;
            ret++;
        }
    }

    return ret;
}


/**
 * Update the motion history for a specific device, with the list of
 * valuators.
 */
static void
updateMotionHistory(DeviceIntPtr pDev, CARD32 ms, int first_valuator,
                    int num_valuators, int *valuators)
{
    char *buff = (char *) pDev->valuator->motion;

    if (!pDev->valuator->numMotionEvents)
        return;

    buff += ((sizeof(INT32) * pDev->valuator->numAxes) + sizeof(CARD32)) *
            pDev->valuator->last_motion;
    memcpy(buff, &ms, sizeof(Time));

    buff += sizeof(Time);
    bzero(buff, sizeof(INT32) * pDev->valuator->numAxes);

    buff += sizeof(INT32) * first_valuator;
    memcpy(buff, valuators, sizeof(INT32) * num_valuators);

    pDev->valuator->last_motion = (pDev->valuator->last_motion + 1) %
                                  pDev->valuator->numMotionEvents;

    /* If we're wrapping around, just keep the circular buffer going. */
    if (pDev->valuator->first_motion == pDev->valuator->last_motion)
        pDev->valuator->first_motion = (pDev->valuator->first_motion + 1) %
                                       pDev->valuator->numMotionEvents;

    return;
}


/**
 * Returns the maximum number of events GetKeyboardEvents,
 * GetKeyboardValuatorEvents, and GetPointerEvents will ever return.
 *
 * Should be used in DIX as:
 * xEvent *events = xcalloc(sizeof(xEvent), GetMaximumEventsNum());
 */
_X_EXPORT int
GetMaximumEventsNum(void) {
    /* Two base events -- core and device, plus valuator events.  Multiply
     * by two if we're doing key repeats. */
    int ret = 2 + MAX_VALUATOR_EVENTS;

#ifdef XKB
    if (noXkbExtension)
#endif
        ret *= 2;

    return ret;
}


/* Originally a part of xf86PostMotionEvent; modifies valuators
 * in-place. */
static void
acceleratePointer(DeviceIntPtr pDev, int first_valuator, int num_valuators,
                  int *valuators)
{
    float mult = 0.0;
    int dx = 0, dy = 0;
    int *px = NULL, *py = NULL;

    if (!num_valuators || !valuators)
        return;

    if (first_valuator == 0) {
        dx = valuators[0];
        px = &valuators[0];
    }
    if (first_valuator <= 1 && num_valuators >= (2 - first_valuator)) {
        dy = valuators[1 - first_valuator];
        py = &valuators[1 - first_valuator];
    }

    if (!dx && !dy)
        return;

    if (pDev->ptrfeed && pDev->ptrfeed->ctrl.num) {
        /* modeled from xf86Events.c */
        if (pDev->ptrfeed->ctrl.threshold) {
            if ((abs(dx) + abs(dy)) >= pDev->ptrfeed->ctrl.threshold) {
                pDev->valuator->dxremaind = ((float)dx *
                                             (float)(pDev->ptrfeed->ctrl.num)) /
                                             (float)(pDev->ptrfeed->ctrl.den) +
                                            pDev->valuator->dxremaind;
                if (px) {
                    *px = (int)pDev->valuator->dxremaind;
                    pDev->valuator->dxremaind = pDev->valuator->dxremaind -
                                                (float)(*px);
                }

                pDev->valuator->dyremaind = ((float)dy *
                                             (float)(pDev->ptrfeed->ctrl.num)) /
                                             (float)(pDev->ptrfeed->ctrl.den) +
                                            pDev->valuator->dyremaind;
                if (py) {
                    *py = (int)pDev->valuator->dyremaind;
                    pDev->valuator->dyremaind = pDev->valuator->dyremaind -
                                                (float)(*py);
                }
            }
        }
        else {
            mult = pow((float)(dx * dx + dy * dy),
                       ((float)(pDev->ptrfeed->ctrl.num) /
                        (float)(pDev->ptrfeed->ctrl.den) - 1.0) /
                       2.0) / 2.0;
            if (dx) {
                pDev->valuator->dxremaind = mult * (float)dx +
                                            pDev->valuator->dxremaind;
                *px = (int)pDev->valuator->dxremaind;
                pDev->valuator->dxremaind = pDev->valuator->dxremaind -
                                            (float)(*px);
            }
            if (dy) {
                pDev->valuator->dyremaind = mult * (float)dy +
                                            pDev->valuator->dyremaind;
                *py = (int)pDev->valuator->dyremaind;
                pDev->valuator->dyremaind = pDev->valuator->dyremaind -
                                            (float)(*py);
            }
        }
    }
}


/**
 * Clip an axis to its bounds, which are declared in the call to
 * InitValuatorAxisClassStruct.
 */
static void
clipAxis(DeviceIntPtr pDev, int axisNum, int *val)
{
    AxisInfoPtr axes = pDev->valuator->axes + axisNum;

    if (*val < axes->min_value)
        *val = axes->min_value;
    if (axes->max_value >= 0 && *val > axes->max_value)
        *val = axes->max_value;
}

/**
 * Clip every axis in the list of valuators to its bounds.
 */
static void
clipValuators(DeviceIntPtr pDev, int first_valuator, int num_valuators,
              int *valuators)
{
    AxisInfoPtr axes = pDev->valuator->axes + first_valuator;
    int i;

    for (i = 0; i < num_valuators; i++, axes++)
        clipAxis(pDev, i + first_valuator, &(valuators[i]));
}


/**
 * Fills events with valuator events for pDev, as given by the other
 * parameters.
 *
 * FIXME: Need to fix ValuatorClassRec to store all the valuators as
 *        last posted, not just x and y; otherwise relative non-x/y
 *        valuators, though a very narrow use case, will be broken.
 */
static EventList *
getValuatorEvents(EventList *events, DeviceIntPtr pDev, int first_valuator,
                  int num_valuators, int *valuators) {
    deviceValuator *xv;
    int i = 0, final_valuator = first_valuator + num_valuators;

    for (i = first_valuator; i < final_valuator; i += 6, events++) {
        xv = (deviceValuator*)events->event;
        xv->type = DeviceValuator;
        xv->first_valuator = i;
        xv->num_valuators = num_valuators;
        xv->deviceid = pDev->id;
        switch (final_valuator - i) {
        case 6:
            xv->valuator5 = valuators[i + 5];
        case 5:
            xv->valuator4 = valuators[i + 4];
        case 4:
            xv->valuator3 = valuators[i + 3];
        case 3:
            xv->valuator2 = valuators[i + 2];
        case 2:
            xv->valuator1 = valuators[i + 1];
        case 1:
            xv->valuator0 = valuators[i];
        }

        if (i + 6 < final_valuator)
            xv->deviceid |= MORE_EVENTS;
    }

    return events;
}


/**
 * Convenience wrapper around GetKeyboardValuatorEvents, that takes no
 * valuators.
 */
_X_EXPORT int
GetKeyboardEvents(EventList *events, DeviceIntPtr pDev, int type, int key_code) {
    return GetKeyboardValuatorEvents(events, pDev, type, key_code, 0, 0, NULL);
}


/**
 * Returns a set of keyboard events for KeyPress/KeyRelease, optionally
 * also with valuator events.  Handles Xi and XKB.
 *
 * events is not NULL-terminated; the return value is the number of events.
 * The DDX is responsible for allocating the event structure in the first
 * place via GetMaximumEventsNum(), and for freeing it.
 *
 * This function does not change the core keymap to that of the device;
 * that is done by SwitchCoreKeyboard, which is called from
 * mieqProcessInputEvents.  If replacing that function, take care to call
 * SetCoreKeyboard before processInputProc, so keymaps are altered to suit.
 *
 * Note that this function recurses!  If called for non-XKB, a repeating
 * key press will trigger a matching KeyRelease, as well as the
 * KeyPresses.
 */
_X_EXPORT int
GetKeyboardValuatorEvents(EventList *events, DeviceIntPtr pDev, int type,
                          int key_code, int first_valuator,
                          int num_valuators, int *valuators) {
    int numEvents = 0;
    CARD32 ms = 0;
    KeySym *map = pDev->key->curKeySyms.map;
    KeySym sym = map[key_code * pDev->key->curKeySyms.mapWidth];
    deviceKeyButtonPointer *kbp = NULL;

    if (!events)
        return 0;

    if (type != KeyPress && type != KeyRelease)
        return 0;

    if (!pDev->key || !pDev->focus || !pDev->kbdfeed ||
        (pDev->coreEvents && !inputInfo.keyboard->key))
        return 0;

    if (pDev->coreEvents)
        numEvents = 2;
    else
        numEvents = 1;

    if (num_valuators) {
        if ((num_valuators / 6) + 1 > MAX_VALUATOR_EVENTS)
            num_valuators = MAX_VALUATOR_EVENTS;
        numEvents += (num_valuators / 6) + 1;
    }

#ifdef XKB
    if (noXkbExtension)
#endif
    {
        switch (sym) {
            case XK_Num_Lock:
            case XK_Caps_Lock:
            case XK_Scroll_Lock:
            case XK_Shift_Lock:
                if (type == KeyRelease)
                    return 0;
                else if (type == KeyPress &&
                         (pDev->key->down[key_code >> 3] & (key_code & 7)) & 1)
                        type = KeyRelease;
        }
    }

    /* Handle core repeating, via press/release/press/release.
     * FIXME: In theory, if you're repeating with two keyboards in non-XKB,
     *        you could get unbalanced events here. */
    if (type == KeyPress &&
        (((pDev->key->down[key_code >> 3] & (key_code & 7))) & 1)) {
        if (!pDev->kbdfeed->ctrl.autoRepeat ||
            pDev->key->modifierMap[key_code] ||
            !(pDev->kbdfeed->ctrl.autoRepeats[key_code >> 3]
                & (1 << (key_code & 7))))
            return 0;

#ifdef XKB
        if (noXkbExtension)
#endif
        {
            numEvents += GetKeyboardValuatorEvents(events, pDev,
                                                   KeyRelease, key_code,
                                                   first_valuator, num_valuators,
                                                   valuators);
            events += numEvents;
        }
    }

    ms = GetTimeInMillis();

    kbp = (deviceKeyButtonPointer *) events->event;
    kbp->time = ms;
    kbp->deviceid = pDev->id;
    if (type == KeyPress)
        kbp->type = DeviceKeyPress;
    else if (type == KeyRelease)
        kbp->type = DeviceKeyRelease;

    events++;
    if (num_valuators) {
        kbp->deviceid |= MORE_EVENTS;
        clipValuators(pDev, first_valuator, num_valuators, valuators);
        events = getValuatorEvents(events, pDev, first_valuator,
                                   num_valuators, valuators);
    }

    if (pDev->coreEvents) {
        xEvent* evt = events->event;
        evt->u.keyButtonPointer.time = ms;
        evt->u.u.type = type;
        evt->u.u.detail = key_code;
    }

    return numEvents;
}

/**
 * Initialize an event list and fill with 32 byte sized events. 
 * This event list is to be passed into GetPointerEvents() and
 * GetKeyboardEvents().
 *
 * @param num_events Number of elements in list.
 */
_X_EXPORT EventListPtr
InitEventList(int num_events)
{
    EventListPtr events;
    int i;

    events = (EventListPtr)xcalloc(num_events, sizeof(EventList));
    if (!events)
        return NULL;

    for (i = 0; i < num_events; i++)
    {
        events[i].evlen = sizeof(xEvent);
        events[i].event = xcalloc(1, sizeof(xEvent));
        if (!events[i].event)
        {
            /* rollback */
            while(i--)
                xfree(events[i].event);
            xfree(events);
            events = NULL;
            break;
        }
    }

    return events;
}

/**
 * Free an event list.
 *
 * @param list The list to be freed.
 * @param num_events Number of elements in list.
 */
_X_EXPORT void
FreeEventList(EventListPtr list, int num_events)
{
    if (!list)
        return;
    while(num_events--)
        xfree(list[num_events].event);
    xfree(list);
}

/**
 * Generate a series of xEvents (filled into the EventList) representing
 * pointer motion, or button presses.  Xi and XKB-aware.
 *
 * events is not NULL-terminated; the return value is the number of events.
 * The DDX is responsible for allocating the event structure in the first
 * place via InitEventList() and GetMaximumEventsNum(), and for freeing it.
 *
 */
_X_EXPORT int
GetPointerEvents(EventList *events, DeviceIntPtr pDev, int type, int buttons,
                 int flags, int first_valuator, int num_valuators,
                 int *valuators) {
    int num_events = 0, final_valuator = 0, i;
    CARD32 ms = 0;
    CARD32* valptr;
    deviceKeyButtonPointer *kbp = NULL;
    rawDeviceEvent* ev;

    /* Thanks to a broken lib, we _always_ have to chase DeviceMotionNotifies
     * with DeviceValuators. */
    Bool sendValuators = (type == MotionNotify || flags & POINTER_ABSOLUTE);
    DeviceIntPtr pointer = NULL;
    int x = 0, y = 0;
    /* The core pointer must not send Xi events. */
    Bool coreOnly = (pDev == inputInfo.pointer);

    /* Sanity checks. */
    if (type != MotionNotify && type != ButtonPress && type != ButtonRelease)
        return 0;

    if ((type == ButtonPress || type == ButtonRelease) && !pDev->button)
        return 0;

    if (!coreOnly && (pDev->coreEvents))
        num_events = 3;
    else
        num_events = 2;

    if (type == MotionNotify && num_valuators <= 0) {
        return 0;
    }

    /* Do we need to send a DeviceValuator event? */
    if (!coreOnly && sendValuators) {
        if ((((num_valuators - 1) / 6) + 1) > MAX_VALUATOR_EVENTS)
            num_valuators = MAX_VALUATOR_EVENTS * 6;
        num_events += ((num_valuators - 1) / 6) + 1;
    }

    final_valuator = num_valuators + first_valuator;

    /* You fail. */
    if (first_valuator < 0 || final_valuator > pDev->valuator->numAxes)
        return 0;

    ms = GetTimeInMillis();


    /* fill up the raw event, after checking that it is large enough to
     * accommodate all valuators. 
     */
    if (events->evlen < 
            (sizeof(xEvent) + ((num_valuators - 4) * sizeof(CARD32))))
    {
        events->evlen = sizeof(xEvent) + 
            ((num_valuators - 4) * sizeof(CARD32));
        events->event = realloc(events->event, events->evlen);
        if (!events->event)
            FatalError("Could not allocate event storage.\n");
    }

    ev = (rawDeviceEvent*)events->event;
    ev->type = GenericEvent;
    ev->evtype = XI_RawDeviceEvent;
    ev->extension = IReqCode;
    ev->length = (num_valuators > 4) ? (num_valuators - 4) : 0;
    ev->buttons = buttons;
    ev->num_valuators = num_valuators;
    ev->first_valuator = first_valuator;
    ev->deviceid = pDev->id;
    valptr = &(ev->valuator0);
    for (i = 0; i < num_valuators; i++, valptr++)
        *valptr = valuators[i];

    events++;
    pointer = pDev;

    /* Set x and y based on whether this is absolute or relative, and
     * accelerate if we need to. */
    if (flags & POINTER_ABSOLUTE) {
        if (num_valuators >= 1 && first_valuator == 0) {
            x = valuators[0];
        }
        else {
            x = pointer->valuator->lastx;
        }

        if (first_valuator <= 1 && num_valuators >= (2 - first_valuator)) {
            y = valuators[1 - first_valuator];
        }
        else {
            y = pointer->valuator->lasty;
        }
    }
    else {
        if (flags & POINTER_ACCELERATE)
            acceleratePointer(pDev, first_valuator, num_valuators,
                              valuators);

        if (first_valuator == 0 && num_valuators >= 1)
            x = pointer->valuator->lastx + valuators[0];
        else
            x = pointer->valuator->lastx;

        if (first_valuator <= 1 && num_valuators >= (2 - first_valuator))
            y = pointer->valuator->lasty + valuators[1 - first_valuator];
        else
            y = pointer->valuator->lasty;
    }

    /* Clip both x and y to the defined limits (usually co-ord space limit). */
    clipAxis(pDev, 0, &x);
    clipAxis(pDev, 1, &y);

    /* This takes care of crossing screens for us, as well as clipping
     * to the current screen.  Right now, we only have one history buffer,
     * so we don't set this for both the device and core.*/
    miPointerSetPosition(pDev, &x, &y, ms);

    /* Drop x and y back into the valuators list, if they were originally
     * present. */
    if (first_valuator == 0 && num_valuators >= 1)
        valuators[0] = x;
    if (first_valuator <= 1 && num_valuators >= (2 - first_valuator))
        valuators[1 - first_valuator] = y;

    updateMotionHistory(pDev, ms, first_valuator, num_valuators, valuators);

    pDev->valuator->lastx = x;
    pDev->valuator->lasty = y;

    /* create Xi event */
    if (!coreOnly)
    {
        kbp = (deviceKeyButtonPointer *) events->event;
        kbp->time = ms;
        kbp->deviceid = pDev->id;

        if (type == MotionNotify) {
            kbp->type = DeviceMotionNotify;
        }
        else {
            if (type == ButtonPress)
                kbp->type = DeviceButtonPress;
            else if (type == ButtonRelease)
                kbp->type = DeviceButtonRelease;
            kbp->detail = pDev->button->map[buttons];
        }

        kbp->root_x = x;
        kbp->root_y = y;

        events++;
        if (sendValuators) {
            kbp->deviceid |= MORE_EVENTS;
            clipValuators(pDev, first_valuator, num_valuators, valuators);
            events = getValuatorEvents(events, pDev, first_valuator,
                                       num_valuators, valuators);
        }
    }

    if (coreOnly || pDev->coreEvents) {
        xEvent* evt = events->event;
        evt->u.u.type = type;
        evt->u.keyButtonPointer.time = ms;
        evt->u.keyButtonPointer.rootX = x;
        evt->u.keyButtonPointer.rootY = y;

        if (type == ButtonPress || type == ButtonRelease) {
            /* We hijack SetPointerMapping to work on all core-sending
             * devices, so we use the device-specific map here instead of
             * the core one. */
            evt->u.u.detail = pDev->button->map[buttons];
        }
        else {
            evt->u.u.detail = 0;
        }
    }

    return num_events;
}


/**
 * Post ProximityIn/ProximityOut events, accompanied by valuators.
 *
 * events is not NULL-terminated; the return value is the number of events.
 * The DDX is responsible for allocating the event structure in the first
 * place via GetMaximumEventsNum(), and for freeing it.
 */
_X_EXPORT int
GetProximityEvents(EventList *events, DeviceIntPtr pDev, int type,
                   int first_valuator, int num_valuators, int *valuators)
{
    int num_events = 0;
    deviceKeyButtonPointer *kbp = (deviceKeyButtonPointer *) events->event;

    /* Sanity checks. */
    if (type != ProximityIn && type != ProximityOut)
        return 0;

    if (!pDev->valuator)
        return 0;

    /* Do we need to send a DeviceValuator event? */
    if ((pDev->valuator->mode & 1) == Relative)
        num_valuators = 0;

    if (num_valuators) {
        if ((((num_valuators - 1) / 6) + 1) > MAX_VALUATOR_EVENTS)
            num_valuators = MAX_VALUATOR_EVENTS * 6;
        num_events += ((num_valuators - 1) / 6) + 1;
    }

    /* You fail. */
    if (first_valuator < 0 ||
        (num_valuators + first_valuator) > pDev->valuator->numAxes)
        return 0;

    kbp->type = type;
    kbp->deviceid = pDev->id;
    kbp->detail = 0;
    kbp->time = GetTimeInMillis();

    if (num_valuators) {
        kbp->deviceid |= MORE_EVENTS;
        events++;
        clipValuators(pDev, first_valuator, num_valuators, valuators);
        events = getValuatorEvents(events, pDev, first_valuator,
                                   num_valuators, valuators);
    }

    return num_events;
}


/**
 * Note that pDev was the last device to send a core event.  This function
 * copies the complete keymap from the originating device to the core
 * device, and makes sure the appropriate notifications are generated.
 *
 * Call this just before processInputProc.
 */
_X_EXPORT void
SwitchCoreKeyboard(DeviceIntPtr pDev)
{
    KeyClassPtr ckeyc = inputInfo.keyboard->key;
    int i = 0;

    if (inputInfo.keyboard->devPrivates[CoreDevicePrivatesIndex].ptr != pDev) {
        memcpy(ckeyc->modifierMap, pDev->key->modifierMap, MAP_LENGTH);
        if (ckeyc->modifierKeyMap)
            xfree(ckeyc->modifierKeyMap);
        ckeyc->modifierKeyMap = xalloc(8 * pDev->key->maxKeysPerModifier);
        memcpy(ckeyc->modifierKeyMap, pDev->key->modifierKeyMap,
                (8 * pDev->key->maxKeysPerModifier));

        ckeyc->maxKeysPerModifier = pDev->key->maxKeysPerModifier;
        ckeyc->curKeySyms.minKeyCode = pDev->key->curKeySyms.minKeyCode;
        ckeyc->curKeySyms.maxKeyCode = pDev->key->curKeySyms.maxKeyCode;
        SetKeySymsMap(&ckeyc->curKeySyms, &pDev->key->curKeySyms);

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
        ckeyc->state &= ~(KEYBOARD_MASK);
        ckeyc->state |= (pDev->key->state & KEYBOARD_MASK);
#undef KEYBOARD_MASK
        for (i = 0; i < 8; i++)
            ckeyc->modifierKeyCount[i] = pDev->key->modifierKeyCount[i];

#ifdef XKB
        if (!noXkbExtension && pDev->key->xkbInfo && pDev->key->xkbInfo->desc) {
            if (!XkbCopyKeymap(pDev->key->xkbInfo->desc, ckeyc->xkbInfo->desc,
                               True))
                FatalError("Couldn't pivot keymap from device to core!\n");
        }
#endif

        SendMappingNotify(pDev, MappingKeyboard, ckeyc->curKeySyms.minKeyCode,
                          (ckeyc->curKeySyms.maxKeyCode -
                           ckeyc->curKeySyms.minKeyCode),
                          serverClient);
        inputInfo.keyboard->devPrivates[CoreDevicePrivatesIndex].ptr = pDev;
    }
}


/**
 * Note that pDev was the last function to send a core pointer event.
 * Currently a no-op.
 *
 * Call this just before processInputProc.
 */
_X_EXPORT void
SwitchCorePointer(DeviceIntPtr pDev)
{
    if (inputInfo.pointer->devPrivates[CoreDevicePrivatesIndex].ptr != pDev)
        inputInfo.pointer->devPrivates[CoreDevicePrivatesIndex].ptr = pDev;
}


/**
 * Synthesize a single motion event for the core pointer.
 *
 * Used in cursor functions, e.g. when cursor confinement changes, and we need
 * to shift the pointer to get it inside the new bounds.
 */
void
PostSyntheticMotion(DeviceIntPtr pDev, 
                    int x, 
                    int y, 
                    int screen,
                    unsigned long time) 
{
    xEvent xE;

#ifdef PANORAMIX
    /* Translate back to the sprite screen since processInputProc
       will translate from sprite screen to screen 0 upon reentry
       to the DIX layer. */
    if (!noPanoramiXExtension) {
        x += panoramiXdataPtr[0].x - panoramiXdataPtr[screen].x;
        y += panoramiXdataPtr[0].y - panoramiXdataPtr[screen].y;
    }
#endif

    memset(&xE, 0, sizeof(xEvent));
    xE.u.u.type = MotionNotify;
    xE.u.keyButtonPointer.rootX = x;
    xE.u.keyButtonPointer.rootY = y;
    xE.u.keyButtonPointer.time = time;

    (*pDev->public.processInputProc)(&xE, pDev, 1);
}
