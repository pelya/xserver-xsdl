/* $XFree86: xc/programs/Xserver/Xext/xf86vmode.c,v 3.25 1996/10/16 14:37:58 dawes Exp $ */

/*

Copyright 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES 
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from Kaleb S. KEITHLEY

*/
/* $Xorg: xf86vmode.c,v 1.3 2000/08/17 19:47:59 cpqbld Exp $ */
/* THIS IS NOT AN X CONSORTIUM STANDARD OR AN X PROJECT TEAM SPECIFICATION */

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86VIDMODE_SERVER_
#include "xf86vmstr.h"
#include "Xfuncproto.h"
#include "../hw/xfree86/common/xf86.h"
#include "../hw/xfree86/common/xf86Priv.h"

extern int xf86ScreenIndex;
extern Bool xf86VidModeEnabled;
extern Bool xf86VidModeAllowNonLocal;

static int vidmodeErrorBase;

static int ProcXF86VidModeDispatch(
#if NeedFunctionPrototypes
    ClientPtr /* client */
#endif
);

static int SProcXF86VidModeDispatch(
#if NeedFunctionPrototypes
    ClientPtr /* client */
#endif
);

static void XF86VidModeResetProc(
#if NeedFunctionPrototypes
    ExtensionEntry* /* extEntry */
#endif
);

extern void Swap32Write(); /* XXX should be in header file */

static unsigned char XF86VidModeReqCode = 0;

/* The XF86VIDMODE_EVENTS code is far from complete */

#ifdef XF86VIDMODE_EVENTS
static int XF86VidModeEventBase = 0;

static void SXF86VidModeNotifyEvent();
#if NeedFunctionPrototypes
    xXF86VidModeNotifyEvent * /* from */,
    xXF86VidModeNotifyEvent * /* to */
#endif
);

extern WindowPtr *WindowTable;

static RESTYPE EventType;	/* resource type for event masks */

typedef struct _XF86VidModeEvent *XF86VidModeEventPtr;

typedef struct _XF86VidModeEvent {
    XF86VidModeEventPtr	next;
    ClientPtr		client;
    ScreenPtr		screen;
    XID			resource;
    CARD32		mask;
} XF86VidModeEventRec;

static int XF86VidModeFreeEvents();

typedef struct _XF86VidModeScreenPrivate {
    XF86VidModeEventPtr	events;
    Bool		hasWindow;
} XF86VidModeScreenPrivateRec, *XF86VidModeScreenPrivatePtr;
   
static int ScreenPrivateIndex;

#define GetScreenPrivate(s) ((ScreenSaverScreenPrivatePtr)(s)->devPrivates[ScreenPrivateIndex].ptr)
#define SetScreenPrivate(s,v) ((s)->devPrivates[ScreenPrivateIndex].ptr = (pointer) v);
#define SetupScreen(s)  ScreenSaverScreenPrivatePtr pPriv = GetScreenPrivate(s)

#define New(t)  ((t *) xalloc (sizeof (t)))
#endif

void
XFree86VidModeExtensionInit()
{
    ExtensionEntry* extEntry;
#ifdef XF86VIDMODE_EVENTS
    int		    i;
    ScreenPtr	    pScreen;

    EventType = CreateNewResourceType(XF86VidModeFreeEvents);
    ScreenPrivateIndex = AllocateScreenPrivateIndex ();
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	SetScreenPrivate (pScreen, NULL);
    }
#endif

    if (
#ifdef XF86VIDMODE_EVENTS
        EventType && ScreenPrivateIndex != -1 &&
#endif
	(extEntry = AddExtension(XF86VIDMODENAME,
				XF86VidModeNumberEvents,
				XF86VidModeNumberErrors,
				ProcXF86VidModeDispatch,
				SProcXF86VidModeDispatch,
				XF86VidModeResetProc,
				StandardMinorOpcode))) {
	XF86VidModeReqCode = (unsigned char)extEntry->base;
	vidmodeErrorBase = extEntry->errorBase;
#ifdef XF86VIDMODE_EVENTS
	XF86VidModeEventBase = extEntry->eventBase;
	EventSwapVector[XF86VidModeEventBase] = SXF86VidModeNotifyEvent;
#endif
    }
}

/*ARGSUSED*/
static void
XF86VidModeResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

#ifdef XF86VIDMODE_EVENTS
static void
CheckScreenPrivate (pScreen)
    ScreenPtr	pScreen;
{
    SetupScreen (pScreen);

    if (!pPriv)
	return;
    if (!pPriv->events && !pPriv->hasWindow) {
	xfree (pPriv);
	SetScreenPrivate (pScreen, NULL);
    }
}
    
static XF86VidModeScreenPrivatePtr
MakeScreenPrivate (pScreen)
    ScreenPtr	pScreen;
{
    SetupScreen (pScreen);

    if (pPriv)
	return pPriv;
    pPriv = New (XF86VidModeScreenPrivateRec);
    if (!pPriv)
	return 0;
    pPriv->events = 0;
    pPriv->hasWindow = FALSE;
    SetScreenPrivate (pScreen, pPriv);
    return pPriv;
}

static unsigned long
getEventMask (pScreen, client)
    ScreenPtr	pScreen;
    ClientPtr	client;
{
    SetupScreen(pScreen);
    XF86VidModeEventPtr pEv;

    if (!pPriv)
	return 0;
    for (pEv = pPriv->events; pEv; pEv = pEv->next)
	if (pEv->client == client)
	    return pEv->mask;
    return 0;
}

