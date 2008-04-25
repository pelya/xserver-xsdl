/*
 * Copyright 2007-2008 Peter Hutterer
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
 * Author: Peter Hutterer, UniSA, NICTA
 */

/***********************************************************************
 *
 * Request to query the pointer location of an extension input device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"

#ifdef PANORAMIX
#include "panoramiXsrv.h"
#endif

#include "querydp.h"

/***********************************************************************
 *
 * This procedure allows a client to query the pointer of a device.
 *
 */

int
SProcXQueryDevicePointer(ClientPtr client)
{
    char n;

    REQUEST(xQueryDevicePointerReq);
    swaps(&stuff->length, n);
    return (ProcXQueryDevicePointer(client));
}

int
ProcXQueryDevicePointer(ClientPtr client)
{
    int rc;
    xQueryDevicePointerReply rep;
    DeviceIntPtr pDev;
    WindowPtr pWin, t;
    SpritePtr pSprite;

    REQUEST(xQueryDevicePointerReq);
    REQUEST_SIZE_MATCH(xQueryDevicePointerReq);

    rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success)
        return rc;

    if (pDev->valuator == NULL)
    {
        client->errorValue = stuff->deviceid;
        return BadDevice;
    }

    rc = dixLookupWindow(&pWin, stuff->win, client, DixReadAccess);
    if (rc != Success)
    {
        SendErrorToClient(client, IReqCode, X_QueryDevicePointer,
                stuff->win, rc);
        return Success;
    }

    if (pDev->valuator->motionHintWindow)
        MaybeStopHint(pDev, client);

    pSprite = pDev->spriteInfo->sprite;
    rep.repType = X_Reply;
    rep.RepType = X_QueryDevicePointer;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.mask = pDev->button->state | inputInfo.keyboard->key->state;
    rep.root = (GetCurrentRootWindow(pDev))->drawable.id;
    rep.rootX = pSprite->hot.x;
    rep.rootY = pSprite->hot.y;
    rep.child = None;
    rep.deviceid = pDev->id;

    if (pSprite->hot.pScreen == pWin->drawable.pScreen)
    {
        rep.sameScreen = xTrue;
        rep.winX = pSprite->hot.x - pWin->drawable.x;
        rep.winY = pSprite->hot.y - pWin->drawable.y;
        for (t = pSprite->win; t; t = t->parent)
            if (t->parent == pWin)
            {
                rep.child = t->drawable.id;
                break;
            }
    } else
    {
        rep.sameScreen = xFalse;
        rep.winX = 0;
        rep.winY = 0;
    }

#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
        rep.rootX += panoramiXdataPtr[0].x;
        rep.rootY += panoramiXdataPtr[0].y;
        if (stuff->win == rep.root)
        {
            rep.winX += panoramiXdataPtr[0].x;
            rep.winY += panoramiXdataPtr[0].y;
        }
    }
#endif

    WriteReplyToClient(client, sizeof(xQueryDevicePointerReply), &rep);
    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XQueryDevicePointer function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXQueryDevicePointer(ClientPtr client, int size,
        xQueryDevicePointerReply * rep)
{
    char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, (char *)rep);
}

