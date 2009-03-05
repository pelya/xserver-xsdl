/*
 * Copyright © 2009 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file eventconvert.c
 * This file contains event conversion routines from InternalEvent to the
 * matching protocol events.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include <X11/X.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2.h>

#include "dix.h"
#include "inputstr.h"
#include "misc.h"
#include "events.h"
#include "exglobals.h"
#include "eventconvert.h"
#include "listdev.h"

static int countValuators(DeviceEvent *ev, int *first);
static int getValuatorEvents(DeviceEvent *ev, deviceValuator *xv);
static int eventToKeyButtonPointer(DeviceEvent *ev, xEvent **xi, int *count);
static int eventToClassesChanged(DeviceChangedEvent *ev, xEvent **dcce,
                                 int *count);
static int eventToDeviceEvent(DeviceEvent *ev, xEvent **xi);
/**
 * Convert the given event to the respective core event.
 *
 * Return values:
 * Success ... core contains the matching core event.
 * BadValue .. One or more values in the internal event are invalid.
 * BadMatch .. The event has no core equivalent.
 *
 * @param[in] event The event to convert into a core event.
 * @param[in] core The memory location to store the core event at.
 * @return Success or the matching error code.
 */
int
EventToCore(InternalEvent *event, xEvent *core)
{
    switch(event->u.any.type)
    {
        case ET_Motion:
        case ET_ButtonPress:
        case ET_ButtonRelease:
        case ET_KeyPress:
        case ET_KeyRelease:
        case ET_ProximityIn:
        case ET_ProximityOut:
            {
                DeviceEvent *e = (DeviceEvent*)event;

                if (e->detail.key > 0xFF)
                    return BadMatch;

                memset(core, 0, sizeof(xEvent));
                core->u.u.type = e->type - ET_KeyPress + KeyPress;
                core->u.u.detail = e->detail.key & 0xFF;
                core->u.keyButtonPointer.time = e->time;
                core->u.keyButtonPointer.rootX = e->root_x;
                core->u.keyButtonPointer.rootY = e->root_y;
                core->u.keyButtonPointer.state = e->corestate;
            }
            break;
        default:
            /* XXX: */
            ErrorF("[dix] EventToCore: Not implemented yet \n");
            return BadImplementation;
    }
    return Success;
}

/**
 * Convert the given event to the respective XI 1.x event and store it in
 * xi. xi is allocated on demand and must be freed by the caller.
 * count returns the number of events in xi. If count is 1, and the type of
 * xi is GenericEvent, then xi may be larger than 32 bytes.
 *
 * If the event cannot be converted into an XI event because of protocol
 * restrictions, count is 0 and Success is returned.
 *
 * @param[in] ev The event to convert into an XI 1 event.
 * @param[out] xi Future memory location for the XI event.
 * @param[out] count Number of elements in xi.
 */
int
EventToXI(InternalEvent *ev, xEvent **xi, int *count)
{
    switch (ev->u.any.type)
    {
        case ET_Motion:
        case ET_ButtonPress:
        case ET_ButtonRelease:
        case ET_KeyPress:
        case ET_KeyRelease:
        case ET_ProximityIn:
        case ET_ProximityOut:
            return eventToKeyButtonPointer((DeviceEvent*)ev, xi, count);
        case ET_DeviceChanged:
            return eventToClassesChanged((DeviceChangedEvent*)ev, xi, count);
            break;
    }

    ErrorF("[dix] EventToXI: Not implemented for %d \n", ev->u.any.type);
    return BadImplementation;
}

/**
 * Convert the given event to the respective XI 2.x event and store it in xi.
 * xi is allocated on demand and must be freed by the caller.
 *
 * If the event cannot be converted into an XI event because of protocol
 * restrictions, xi is NULL and Success is returned.
 *
 * @param[in] ev The event to convert into an XI2 event
 * @param[out] xi Future memory location for the XI2 event.
 *
 * @return Success or the error code.
 */
int
EventToXI2(InternalEvent *ev, xEvent **xi)
{
    switch (ev->u.any.type)
    {
        case ET_Motion:
        case ET_ButtonPress:
        case ET_ButtonRelease:
        case ET_KeyPress:
        case ET_KeyRelease:
            return eventToDeviceEvent((DeviceEvent*)ev, xi);
        case ET_ProximityIn:
        case ET_ProximityOut:
            *xi = NULL;
            return Success;
    }

    ErrorF("[dix] EventToXI2: Not implemented for %d \n", ev->u.any.type);
    return BadImplementation;
}