static Bool
setEventMask (pScreen, client, mask)
    ScreenPtr	pScreen;
    ClientPtr	client;
    unsigned long mask;
{
    SetupScreen(pScreen);
    XF86VidModeEventPtr pEv, *pPrev;

    if (getEventMask (pScreen, client) == mask)
	return TRUE;
    if (!pPriv) {
	pPriv = MakeScreenPrivate (pScreen);
	if (!pPriv)
	    return FALSE;
    }
    for (pPrev = &pPriv->events; pEv = *pPrev; pPrev = &pEv->next)
	if (pEv->client == client)
	    break;
    if (mask == 0) {
	*pPrev = pEv->next;
	xfree (pEv);
	CheckScreenPrivate (pScreen);
    } else {
	if (!pEv) {
	    pEv = New (ScreenSaverEventRec);
	    if (!pEv) {
		CheckScreenPrivate (pScreen);
		return FALSE;
	    }
	    *pPrev = pEv;
	    pEv->next = NULL;
	    pEv->client = client;
	    pEv->screen = pScreen;
	    pEv->resource = FakeClientID (client->index);
	}
	pEv->mask = mask;
    }
    return TRUE;
}

static int
XF86VidModeFreeEvents (value, id)
    pointer value;
    XID id;
{
    XF86VidModeEventPtr	pOld = (XF86VidModeEventPtr)value;
    ScreenPtr pScreen = pOld->screen;
    SetupScreen (pScreen);
    XF86VidModeEventPtr	pEv, *pPrev;

    if (!pPriv)
	return TRUE;
    for (pPrev = &pPriv->events; pEv = *pPrev; pPrev = &pEv->next)
	if (pEv == pOld)
	    break;
    if (!pEv)
	return TRUE;
    *pPrev = pEv->next;
    xfree (pEv);
    CheckScreenPrivate (pScreen);
    return TRUE;
}

static void
SendXF86VidModeNotify (pScreen, state, forced)
    ScreenPtr	pScreen;
    int	    state;
    Bool    forced;
{
    XF86VidModeScreenPrivatePtr	pPriv;
    XF86VidModeEventPtr		pEv;
    unsigned long		mask;
    xXF86VidModeNotifyEvent	ev;
    ClientPtr			client;
    int				kind;

    UpdateCurrentTimeIf ();
    mask = XF86VidModeNotifyMask;
    pScreen = screenInfo.screens[pScreen->myNum];
    pPriv = GetScreenPrivate(pScreen);
    if (!pPriv)
	return;
    kind = XF86VidModeModeChange;
    for (pEv = pPriv->events; pEv; pEv = pEv->next)
    {
	client = pEv->client;
	if (client->clientGone)
	    continue;
	if (!(pEv->mask & mask))
	    continue;
	ev.type = XF86VidModeNotify + XF86VidModeEventBase;
	ev.state = state;
	ev.sequenceNumber = client->sequence;
	ev.timestamp = currentTime.milliseconds;
	ev.root = WindowTable[pScreen->myNum]->drawable.id;
	ev.kind = kind;
	ev.forced = forced;
	WriteEventsToClient (client, 1, (xEvent *) &ev);
    }
}

static void
SXF86VidModeNotifyEvent (from, to)
    xXF86VidModeNotifyEvent *from, *to;
{
    to->type = from->type;
    to->state = from->state;
    cpswaps (from->sequenceNumber, to->sequenceNumber);
    cpswapl (from->timestamp, to->timestamp);    
    cpswapl (from->root, to->root);    
    to->kind = from->kind;
    to->forced = from->forced;
}
#endif
	
static int
ProcXF86VidModeQueryVersion(client)
    register ClientPtr client;
{
    xXF86VidModeQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86VidModeQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XF86VIDMODE_MAJOR_VERSION;
    rep.minorVersion = XF86VIDMODE_MINOR_VERSION;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swaps(&rep.majorVersion, n);
    	swaps(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xXF86VidModeQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86VidModeGetModeLine(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeGetModeLineReq);
    xXF86VidModeGetModeLineReply rep;
    register int n;
    ScrnInfoPtr vptr;
    DisplayModePtr mptr;
    int privsize;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    if (!mptr->Private)
	privsize = 0;
    else
	privsize = mptr->PrivSize;

    REQUEST_SIZE_MATCH(xXF86VidModeGetModeLineReq);
    rep.type = X_Reply;
    rep.length = (SIZEOF(xXF86VidModeGetModeLineReply) - SIZEOF(xGenericReply) +
		  privsize * sizeof(INT32)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.dotclock = vptr->clock[mptr->Clock];
    rep.hdisplay = mptr->HDisplay;
    rep.hsyncstart = mptr->HSyncStart;
    rep.hsyncend = mptr->HSyncEnd;
    rep.htotal = mptr->HTotal;
    rep.vdisplay = mptr->VDisplay;
    rep.vsyncstart = mptr->VSyncStart;
    rep.vsyncend = mptr->VSyncEnd;
    rep.vtotal = mptr->VTotal;
    rep.flags = mptr->Flags;
    rep.privsize = privsize;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.dotclock, n);
    	swaps(&rep.hdisplay, n);
    	swaps(&rep.hsyncstart, n);
    	swaps(&rep.hsyncend, n);
    	swaps(&rep.htotal, n);
    	swaps(&rep.vdisplay, n);
    	swaps(&rep.vsyncstart, n);
    	swaps(&rep.vsyncend, n);
    	swaps(&rep.vtotal, n);
	swapl(&rep.flags, n);
	swapl(&rep.privsize, n);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetModeLineReply), (char *)&rep);
    if (privsize) {
	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient(client, privsize * sizeof(INT32),
				 mptr->Private);
    }
    return (client->noClientException);
}

