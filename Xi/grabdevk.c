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

/***********************************************************************
 *
 * Extension function to grab a key on an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exglobals.h"

#include "grabdev.h"
#include "grabdevk.h"

/***********************************************************************
 *
 * Handle requests from clients with a different byte order.
 *
 */

int
SProcXGrabDeviceKey(ClientPtr client)
{
    char n;
    long *p;
    int i;

    REQUEST(xGrabDeviceKeyReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceKeyReq);
    swapl(&stuff->grabWindow, n);
    swaps(&stuff->modifiers, n);
    swaps(&stuff->event_count, n);
    p = (long *)&stuff[1];
    for (i = 0; i < stuff->event_count; i++) {
	swapl(p, n);
	p++;
    }
    return (ProcXGrabDeviceKey(client));
}

/***********************************************************************
 *
 * Grab a key on an extension device.
 *
 */

int
ProcXGrabDeviceKey(ClientPtr client)
{
    int ret;
    DeviceIntPtr dev;
    DeviceIntPtr mdev;
    XEventClass *class;
    struct tmask tmp[EMASKSIZE];

    REQUEST(xGrabDeviceKeyReq);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceKeyReq);

    if (stuff->length != (sizeof(xGrabDeviceKeyReq) >> 2) + stuff->event_count)
	return BadLength;

    dev = LookupDeviceIntRec(stuff->grabbed_device);
    if (dev == NULL)
	return BadDevice;

    if (stuff->modifier_device != UseXKeyboard) {
	mdev = LookupDeviceIntRec(stuff->modifier_device);
	if (mdev == NULL)
	    return BadDevice;
	if (mdev->key == NULL)
	    return BadMatch;
    } else
	mdev = PickKeyboard(client);

    class = (XEventClass *) (&stuff[1]);	/* first word of values */

    if ((ret = CreateMaskFromList(client, class,
				  stuff->event_count, tmp, dev,
				  X_GrabDeviceKey)) != Success)
	return ret;

    ret = GrabKey(client, dev, stuff->this_device_mode,
		  stuff->other_devices_mode, stuff->modifiers, mdev,
		  stuff->key, stuff->grabWindow, stuff->ownerEvents,
		  tmp[stuff->grabbed_device].mask);

    return ret;
}