static int
eventToKeyButtonPointer(DeviceEvent *ev, xEvent **xi, int *count)
{
    int num_events;
    int first; /* dummy */
    deviceKeyButtonPointer *kbp;

    /* Sorry, XI 1.x protocol restrictions. */
    if (ev->detail.button > 0xFF || ev->deviceid >= 0x80)
    {
        *count = 0;
        return Success;
    }

    num_events = (countValuators(ev, &first) + 5)/6; /* valuator ev */
    num_events++; /* the actual event event */

    *xi = xcalloc(num_events, sizeof(xEvent));
    if (!(*xi))
    {
        return BadAlloc;
    }

    kbp           = (deviceKeyButtonPointer*)(*xi);
    kbp->detail   = ev->detail.button;
    kbp->time     = ev->time;
    kbp->root     = ev->root;
    kbp->root_x   = ev->root_x;
    kbp->root_y   = ev->root_y;
    kbp->deviceid = ev->deviceid;
    kbp->state    = ev->corestate;

    if (num_events > 1)
        kbp->deviceid |= MORE_EVENTS;

    switch(ev->type)
    {
        case ET_Motion:        kbp->type = DeviceMotionNotify;  break;
        case ET_ButtonPress:   kbp->type = DeviceButtonPress;   break;
        case ET_ButtonRelease: kbp->type = DeviceButtonRelease; break;
        case ET_KeyPress:      kbp->type = DeviceKeyPress;      break;
        case ET_KeyRelease:    kbp->type = DeviceKeyRelease;    break;
        case ET_ProximityIn:   kbp->type = ProximityIn;         break;
        case ET_ProximityOut:  kbp->type = ProximityOut;        break;
    }

    if (num_events > 1)
    {
        getValuatorEvents(ev, (deviceValuator*)(kbp + 1));
    }

    *count = num_events;
    return Success;
}


/**
 * Set first to the first valuator in the event ev and return the number of
 * valuators from first to the last set valuator.
 */
static int
countValuators(DeviceEvent *ev, int *first)
{
    int first_valuator = -1, last_valuator = -1, num_valuators = 0;
    int i;

    for (i = 0; i < sizeof(ev->valuators.mask) * 8; i++)
    {
        if (BitIsOn(ev->valuators.mask, i))
        {
            if (first_valuator == -1)
                first_valuator = i;
            last_valuator = i;
        }
    }

    if (first_valuator != -1)
    {
        num_valuators = last_valuator - first_valuator + 1;
        *first = first_valuator;
    }

    return num_valuators;
}

static int
getValuatorEvents(DeviceEvent *ev, deviceValuator *xv)
{
    int i;
    int first_valuator, num_valuators;

    num_valuators = countValuators(ev, &first_valuator);

    /* FIXME: non-continuous valuator data in internal events*/
    for (i = 0; i < num_valuators; i += 6, xv++) {
        xv->type = DeviceValuator;
        xv->first_valuator = first_valuator + i;
        xv->num_valuators = ((num_valuators - i) > 6) ? 6 : (num_valuators - i);
        xv->deviceid = ev->deviceid;
        switch (xv->num_valuators) {
        case 6:
            xv->valuator5 = ev->valuators.data[i + 5];
        case 5:
            xv->valuator4 = ev->valuators.data[i + 4];
        case 4:
            xv->valuator3 = ev->valuators.data[i + 3];
        case 3:
            xv->valuator2 = ev->valuators.data[i + 2];
        case 2:
            xv->valuator1 = ev->valuators.data[i + 1];
        case 1:
            xv->valuator0 = ev->valuators.data[i + 0];
        }

        if (i + 6 < num_valuators)
            xv->deviceid |= MORE_EVENTS;
    }

    return (num_valuators + 5) / 6;
}

static int
eventToClassesChanged(DeviceChangedEvent *ev, xEvent **xi, int *count)
{
    int len = sizeof(xEvent);
    int namelen = 0; /* dummy */
    DeviceIntPtr slave;
    int rc;
    deviceClassesChangedEvent *dcce;


    rc = dixLookupDevice(&slave, ev->new_slaveid,
                         serverClient, DixReadAccess);

    if (rc != Success)
        return rc;

    SizeDeviceInfo(slave, &namelen, &len);

    *xi = xcalloc(1, len);
    if (!(*xi))
        return BadAlloc;

    dcce = (deviceClassesChangedEvent*)(*xi);
    dcce->type = GenericEvent;
    dcce->extension = IReqCode;
    dcce->evtype = XI_DeviceClassesChangedNotify;
    dcce->time = GetTimeInMillis();
    dcce->new_slave = slave->id;
    dcce->length = (len - sizeof(xEvent))/4;

    *count = 1;
    return Success;
}

static int count_bits(unsigned char* ptr, int len)
{
    int bits = 0;
    unsigned int i;
    unsigned char x;

    for (i = 0; i < len; i++)
    {
        x = ptr[i];
        while(x > 0)
        {
            bits += (x & 0x1);
            x >>= 1;
        }
    }
    return bits;
}

