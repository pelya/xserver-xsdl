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
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

/***********************************************************************
 *
 * Request to set the client pointer for the owner of the given window.
 * All subsequent calls that are ambiguous will choose the client pointer as
 * default value.
 */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include "scrnintstr.h"	/* screen structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"

#include "setcptr.h"

int
SProcXISetClientPointer(ClientPtr client)
{
    char n;

    REQUEST(xXISetClientPointerReq);
    swaps(&stuff->length, n);
    swapl(&stuff->win, n);
    swaps(&stuff->deviceid, n);
    REQUEST_SIZE_MATCH(xXISetClientPointerReq);
    return (ProcXISetClientPointer(client));
}

int
ProcXISetClientPointer(ClientPtr client)
{
    DeviceIntPtr pDev;
    ClientPtr targetClient;
    int rc;

    REQUEST(xXISetClientPointerReq);
    REQUEST_SIZE_MATCH(xXISetClientPointerReq);

    if (stuff->deviceid > 0xFF) /* FIXME */
    {
        client->errorValue = stuff->deviceid;
        return BadImplementation;
    }

    rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixWriteAccess);
    if (rc != Success)
        return rc;

    if (!IsPointerDevice(pDev) || !IsMaster(pDev))
    {
        client->errorValue = stuff->deviceid;
        return BadDevice;
    }

    if (stuff->win != None)
    {
        rc = dixLookupClient(&targetClient, stuff->win, client,
                DixWriteAccess);

        if (rc != Success)
            return rc;

    } else
        targetClient = client;

    if (!SetClientPointer(targetClient, client, pDev))
    {
        client->errorValue = stuff->win;
        return BadAccess;
    }

    return Success;
}
