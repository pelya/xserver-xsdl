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

int main(int argc, char** argv)
{
    g_test_init(&argc, &argv,NULL);
    g_test_bug_base("https://bugzilla.freedesktop.org/show_bug.cgi?id=");

    g_test_add_func("/dix/input/init-valuators", dix_init_valuators);

    return g_test_run();
}
