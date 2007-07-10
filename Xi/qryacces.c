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
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exevents.h"
#include "exglobals.h"

#include "qryacces.h"

/***********************************************************************
 * This procedure allows a client to query window access control.
 */

int
SProcXQueryWindowAccess(ClientPtr client)
{
    char n;
    REQUEST(xQueryWindowAccessReq);

    swaps(&stuff->length, n);
    swapl(&stuff->win, n);
    return ProcXQueryWindowAccess(client);
}

int
ProcXQueryWindowAccess(ClientPtr client)
{
    int err;
    WindowPtr win;
    DeviceIntPtr *perm, *deny;
    int nperm, ndeny, i;
    int defaultRule;
    XID* deviceids;
    xQueryWindowAccessReply rep;

    REQUEST(xQueryWindowAccessReq);
    REQUEST_SIZE_MATCH(xQueryWindowAccessReq);

    err = dixLookupWindow(&win, stuff->win, client, DixReadAccess);
    if (err != Success)
    {
        SendErrorToClient(client, IReqCode, X_QueryWindowAccess, 
                          stuff->win, err); 
        return Success;
    }

    ACQueryWindowAccess(win, &defaultRule, &perm, &nperm, &deny, &ndeny);

    rep.repType = X_Reply;
    rep.RepType = X_QueryWindowAccess;
    rep.sequenceNumber = client->sequence;
    rep.length = ((nperm + ndeny) * sizeof(XID) + 3) >> 2;
    rep.defaultRule = defaultRule;
    rep.npermit = nperm;
    rep.ndeny = ndeny;
    WriteReplyToClient(client, sizeof(xQueryWindowAccessReply), &rep);

    if (nperm + ndeny)
    {
        deviceids = (XID*)xalloc((nperm + ndeny) * sizeof(XID));
        if (!deviceids)
        {
            ErrorF("ProcXQueryWindowAccess: xalloc failure.\n");
            SendErrorToClient(client, IReqCode, X_QueryWindowAccess, 
                    0, BadImplementation); 
            return Success;
        }

        for (i = 0; i < nperm; i++)
            deviceids[i] = perm[i]->id;
        for (i = 0; i < ndeny; i++)
            deviceids[i + nperm] = deny[i]->id;

        WriteToClient(client, (nperm + ndeny) * sizeof(XID), (char*)deviceids);
        xfree(deviceids);
    }
    return Success;
}

void
SRepXQueryWindowAccess(ClientPtr client, 
                      int size, 
                      xQueryWindowAccessReply* rep)
{
    char n;
    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, (char*)rep);
}