static int
ProcXF86VidModeGetAllModeLines(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeGetAllModeLinesReq);
    xXF86VidModeGetAllModeLinesReply rep;
    xXF86VidModeModeInfo mdinf;
    register int n;
    ScrnInfoPtr vptr;
    DisplayModePtr mptr, curmptr;
    int privsize, modecount=1;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    curmptr = mptr = vptr->modes;

    while (mptr->next != curmptr) {
	++modecount;
	mptr = mptr->next;
    }

    REQUEST_SIZE_MATCH(xXF86VidModeGetAllModeLinesReq);
    rep.type = X_Reply;
    rep.length = (SIZEOF(xXF86VidModeGetAllModeLinesReply) - SIZEOF(xGenericReply) +
		  modecount * sizeof(xXF86VidModeModeInfo)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.modecount = modecount;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.modecount, n);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetAllModeLinesReply), (char *)&rep);
    mptr = curmptr;
    do {
        if (!mptr->Private)
	    privsize = 0;
        else
	    privsize = mptr->PrivSize;

        mdinf.dotclock = vptr->clock[mptr->Clock];
        mdinf.hdisplay = mptr->HDisplay;
        mdinf.hsyncstart = mptr->HSyncStart;
        mdinf.hsyncend = mptr->HSyncEnd;
        mdinf.htotal = mptr->HTotal;
        mdinf.vdisplay = mptr->VDisplay;
        mdinf.vsyncstart = mptr->VSyncStart;
        mdinf.vsyncend = mptr->VSyncEnd;
        mdinf.vtotal = mptr->VTotal;
        mdinf.flags = mptr->Flags;
        mdinf.privsize = privsize;
        if (client->swapped) {
	    swapl(&mdinf.dotclock, n);
    	    swaps(&mdinf.hdisplay, n);
    	    swaps(&mdinf.hsyncstart, n);
    	    swaps(&mdinf.hsyncend, n);
    	    swaps(&mdinf.htotal, n);
    	    swaps(&mdinf.vdisplay, n);
    	    swaps(&mdinf.vsyncstart, n);
    	    swaps(&mdinf.vsyncend, n);
    	    swaps(&mdinf.vtotal, n);
	    swapl(&mdinf.flags, n);
	    swapl(&mdinf.privsize, n);
        }
        WriteToClient(client, sizeof(xXF86VidModeModeInfo), (char *)&mdinf);
        mptr = mptr->next;
    } while (mptr != curmptr);
    return (client->noClientException);
}

#define CLOCKSPD(clk,scrp)	((clk>MAXCLOCKS)? clk: scrp->clock[clk])
#define MODEMATCH(mptr,stuff,scrp)	\
	(CLOCKSPD(mptr->Clock,scrp) == CLOCKSPD(stuff->dotclock,scrp) \
				&& mptr->HDisplay  == stuff->hdisplay \
				&& mptr->HSyncStart== stuff->hsyncstart \
				&& mptr->HSyncEnd  == stuff->hsyncend \
				&& mptr->HTotal    == stuff->htotal \
				&& mptr->VDisplay  == stuff->vdisplay \
				&& mptr->VSyncStart== stuff->vsyncstart \
				&& mptr->VSyncEnd  == stuff->vsyncend \
				&& mptr->VTotal    == stuff->vtotal \
				&& mptr->Flags     == stuff->flags )

static int
ProcXF86VidModeAddModeLine(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeAddModeLineReq);
    ScrnInfoPtr vptr;
    DisplayModePtr curmptr, mptr, newmptr;
    DisplayModeRec modetmp;
    int len;

    if (xf86Verbose) {
	ErrorF("AddModeLine - scrn: %d clock: %d\n",
		stuff->screen, stuff->dotclock);
	ErrorF("AddModeLine - hdsp: %d hbeg: %d hend: %d httl: %d\n",
		stuff->hdisplay, stuff->hsyncstart,
		stuff->hsyncend, stuff->htotal);
	ErrorF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend,
		stuff->vtotal, stuff->flags);
	ErrorF("      after - scrn: %d clock: %d\n",
		stuff->screen, stuff->after_dotclock);
	ErrorF("              hdsp: %d hbeg: %d hend: %d httl: %d\n",
		stuff->after_hdisplay, stuff->after_hsyncstart,
		stuff->after_hsyncend, stuff->after_htotal);
	ErrorF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		stuff->after_vdisplay, stuff->after_vsyncstart,
		stuff->after_vsyncend, stuff->after_vtotal, stuff->after_flags);
    }
    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    curmptr = mptr = vptr->modes;

    REQUEST_AT_LEAST_SIZE(xXF86VidModeAddModeLineReq);
    len = client->req_len - (sizeof(xXF86VidModeAddModeLineReq) >> 2);
    if (len != stuff->privsize)
	return BadLength;

    if (stuff->hsyncstart < stuff->hdisplay   ||
	stuff->hsyncend   < stuff->hsyncstart ||
	stuff->htotal     < stuff->hsyncend   ||
	stuff->vsyncstart < stuff->vdisplay   ||
	stuff->vsyncend   < stuff->vsyncstart ||
	stuff->vtotal     < stuff->vsyncend)
	return BadValue;

    if (stuff->after_hsyncstart < stuff->after_hdisplay   ||
	stuff->after_hsyncend   < stuff->after_hsyncstart ||
	stuff->after_htotal     < stuff->after_hsyncend   ||
	stuff->after_vsyncstart < stuff->after_vdisplay   ||
	stuff->after_vsyncend   < stuff->after_vsyncstart ||
	stuff->after_vtotal     < stuff->after_vsyncend)
	return BadValue;

    if (stuff->after_htotal != 0 || stuff->after_vtotal != 0) {
	Bool found = FALSE;
	do {
	    if (MODEMATCH(mptr, stuff, vptr)) {
		found = TRUE;
		break;
	    }
	} while ((mptr = mptr->next) != curmptr);
	if (!found)
	    return BadValue;
    }

    newmptr = (DisplayModePtr) xalloc(sizeof(DisplayModeRec));

    newmptr->Clock         = stuff->dotclock;
    newmptr->CrtcHDisplay  = newmptr->HDisplay      = stuff->hdisplay;
    newmptr->CrtcHSyncStart= newmptr->HSyncStart    = stuff->hsyncstart;
    newmptr->CrtcHSyncEnd  = newmptr->HSyncEnd      = stuff->hsyncend;
    newmptr->CrtcHTotal    = newmptr->HTotal        = stuff->htotal;
    newmptr->CrtcVDisplay  = newmptr->VDisplay      = stuff->vdisplay;
    newmptr->CrtcVSyncStart= newmptr->VSyncStart    = stuff->vsyncstart;
    newmptr->CrtcVSyncEnd  = newmptr->VSyncEnd      = stuff->vsyncend;
    newmptr->CrtcVTotal    = newmptr->VTotal        = stuff->vtotal;
    newmptr->Flags         = stuff->flags;
