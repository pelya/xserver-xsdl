/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>
#include "extnsionst.h"
#include "mpxextinit.h"	/* LookupDeviceIntRec */
#include "mpxglobals.h"
#include "mpxevents.h"

#include "selectev.h"

/* functions borrowed from XI */
extern void RecalculateDeviceDeliverableEvents(
	WindowPtr              /* pWin */);

extern int AddExtensionClient (
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* mskidx */);
extern Bool
ShouldFreeInputMasks(WindowPtr /* pWin */, 
                     Bool /* ignoreSelectedEvents */);

/***********************************************************************
 *
 * Handle requests from clients with a different byte order.
 *
 */

int
SProcMPXSelectEvents(register ClientPtr client)
{
    register char n;

    REQUEST(xMPXSelectEventsReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xMPXSelectEventsReq);
    swapl(&stuff->window, n);
    return (ProcMPXSelectEvents(client));
}

/***********************************************************************
 *
 * This procedure selects input from an extension device.
 *
 */

int
ProcMPXSelectEvents(register ClientPtr client)
{
    int ret;
    WindowPtr pWin;

    REQUEST(xMPXSelectEventsReq);
    REQUEST_SIZE_MATCH(xMPXSelectEventsReq);

    if (stuff->length != (sizeof(xMPXSelectEventsReq) >> 2))
    { 
        SendErrorToClient(client, MPXReqCode, X_MPXSelectEvents, 0,
			  BadLength);
	return Success;
    }

    pWin = (WindowPtr) LookupWindow(stuff->window, client);
    if (!pWin) 
    {
	client->errorValue = stuff->window;
	SendErrorToClient(client, MPXReqCode, X_MPXSelectEvents, 0,
			  BadWindow);
	return Success;
    }

    if (stuff->mask >= MPXHighestMask)
    {
        client->errorValue = stuff->mask;
	SendErrorToClient(client, MPXReqCode, X_MPXSelectEvents, 0,
			  BadValue);
    }

    if ((ret = MPXSelectForWindow(pWin, client, stuff->mask)) != Success) 
    { 
        SendErrorToClient(client, MPXReqCode, X_MPXSelectEvents, 0, ret);
        return Success;
    }

    return Success;
}

/**
 * Selects a set of events for a given window. 
 * Different to XI, MPX is not device dependent. Either the client gets events
 * from all devices or none.
 *
 * This method borrows some functions from XI, due to the piggyback on the
 * core pointer (see comment in extinit.c)
 */
int 
MPXSelectForWindow(WindowPtr pWin, ClientPtr client, int mask)
{
    InputClientsPtr others;
    int ret;

    if (mask >= MPXHighestMask)
    {
        client->errorValue = mask;
        return BadValue;
    }

    if (wOtherInputMasks(pWin))
    {
        for (others = wOtherInputMasks(pWin)->inputClients; others; 
                others = others->next)
        {
            if (SameClient(others, client)) {
                others->mask[MPXmskidx] = mask;
                if (mask == 0)
                {
                    /* clean up stuff */
                    RecalculateDeviceDeliverableEvents(pWin);
                    if (ShouldFreeInputMasks(pWin, FALSE))
                        FreeResource(others->resource, RT_NONE);
                    return Success;
                }
                goto maskSet;
            }
        }
    }
    /* borrow from XI here */
    if ((ret = AddExtensionClient(pWin, client, mask, MPXmskidx)) != Success)
        return ret;
maskSet:
    RecalculateDeviceDeliverableEvents(pWin);
    return Success;

}

