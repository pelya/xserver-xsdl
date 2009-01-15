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


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/extensions/XIproto.h>

#include "dixstruct.h"
#include "windowstr.h"

#include "exglobals.h"
#include "xiselev.h"
#include "geext.h"

int
SProcXiSelectEvent(ClientPtr client)
{
    char n;

    REQUEST(xXiSelectEventReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXiSelectEventReq);
    swapl(&stuff->window, n);
    swapl(&stuff->mask, n);
    return (ProcXiSelectEvent(client));
}


int
ProcXiSelectEvent(ClientPtr client)
{
    int rc;
    WindowPtr pWin;
    DeviceIntPtr pDev;
    REQUEST(xXiSelectEventReq);
    REQUEST_SIZE_MATCH(xXiSelectEventReq);

    rc = dixLookupWindow(&pWin, stuff->window, client, DixReceiveAccess);
    if (rc != Success)
        return rc;

    if (stuff->deviceid & (0x1 << 7)) /* all devices */
        pDev = NULL;
    else {
        rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }

    /* XXX: THIS FUNCTION IS NOW DYSFUNCTIONAL */

    return Success;
}

