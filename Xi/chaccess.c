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

#include "chaccess.h"

/***********************************************************************
 * This procedure allows a client to change window access control.
 */

int
SProcXChangeWindowAccess(ClientPtr client)
{
    char n;
    REQUEST(xChangeWindowAccessReq);

    swaps(&stuff->length, n);
    swapl(&stuff->win, n);
    return ProcXChangeWindowAccess(client);
}

int
ProcXChangeWindowAccess(ClientPtr client)
{
    int padding, err, i;
    XID* deviceids = NULL;
    WindowPtr win;
    DeviceIntPtr* perm_devices = NULL;
    DeviceIntPtr* deny_devices = NULL;
    REQUEST(xChangeWindowAccessReq);
    REQUEST_AT_LEAST_SIZE(xChangeWindowAccessReq);


    padding = (4 - (((stuff->npermit + stuff->ndeny) * sizeof(XID)) % 4)) % 4;

    if (stuff->length != ((sizeof(xChangeWindowAccessReq)  +
            (((stuff->npermit + stuff->ndeny) * sizeof(XID)) + padding)) >> 2))
    {
        SendErrorToClient(client, IReqCode, X_ChangeWindowAccess,
                0, BadLength);
        return Success;
    }


    err = dixLookupWindow(&win, stuff->win, client, DixWriteAccess);
    if (err != Success)
    {
        SendErrorToClient(client, IReqCode, X_ChangeWindowAccess,
                          stuff->win, err);
        return Success;
    }

    /* Are we clearing? if so, ignore the rest */
    if (stuff->clear)
    {
        err = ACClearWindowAccess(client, win, stuff->clear);
        if (err != Success)
            SendErrorToClient(client, IReqCode, X_ChangeWindowAccess, 0, err);
        return Success;
    }

    if (stuff->npermit || stuff->ndeny)
        deviceids = (XID*)&stuff[1];

    if (stuff->npermit)
    {
        perm_devices =
            (DeviceIntPtr*)xalloc(stuff->npermit * sizeof(DeviceIntPtr));
        if (!perm_devices)
        {
            ErrorF("[Xi] ProcXChangeWindowAccess: alloc failure.\n");
            SendErrorToClient(client, IReqCode, X_ChangeWindowAccess, 0,
                    BadImplementation);
            return Success;
        }

        /* if one of the devices cannot be accessed, we don't do anything.*/
        for (i = 0; i < stuff->npermit; i++)
        {
            perm_devices[i] = LookupDeviceIntRec(deviceids[i]);
            if (!perm_devices[i])
            {
                xfree(perm_devices);
                SendErrorToClient(client, IReqCode, X_ChangeWindowAccess,
                        deviceids[i], BadDevice);
                return Success;
            }
        }
    }

    if (stuff->ndeny)
    {
        deny_devices =
            (DeviceIntPtr*)xalloc(stuff->ndeny * sizeof(DeviceIntPtr));
        if (!deny_devices)
        {
            ErrorF("[Xi] ProcXChangeWindowAccecss: alloc failure.\n");
            SendErrorToClient(client, IReqCode, X_ChangeWindowAccess, 0,
                    BadImplementation);

            xfree(perm_devices);
            return Success;
        }

        for (i = 0; i < stuff->ndeny; i++)
        {
            deny_devices[i] =
                LookupDeviceIntRec(deviceids[i+stuff->npermit]);

            if (!deny_devices[i])
            {
                xfree(perm_devices);
                xfree(deny_devices);
                SendErrorToClient(client, IReqCode, X_ChangeWindowAccess,
                        deviceids[i + stuff->npermit], BadDevice);
                return Success;
            }
        }
    }

    err = ACChangeWindowAccess(client, win, stuff->defaultRule,
                               perm_devices, stuff->npermit,
                               deny_devices, stuff->ndeny);
    if (err != Success)
    {
        SendErrorToClient(client, IReqCode, X_ChangeWindowAccess,
                          stuff->win, err);
        return Success;
    }

    xfree(perm_devices);
    xfree(deny_devices);
    return Success;
}