#if 0
    newmptr->CrtcHSkew     = newmptr->HSkew         = stuff->hskew;
#endif
    newmptr->CrtcHAdjusted = FALSE;
    newmptr->CrtcVAdjusted = FALSE;
    newmptr->name          = "";
    newmptr->Private       = NULL;
    if (stuff->privsize) {
	if (xf86Verbose)
	    ErrorF("AddModeLine - Request includes privates\n");
	newmptr->Private =
	    (INT32 *) ALLOCATE_LOCAL(stuff->privsize * sizeof(INT32));
	memcpy(newmptr->Private, &stuff[1], stuff->privsize*sizeof(INT32));
    }

    /* Check that the mode is consistent with the monitor specs */
    switch (xf86CheckMode(vptr, newmptr, vptr->monitor, FALSE)) {
	case MODE_HSYNC:
	    xfree(newmptr->Private);
	    xfree(newmptr);
	    return vidmodeErrorBase + XF86VidModeBadHTimings;
	case MODE_VSYNC:
	    xfree(newmptr->Private);
	    xfree(newmptr);
	    return vidmodeErrorBase + XF86VidModeBadVTimings;
    }

    /* Check that the driver is happy with the mode */
    if (vptr->ValidMode(newmptr, xf86Verbose, MODE_VID) != MODE_OK) {
	xfree(newmptr->Private);
	xfree(newmptr);
	return vidmodeErrorBase + XF86VidModeModeUnsuitable;
    }

    if (newmptr->Flags & V_DBLSCAN)
    {
	newmptr->CrtcVDisplay *= 2;
	newmptr->CrtcVSyncStart *= 2;
	newmptr->CrtcVSyncEnd *= 2;
	newmptr->CrtcVTotal *= 2;
	newmptr->CrtcVAdjusted = TRUE;
    }

    newmptr->next       = mptr->next;
    newmptr->prev       = mptr;
    mptr->next          = newmptr;
    newmptr->next->prev = newmptr;

#if 0  /* Do we want this? */
    (vptr->SwitchMode)(newmptr);
#endif

    if (xf86Verbose)
	ErrorF("AddModeLine - Succeeded\n");
    return(client->noClientException);
}

static int
ProcXF86VidModeDeleteModeLine(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeDeleteModeLineReq);
    ScrnInfoPtr vptr;
    DisplayModePtr curmptr, mptr;
    DisplayModeRec modetmp;
    int len;

    if (xf86Verbose) {
	ErrorF("DeleteModeLine - scrn: %d clock: %d\n",
		stuff->screen, stuff->dotclock, stuff->dotclock);
	ErrorF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
		stuff->hdisplay, stuff->hsyncstart,
		stuff->hsyncend, stuff->htotal);
	ErrorF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend,
		stuff->vtotal, stuff->flags);
    }
    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    curmptr = mptr = vptr->modes;

    REQUEST_AT_LEAST_SIZE(xXF86VidModeDeleteModeLineReq);
    len = client->req_len - (sizeof(xXF86VidModeDeleteModeLineReq) >> 2);
    if (len != stuff->privsize) {
	ErrorF("req_len = %d, sizeof(Req) = %d, privsize = %d, len = %d, length = %d\n",
		client->req_len, sizeof(xXF86VidModeDeleteModeLineReq)>>2, stuff->privsize, len, stuff->length);
	return BadLength;
    }

	ErrorF("Checking against clock: %d (%d)\n",
		mptr->Clock, CLOCKSPD(mptr->Clock, vptr));
	ErrorF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
		mptr->HDisplay, mptr->HSyncStart,
		mptr->HSyncEnd, mptr->HTotal);
	ErrorF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		mptr->VDisplay, mptr->VSyncStart, mptr->VSyncEnd,
		mptr->VTotal, mptr->Flags);
    if (MODEMATCH(mptr, stuff, vptr))
	return BadValue;

    while ((mptr = mptr->next) != curmptr) {
	ErrorF("Checking against clock: %d (%d)\n",
		mptr->Clock, CLOCKSPD(mptr->Clock, vptr));
	ErrorF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
		mptr->HDisplay, mptr->HSyncStart,
		mptr->HSyncEnd, mptr->HTotal);
	ErrorF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		mptr->VDisplay, mptr->VSyncStart, mptr->VSyncEnd,
		mptr->VTotal, mptr->Flags);
	if (MODEMATCH(mptr, stuff, vptr)) {
	    mptr->prev->next = mptr->next;
	    mptr->next->prev = mptr->prev;
	    xfree(mptr->name);
	    xfree(mptr->Private);
	    xfree(mptr);
	    if (xf86Verbose)
		ErrorF("DeleteModeLine - Succeeded\n");
	    return(client->noClientException);
	}
    }
    return BadValue;
}

