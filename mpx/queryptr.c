/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "windowstr.h"
#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>

#include "mpxglobals.h"

#include "queryptr.h"

/***********************************************************************
 *
 * This procedure writes the reply for the MPXQueryPointer function.
 */
int
ProcMPXQueryPointer(register ClientPtr client)
{
    xMPXQueryPointerReply rep;
    DeviceIntPtr pDev;
    WindowPtr root, win;
    int x, y;

    REQUEST(xMPXQueryPointerReq);
    REQUEST_SIZE_MATCH(xMPXQueryPointerReq);

    pDev = LookupDeviceIntRec(stuff->deviceid);
    if (!pDev->isMPDev)
    {
        SendErrorToClient(client, MPXReqCode, X_MPXQueryPointer,
                stuff->deviceid, BadValue);
        return Success;
    }


    memset(&rep, 0, sizeof(xMPXQueryPointerReply));
    rep.repType = X_Reply;
    rep.RepType = X_MPXQueryPointer;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    root = GetCurrentRootWindow();
    win = GetSpriteWindow(pDev);
    GetSpritePosition(pDev, &x, &y);

    rep.root = root->drawable.id;
    rep.root_x = x;
    rep.root_y = y;
    if (win != root)
    {
        rep.child = win->drawable.id;
        rep.win_x = x - win->drawable.x;
        rep.win_y = y - win->drawable.y;
    }
    else
    {
        rep.child = None;
        rep.win_x = x;
        rep.win_y = y;
    }


    rep.mask = pDev->button->state | inputInfo.keyboard->key->state;

    WriteReplyToClient(client, sizeof(xMPXQueryPointerReply), &rep);

    return Success;
}


/***********************************************************************
 *
 * This procedure writes the reply for the MPXQueryPointer function.
 */
int
SProcMPXQueryPointer(register ClientPtr client)
{
    register char n;

    REQUEST(xMPXQueryPointerReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xMPXQueryPointerReq);
    return (ProcMPXQueryPointer(client));
}
