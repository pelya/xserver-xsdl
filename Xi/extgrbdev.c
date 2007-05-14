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
#include "gestr.h"
#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exevents.h"
#include "exglobals.h"

#include "extgrbdev.h"

int 
SProcXExtendedGrabDevice(ClientPtr client)
{
    char        n;
    int         i;
    long*       p;

    REQUEST(xExtendedGrabDeviceReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xExtendedGrabDeviceReq);

    swapl(&stuff->grab_window, n);
    swapl(&stuff->time, n);
    swapl(&stuff->confine_to, n);
    swapl(&stuff->cursor, n);
    swaps(&stuff->event_mask, n);
    swaps(&stuff->event_count, n);
    swaps(&stuff->ge_event_count, n);

    p = (long *)&stuff[1];
    for (i = 0; i < stuff->event_count; i++) {
	swapl(p, n);
	p++;
    }

    for (i = 0; i < stuff->ge_event_count; i++) {
        p++; /* first 4 bytes are extension type and padding */
        swapl(p, n);
        p++;
    }

    return ProcXExtendedGrabDevice(client);
}


int 
ProcXExtendedGrabDevice(ClientPtr client)
{
    xExtendedGrabDeviceReply rep;
    DeviceIntPtr             dev;
    int                      err, 
                             errval = 0,
                             i;
    WindowPtr                grab_window, 
                             confineTo = 0;
    CursorPtr                cursor = NULL;
    struct tmask             tmp[EMASKSIZE];
    TimeStamp                time;
    XgeEventMask*            xgeMask;
    GenericMaskPtr           gemasks = NULL;

    REQUEST(xExtendedGrabDeviceReq);
    REQUEST_AT_LEAST_SIZE(xExtendedGrabDeviceReq);

    if (stuff->length != (sizeof(xExtendedGrabDeviceReq) >> 2) + 
            stuff->event_count + 2 * stuff->ge_event_count)
    {
        errval = 0;
        err = BadLength;
        goto cleanup;
    }

    dev = LookupDeviceIntRec(stuff->deviceid);
    if (dev == NULL) {
        errval = stuff->deviceid;
        err = BadDevice;
	goto cleanup;
    }

    err = dixLookupWindow(&grab_window, 
                          stuff->grab_window, 
                          client, 
                          DixReadAccess);
    if (err != Success)
    {
        errval = stuff->grab_window;
        goto cleanup;
    }
    
    if (stuff->confine_to)
    {
        err = dixLookupWindow(&confineTo, 
                              stuff->confine_to, 
                              client, 
                              DixReadAccess);
        if (err != Success)
        {
            errval = stuff->confine_to;
            goto cleanup;
        }
    }

    if (stuff->cursor)
    {
        cursor = (CursorPtr)SecurityLookupIDByType(client, 
                                                    stuff->cursor,
                                                    RT_CURSOR, 
                                                    DixReadAccess); 
        if (!cursor)
        {
            errval = stuff->cursor;
            err = BadCursor;
            goto cleanup;
        }
    }

    rep.repType         = X_Reply;
    rep.RepType         = X_ExtendedGrabDevice;
    rep.sequenceNumber  = client->sequence;
    rep.length          = 0;

    if (CreateMaskFromList(client, 
                           (XEventClass*)&stuff[1],
                           stuff->event_count, 
                           tmp, 
                           dev, 
                           X_GrabDevice) != Success)
        return Success;

    time = ClientTimeToServerTime(stuff->time);

    if (stuff->ge_event_count)
    {
        xgeMask = 
            (XgeEventMask*)(((XEventClass*)&stuff[1]) + stuff->event_count);

        gemasks = xcalloc(1, sizeof(GenericMaskRec));
        gemasks->extension = xgeMask->extension;
        gemasks->mask = xgeMask->evmask;
        gemasks->next = NULL;
        xgeMask++;

        for (i = 1; i < stuff->ge_event_count; i++, xgeMask++)
        {
            gemasks->next = xcalloc(1, sizeof(GenericMaskRec));
            gemasks = gemasks->next;
            gemasks->extension = xgeMask->extension;
            gemasks->mask = xgeMask->evmask;
            gemasks->next = NULL;
        }
    }

    ExtGrabDevice(client, dev, stuff->grabmode, stuff->device_mode, 
                  grab_window, confineTo, time, stuff->owner_events, 
                  cursor, stuff->event_mask, tmp[stuff->deviceid].mask, 
                  gemasks);

    if (err != Success) {
        errval = 0;
        goto cleanup;
    }
    
cleanup:

    while(gemasks)
    {
        GenericMaskPtr prev = gemasks;
        gemasks = gemasks->next;
        xfree(prev);
    }

    if (err == Success)
    {
        WriteReplyToClient(client, sizeof(xGrabDeviceReply), &rep);
    } 
    else 
    {
        SendErrorToClient(client, IReqCode, 
                          X_ExtendedGrabDevice, 
                          errval, err);
    }
    return Success;
}

void
SRepXExtendedGrabDevice(ClientPtr client, int size, 
                        xExtendedGrabDeviceReply* rep)
{
    char n;
    swaps(&rep->sequenceNumber, n);
    swaps(&rep->length, n);
    WriteToClient(client, size, (char*)rep);
}