static int
ProcXF86VidModeModModeLine(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeModModeLineReq);
    ScrnInfoPtr vptr;
    DisplayModePtr mptr;
    DisplayModeRec modetmp;
    int len;

    if (xf86Verbose) {
	ErrorF("ModModeLine - scrn: %d hdsp: %d hbeg: %d hend: %d httl: %d\n",
		stuff->screen, stuff->hdisplay, stuff->hsyncstart,
		stuff->hsyncend, stuff->htotal);
	ErrorF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend,
		stuff->vtotal, stuff->flags);
    }
    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    REQUEST_AT_LEAST_SIZE(xXF86VidModeModModeLineReq);
    len = client->req_len - (sizeof(xXF86VidModeModModeLineReq) >> 2);
    if (len != stuff->privsize)
	return BadLength;

    if (stuff->hsyncstart < stuff->hdisplay   ||
	stuff->hsyncend   < stuff->hsyncstart ||
	stuff->htotal     < stuff->hsyncend   ||
	stuff->vsyncstart < stuff->vdisplay   ||
	stuff->vsyncend   < stuff->vsyncstart ||
	stuff->vtotal     < stuff->vsyncend)
	return BadValue;

    memcpy(&modetmp, mptr, sizeof(DisplayModeRec));

    modetmp.HDisplay   = stuff->hdisplay;
    modetmp.HSyncStart = stuff->hsyncstart;
    modetmp.HSyncEnd   = stuff->hsyncend;
    modetmp.HTotal     = stuff->htotal;
    modetmp.VDisplay   = stuff->vdisplay;
    modetmp.VSyncStart = stuff->vsyncstart;
    modetmp.VSyncEnd   = stuff->vsyncend;
    modetmp.VTotal     = stuff->vtotal;
    modetmp.Flags      = stuff->flags;
    if (mptr->PrivSize && stuff->privsize) {
	if (mptr->PrivSize != stuff->privsize)
	    return BadValue;
    }
    if (mptr->PrivSize && mptr->Private) {
	modetmp.Private =
		(INT32 *)ALLOCATE_LOCAL(mptr->PrivSize * sizeof(INT32));
	if (stuff->privsize) {
	    if (xf86Verbose)
		ErrorF("ModModeLine - Request includes privates\n");
	    memcpy(modetmp.Private, &stuff[1], mptr->PrivSize * sizeof(INT32));
	} else
	    memcpy(modetmp.Private, mptr->Private,
		   mptr->PrivSize * sizeof(INT32));
    }

    /* Check that the mode is consistent with the monitor specs */
    switch (xf86CheckMode(vptr, &modetmp, vptr->monitor, FALSE)) {
	case MODE_HSYNC:
	    DEALLOCATE_LOCAL(modetmp.Private);
	    return vidmodeErrorBase + XF86VidModeBadHTimings;
	case MODE_VSYNC:
	    DEALLOCATE_LOCAL(modetmp.Private);
	    return vidmodeErrorBase + XF86VidModeBadVTimings;
    }

    /* Check that the driver is happy with the mode */
    if (vptr->ValidMode(&modetmp, xf86Verbose, MODE_VID) != MODE_OK) {
	DEALLOCATE_LOCAL(modetmp.Private);
	return vidmodeErrorBase + XF86VidModeModeUnsuitable;
    }

    DEALLOCATE_LOCAL(modetmp.Private);

    mptr->HDisplay   = stuff->hdisplay;
    mptr->HSyncStart = stuff->hsyncstart;
    mptr->HSyncEnd   = stuff->hsyncend;
    mptr->HTotal     = stuff->htotal;
    mptr->VDisplay   = stuff->vdisplay;
    mptr->VSyncStart = stuff->vsyncstart;
    mptr->VSyncEnd   = stuff->vsyncend;
    mptr->VTotal     = stuff->vtotal;
    mptr->Flags      = stuff->flags;
    mptr->CrtcHDisplay   = stuff->hdisplay;
    mptr->CrtcHSyncStart = stuff->hsyncstart;
    mptr->CrtcHSyncEnd   = stuff->hsyncend;
    mptr->CrtcHTotal     = stuff->htotal;
    mptr->CrtcVDisplay   = stuff->vdisplay;
    mptr->CrtcVSyncStart = stuff->vsyncstart;
    mptr->CrtcVSyncEnd   = stuff->vsyncend;
    mptr->CrtcVTotal     = stuff->vtotal;
    mptr->CrtcVAdjusted = FALSE;
    mptr->CrtcHAdjusted = FALSE;
    if (mptr->Flags & V_DBLSCAN)
    {
	mptr->CrtcVDisplay *= 2;
	mptr->CrtcVSyncStart *= 2;
	mptr->CrtcVSyncEnd *= 2;
	mptr->CrtcVTotal *= 2;
	mptr->CrtcVAdjusted = TRUE;
    }
    if (mptr->PrivSize && stuff->privsize) {
	memcpy(mptr->Private, &stuff[1], mptr->PrivSize * sizeof(INT32));
    }

    (vptr->SwitchMode)(mptr);
    (vptr->AdjustFrame)(vptr->frameX0, vptr->frameY0);

    if (xf86Verbose)
	ErrorF("ModModeLine - Succeeded\n");
    return(client->noClientException);
}

