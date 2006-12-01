/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
//#include "gcstruct.h"	/* pointer for extnsionst.h */
#include "extnsionst.h"	/* extension entry   */
#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>
#include <X11/extensions/MPXconst.h>

#include "mpxglobals.h"
#include "mpxextinit.h"
#include "swaprep.h"

#include "getvers.h"
#include "listdev.h"
#include "selectev.h"
#include "getevbase.h"

static Mask lastExtEventMask = 1;
int MPXEventIndex;
MPXExtEventInfo EventInfo[32];

/**
 * MPX piggybacks on the X Input extension's event system. Each window has an
 * array of event masks, from 0 to MAX_DEVICES. In XI, each device can have
 * separate event masks. Before an event is delivered, the array at the index
 * of the device is checked.
 *
 * Two things:
 * -) core devices do not send input extension events
 * -) MPX events are not device specific.
 *
 * Since the mask of the core pointer (index 1) is thus not used by XI, MPX
 * can use it for the event mask. This also makes MPX less intrusive.  
 */
int MPXmskidx = 1;


/*****************************************************************
 *
 * Externs defined elsewhere in the X server.
 *
 */

extern MPXExtensionVersion AllExtensionVersions[];

Mask PropagateMask[MAX_DEVICES];

/*****************************************************************
 *
 * Globals referenced elsewhere in the server.
 *
 */

int MPXReqCode = 0;
int MPXEventBase = 0;
int MPXErrorBase = 0;

int MPXButtonPress;
int MPXButtonRelease;
int MPXMotionNotify;
int MPXLastEvent;

/*****************************************************************
 *
 * Declarations of local routines.
 *
 */

static MPXExtensionVersion thisversion = { 
    MPX_Present,
    MPX_Major,
    MPX_Minor
};

/**********************************************************************
 *
 * MPXExtensionInit - initialize the input extension.
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 * This extension has several events and errors.
 *
 */

void
MPXExtensionInit(void)
{
    ExtensionEntry *extEntry;

    extEntry = AddExtension(MPXNAME, MPXEVENTS, MPXERRORS, 
                            ProcMPXDispatch, SProcMPXDispatch, 
                            MPXResetProc, StandardMinorOpcode);
    if (extEntry) {
	MPXReqCode = extEntry->base;
        MPXEventBase = extEntry->eventBase;
        MPXErrorBase = extEntry->errorBase;

	AllExtensionVersions[MPXReqCode - 128] = thisversion;
	MPXFixExtensionEvents(extEntry);
	ReplySwapVector[MPXReqCode] = (ReplySwapPtr) SReplyMPXDispatch;
	EventSwapVector[MPXButtonPress] = SEventMPXDispatch;
	EventSwapVector[MPXButtonRelease] = SEventMPXDispatch;
	EventSwapVector[MPXMotionNotify] = SEventMPXDispatch;
    } else {
	FatalError("MPXExtensionInit: AddExtensions failed\n");
    }
}

/*************************************************************************
 *
 * ProcMPXDispatch - main dispatch routine for requests to this extension.
 * This routine is used if server and client have the same byte ordering.
 *
 */

int 
ProcMPXDispatch(register ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data == MPX_GetExtensionVersion)
        return (ProcMPXGetExtensionVersion(client));
    if (stuff->data == MPX_ListDevices)
        return (ProcMPXListDevices(client));
    if (stuff->data == MPX_SelectEvents)
        return (ProcMPXSelectEvents(client));
    if (stuff->data == MPX_GetEventBase)
        return (ProcMPXGetEventBase(client));
    else {
        SendErrorToClient(client, MPXReqCode, stuff->data, 0, BadRequest);
    }

    return (BadRequest);
}

/*******************************************************************************
 *
 * SProcMPXDispatch 
 *
 * Main swapped dispatch routine for requests to this extension.
 * This routine is used if server and client do not have the same byte ordering.
 *
 */

