/**
 * Copyright © 2009 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "misc.h"
#include "resource.h"
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include "windowstr.h"
#include "inputstr.h"
#include "eventconvert.h"
#include "exevents.h"

#include <glib.h>

/**
 * Init a device with axes.
 * Verify values set on the device.
 *
 * Result: All axes set to default values (usually 0).
 */
static void dix_init_valuators(void)
{
    DeviceIntRec dev;
    ValuatorClassPtr val;
    const int num_axes = 2;
    int i;


    memset(&dev, 0, sizeof(DeviceIntRec));
    dev.isMaster = TRUE; /* claim it's a master to stop ptracccel */

    g_assert(InitValuatorClassDeviceStruct(NULL, 0, 0, 0) == FALSE);
    g_assert(InitValuatorClassDeviceStruct(&dev, num_axes, 0, Absolute));

    val = dev.valuator;
    g_assert(val);
    g_assert(val->numAxes == num_axes);
    g_assert(val->numMotionEvents == 0);
    g_assert(val->mode == Absolute);
    g_assert(val->axisVal);

    for (i = 0; i < num_axes; i++)
    {
        g_assert(val->axisVal[i] == 0);
        g_assert(val->axes->min_value == NO_AXIS_LIMITS);
        g_assert(val->axes->max_value == NO_AXIS_LIMITS);
    }

    g_assert(dev.last.numValuators == num_axes);
}

/* just check the known success cases, and that error cases set the client's
 * error value correctly. */
static void dix_check_grab_values(void)
{
    ClientRec client;
    GrabParameters param;
    int rc;

    memset(&client, 0, sizeof(client));

    param.this_device_mode = GrabModeSync;
    param.other_devices_mode = GrabModeSync;
    param.modifiers = AnyModifier;
    param.ownerEvents = FALSE;

    rc = CheckGrabValues(&client, &param);
    g_assert(rc == Success);

    param.this_device_mode = GrabModeAsync;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == Success);

    param.this_device_mode = GrabModeAsync + 1;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == BadValue);
    g_assert(client.errorValue == param.this_device_mode);
    g_assert(client.errorValue == GrabModeAsync + 1);

    param.this_device_mode = GrabModeSync;
    param.other_devices_mode = GrabModeAsync;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == Success);

    param.other_devices_mode = GrabModeAsync + 1;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == BadValue);
    g_assert(client.errorValue == param.other_devices_mode);
    g_assert(client.errorValue == GrabModeAsync + 1);

    param.other_devices_mode = GrabModeSync;

    param.modifiers = 1 << 13;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == BadValue);
    g_assert(client.errorValue == param.modifiers);
    g_assert(client.errorValue == (1 << 13));


    param.modifiers = AnyModifier;
    param.ownerEvents = TRUE;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == Success);

    param.ownerEvents = 3;
    rc = CheckGrabValues(&client, &param);
    g_assert(rc == BadValue);
    g_assert(client.errorValue == param.ownerEvents);
    g_assert(client.errorValue == 3);
}


/**
 * Convert various internal events to the matching core event and verify the
 * parameters.
 */
static void dix_event_to_core(int type)
{
    DeviceEvent ev;
    xEvent core;
    int time;
    int x, y;
    int rc;
    int state;
    int detail;

    /* EventToCore memsets the event to 0 */
#define test_event() \
    g_assert(rc == Success); \
    g_assert(core.u.u.type == type); \
    g_assert(core.u.u.detail == detail); \
    g_assert(core.u.keyButtonPointer.time == time); \
    g_assert(core.u.keyButtonPointer.rootX == x); \
    g_assert(core.u.keyButtonPointer.rootY == y); \
    g_assert(core.u.keyButtonPointer.state == state); \
    g_assert(core.u.keyButtonPointer.eventX == 0); \
    g_assert(core.u.keyButtonPointer.eventY == 0); \
    g_assert(core.u.keyButtonPointer.root == 0); \
    g_assert(core.u.keyButtonPointer.event == 0); \
    g_assert(core.u.keyButtonPointer.child == 0); \
    g_assert(core.u.keyButtonPointer.sameScreen == FALSE);

    x = 0;
    y = 0;
    time = 12345;
    state = 0;
    detail = 0;

    ev.header   = 0xFF;
    ev.length   = sizeof(DeviceEvent);
    ev.time     = time;
    ev.root_y   = x;
    ev.root_x   = y;
    ev.corestate = state;
    ev.detail.key = detail;

    ev.type = type;
    ev.detail.key = 0;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    x = 1;
    y = 2;
    ev.root_x = x;
    ev.root_y = y;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    x = 0x7FFF;
    y = 0x7FFF;
    ev.root_x = x;
    ev.root_y = y;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    x = 0x8000; /* too high */
    y = 0x8000; /* too high */
    ev.root_x = x;
    ev.root_y = y;
    rc = EventToCore((InternalEvent*)&ev, &core);
    g_assert(core.u.keyButtonPointer.rootX != x);
    g_assert(core.u.keyButtonPointer.rootY != y);

    x = 0x7FFF;
    y = 0x7FFF;
    ev.root_x = x;
    ev.root_y = y;
    time = 0;
    ev.time = time;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    detail = 1;
    ev.detail.key = detail;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    detail = 0xFF; /* highest value */
    ev.detail.key = detail;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    detail = 0xFFF; /* too big */
    ev.detail.key = detail;
    rc = EventToCore((InternalEvent*)&ev, &core);
    g_assert(rc == BadMatch);

    detail = 0xFF; /* too big */
    ev.detail.key = detail;
    state = 0xFFFF; /* highest value */
    ev.corestate = state;
    rc = EventToCore((InternalEvent*)&ev, &core);
    test_event();

    state = 0x10000; /* too big */
    ev.corestate = state;
    rc = EventToCore((InternalEvent*)&ev, &core);
    g_assert(core.u.keyButtonPointer.state != state);
    g_assert(core.u.keyButtonPointer.state == (state & 0xFFFF));

#undef test_event
}

static void dix_event_to_core_conversion(void)
{
    DeviceEvent ev;
    xEvent core;
    int rc;

    ev.header   = 0xFF;
    ev.length   = sizeof(DeviceEvent);

    ev.type     = 0;
    rc = EventToCore((InternalEvent*)&ev, &core);
    g_assert(rc == BadImplementation);

    ev.type     = 1;
    rc = EventToCore((InternalEvent*)&ev, &core);
    g_assert(rc == BadImplementation);

    ev.type     = ET_ProximityOut + 1;
    rc = EventToCore((InternalEvent*)&ev, &core);
    g_assert(rc == BadImplementation);

    dix_event_to_core(ET_KeyPress);
    dix_event_to_core(ET_KeyRelease);
    dix_event_to_core(ET_ButtonPress);
    dix_event_to_core(ET_ButtonRelease);
    dix_event_to_core(ET_Motion);
    dix_event_to_core(ET_ProximityIn);
    dix_event_to_core(ET_ProximityOut);
}

int main(int argc, char** argv)
{
    g_test_init(&argc, &argv,NULL);
    g_test_bug_base("https://bugzilla.freedesktop.org/show_bug.cgi?id=");

    g_test_add_func("/dix/input/init-valuators", dix_init_valuators);
    g_test_add_func("/dix/input/event-core-conversion", dix_event_to_core_conversion);
    g_test_add_func("/dix/input/check-grab-values", dix_check_grab_values);

    return g_test_run();
}