static int
ProcXF86VidModeValidateModeLine(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeValidateModeLineReq);
    xXF86VidModeValidateModeLineReply rep;
    ScrnInfoPtr vptr;
    DisplayModePtr mptr;
    DisplayModeRec modetmp;
    int len, status;

    if (xf86Verbose) {
	ErrorF("ValidateModeLine - scrn: %d clock: %d\n",
		stuff->screen, stuff->dotclock, stuff->dotclock);
	ErrorF("                   hdsp: %d hbeg: %d hend: %d httl: %d\n",
		stuff->hdisplay, stuff->hsyncstart,
		stuff->hsyncend, stuff->htotal);
	ErrorF("                   vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
		stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend,
		stuff->vtotal, stuff->flags);
    }
    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    REQUEST_AT_LEAST_SIZE(xXF86VidModeValidateModeLineReq);
    len = client->req_len - (sizeof(xXF86VidModeValidateModeLineReq) >> 2);
    if (len != stuff->privsize)
	return BadLength;

    status = MODE_OK;
    modetmp.Private = NULL;

    if (stuff->hsyncstart < stuff->hdisplay   ||
	stuff->hsyncend   < stuff->hsyncstart ||
	stuff->htotal     < stuff->hsyncend   ||
	stuff->vsyncstart < stuff->vdisplay   ||
	stuff->vsyncend   < stuff->vsyncstart ||
	stuff->vtotal     < stuff->vsyncend)
    {
	status = MODE_BAD;
	goto status_reply;
    }

    memcpy(&modetmp, mptr, sizeof(DisplayModeRec));

    modetmp.HDisplay   = stuff->hdisplay;
    modetmp.HSyncStart = stuff->hsyncstart;
    modetmp.HSyncEnd   = stuff->hsyncend;
    modetmp.HTotal     = stuff->htotal;
    modetmp.VDisplay   = stuff->vdisplay;
    modetmp.VSyncStart = stuff->vsyncstart;
    modetmp.VSyncEnd   = stuff->vsyncend;
    modetmp.VTotal     = stuff->vtotal;
    modetmp.Flags      = stuff->flags;
    modetmp.Private    = NULL;
    if (mptr->PrivSize && stuff->privsize) {
	if (mptr->PrivSize != stuff->privsize) {
	    status = MODE_BAD;
	    goto status_reply;
	}
    }
    if (mptr->PrivSize && mptr->Private) {
	modetmp.Private =
		(INT32 *)ALLOCATE_LOCAL(mptr->PrivSize * sizeof(INT32));
	if (stuff->privsize) {
	    if (xf86Verbose)
		ErrorF("ValidateModeLine - Request includes privates\n");
	    memcpy(modetmp.Private, &stuff[1], mptr->PrivSize * sizeof(INT32));
	} else
	    memcpy(modetmp.Private, mptr->Private,
		   mptr->PrivSize * sizeof(INT32));
    }

    /* Check that the mode is consistent with the monitor specs */
    if ((status = xf86CheckMode(vptr, &modetmp, vptr->monitor, FALSE)) != MODE_OK)
	goto status_reply;

    /* Check that the driver is happy with the mode */
    status = vptr->ValidMode(&modetmp, xf86Verbose, MODE_VID);

status_reply:
    if (modetmp.Private)
	DEALLOCATE_LOCAL(modetmp.Private);
    rep.type = X_Reply;
    rep.length = (SIZEOF(xXF86VidModeValidateModeLineReply)
   			 - SIZEOF(xGenericReply)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.status = status;
    if (client->swapped) {
        register int n;
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.status, n);
    }
    WriteToClient(client, sizeof(xXF86VidModeValidateModeLineReply), (char *)&rep);
    if (xf86Verbose)
	ErrorF("ValidateModeLine - Succeeded\n");
    return(client->noClientException);
}

static int
ProcXF86VidModeSwitchMode(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeSwitchModeReq);
    ScreenPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = screenInfo.screens[stuff->screen];
    if (xf86Info.dontZoom)
	return vidmodeErrorBase + XF86VidModeZoomLocked;

    REQUEST_SIZE_MATCH(xXF86VidModeSwitchModeReq);

    xf86ZoomViewport(vptr, (short)stuff->zoom);
    return (client->noClientException);
}

static int
ProcXF86VidModeSwitchToMode(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeSwitchToModeReq);
    ScrnInfoPtr vptr;
    DisplayModePtr curmptr, mptr;
    DisplayModeRec modetmp;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    if (xf86Info.dontZoom)
	return vidmodeErrorBase + XF86VidModeZoomLocked;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    curmptr = mptr = vptr->modes;

    REQUEST_SIZE_MATCH(xXF86VidModeSwitchToModeReq);

    if (MODEMATCH(mptr, stuff, vptr))
	return (client->noClientException);

    while ((mptr = mptr->next) != curmptr) {
	if (MODEMATCH(mptr, stuff, vptr)) {
	    if ((vptr->SwitchMode)(mptr)) {
		vptr->modes = mptr;
		vptr->frameX0 = (vptr->frameX1 +vptr->frameX0 -mptr->HDisplay)/2;
		vptr->frameX1 = vptr->frameX0 + mptr->HDisplay -1;
		if (vptr->frameX0 < 0) {
		    vptr->frameX0 = 0;
		    vptr->frameX1 = mptr->HDisplay -1;
		} else if (vptr->frameX1 >= vptr->virtualX) {
		    vptr->frameX0 = vptr->virtualX - mptr->HDisplay;
		    vptr->frameX1 = vptr->frameX0 + mptr->HDisplay -1;
		}
		vptr->frameY0 = (vptr->frameY1 +vptr->frameY0 -mptr->VDisplay)/2;
		vptr->frameY1 = vptr->frameY0 + mptr->VDisplay -1;
		if (vptr->frameY0 < 0) {
		    vptr->frameY0 = 0;
		    vptr->frameY1 = mptr->VDisplay -1;
		} else if (vptr->frameY1 >= vptr->virtualY) {
		    vptr->frameY0 = vptr->virtualY - mptr->VDisplay;
		    vptr->frameY1 = vptr->frameY0 + mptr->VDisplay -1;
		}
	    }
	    (vptr->AdjustFrame)(vptr->frameX0, vptr->frameY0);
	    return(client->noClientException);
	}
    }
    return BadValue;
}

static int
ProcXF86VidModeLockModeSwitch(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeLockModeSwitchReq);
    ScreenPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = screenInfo.screens[stuff->screen];
    if (xf86Info.dontZoom)
	return vidmodeErrorBase + XF86VidModeZoomLocked;

    REQUEST_SIZE_MATCH(xXF86VidModeLockModeSwitchReq);

    xf86LockZoom(vptr, (short)stuff->lock);
    return (client->noClientException);
}

