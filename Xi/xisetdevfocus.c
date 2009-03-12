/*
 * Copyright 2008 Red Hat, Inc.
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
 * Request to set and get an input device's focus.
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
#include "xisetdevfocus.h"

int
SProcXISetDeviceFocus(ClientPtr client)
{
    char n;

    REQUEST(xXISetDeviceFocusReq);
    swaps(&stuff->length, n);
    swaps(&stuff->deviceid, n);
    swapl(&stuff->focus, n);
    swapl(&stuff->time, n);

    return ProcXISetDeviceFocus(client);
}

int
SProcXIGetDeviceFocus(ClientPtr client)
{
    char n;

    REQUEST(xXIGetDeviceFocusReq);
    swaps(&stuff->length, n);
    swaps(&stuff->deviceid, n);

    return ProcXIGetDeviceFocus(client);
}

int
ProcXISetDeviceFocus(ClientPtr client)
{
    DeviceIntPtr dev;
    int ret;

    REQUEST(xXISetDeviceFocusReq);
    REQUEST_AT_LEAST_SIZE(xXISetDeviceFocusReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixSetFocusAccess);
    if (ret != Success)
	return ret;
    if (!dev->focus)
	return BadDevice;

    return SetInputFocus(client, dev, stuff->focus, RevertToParent,
                        stuff->time, TRUE);
}

int
ProcXIGetDeviceFocus(ClientPtr client)
{
    xXIGetDeviceFocusReply rep;
    DeviceIntPtr dev;
    int ret;

    REQUEST(xXIGetDeviceFocusReq);
    REQUEST_AT_LEAST_SIZE(xXIGetDeviceFocusReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixSetFocusAccess);
    if (ret != Success)
	return ret;
    if (!dev->focus)
	return BadDevice;

    rep.repType = X_Reply;
    rep.RepType = X_XIGetDeviceFocus;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (dev->focus->win == NoneWin)
	rep.focus = None;
    else if (dev->focus->win == PointerRootWin)
	rep.focus = PointerRoot;
    else if (dev->focus->win == FollowKeyboardWin)
	rep.focus = FollowKeyboard;
    else
	rep.focus = dev->focus->win->drawable.id;

    WriteReplyToClient(client, sizeof(xXIGetDeviceFocusReply), &rep);
    return Success;
}