int 
SProcMPXDispatch(register ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data == MPX_GetExtensionVersion)
        return (SProcMPXGetExtensionVersion(client));
    if (stuff->data == MPX_ListDevices)
        return (SProcMPXListDevices(client));
    if (stuff->data == MPX_SelectEvents)
        return (SProcMPXSelectEvents(client));
    if (stuff->data == MPX_GetEventBase)
        return (SProcMPXGetEventBase(client));
    else {
        SendErrorToClient(client, MPXReqCode, stuff->data, 0, BadRequest);
    }

    return (BadRequest);
}

/***********************************************************************
 *
 * MPXResetProc.
 * Remove reply-swapping routine.
 * Remove event-swapping routine.
 *
 */

void
MPXResetProc(ExtensionEntry* unused)
{
    ReplySwapVector[MPXReqCode] = ReplyNotSwappd;
    EventSwapVector[MPXButtonPress] = NotImplemented;
    EventSwapVector[MPXButtonRelease] = NotImplemented;
    EventSwapVector[MPXMotionNotify] = NotImplemented;

    MPXRestoreExtensionEvents();

}

void SReplyMPXDispatch(ClientPtr client, int len, mpxGetExtensionVersionReply* rep)
{
    if (rep->RepType ==  MPX_GetExtensionVersion)
        SRepMPXGetExtensionVersion(client, len, 
                (mpxGetExtensionVersionReply*) rep);
    if (rep->RepType ==  MPX_ListDevices)
        SRepMPXListDevices(client, len, 
                (mpxListDevicesReply*) rep);
    else {
	FatalError("MPX confused sending swapped reply");
    }
}

void 
SEventMPXDispatch(xEvent* from, xEvent* to)
{
    int type = from->u.u.type & 0177;
    if (type == MPXButtonPress) {
	SKeyButtonPtrEvent(from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    } else if (type == MPXButtonRelease) {
	SKeyButtonPtrEvent(from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    } else if (type == MPXMotionNotify) {
	SKeyButtonPtrEvent(from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    } else {
	FatalError("XInputExtension: Impossible event!\n");
    }
}

void 
MPXFixExtensionEvents(ExtensionEntry* extEntry)
{
    MPXButtonPress = extEntry->eventBase;
    MPXButtonRelease = MPXButtonPress + 1;
    MPXMotionNotify = MPXButtonRelease + 1;
    MPXLastEvent = MPXMotionNotify + 1;

    MPXSetMaskForExtEvent(MPXButtonPressMask, MPXButtonPress);
    MPXSetMaskForExtEvent(MPXButtonReleaseMask, MPXButtonRelease);
    MPXSetMaskForExtEvent(MPXPointerMotionMask, MPXMotionNotify);
}


/**************************************************************************
 *
 * Assign the specified mask to the specified event.
 *
 */

void
MPXSetMaskForExtEvent(Mask mask, int event)
{

    EventInfo[MPXEventIndex].mask = mask;
    EventInfo[MPXEventIndex++].type = event;

    if ((event < LASTEvent) || (event >= 128))
	FatalError("MaskForExtensionEvent: bogus event number");
    SetMaskForEvent(mask, event);
}

/************************************************************************
 *
 * This function restores extension event types and masks to their 
 * initial state.
 *
 */

void
MPXRestoreExtensionEvents(void)
{
    int i;

    MPXReqCode = 0;
    for (i = 0; i < MPXEventIndex - 1; i++) {
	if ((EventInfo[i].type >= LASTEvent) && (EventInfo[i].type < 128))
	    SetMaskForEvent(0, EventInfo[i].type);
	EventInfo[i].mask = 0;
	EventInfo[i].type = 0;
    }

    MPXEventIndex = 0;
    lastExtEventMask = 1;
    MPXButtonPress = 0;
    MPXButtonRelease = 1;
    MPXMotionNotify = 2;
}

/**************************************************************************
 *
 * Allow the specified event to have its propagation suppressed.
 * The default is to not allow suppression of propagation.
 *
 */

void
MPXAllowPropagateSuppress(Mask mask)
{
    int i;

    for (i = 0; i < MAX_DEVICES; i++)
	PropagateMask[i] |= mask;
}