static int
ProcXF86VidModeGetMonitor(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeGetMonitorReq);
    xXF86VidModeGetMonitorReply rep;
    register int n;
    ScrnInfoPtr vptr;
    MonPtr mptr;
    CARD32 *hsyncdata, *vsyncdata;
    int i;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->monitor;

    REQUEST_SIZE_MATCH(xXF86VidModeGetMonitorReq);
    rep.type = X_Reply;
    if (mptr->vendor)
	rep.vendorLength = strlen(mptr->vendor);
    else
	rep.vendorLength = 0;
    if (mptr->model)
	rep.modelLength = strlen(mptr->model);
    else
	rep.modelLength = 0;
    rep.length = (SIZEOF(xXF86VidModeGetMonitorReply) - SIZEOF(xGenericReply) +
		  (mptr->n_hsync + mptr->n_vrefresh) * sizeof(CARD32) +
	          ((rep.vendorLength + 3) & ~3) +
		  ((rep.modelLength + 3) & ~3)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.nhsync = mptr->n_hsync;
    rep.nvsync = mptr->n_vrefresh;
#if 0
    rep.bandwidth = (unsigned long)(mptr->bandwidth * 1e6);
#endif
    hsyncdata = ALLOCATE_LOCAL(mptr->n_hsync * sizeof(CARD32));
    if (!hsyncdata) {
	return BadAlloc;
    }
    vsyncdata = ALLOCATE_LOCAL(mptr->n_vrefresh * sizeof(CARD32));
    if (!vsyncdata) {
	DEALLOCATE_LOCAL(hsyncdata);
	return BadAlloc;
    }
    for (i = 0; i < mptr->n_hsync; i++) {
	hsyncdata[i] = (unsigned short)(mptr->hsync[i].lo * 100.0) |
		       (unsigned short)(mptr->hsync[i].hi * 100.0) << 16;
    }
    for (i = 0; i < mptr->n_vrefresh; i++) {
	vsyncdata[i] = (unsigned short)(mptr->vrefresh[i].lo * 100.0) |
		       (unsigned short)(mptr->vrefresh[i].hi * 100.0) << 16;
    }
    
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.bandwidth, n);
    }
    WriteToClient(client, SIZEOF(xXF86VidModeGetMonitorReply), (char *)&rep);
    client->pSwapReplyFunc = Swap32Write;
    WriteSwappedDataToClient(client, mptr->n_hsync * sizeof(CARD32),
			     hsyncdata);
    WriteSwappedDataToClient(client, mptr->n_vrefresh * sizeof(CARD32),
			     vsyncdata);
    if (rep.vendorLength)
	WriteToClient(client, rep.vendorLength, mptr->vendor);
    if (rep.modelLength)
	WriteToClient(client, rep.modelLength, mptr->model);
    DEALLOCATE_LOCAL(hsyncdata);
    DEALLOCATE_LOCAL(vsyncdata);
    return (client->noClientException);
}

static int
ProcXF86VidModeGetViewPort(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeGetViewPortReq);
    xXF86VidModeGetViewPortReply rep;
    register int n;
    ScrnInfoPtr vptr;
    MonPtr mptr;
    CARD32 *hsyncdata, *vsyncdata;
    int i;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->monitor;

    REQUEST_SIZE_MATCH(xXF86VidModeGetViewPortReq);
    rep.type = X_Reply;
    return (client->noClientException);
}

static int
ProcXF86VidModeSetViewPort(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeSetViewPortReq);
    register int n;
    ScrnInfoPtr vptr;
    MonPtr mptr;
    CARD32 *hsyncdata, *vsyncdata;
    int i;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->monitor;

    REQUEST_SIZE_MATCH(xXF86VidModeSetViewPortReq);
    return (client->noClientException);
}

static int
ProcXF86VidModeDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XF86VidModeQueryVersion:
	return ProcXF86VidModeQueryVersion(client);
    case X_XF86VidModeGetModeLine:
	return ProcXF86VidModeGetModeLine(client);
    case X_XF86VidModeGetAllModeLines:
	return ProcXF86VidModeGetAllModeLines(client);
    case X_XF86VidModeGetMonitor:
	return ProcXF86VidModeGetMonitor(client);
    case X_XF86VidModeValidateModeLine:
	return ProcXF86VidModeValidateModeLine(client);
    case X_XF86VidModeGetViewPort:
	return ProcXF86VidModeGetViewPort(client);
    default:
	if (!xf86VidModeEnabled)
	    return vidmodeErrorBase + XF86VidModeExtensionDisabled;
	if (xf86VidModeAllowNonLocal || LocalClient (client)) {
	    switch (stuff->data) {
	    case X_XF86VidModeAddModeLine:
		return ProcXF86VidModeAddModeLine(client);
	    case X_XF86VidModeDeleteModeLine:
		return ProcXF86VidModeDeleteModeLine(client);
	    case X_XF86VidModeModModeLine:
		return ProcXF86VidModeModModeLine(client);
	    case X_XF86VidModeSwitchMode:
		return ProcXF86VidModeSwitchMode(client);
	    case X_XF86VidModeSwitchToMode:
		return ProcXF86VidModeSwitchToMode(client);
	    case X_XF86VidModeLockModeSwitch:
		return ProcXF86VidModeLockModeSwitch(client);
	    case X_XF86VidModeSetViewPort:
		return ProcXF86VidModeSetViewPort(client);
	    default:
		return BadRequest;
	    }
	} else
	    return vidmodeErrorBase + XF86VidModeClientNotLocal;
    }
}

static int
SProcXF86VidModeQueryVersion(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xXF86VidModeQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcXF86VidModeQueryVersion(client);
}

static int
SProcXF86VidModeGetModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeGetModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeGetModeLineReq);
    swaps(&stuff->screen, n);
    return ProcXF86VidModeGetModeLine(client);
}

