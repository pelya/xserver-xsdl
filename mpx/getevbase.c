/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>

#include "mpxglobals.h"
#include "getevbase.h"

/***********************************************************************
 *
 * This procedure writes the reply for the MPXGetEventBase function.
 */
int
ProcMPXGetEventBase(register ClientPtr client)
{
    mpxGetEventBaseReply rep;

    REQUEST(mpxGetEventBaseReq);
    REQUEST_SIZE_MATCH(mpxGetEventBaseReq);

    memset(&rep, 0, sizeof(mpxGetEventBaseReply));
    rep.repType = X_Reply;
    rep.RepType = MPX_GetEventBase;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rep.eventBase = MPXEventBase;

    WriteReplyToClient(client, sizeof(mpxGetEventBaseReply), &rep);

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the MPXGetEventBase function.
 */
int
SProcMPXGetEventBase(register ClientPtr client)
{
    register char n;

    REQUEST(mpxGetEventBaseReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(mpxGetEventBaseReq);
    return (ProcMPXGetEventBase(client));
}

