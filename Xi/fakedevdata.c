/*

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the author.

*/

/***********************************************************************
 *
 * Request to fake data for a given device.
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
#include "scrnintstr.h"	/* screen structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"
#include "mi.h"

#include "fakedevdata.h"

static EventListPtr fake_events = NULL;

int
SProcXFakeDeviceData(ClientPtr client)
{
    char n;
    int i;
    ValuatorData* p;

    REQUEST(xFakeDeviceDataReq);

    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xFakeDeviceDataReq);

    p = (ValuatorData*)&stuff[1];
    for (i = 0; i <  stuff->num_valuators; i++, p++)
        swapl(p, n);

    return ProcXFakeDeviceData(client);;
}

int
ProcXFakeDeviceData(ClientPtr client)
{
    DeviceIntPtr dev;
    int nevents, i;
    int* valuators = NULL;
    int rc;

    REQUEST(xFakeDeviceDataReq);
    REQUEST_AT_LEAST_SIZE(xFakeDeviceDataReq);

    if (stuff->length != (sizeof(xFakeDeviceDataReq) >> 2) + stuff->num_valuators)
    {
        SendErrorToClient(client, IReqCode, X_FakeDeviceData, 0, BadLength);
        return Success;
    }

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixWriteAccess);
    if (rc != Success)
        return rc;

    if (!fake_events && !(fake_events = InitEventList(GetMaximumEventsNum())))
    {
        SendErrorToClient(client, IReqCode, X_FakeDeviceData, 0, BadAlloc);
        return Success;
    }
    if (stuff->num_valuators)
    {
        CARD32* valptr = (CARD32*)&stuff[1];

        valuators = xcalloc(stuff->num_valuators, sizeof(int));
        if (!valuators)
        {
            SendErrorToClient(client, IReqCode, X_FakeDeviceData, 0, BadAlloc);
            return Success;
        }
        for (i = 0; i < stuff->num_valuators; i++, valptr++)
            valuators[i] = (int)(*valptr);
    }

    nevents = GetPointerEvents(fake_events, dev, stuff->type, stuff->buttons,
            POINTER_RELATIVE, stuff->first_valuator, stuff->num_valuators,
            valuators);

    OsBlockSignals();
    for (i = 0; i < nevents; i++)
        mieqEnqueue(dev, (fake_events+ i)->event);
    OsReleaseSignals();
    xfree(valuators);
    return Success;
}