static int
SProcXF86VidModeGetAllModeLines(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeGetAllModeLinesReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeGetAllModeLinesReq);
    swaps(&stuff->screen, n);
    return ProcXF86VidModeGetAllModeLines(client);
}

static int
SProcXF86VidModeAddModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeAddModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xXF86VidModeAddModeLineReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->hdisplay, n);
    swaps(&stuff->hsyncstart, n);
    swaps(&stuff->hsyncend, n);
    swaps(&stuff->htotal, n);
    swaps(&stuff->vdisplay, n);
    swaps(&stuff->vsyncstart, n);
    swaps(&stuff->vsyncend, n);
    swaps(&stuff->vtotal, n);
    swapl(&stuff->flags, n);
    swapl(&stuff->privsize, n);
    SwapRestL(stuff);
    return ProcXF86VidModeAddModeLine(client);
}

static int
SProcXF86VidModeDeleteModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeDeleteModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xXF86VidModeDeleteModeLineReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->hdisplay, n);
    swaps(&stuff->hsyncstart, n);
    swaps(&stuff->hsyncend, n);
    swaps(&stuff->htotal, n);
    swaps(&stuff->vdisplay, n);
    swaps(&stuff->vsyncstart, n);
    swaps(&stuff->vsyncend, n);
    swaps(&stuff->vtotal, n);
    swapl(&stuff->flags, n);
    swapl(&stuff->privsize, n);
    SwapRestL(stuff);
    return ProcXF86VidModeDeleteModeLine(client);
}

static int
SProcXF86VidModeModModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeModModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xXF86VidModeModModeLineReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->hdisplay, n);
    swaps(&stuff->hsyncstart, n);
    swaps(&stuff->hsyncend, n);
    swaps(&stuff->htotal, n);
    swaps(&stuff->vdisplay, n);
    swaps(&stuff->vsyncstart, n);
    swaps(&stuff->vsyncend, n);
    swaps(&stuff->vtotal, n);
    swapl(&stuff->flags, n);
    swapl(&stuff->privsize, n);
    SwapRestL(stuff);
    return ProcXF86VidModeModModeLine(client);
}

static int
SProcXF86VidModeValidateModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeValidateModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xXF86VidModeValidateModeLineReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->hdisplay, n);
    swaps(&stuff->hsyncstart, n);
    swaps(&stuff->hsyncend, n);
    swaps(&stuff->htotal, n);
    swaps(&stuff->vdisplay, n);
    swaps(&stuff->vsyncstart, n);
    swaps(&stuff->vsyncend, n);
    swaps(&stuff->vtotal, n);
    swapl(&stuff->flags, n);
    swapl(&stuff->privsize, n);
    SwapRestL(stuff);
    return ProcXF86VidModeValidateModeLine(client);
}

static int
SProcXF86VidModeSwitchMode(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeSwitchModeReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeSwitchModeReq);
    swaps(&stuff->screen, n);
    swaps(&stuff->zoom, n);
    return ProcXF86VidModeSwitchMode(client);
}

static int
SProcXF86VidModeSwitchToMode(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeSwitchToModeReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeSwitchToModeReq);
    swaps(&stuff->screen, n);
    return ProcXF86VidModeSwitchToMode(client);
}

static int
SProcXF86VidModeLockModeSwitch(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeLockModeSwitchReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeLockModeSwitchReq);
    swaps(&stuff->screen, n);
    swaps(&stuff->lock, n);
    return ProcXF86VidModeLockModeSwitch(client);
}

static int
SProcXF86VidModeGetMonitor(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeGetMonitorReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeGetMonitorReq);
    swaps(&stuff->screen, n);
    return ProcXF86VidModeGetMonitor(client);
}

static int
SProcXF86VidModeGetViewPort(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeGetViewPortReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeGetViewPortReq);
    swaps(&stuff->screen, n);
    return ProcXF86VidModeGetViewPort(client);
}

static int
SProcXF86VidModeSetViewPort(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeSetViewPortReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeSetViewPortReq);
    swaps(&stuff->screen, n);
    return ProcXF86VidModeSetViewPort(client);
}

static int
SProcXF86VidModeDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XF86VidModeQueryVersion:
	return SProcXF86VidModeQueryVersion(client);
    case X_XF86VidModeGetModeLine:
	return SProcXF86VidModeGetModeLine(client);
    case X_XF86VidModeGetAllModeLines:
	return SProcXF86VidModeGetAllModeLines(client);
    case X_XF86VidModeGetMonitor:
	return SProcXF86VidModeGetMonitor(client);
    case X_XF86VidModeGetViewPort:
	return SProcXF86VidModeGetViewPort(client);
    case X_XF86VidModeValidateModeLine:
	return SProcXF86VidModeValidateModeLine(client);
    default:
	if (!xf86VidModeEnabled)
	    return vidmodeErrorBase + XF86VidModeExtensionDisabled;
	if (xf86VidModeAllowNonLocal || LocalClient(client)) {
	    switch (stuff->data) {
	    case X_XF86VidModeAddModeLine:
		return SProcXF86VidModeAddModeLine(client);
	    case X_XF86VidModeDeleteModeLine:
		return SProcXF86VidModeDeleteModeLine(client);
	    case X_XF86VidModeModModeLine:
		return SProcXF86VidModeModModeLine(client);
	    case X_XF86VidModeSwitchMode:
		return SProcXF86VidModeSwitchMode(client);
	    case X_XF86VidModeSwitchToMode:
		return SProcXF86VidModeSwitchToMode(client);
	    case X_XF86VidModeLockModeSwitch:
		return SProcXF86VidModeLockModeSwitch(client);
	    case X_XF86VidModeSetViewPort:
		return SProcXF86VidModeSetViewPort(client);
	    default:
		return BadRequest;
	    }
	} else
	    return vidmodeErrorBase + XF86VidModeClientNotLocal;
    }
}
