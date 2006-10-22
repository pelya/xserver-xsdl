/*
 * Copyright © 2006 Nokia Corporation
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/keysym.h>
#include "misc.h"
#include "resource.h"
#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "cursorstr.h"

#include "dixstruct.h"
#include "globals.h"

#include "mipointer.h"

#ifdef XKB
#include <X11/extensions/XKBproto.h>
#include <X11/extensions/XKBsrv.h>
extern Bool XkbCopyKeymap(XkbDescPtr src, XkbDescPtr dst, Bool sendNotifies);
#endif

#ifdef XACE
#include "xace.h"
#endif

#include <X11/extensions/XIproto.h>
#include "exglobals.h"
#include "exevents.h"
#include "exglobals.h"
#include "extnsionst.h"

/* Maximum number of valuators, divided by six, rounded up. */
#define MAX_VALUATOR_EVENTS 6

/**
 * Returns the maximum number of events GetKeyboardEvents,
 * GetKeyboardValuatorEvents, and GetPointerEvents will ever return.
 *
 * Should be used in DIX as:
 * xEvent *events = xcalloc(sizeof(xEvent), GetMaximumEventsNum());
 */
_X_EXPORT int
GetMaximumEventsNum() {
    /* Two base events -- core and device, plus valuator events.  Multiply
     * by two if we're doing key repeats. */
    int ret = 2 + MAX_VALUATOR_EVENTS;

#ifdef XKB
    if (noXkbExtension)
#endif
        ret *= 2;

    return ret;
}

/**
 * Convenience wrapper around GetKeyboardValuatorEvents, that takes no
 * valuators.
 */
