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
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exevents.h"
#include "exglobals.h"

#ifdef PANORAMIX
#include "panoramiXsrv.h"
#endif

#include "getpairp.h"

/***********************************************************************
 *
 * This procedure allows a client to query the paired pointer for a keyboard
 * device.
 *
 */

int
SProcXGetPairedPointer(ClientPtr client)
{
    char n;
    REQUEST(xGetPairedPointerReq);
    swaps(&stuff->length, n);
    return (ProcXGetPairedPointer(client));
}

int 
ProcXGetPairedPointer(ClientPtr client)
{
    xGetPairedPointerReply rep;
    DeviceIntPtr kbd, ptr;

    REQUEST(xGetPairedPointerReq);
    REQUEST_SIZE_MATCH(xGetPairedPointerReq);

    kbd = LookupDeviceIntRec(stuff->deviceid);
    if (!kbd || !kbd->key) {
        SendErrorToClient(client, IReqCode, X_GetPairedPointer,
                stuff->deviceid, BadDevice);
        return Success;
    }

    ptr = GetPairedPointer(kbd);

    rep.repType = X_Reply;
    rep.RepType = X_GetPairedPointer;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    if (ptr == inputInfo.pointer)
    {
        rep.paired = FALSE;
        rep.deviceid = 0;
    } else 
    {
        rep.paired = TRUE;
        rep.deviceid = ptr->id;
    }
    WriteReplyToClient(client, sizeof(xGetPairedPointerReply), &rep);
    return Success;
}

void
SRepXGetPairedPointer(ClientPtr client, int size,
        xGetPairedPointerReply* rep)
{
    char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, (char *)rep);
}