static int
eventToDeviceEvent(DeviceEvent *ev, xEvent **xi)
{
    int len = sizeof(xXIDeviceEvent);
    xXIDeviceEvent *xde;
    int i, btlen, vallen;
    char *ptr;
    int32_t *axisval;


    /* FIXME: this should just send the buttons we have, not MAX_BUTTONs. Same
     * with MAX_VALUATORS below */
    /* btlen is in 4 byte units */
    btlen = (((MAX_BUTTONS + 7)/8) + 3)/4;
    len += btlen * 4; /* buttonmask len */


    vallen = count_bits(ev->valuators.mask, sizeof(ev->valuators.mask)/sizeof(ev->valuators.mask[0]));
    len += vallen * 2 * sizeof(uint32_t); /* axisvalues */
    vallen = (((MAX_VALUATORS + 7)/8) + 3)/4;
    len += vallen * 4; /* valuators mask */

    *xi = xcalloc(1, len);
    xde = (xXIDeviceEvent*)*xi;
    xde->type           = GenericEvent;
    xde->extension      = IReqCode;
    xde->evtype         = GetXI2Type((InternalEvent*)ev);
    xde->time           = ev->time;
    xde->length         = (len - sizeof(xEvent) + 3)/4;
    xde->detail         = ev->detail.button;
    xde->root           = ev->root;
    xde->buttons_len    = btlen;
    xde->valuators_len  = vallen;
    xde->deviceid       = ev->deviceid;
    xde->sourceid       = ev->sourceid;

    xde->mods.base_mods         = ev->mods.base;
    xde->mods.latched_mods      = ev->mods.latched;
    xde->mods.locked_mods       = ev->mods.locked;

    xde->group.base_group       = ev->group.base;
    xde->group.latched_group    = ev->group.latched;
    xde->group.locked_group     = ev->group.locked;

    ptr = (char*)&xde[1];
    for (i = 0; i < sizeof(ev->buttons) * 8; i++)
    {
        if (BitIsOn(ev->buttons, i))
            SetBit(ptr, i);
    }

    ptr += xde->buttons_len * 4;
    axisval = (int32_t*)(ptr + xde->valuators_len * 4);
    for (i = 0; i < sizeof(ev->valuators.mask) * 8; i++)
    {
        if (BitIsOn(ev->valuators.mask, i))
        {
            SetBit(ptr, i);
            *axisval = ev->valuators.data[i];
            axisval++;
            axisval++; /* FIXME: this should be the frac. part */
        }
    }

    return Success;
}

/**
 * Return the corresponding core type for the given event or 0 if no core
 * equivalent exists.
 */
int
GetCoreType(InternalEvent *event)
{
    int coretype = 0;
    switch(event->u.any.type)
    {
        case ET_Motion:         coretype = MotionNotify;  break;
        case ET_ButtonPress:    coretype = ButtonPress;   break;
        case ET_ButtonRelease:  coretype = ButtonRelease; break;
        case ET_KeyPress:       coretype = KeyPress;      break;
        case ET_KeyRelease:     coretype = KeyRelease;    break;
    }
    return coretype;
}

/**
 * Return the corresponding XI 1.x type for the given event or 0 if no
 * equivalent exists.
 */
int
GetXIType(InternalEvent *event)
{
    int xitype = 0;
    switch(event->u.any.type)
    {
        case ET_Motion:         xitype = DeviceMotionNotify;  break;
        case ET_ButtonPress:    xitype = DeviceButtonPress;   break;
        case ET_ButtonRelease:  xitype = DeviceButtonRelease; break;
        case ET_KeyPress:       xitype = DeviceKeyPress;      break;
        case ET_KeyRelease:     xitype = DeviceKeyRelease;    break;
        case ET_ProximityIn:    xitype = ProximityIn;         break;
        case ET_ProximityOut:   xitype = ProximityOut;        break;
    }
    return xitype;
}

/**
 * Return the corresponding XI 2.x type for the given event or 0 if no
 * equivalent exists.
 */
int
GetXI2Type(InternalEvent *event)
{
    int xi2type = 0;

    switch(event->u.any.type)
    {
        case ET_Motion:         xi2type = XI_Motion;           break;
        case ET_ButtonPress:    xi2type = XI_ButtonPress;      break;
        case ET_ButtonRelease:  xi2type = XI_ButtonRelease;    break;
        case ET_KeyPress:       xi2type = XI_KeyPress;         break;
        case ET_KeyRelease:     xi2type = XI_KeyRelease;       break;
        case ET_Enter:          xi2type = XI_Enter;            break;
        case ET_Leave:          xi2type = XI_Leave;            break;
        case ET_Hierarchy:      xi2type = XI_HierarchyChanged; break;
        case ET_DeviceChanged:  xi2type = XI_DeviceChanged;    break;
        default:
            break;
    }
    return xi2type;
}
