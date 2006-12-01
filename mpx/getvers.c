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
#include "getvers.h"

MPXExtensionVersion AllExtensionVersions[128];

/***********************************************************************
 *
 * Handle a request from a client with a different byte order than us.
 *
 */

int
SProcMPXGetExtensionVersion(register ClientPtr client)
{
    register char n;

    REQUEST(mpxGetExtensionVersionReq);
    swaps(&stuff->length, n);
    swaps(&stuff->major_version, n);
    swaps(&stuff->minor_version, n);
    REQUEST_AT_LEAST_SIZE(mpxGetExtensionVersionReq);
    return (ProcMPXGetExtensionVersion(client));
}
/***********************************************************************
 *
 * This procedure writes the reply for the XGetExtensionVersion function.
 */

int
ProcMPXGetExtensionVersion(register ClientPtr client)
{
    mpxGetExtensionVersionReply rep;

    REQUEST(mpxGetExtensionVersionReq);
    REQUEST_SIZE_MATCH(mpxGetExtensionVersionReq);

    rep.repType = X_Reply;
    rep.RepType = MPX_GetExtensionVersion;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rep.major_version = AllExtensionVersions[MPXReqCode - 128].major_version;
    rep.minor_version = AllExtensionVersions[MPXReqCode - 128].minor_version;
    WriteReplyToClient(client, sizeof(mpxGetExtensionVersionReply), &rep);

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the MPXGetExtensionVersion function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepMPXGetExtensionVersion(ClientPtr client, int size,
			 mpxGetExtensionVersionReply * rep)
{
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swaps(&rep->major_version, n);
    swaps(&rep->minor_version, n);
    WriteToClient(client, size, (char *)rep);
}


