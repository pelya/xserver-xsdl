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
 * Author: Peter Hutterer
 */

/***********************************************************************
 *
 * Request to grab or ungrab input device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include <X11/extensions/XI2.h>
#include <X11/extensions/XI2proto.h>

#include "exglobals.h" /* BadDevice */
#include "xigrabdev.h"

int
SProcXIGrabDevice(ClientPtr client)
{
    char n;

    REQUEST(xXIGrabDeviceReq);

    swaps(&stuff->length, n);
    swaps(&stuff->deviceid, n);
    swapl(&stuff->grab_window, n);
    swapl(&stuff->time, n);
    swaps(&stuff->mask_len, n);

    return ProcXIGrabDevice(client);
}

int
ProcXIGrabDevice(ClientPtr client)
{
    DeviceIntPtr dev;
    xXIGrabDeviceReply rep;
    int ret = Success;
    int status;

    REQUEST(xXIGrabDeviceReq);
    REQUEST_AT_LEAST_SIZE(xXIGetDeviceFocusReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixSetFocusAccess);
    if (ret != Success)
	return ret;

    if (!dev->isMaster)
        stuff->paired_device_mode = GrabModeAsync;

    ret = GrabDevice(client, dev, stuff->grab_mode,
                     stuff->paired_device_mode,
                     stuff->grab_window,
                     stuff->owner_events,
                     stuff->time,
                     0 /* mask */,
                     GRABTYPE_XI2,
                     None /* cursor */,
                     None /* confineTo */,
                     &status);

    if (ret != Success)
        return ret;

    rep.repType = X_Reply;
    rep.RepType = X_XIGrabDevice;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.status = status;


    WriteReplyToClient(client, sizeof(rep), &rep);
    return ret;
}

