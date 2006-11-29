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

#include "mpxextinit.h"
#include "swaprep.h"

#include "getvers.h"

static Mask lastExtEventMask = 1;
int ExtEventIndex;
Mask ExtValidMasks[EMASKSIZE];

XExtEventInfo EventInfo[32];

/*****************************************************************
 *
 * Externs defined elsewhere in the X server.
 *
 */

extern XExtensionVersion AllExtensionVersions[];

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

/*****************************************************************
 *
 * Declarations of local routines.
 *
 */

static XExtensionVersion thisversion = { MPX_Present,
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

}

void SReplyMPXDispatch(ClientPtr client, int len, mpxGetExtensionVersionReply* rep)
{
    if (rep->RepType ==  MPX_GetExtensionVersion)
        SRepMPXGetExtensionVersion(client, len, 
                (mpxGetExtensionVersionReply*) rep);
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
    Mask mask;

    MPXButtonPress = extEntry->eventBase;
    MPXButtonRelease = MPXButtonPress + 1;
    MPXMotionNotify = MPXButtonRelease + 1;

    mask = MPXGetNextExtEventMask();
    MPXSetMaskForExtEvent(mask, MPXButtonPress);
    MPXAllowPropagateSuppress(mask);

    mask = MPXGetNextExtEventMask();
    MPXSetMaskForExtEvent(mask, MPXButtonRelease);
    MPXAllowPropagateSuppress(mask);

    mask = MPXGetNextExtEventMask();
    MPXSetMaskForExtEvent(mask, MPXMotionNotify);
    MPXAllowPropagateSuppress(mask);

}

/**************************************************************************
 *
 * Return the next available extension event mask.
 *
 */
Mask
MPXGetNextExtEventMask(void)
{
    int i;
    Mask mask = lastExtEventMask;

    if (lastExtEventMask == 0) {
	FatalError("MPXGetNextExtEventMask: no more events are available.");
    }
    lastExtEventMask <<= 1;

    for (i = 0; i < MAX_DEVICES; i++)
	ExtValidMasks[i] |= mask;
    return mask;
}

/**************************************************************************
 *
 * Assign the specified mask to the specified event.
 *
 */

void
MPXSetMaskForExtEvent(Mask mask, int event)
{

    EventInfo[ExtEventIndex].mask = mask;
    EventInfo[ExtEventIndex++].type = event;

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
    for (i = 0; i < ExtEventIndex - 1; i++) {
	if ((EventInfo[i].type >= LASTEvent) && (EventInfo[i].type < 128))
	    SetMaskForEvent(0, EventInfo[i].type);
	EventInfo[i].mask = 0;
	EventInfo[i].type = 0;
    }

    ExtEventIndex = 0;
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