_X_EXPORT int
GetKeyboardEvents(xEvent *events, DeviceIntPtr pDev, int type, int key_code) {
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
 * mieqProcessInputEvents.  If replacing function, take care to call
 * SetCoreKeyboard before processInputProc, so keymaps are altered to suit.
 *
 * Note that this function recurses!  If called for non-XKB, a repeating
 * key press will trigger a matching KeyRelease, as well as the
 * KeyPresses.
 */
_X_EXPORT int
GetKeyboardValuatorEvents(xEvent *events, DeviceIntPtr pDev, int type,
                          int key_code, int first_valuator,
                          int num_valuators, int *valuators) {
    int numEvents = 0, ms = 0, i = 0;
    int final_valuator = first_valuator + num_valuators;
    KeySym sym = pDev->key->curKeySyms.map[key_code *
                                           pDev->key->curKeySyms.mapWidth];
    deviceKeyButtonPointer *kbp = NULL;
    deviceValuator *xv = NULL;

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
     * FIXME: In theory, if you're repeating with two keyboards,
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

    kbp = (deviceKeyButtonPointer *) events;
    kbp->time = ms;
    kbp->deviceid = pDev->id;
    if (type == KeyPress)
        kbp->type = DeviceKeyPress;
    else if (type == KeyRelease)
        kbp->type = DeviceKeyRelease;

    if (num_valuators) {
        kbp->deviceid |= MORE_EVENTS;
        for (i = first_valuator; i < final_valuator; i += 6) {
            xv = (deviceValuator *) ++events;
            xv->type = DeviceValuator;
            xv->first_valuator = i;
            xv->num_valuators = num_valuators;
            xv->deviceid = kbp->deviceid;
            switch (num_valuators - first_valuator) {
            case 6:
                xv->valuator5 = valuators[i+5];
            case 5:
                xv->valuator4 = valuators[i+4];
            case 4:
                xv->valuator3 = valuators[i+3];
            case 3:
                xv->valuator2 = valuators[i+2];
            case 2:
                xv->valuator1 = valuators[i+1];
            case 1:
                xv->valuator0 = valuators[i];
            }
        }
    }

    if (pDev->coreEvents) {
        events++;
        events->u.keyButtonPointer.time = ms;
        events->u.u.type = type;
        events->u.u.detail = key_code;
    }

    return numEvents;
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
 * Generate a series of xEvents (returned in xE) representing pointer
 * motion, or button presses.  Xi and XKB-aware.
 *
 * events is not NULL-terminated; the return value is the number of events.
 * The DDX is responsible for allocating the event structure in the first
 * place via GetMaximumEventsNum(), and for freeing it.
 */
_X_EXPORT int
GetPointerEvents(xEvent *events, DeviceIntPtr pDev, int type, int buttons,
                 int flags, int first_valuator, int num_valuators,
                 int *valuators) {
    int num_events = 0, ms = 0, final_valuator = 0, i = 0;
    deviceKeyButtonPointer *kbp = NULL;
    deviceValuator *xv = NULL;
    AxisInfoPtr axes = NULL;
    Bool sendValuators = (type == MotionNotify || flags & POINTER_ABSOLUTE);
    DeviceIntPtr cp = inputInfo.pointer;
    int x = 0, y = 0;

    if (type != MotionNotify && type != ButtonPress && type != ButtonRelease)
        return 0;

    if (!pDev->button || (pDev->coreEvents && (!cp->button || !cp->valuator)))
        return 0;

    /* You fail. */
    if (first_valuator < 0)
        return 0;

    if (pDev->coreEvents)
        num_events = 2;
    else
        num_events = 1;

    /* Do we need to send a DeviceValuator event? */
    if ((num_valuators + first_valuator) >= 2 && sendValuators) {
        if (((num_valuators / 6) + 1) > MAX_VALUATOR_EVENTS)
            num_valuators = MAX_VALUATOR_EVENTS;
        num_events += (num_valuators / 6) + 1;
    }
    else if (type == MotionNotify && num_valuators <= 0) {
        return 0;
    }

    final_valuator = num_valuators + first_valuator;

    ms = GetTimeInMillis();

    kbp = (deviceKeyButtonPointer *) events;
    kbp->time = ms;
    kbp->deviceid = pDev->id;

    if (flags & POINTER_ABSOLUTE) {
        if (num_valuators >= 1 && first_valuator == 0) {
            x = valuators[0];
        }
        else {
            if (pDev->coreEvents)
                x = cp->valuator->lastx;
            else
                x = pDev->valuator->lastx;
        }

        if (first_valuator <= 1 && num_valuators >= (2 - first_valuator)) {
            y = valuators[1 - first_valuator];
        }
        else {
            if (pDev->coreEvents)
                x = cp->valuator->lasty;
            else
                y = pDev->valuator->lasty;
        }
    }
    else {
        if (flags & POINTER_ACCELERATE)
            acceleratePointer(pDev, first_valuator, num_valuators,
                              valuators);

        if (pDev->coreEvents) {
            if (first_valuator == 0 && num_valuators >= 1)
                x = cp->valuator->lastx + valuators[0];
            else
                x = cp->valuator->lastx;

            if (first_valuator <= 1 && num_valuators >= (2 - first_valuator))
                y = cp->valuator->lasty + valuators[1 - first_valuator];
            else
                y = cp->valuator->lasty;
        }
        else {
            if (first_valuator == 0 && num_valuators >= 1)
                x = pDev->valuator->lastx + valuators[0];
            else
                x = pDev->valuator->lastx;

            if (first_valuator <= 1 && num_valuators >= (2 - first_valuator))
                y = pDev->valuator->lasty + valuators[1 - first_valuator];
            else
                y = pDev->valuator->lasty;
        }
    }


    axes = pDev->valuator->axes;
    if (x < axes->min_value)
        x = axes->min_value;
    if (axes->max_value > 0 && x > axes->max_value)
        x = axes->max_value;

    axes++;
    if (y < axes->min_value)
        y = axes->min_value;
    if (axes->max_value > 0 && y > axes->max_value)
        y = axes->max_value;

    /* This takes care of crossing screens for us, as well as clipping
     * to the current screen.  Right now, we only have one history buffer,
     * so we don't set this for both the device and core.*/
    miPointerSetPosition(pDev, &x, &y, ms);

    if (pDev->coreEvents) {
        cp->valuator->lastx = x;
        cp->valuator->lasty = y;
    }
    pDev->valuator->lastx = x;
    pDev->valuator->lasty = y;

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

    if (final_valuator > 2 && sendValuators) {
        kbp->deviceid |= MORE_EVENTS;
        for (i = first_valuator; i < final_valuator; i += 6) {
            xv = (deviceValuator *) ++events;
            xv->type = DeviceValuator;
            xv->first_valuator = i;
            xv->num_valuators = num_valuators;
            xv->deviceid = kbp->deviceid;
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
                /* x and y may have been accelerated. */
                if (i == 0)
                    xv->valuator1 = kbp->root_y;
                else
                    xv->valuator1 = valuators[i + 1];
            case 1:
                /* x and y may have been accelerated. */
                if (i == 0)
                    xv->valuator0 = kbp->root_x;
                else
                    xv->valuator0 = valuators[i];
            }
        }
    }

    if (pDev->coreEvents) {
        events++;
        events->u.u.type = type;
        events->u.keyButtonPointer.time = ms;
        events->u.keyButtonPointer.rootX = x;
        events->u.keyButtonPointer.rootY = y;

        if (type == ButtonPress || type == ButtonRelease) {
            /* We hijack SetPointerMapping to work on all core-sending
             * devices, so we use the device-specific map here instead of
             * the core one. */
            events->u.u.detail = pDev->button->map[buttons];
        }
        else {
            events->u.u.detail = 0;
        }
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

#ifdef XKB
        if (!noXkbExtension && pDev->key->xkbInfo && pDev->key->xkbInfo->desc) {
            if (!XkbCopyKeymap(pDev->key->xkbInfo->desc, ckeyc->xkbInfo->desc,
                               True))
                FatalError("Couldn't pivot keymap from device to core!\n");
        }
#endif

        SendMappingNotify(MappingKeyboard, ckeyc->curKeySyms.minKeyCode,
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
