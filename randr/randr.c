/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#define NEED_REPLIES
#define NEED_EVENTS
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "servermd.h"
#include <X11/extensions/randr.h>
#include <X11/extensions/randrproto.h>
#include "randrstr.h"
#ifdef RENDER
#include <X11/extensions/render.h> 	/* we share subpixel order information */
#include "picturestr.h"
#endif
#include <X11/Xfuncproto.h>

/* From render.h */
#ifndef SubPixelUnknown
#define SubPixelUnknown 0
#endif

#define RR_VALIDATE
int	RRGeneration;
int	RRNScreens;

static RESTYPE	    ModeType, CrtcType, OutputType;

static int ProcRRQueryVersion (ClientPtr pClient);
static int ProcRRDispatch (ClientPtr pClient);
static int SProcRRDispatch (ClientPtr pClient);
static int SProcRRQueryVersion (ClientPtr pClient);

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

#if 0
static CARD8	RRReqCode;
static int	RRErrBase;
#endif
static int	RREventBase;
static RESTYPE ClientType, EventType; /* resource types for event masks */
static int	RRClientPrivateIndex;

typedef struct _RRTimes {
    TimeStamp	setTime;
    TimeStamp	configTime;
} RRTimesRec, *RRTimesPtr;

typedef struct _RRClient {
    int		major_version;
    int		minor_version;
/*  RRTimesRec	times[0]; */
} RRClientRec, *RRClientPtr;

/*
 * each window has a list of clients requesting
 * RRNotify events.  Each client has a resource
 * for each window it selects RRNotify input for,
 * this resource is used to delete the RRNotifyRec
 * entry from the per-window queue.
 */

typedef struct _RREvent *RREventPtr;

typedef struct _RREvent {
    RREventPtr  next;
    ClientPtr	client;
    WindowPtr	window;
    XID		clientResource;
    int		mask;
} RREventRec;

int	rrPrivIndex = -1;

#define GetRRClient(pClient)    ((RRClientPtr) (pClient)->devPrivates[RRClientPrivateIndex].ptr)
#define rrClientPriv(pClient)	RRClientPtr pRRClient = GetRRClient(pClient)

static Bool
RRClientKnowsRates (ClientPtr	pClient)
{
    rrClientPriv(pClient);

    return (pRRClient->major_version > 1 ||
	    (pRRClient->major_version == 1 && pRRClient->minor_version >= 1));
}

static void
RRClientCallback (CallbackListPtr	*list,
		  pointer		closure,
		  pointer		data)
{
    NewClientInfoRec	*clientinfo = (NewClientInfoRec *) data;
    ClientPtr		pClient = clientinfo->client;
    rrClientPriv(pClient);
    RRTimesPtr		pTimes = (RRTimesPtr) (pRRClient + 1);
    int			i;

    pRRClient->major_version = 0;
    pRRClient->minor_version = 0;
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	ScreenPtr   pScreen = screenInfo.screens[i];
	rrScrPriv(pScreen);

	if (pScrPriv)
	{
	    pTimes[i].setTime = pScrPriv->lastSetTime;
	    pTimes[i].configTime = pScrPriv->lastConfigTime;
	}
    }
}

static void
RRResetProc (ExtensionEntry *extEntry)
{
}
    
static Bool
RRCloseScreen (int i, ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    int		    j;

    unwrap (pScrPriv, pScreen, CloseScreen);
    for (j = pScrPriv->numCrtcs - 1; j >= 0; j--)
	RRCrtcDestroy (pScrPriv->crtcs[j]);
    for (j = pScrPriv->numOutputs - 1; j >= 0; j--)
	RROutputDestroy (pScrPriv->outputs[j]);
    for (j = pScrPriv->numModes - 1; j >= 0; j--)
	RRModeDestroy (pScrPriv->modes[j]);
    
    xfree (pScrPriv);
    RRNScreens -= 1;	/* ok, one fewer screen with RandR running */
    return (*pScreen->CloseScreen) (i, pScreen);    
}

static void
SRRScreenChangeNotifyEvent(xRRScreenChangeNotifyEvent *from,
			   xRRScreenChangeNotifyEvent *to)
{
    to->type = from->type;
    to->rotation = from->rotation;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    cpswaps(from->sizeID, to->sizeID);
    cpswaps(from->widthInPixels, to->widthInPixels);
    cpswaps(from->heightInPixels, to->heightInPixels);
    cpswaps(from->widthInMillimeters, to->widthInMillimeters);
    cpswaps(from->heightInMillimeters, to->heightInMillimeters);
    cpswaps(from->subpixelOrder, to->subpixelOrder);
}

#if 0
static void
SRRMonitorChangeNotifyEvent(xRRMonitorChangeNotifyEvent *from,
			    xRRMonitorChangeNotifyEvent *to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    cpswaps(from->monitor, to->monitor);
    cpswaps(from->modeID, to->modeID);
    cpswaps(from->rotation, to->rotation);
    cpswaps(from->subpixelOrder, to->subpixelOrder);
    cpswaps(from->x, to->x);
    cpswaps(from->y, to->y);
}
#endif

static void
SRRNotifyEvent (xEvent *from,
		xEvent *to)
{
    switch (from->u.u.detail) {
#if 0
    case RRNotify_MonitorChange:
	SRRMonitorChangeNotifyEvent ((xRRMonitorChangeNotifyEvent *) from,
				     (xRRMonitorChangeNotifyEvent *) to);
	break;
#endif
    default:
	break;
    }
}

static int
RRModeDestroyResource (pointer value, XID pid)
{
    RRModeDestroy ((RRModePtr) value);
    return 1;
}

static int
RRCrtcDestroyResource (pointer value, XID pid)
{
    RRCrtcDestroy ((RRCrtcPtr) value);
    return 1;
}

static int
RROutputDestroyResource (pointer value, XID pid)
{
    RROutputDestroy ((RROutputPtr) value);
    return 1;
}

Bool RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr   pScrPriv;

    if (RRGeneration != serverGeneration)
    {
	ModeType = CreateNewResourceType (RRModeDestroyResource);
	if (!ModeType)
	    return FALSE;
	CrtcType = CreateNewResourceType (RRCrtcDestroyResource);
	if (!ModeType)
	    return FALSE;
	OutputType = CreateNewResourceType (RROutputDestroyResource);
	if (!ModeType)
	    return FALSE;
	if ((rrPrivIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	RRGeneration = serverGeneration;
#ifdef XResExtension
	RegisterResourceName (ModeType, "MODE");
	RegisterResourceName (CrtcType, "CRTC");
	RegisterResourceName (OutputType, "OUTPUT");
#endif
    }

    pScrPriv = (rrScrPrivPtr) xalloc (sizeof (rrScrPrivRec));
    if (!pScrPriv)
	return FALSE;

    SetRRScreen(pScreen, pScrPriv);

    /*
     * Calling function best set these function vectors
     */
    pScrPriv->rrCrtcSet = 0;
    pScrPriv->rrGetInfo = 0;
    pScrPriv->maxWidth = pScrPriv->minWidth = pScreen->width;
    pScrPriv->maxHeight = pScrPriv->minHeight = pScreen->height;
#ifdef RANDR_SCREEN_INTERFACE    
    pScrPriv->rrSetConfig = 0;
    pScrPriv->reqWidth = pScreen->width;
    pScrPriv->reqHeight = pScreen->height;
#endif
    
    /*
     * This value doesn't really matter -- any client must call
     * GetScreenInfo before reading it which will automatically update
     * the time
     */
    pScrPriv->lastSetTime = currentTime;
    pScrPriv->lastConfigTime = currentTime;
    
    wrap (pScrPriv, pScreen, CloseScreen, RRCloseScreen);

    pScrPriv->numModes = 0;
    pScrPriv->modes = NULL;
    pScrPriv->numOutputs = 0;
    pScrPriv->outputs = NULL;
    pScrPriv->numCrtcs = 0;
    pScrPriv->crtcs = NULL;
    
    RRNScreens += 1;	/* keep count of screens that implement randr */
    return TRUE;
}

/*ARGSUSED*/
static int
RRFreeClient (pointer data, XID id)
{
    RREventPtr   pRREvent;
    WindowPtr	    pWin;
    RREventPtr   *pHead, pCur, pPrev;

    pRREvent = (RREventPtr) data;
    pWin = pRREvent->window;
    pHead = (RREventPtr *) LookupIDByType(pWin->drawable.id, EventType);
    if (pHead) {
	pPrev = 0;
	for (pCur = *pHead; pCur && pCur != pRREvent; pCur=pCur->next)
	    pPrev = pCur;
	if (pCur)
	{
	    if (pPrev)
	    	pPrev->next = pRREvent->next;
	    else
	    	*pHead = pRREvent->next;
	}
    }
    xfree ((pointer) pRREvent);
    return 1;
}

/*ARGSUSED*/
static int
RRFreeEvents (pointer data, XID id)
{
    RREventPtr   *pHead, pCur, pNext;

    pHead = (RREventPtr *) data;
    for (pCur = *pHead; pCur; pCur = pNext) {
	pNext = pCur->next;
	FreeResource (pCur->clientResource, ClientType);
	xfree ((pointer) pCur);
    }
    xfree ((pointer) pHead);
    return 1;
}

void
RRExtensionInit (void)
{
    ExtensionEntry *extEntry;

    if (RRNScreens == 0) return;

    RRClientPrivateIndex = AllocateClientPrivateIndex ();
    if (!AllocateClientPrivate (RRClientPrivateIndex,
				sizeof (RRClientRec) +
				screenInfo.numScreens * sizeof (RRTimesRec)))
	return;
    if (!AddCallback (&ClientStateCallback, RRClientCallback, 0))
	return;

    ClientType = CreateNewResourceType(RRFreeClient);
    if (!ClientType)
	return;
    EventType = CreateNewResourceType(RRFreeEvents);
    if (!EventType)
	return;
    extEntry = AddExtension (RANDR_NAME, RRNumberEvents, RRNumberErrors,
			     ProcRRDispatch, SProcRRDispatch,
			     RRResetProc, StandardMinorOpcode);
    if (!extEntry)
	return;
#if 0
    RRReqCode = (CARD8) extEntry->base;
    RRErrBase = extEntry->errorBase;
#endif
    RREventBase = extEntry->eventBase;
    EventSwapVector[RREventBase + RRScreenChangeNotify] = (EventSwapPtr) 
      SRRScreenChangeNotifyEvent;
    EventSwapVector[RREventBase + RRNotify] = (EventSwapPtr)
	SRRNotifyEvent;

    return;
}
		
static void
DeliverScreenEvent (ClientPtr client, WindowPtr pWin, ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    xRRScreenChangeNotifyEvent	se;
    RRCrtcPtr	crtc = pScrPriv->numCrtcs ? pScrPriv->crtcs[0] : NULL;
    RROutputPtr	output = pScrPriv->numOutputs ? pScrPriv->outputs[0] : NULL;
    RRModePtr	mode = crtc ? crtc->mode : NULL;
    WindowPtr	pRoot = WindowTable[pScreen->myNum];
    int		i;
    
    se.type = RRScreenChangeNotify + RREventBase;
    se.rotation = (CARD8) (crtc ? crtc->rotation : RR_Rotate_0);
    se.timestamp = pScrPriv->lastSetTime.milliseconds;
    se.sequenceNumber = client->sequence;
    se.configTimestamp = pScrPriv->lastConfigTime.milliseconds;
    se.root =  pRoot->drawable.id;
    se.window = pWin->drawable.id;
#ifdef RENDER
    se.subpixelOrder = PictureGetSubpixelOrder (pScreen);
#else
    se.subpixelOrder = SubPixelUnknown;
#endif

    se.sequenceNumber = client->sequence;
    if (mode) 
    {
	se.sizeID = -1;
	for (i = 0; i < output->numModes; i++)
	    if (mode == output->modes[i])
	    {
		se.sizeID = i;
		break;
	    }
	se.widthInPixels = mode->mode.width;
	se.heightInPixels = mode->mode.height;
	se.widthInMillimeters = mode->mode.mmWidth;
	se.heightInMillimeters = mode->mode.mmHeight;
    }
    else
    {
	/*
	 * This "shouldn't happen", but a broken DDX can
	 * forget to set the current configuration on GetInfo
	 */
	se.sizeID = 0xffff;
	se.widthInPixels = 0;
	se.heightInPixels = 0;
	se.widthInMillimeters = 0;
	se.heightInMillimeters = 0;
    }    
    WriteEventsToClient (client, 1, (xEvent *) &se);
}

static void
DeliverCrtcEvent (ClientPtr client, WindowPtr pWin, RRCrtcPtr crtc)
{
}

static void
DeliverOutputEvent (ClientPtr client, WindowPtr pWin, RROutputPtr output)
{
}

static int
TellChanged (WindowPtr pWin, pointer value)
{
    RREventPtr			*pHead, pRREvent;
    ClientPtr			client;
    ScreenPtr			pScreen = pWin->drawable.pScreen;
    rrScrPriv(pScreen);
    int				i;

    pHead = (RREventPtr *) LookupIDByType (pWin->drawable.id, EventType);
    if (!pHead)
	return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) 
    {
	client = pRREvent->client;
	if (client == serverClient || client->clientGone)
	    continue;

	if (pRREvent->mask & RRScreenChangeNotifyMask)
	    DeliverScreenEvent (client, pWin, pScreen);
	
	if (pRREvent->mask & RRCrtcChangeNotifyMask)
	{
	    for (i = 0; i < pScrPriv->numCrtcs; i++)
	    {
		RRCrtcPtr   crtc = pScrPriv->crtcs[i];
		if (crtc->changed)
		    DeliverCrtcEvent (client, pWin, crtc);
	    }
	}
	
	if (pRREvent->mask & RROutputChangeNotifyMask)
	{
	    for (i = 0; i < pScrPriv->numOutputs; i++)
	    {
		RROutputPtr   output = pScrPriv->outputs[i];
		if (output->changed)
		    DeliverOutputEvent (client, pWin, output);
	    }
	}
    }
    return WT_WALKCHILDREN;
}

void
RRTellChanged (ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    int i;
    
    if (pScrPriv->changed)
    {
	UpdateCurrentTime ();
	pScrPriv->lastConfigTime = currentTime;
	WalkTree (pScreen, TellChanged, (pointer) pScreen);
	pScrPriv->changed = FALSE;
	for (i = 0; i < pScrPriv->numOutputs; i++)
	    pScrPriv->outputs[i]->changed = FALSE;
	for (i = 0; i < pScrPriv->numCrtcs; i++)
	    pScrPriv->crtcs[i]->changed = FALSE;
    }
}

RRModePtr
RRModeGet (ScreenPtr	pScreen,
	   xRRModeInfo	*modeInfo,
	   char		*name)
{
    rrScrPriv (pScreen);
    int	i;
    RRModePtr	mode;

    for (i = 0; i < pScrPriv->numModes; i++)
    {
	mode = pScrPriv->modes[i];
	if (!memcmp (modeInfo, &mode->mode, sizeof (xRRModeInfo)) &&
	    !memcmp (name, mode->name, modeInfo->nameLength))
	{
	    ++mode->refcnt;
	    return mode;
	}
    }
    mode = xalloc (sizeof (RRModeRec) + modeInfo->nameLength + 1);
    mode->refcnt = 1;
    mode->mode = *modeInfo;
    mode->name = (char *) (mode + 1);
    memcpy (mode->name, name, modeInfo->nameLength);
    mode->name[modeInfo->nameLength] = '\0';
    mode->id = FakeClientID(0);
    if (!AddResource (mode->id, ModeType, (pointer) mode))
	return NULL;
    ++mode->refcnt;
    pScrPriv->changed = TRUE;
    return mode;
}

void
RRModeDestroy (RRModePtr mode)
{
    if (--mode->refcnt > 0)
	return;
    xfree (mode);
}

/*
 * Return the first output which is connected to an active CRTC
 * Used in emulating 1.0 behaviour
 */
static RROutputPtr
RRFirstOutput (ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    RROutputPtr		    output;
    int	i, j;
    
    for (i = 0; i < pScrPriv->numCrtcs; i++)
    {
	RRCrtcPtr   crtc = pScrPriv->crtcs[i];
	for (j = 0; j < pScrPriv->numOutputs; j++)
	{
	    output = pScrPriv->outputs[j];
	    if (output->crtc == crtc)
		return output;
	}
    }
    return NULL;
}

#ifdef RANDR_SCREEN_INTERFACE

static Bool
RROldModeAdd (RROutputPtr output, RRScreenSizePtr size, int refresh)
{
    xRRModeInfo	modeInfo;
    char	name[100];
    RRModePtr	mode;
    int		i;
    RRModePtr   *modes;
    
    memset (&modeInfo, '\0', sizeof (modeInfo));
    sprintf (name, "%dx%d", size->width, size->height);
    
    modeInfo.width = size->width;
    modeInfo.height = size->height;
    modeInfo.mmWidth = size->mmWidth;
    modeInfo.mmHeight = size->mmHeight;
    modeInfo.hTotal = size->width;
    modeInfo.vTotal = size->height;
    modeInfo.dotClock = ((CARD32) size->width * (CARD32) size->width *
			 (CARD32) refresh);
    modeInfo.nameLength = strlen (name);
    mode = RRModeGet (output->pScreen, &modeInfo, name);
    if (!mode)
	return FALSE;
    for (i = 0; i < output->numModes; i++)
	if (output->modes[i] == mode)
	    return TRUE;
    
    if (output->numModes)
	modes = xrealloc (output->modes, 
			  (output->numModes + 1) * sizeof (RRModePtr));
    else
	modes = xalloc (sizeof (RRModePtr));
    if (!modes)
	return FALSE;
    modes[output->numModes++] = mode;
    output->modes = modes;
    output->changed = TRUE;
    return TRUE;
}

static void
RRScanOldConfig (ScreenPtr pScreen, Rotation rotations)
{
    rrScrPriv(pScreen);
    RROutputPtr	output = RRFirstOutput (pScreen);
    RRCrtcPtr	crtc;
    int		i;
    CARD16	minWidth = MAXSHORT, minHeight = MAXSHORT;
    CARD16	maxWidth = 0, maxHeight = 0;
    
    if (!output)
	return;
    crtc = output->crtc;

    /* check rotations */
    if (rotations != crtc->rotations)
    {
        crtc->rotations = rotations;
	crtc->changed = TRUE;
	pScrPriv->changed = TRUE;
    }
	
    /* regenerate mode list */
    for (i = 0; i < pScrPriv->nSizes; i++)
    {
	RRScreenSizePtr	size = &pScrPriv->pSizes[i];
	int		r;

	if (size->nRates)
	    for (r = 0; r < size->nRates; r++)
		RROldModeAdd (output, size, size->pRates[r].rate);
	else
	    RROldModeAdd (output, size, 0);
    }
	    
    /* find size bounds */
    for (i = 0; i < output->numModes; i++) 
    {
	RRModePtr   mode = output->modes[i];
        CARD16	    width = mode->mode.width;
        CARD16	    height = mode->mode.height;
	
	if (width < minWidth) minWidth = width;
	if (width > maxWidth) maxWidth = width;
	if (height < minHeight) minHeight = height;
	if (height > maxHeight) maxHeight = height;
    }

    if (minWidth != pScrPriv->minWidth) {
	pScrPriv->minWidth = minWidth; pScrPriv->changed = TRUE;
    }
    if (maxWidth != pScrPriv->maxWidth) {
	pScrPriv->maxWidth = maxWidth; pScrPriv->changed = TRUE;
    }
    if (minHeight != pScrPriv->minHeight) {
	pScrPriv->minHeight = minHeight; pScrPriv->changed = TRUE;
    }
    if (maxHeight != pScrPriv->maxHeight) {
	pScrPriv->maxHeight = maxHeight; pScrPriv->changed = TRUE;
    }
}
#endif

static Bool
RRGetInfo (ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    Rotation	    rotations;
    int		    i;

    for (i = 0; i < pScrPriv->numOutputs; i++)
	pScrPriv->outputs[i]->changed = FALSE;
    for (i = 0; i < pScrPriv->numCrtcs; i++)
	pScrPriv->crtcs[i]->changed = FALSE;
    
    rotations = 0;
    pScrPriv->changed = FALSE;
    
    if (!(*pScrPriv->rrGetInfo) (pScreen, &rotations))
	return FALSE;

#if RANDR_SCREEN_INTERFACE
    if (pScrPriv->nSizes)
	RRScanOldConfig (pScreen, rotations);
#endif
    RRTellChanged (pScreen);
    return TRUE;
}

static void
RRSendConfigNotify (ScreenPtr pScreen)
{
    WindowPtr	pWin = WindowTable[pScreen->myNum];
    xEvent	event;

    event.u.u.type = ConfigureNotify;
    event.u.configureNotify.window = pWin->drawable.id;
    event.u.configureNotify.aboveSibling = None;
    event.u.configureNotify.x = 0;
    event.u.configureNotify.y = 0;

    /* XXX xinerama stuff ? */
    
    event.u.configureNotify.width = pWin->drawable.width;
    event.u.configureNotify.height = pWin->drawable.height;
    event.u.configureNotify.borderWidth = wBorderWidth (pWin);
    event.u.configureNotify.override = pWin->overrideRedirect;
    DeliverEvents(pWin, &event, 1, NullWindow);
}

static int
ProcRRQueryVersion (ClientPtr client)
{
    xRRQueryVersionReply rep;
    register int n;
    REQUEST(xRRQueryVersionReq);
    rrClientPriv(client);

    REQUEST_SIZE_MATCH(xRRQueryVersionReq);
    pRRClient->major_version = stuff->majorVersion;
    pRRClient->minor_version = stuff->minorVersion;
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    /*
     * Report the current version; the current
     * spec says they're all compatible after 1.0
     */
    rep.majorVersion = RANDR_MAJOR;
    rep.minorVersion = RANDR_MINOR;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.majorVersion, n);
	swapl(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xRRQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}


extern char	*ConnectionInfo;

static int padlength[4] = {0, 3, 2, 1};

static void
RREditConnectionInfo (ScreenPtr pScreen)
{
    xConnSetup	    *connSetup;
    char	    *vendor;
    xPixmapFormat   *formats;
    xWindowRoot	    *root;
    xDepth	    *depth;
    xVisualType	    *visual;
    int		    screen = 0;
    int		    d;

    connSetup = (xConnSetup *) ConnectionInfo;
    vendor = (char *) connSetup + sizeof (xConnSetup);
    formats = (xPixmapFormat *) ((char *) vendor +
				 connSetup->nbytesVendor +
				 padlength[connSetup->nbytesVendor & 3]);
    root = (xWindowRoot *) ((char *) formats +
			    sizeof (xPixmapFormat) * screenInfo.numPixmapFormats);
    while (screen != pScreen->myNum)
    {
	depth = (xDepth *) ((char *) root + 
			    sizeof (xWindowRoot));
	for (d = 0; d < root->nDepths; d++)
	{
	    visual = (xVisualType *) ((char *) depth +
				      sizeof (xDepth));
	    depth = (xDepth *) ((char *) visual +
				depth->nVisuals * sizeof (xVisualType));
	}
	root = (xWindowRoot *) ((char *) depth);
	screen++;
    }
    root->pixWidth = pScreen->width;
    root->pixHeight = pScreen->height;
    root->mmWidth = pScreen->mmWidth;
    root->mmHeight = pScreen->mmHeight;
}

typedef struct _RR10Data {
    RRScreenSizePtr sizes;
    int		    nsize;
    int		    nrefresh;
    int		    size;
    CARD16	    refresh;
} RR10DataRec, *RR10DataPtr;

static CARD16
RRVerticalRefresh (xRRModeInfo *mode)
{
    CARD32  refresh;
    if (!mode->hTotal || !mode->vTotal)
	return 0;
    refresh = mode->dotClock / (mode->hTotal * mode->vTotal);
    if (refresh > 0xffff)
	refresh = 0xffff;
    return (CARD16) refresh;
}

/*
 * Convert 1.2 monitor data into 1.0 screen data
 */
static RR10DataPtr
RR10GetData (ScreenPtr pScreen, RROutputPtr output)
{
    RR10DataPtr	    data;
    RRScreenSizePtr size;
    int		    nmode = output->numModes;
    int		    i, j, k;
    RRScreenRatePtr refresh;
    CARD16	    vRefresh;
    RRModePtr	    mode;

    /* Make sure there is plenty of space for any combination */
    data = malloc (sizeof (RR10DataRec) + 
		   sizeof (RRScreenSize) * nmode + 
		   sizeof (RRScreenRate) * nmode);
    if (!data)
	return NULL;
    size = (RRScreenSizePtr) (data + 1);
    refresh = (RRScreenRatePtr) (size + nmode);
    data->sizes = size;
    data->nsize = 0;
    data->nrefresh = 0;
    data->size = 0;
    data->refresh = 0;
    for (i = 0; i < output->numModes; i++)
    {
	mode = output->modes[i];
	for (j = 0; j < data->nsize; j++)
	    if (mode->mode.width == size[j].width &&
		mode->mode.height == size[j].height)
		break;
	if (j == data->nsize)
	{
	    size[j].id = j;
	    size[j].width = mode->mode.width;
	    size[j].height = mode->mode.height;
	    size[j].mmWidth = mode->mode.mmWidth;
	    size[j].mmHeight = mode->mode.mmHeight;
	    size[j].nRates = 0;
	    size[j].pRates = &refresh[data->nrefresh];
	    data->nsize++;
	}
	vRefresh = RRVerticalRefresh (&mode->mode);
	for (k = 0; k < size[j].nRates; k++)
	    if (vRefresh == size[j].pRates[k].rate)
		break;
	if (k == size[j].nRates)
	{
	    size[j].pRates[k].rate = vRefresh;
	    size[j].pRates[k].mode = mode;
	    size[j].nRates++;
	    data->nrefresh++;
	}
	if (mode == output->crtc->mode)
	{
	    data->size = j;
	    data->refresh = vRefresh;
	}
    }
    return data;
}

static int
ProcRRGetScreenInfo (ClientPtr client)
{
    REQUEST(xRRGetScreenInfoReq);
    xRRGetScreenInfoReply   rep;
    WindowPtr	    	    pWin;
    int			    n;
    ScreenPtr		    pScreen;
    rrScrPrivPtr	    pScrPriv;
    CARD8		    *extra;
    unsigned long	    extraLen;
    RROutputPtr		    output;

    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    rep.pad = 0;
    
    if (pScrPriv)
	RRGetInfo (pScreen);

    output = RRFirstOutput (pScreen);
    
    if (!pScrPriv || !output)
    {
	rep.type = X_Reply;
	rep.setOfRotations = RR_Rotate_0;;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.root = WindowTable[pWin->drawable.pScreen->myNum]->drawable.id;
	rep.timestamp = currentTime.milliseconds;
	rep.configTimestamp = currentTime.milliseconds;
	rep.nSizes = 0;
	rep.sizeID = 0;
	rep.rotation = RR_Rotate_0;
	rep.rate = 0;
	rep.nrateEnts = 0;
	extra = 0;
	extraLen = 0;
    }
    else
    {
	int			i, j;
	xScreenSizes		*size;
	CARD16			*rates;
	CARD8			*data8;
	Bool			has_rate = RRClientKnowsRates (client);
	RR10DataPtr		pData;
	RRScreenSizePtr		pSize;
    
	pData = RR10GetData (pScreen, output);
	if (!pData)
	    return BadAlloc;
	
	rep.type = X_Reply;
	rep.setOfRotations = output->crtc->rotations;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.root = WindowTable[pWin->drawable.pScreen->myNum]->drawable.id;
	rep.timestamp = pScrPriv->lastSetTime.milliseconds;
	rep.configTimestamp = pScrPriv->lastConfigTime.milliseconds;
	rep.rotation = output->crtc->rotation;
	rep.nSizes = pData->nsize;
        rep.nrateEnts = pData->nrefresh + pData->nsize;
	rep.sizeID = pData->size;
	rep.rate = pData->refresh;

	extraLen = (rep.nSizes * sizeof (xScreenSizes) +
		    rep.nrateEnts * sizeof (CARD16));

	extra = (CARD8 *) xalloc (extraLen);
	if (!extra)
	{
	    xfree (pData);
	    return BadAlloc;
	}
	/*
	 * First comes the size information
	 */
	size = (xScreenSizes *) extra;
	rates = (CARD16 *) (size + rep.nSizes);
	for (i = 0; i < pData->nsize; i++)
	{
	    pSize = &pData->sizes[i];
	    size->widthInPixels = pSize->width;
	    size->heightInPixels = pSize->height;
	    size->widthInMillimeters = pSize->mmWidth;
	    size->heightInMillimeters = pSize->mmHeight;
	    if (client->swapped)
	    {
	        swaps (&size->widthInPixels, n);
	        swaps (&size->heightInPixels, n);
	        swaps (&size->widthInMillimeters, n);
	        swaps (&size->heightInMillimeters, n);
	    }
	    size++;
	    if (has_rate)
	    {
		*rates = pSize->nRates;
		if (client->swapped)
		{
		    swaps (rates, n);
		}
		rates++;
		for (j = 0; j < pSize->nRates; j++)
		{
		    *rates = pSize->pRates[j].rate;
		    if (client->swapped)
		    {
			swaps (rates, n);
		    }
		    rates++;
		}
	    }
	}
        xfree (pData);
	
	data8 = (CARD8 *) rates;

	if (data8 - (CARD8 *) extra != extraLen)
	    FatalError ("RRGetScreenInfo bad extra len %ld != %ld\n",
			(unsigned long)(data8 - (CARD8 *) extra), extraLen);
	rep.length =  (extraLen + 3) >> 2;
    }
    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.timestamp, n);
	swaps(&rep.rotation, n);
	swaps(&rep.nSizes, n);
	swaps(&rep.sizeID, n);
	swaps(&rep.rate, n);
	swaps(&rep.nrateEnts, n);
    }
    WriteToClient(client, sizeof(xRRGetScreenInfoReply), (char *)&rep);
    if (extraLen)
    {
	WriteToClient (client, extraLen, (char *) extra);
	xfree (extra);
    }
    return (client->noClientException);
}

#if 0
static int
RRMonitorSetMode (ScreenPtr pScreen, RRMonitorPtr pMonitor, 
		  RRModePtr pMode, int x, int y, Rotation rotation,
		  TimeStamp time)
{
    rrScrPriv(pScreen);
    short   oldWidth, oldHeight;
    
    oldWidth = pScreen->width;
    oldHeight = pScreen->height;
    
    /*
     * call out to ddx routine to effect the change
     */
    if (pScrPriv->rrSetScreenSize && pScrPriv->rrSetMode)
    {
	xScreenSizes	oldSize;
	if (!(*pScrPriv->rrSetMode) (pScreen, 0, NULL, 0, 0, RR_Rotate_0))
	    return RRSetConfigFailed;
	oldSize.widthInPixels = pScreen->width;
	oldSize.heightInPixels = pScreen->width;
	oldSize.widthInMillimeters = pScreen->mmWidth;
	oldSize.heightInMillimeters = pScreen->mmHeight;
	if (!(*pScrPriv->rrSetScreenSize) (pScreen,
					   pMode->mode.width,
					   pMode->mode.height,
					   pMode->mode.widthInMillimeters,
					   pMode->mode.heightInMillimeters))
	{
	    (void) (*pScrPriv->rrSetMode) (pScreen, 0, pMonitor->pMode,
					   pMonitor->x, pMonitor->y,
					   pMonitor->rotation);
	    return RRSetConfigFailed;
	}
	if (!(*pScrPriv->rrSetMode) (pScreen, 0, pMode, 0, 0, rotation))
	{
	    (void) (*pScrPriv->rrSetScreenSize) (pScreen,
						 oldSize.widthInPixels,
						 oldSize.heightInPixels,
						 oldSize.widthInMillimeters,
						 oldSize.heightInMillimeters);
	    (void) (*pScrPriv->rrSetMode) (pScreen, 0, pMonitor->pMode,
					   pMonitor->x, pMonitor->y,
					   pMonitor->rotation);
	    return RRSetConfigFailed;
	}
    }
#ifdef RANDR_SCREEN_INTERFACE
    else if (pScrPriv->rrSetConfig)
    {
	int rate = RRVerticalRefresh (&pMode->mode);
	RRScreenSizeRec	size;

	size.width = pMode->mode.width;
	size.height = pMode->mode.height;
	size.mmWidth = pMode->mode.widthInMillimeters;
	size.mmHeight = pMode->mode.heightInMillimeters;
	if (!(*pScrPriv->rrSetConfig) (pScreen, rotation, rate, &size))
	    return RRSetConfigFailed;
    }
#endif
    else
	return RRSetConfigFailed;
    
    /*
     * set current extension configuration pointers
     */
    RRSetCurrentMode (pMonitor, pMode, 0, 0, rotation);
    
    /*
     * Deliver ScreenChangeNotify events whenever
     * the configuration is updated
     */
    WalkTree (pScreen, TellChanged, (pointer) pScreen);
    
    /*
     * Deliver ConfigureNotify events when root changes
     * pixel size
     */
    if (oldWidth != pScreen->width || oldHeight != pScreen->height)
	RRSendConfigNotify (pScreen);
    RREditConnectionInfo (pScreen);
    
    /*
     * Fix pointer bounds and location
     */
    ScreenRestructured (pScreen);
    pScrPriv->lastSetTime = time;
    return RRSetConfigSuccess;
}
#endif

static int
ProcRRSetScreenConfig (ClientPtr client)
{
    REQUEST(xRRSetScreenConfigReq);
    xRRSetScreenConfigReply rep;
    DrawablePtr		    pDraw;
    int			    n;
    ScreenPtr		    pScreen;
    rrScrPrivPtr	    pScrPriv;
    TimeStamp		    configTime;
    TimeStamp		    time;
    int			    i;
    Rotation		    rotation;
    int			    rate;
    Bool		    has_rate;
    RROutputPtr		    output;
    RRModePtr		    mode;
    RR10DataPtr		    pData = NULL;
    RRScreenSizePtr    	    pSize;
    
    UpdateCurrentTime ();

    if (RRClientKnowsRates (client))
    {
	REQUEST_SIZE_MATCH (xRRSetScreenConfigReq);
	has_rate = TRUE;
    }
    else
    {
	REQUEST_SIZE_MATCH (xRR1_0SetScreenConfigReq);
	has_rate = FALSE;
    }
    
    SECURITY_VERIFY_DRAWABLE(pDraw, stuff->drawable, client,
			     SecurityWriteAccess);

    pScreen = pDraw->pScreen;

    pScrPriv = rrGetScrPriv(pScreen);
    
    time = ClientTimeToServerTime(stuff->timestamp);
    configTime = ClientTimeToServerTime(stuff->configTimestamp);
    
    if (!pScrPriv)
    {
	time = currentTime;
	rep.status = RRSetConfigFailed;
	goto sendReply;
    }
    if (!RRGetInfo (pScreen))
	return BadAlloc;
    
    output = RRFirstOutput (pScreen);
    if (!output)
    {
	time = currentTime;
	rep.status = RRSetConfigFailed;
	goto sendReply;
    }
    
    /*
     * if the client's config timestamp is not the same as the last config
     * timestamp, then the config information isn't up-to-date and
     * can't even be validated
     */
    if (CompareTimeStamps (configTime, pScrPriv->lastConfigTime) != 0)
    {
	rep.status = RRSetConfigInvalidConfigTime;
	goto sendReply;
    }
    
    pData = RR10GetData (pScreen, output);
    if (!pData)
	return BadAlloc;
    
    if (stuff->sizeID >= pData->nsize)
    {
	/*
	 * Invalid size ID
	 */
	client->errorValue = stuff->sizeID;
	xfree (pData);
	return BadValue;
    }
    pSize = &pData->sizes[stuff->sizeID];
    
    /*
     * Validate requested rotation
     */
    rotation = (Rotation) stuff->rotation;

    /* test the rotation bits only! */
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_90:
    case RR_Rotate_180:
    case RR_Rotate_270:
	break;
    default:
	/*
	 * Invalid rotation
	 */
	client->errorValue = stuff->rotation;
	xfree (pData);
	return BadValue;
    }

    if ((~output->crtc->rotations) & rotation)
    {
	/*
	 * requested rotation or reflection not supported by screen
	 */
	client->errorValue = stuff->rotation;
	xfree (pData);
	return BadMatch;
    }

    /*
     * Validate requested refresh
     */
    if (has_rate)
	rate = (int) stuff->rate;
    else
	rate = 0;

    if (rate)
    {
	for (i = 0; i < pSize->nRates; i++)
	{
	    if (pSize->pRates[i].rate == rate)
		break;
	}
	if (i == pSize->nRates)
	{
	    /*
	     * Invalid rate
	     */
	    client->errorValue = rate;
	    xfree (pData);
	    return BadValue;
	}
	mode = pSize->pRates[i].mode;
    }
    else
	mode = pSize->pRates[0].mode;
    
    /*
     * Make sure the requested set-time is not older than
     * the last set-time
     */
    if (CompareTimeStamps (time, pScrPriv->lastSetTime) < 0)
    {
	rep.status = RRSetConfigInvalidTime;
	goto sendReply;
    }

    rep.status = RRCrtcSet (output->crtc, mode, 0, 0, stuff->rotation,
			    1, &output);
    
sendReply:
    
    if (pData)
	xfree (pData);

    rep.type = X_Reply;
    /* rep.status has already been filled in */
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rep.newTimestamp = pScrPriv->lastSetTime.milliseconds;
    rep.newConfigTimestamp = pScrPriv->lastConfigTime.milliseconds;
    rep.root = WindowTable[pDraw->pScreen->myNum]->drawable.id;

    if (client->swapped) 
    {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.newTimestamp, n);
	swapl(&rep.newConfigTimestamp, n);
	swapl(&rep.root, n);
    }
    WriteToClient(client, sizeof(xRRSetScreenConfigReply), (char *)&rep);

    return (client->noClientException);
}

#if 0
int
RRSetScreenConfig (ScreenPtr		pScreen,
		   Rotation		rotation,
		   int			rate,
		   RRScreenSizePtr	pSize)
{
    rrScrPrivPtr	    pScrPriv;
    RRMonitorPtr	    pMonitor;
    short		    oldWidth, oldHeight;
    RRModePtr		    pMode;
    int			    status;

    pScrPriv = rrGetScrPriv(pScreen);
    
    if (!pScrPriv)
	return BadImplementation;
    
    pMonitor = pScrPriv->pMonitors;
    if (!pMonitor)
	return BadImplementation;

    oldWidth = pScreen->width;
    oldHeight = pScreen->height;
    
    if (!RRGetInfo (pScreen))
	return BadAlloc;
    
    /*
     * Validate requested rotation
     */

    /* test the rotation bits only! */
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_90:
    case RR_Rotate_180:
    case RR_Rotate_270:
	break;
    default:
	/*
	 * Invalid rotation
	 */
	return BadValue;
    }

    if ((~pScrPriv->rotations) & rotation)
    {
	/*
	 * requested rotation or reflection not supported by screen
	 */
	return BadMatch;
    }

    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
    {
	if (pMode->mode.width == pSize->width &&
	    pMode->mode.height == pSize->height &&
	    pMode->mode.widthInMillimeters == pSize->mmWidth &&
	    pMode->mode.heightInMillimeters == pSize->mmHeight &&
	    (RRVerticalRefresh (&pMode->mode) == rate || rate == 0))
	{
	    break;
	}
    }
    if (!pMode)
	return BadValue;
    
    status = RRMonitorSetMode (pScreen, pMonitor, pMode, 0, 0, 
			       rotation, currentTime);
    
    if (status != RRSetConfigSuccess)
	return BadImplementation;
    return Success;
}
#endif

static Bool
RRSetScreenSize (ScreenPtr pScreen,
		 CARD16 width, CARD16 height,
		 CARD16 widthInMillimeters, CARD16 heightInMillimeters)
{
    rrScrPriv(pScreen);
    
    if (pScrPriv->rrScreenSetSize) 
    {
	return (*pScrPriv->rrScreenSetSize) (pScreen, width, height,
					     widthInMillimeters,
					     heightInMillimeters);
    }
#ifdef RANDR_SCREEN_INTERFACE
    else
    {
	/* Pend the size change until we get the set mode request.
	 * Yes, this is 'illegal', but the best we can do until
	 * drivers are updated
	 */
	pScrPriv->reqWidth = width;
	pScrPriv->reqHeight = height;
	pScreen->mmWidth = widthInMillimeters;
	pScreen->mmHeight = heightInMillimeters;
	return TRUE;
    }
#endif
    return FALSE;
}

static int
ProcRRSelectInput (ClientPtr client)
{
    REQUEST(xRRSelectInputReq);
    rrClientPriv(client);
    RRTimesPtr	pTimes;
    WindowPtr	pWin;
    RREventPtr	pRREvent, *pHead;
    XID		clientResource;

    REQUEST_SIZE_MATCH(xRRSelectInputReq);
    pWin = SecurityLookupWindow (stuff->window, client, SecurityWriteAccess);
    if (!pWin)
	return BadWindow;
    pHead = (RREventPtr *)SecurityLookupIDByType(client,
						 pWin->drawable.id, EventType,
						 SecurityWriteAccess);

    if (stuff->enable & (RRScreenChangeNotifyMask|
			 RRCrtcChangeNotifyMask|
			 RROutputChangeNotifyMask)) 
    {
	ScreenPtr	pScreen = pWin->drawable.pScreen;
	rrScrPriv	(pScreen);

	pRREvent = NULL;
	if (pHead) 
	{
	    /* check for existing entry. */
	    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next)
		if (pRREvent->client == client)
		    break;
	}

	if (!pRREvent)
	{
	    /* build the entry */
	    pRREvent = (RREventPtr) xalloc (sizeof (RREventRec));
	    if (!pRREvent)
		return BadAlloc;
	    pRREvent->next = 0;
	    pRREvent->client = client;
	    pRREvent->window = pWin;
	    pRREvent->mask = stuff->enable;
	    /*
	     * add a resource that will be deleted when
	     * the client goes away
	     */
	    clientResource = FakeClientID (client->index);
	    pRREvent->clientResource = clientResource;
	    if (!AddResource (clientResource, ClientType, (pointer)pRREvent))
		return BadAlloc;
	    /*
	     * create a resource to contain a pointer to the list
	     * of clients selecting input.  This must be indirect as
	     * the list may be arbitrarily rearranged which cannot be
	     * done through the resource database.
	     */
	    if (!pHead)
	    {
		pHead = (RREventPtr *) xalloc (sizeof (RREventPtr));
		if (!pHead ||
		    !AddResource (pWin->drawable.id, EventType, (pointer)pHead))
		{
		    FreeResource (clientResource, RT_NONE);
		    return BadAlloc;
		}
		*pHead = 0;
	    }
	    pRREvent->next = *pHead;
	    *pHead = pRREvent;
	}
	/*
	 * Now see if the client needs an event
	 */
	if (pScrPriv && (pRREvent->mask & RRScreenChangeNotifyMask))
	{
	    pTimes = &((RRTimesPtr) (pRRClient + 1))[pScreen->myNum];
	    if (CompareTimeStamps (pTimes->setTime, 
				   pScrPriv->lastSetTime) != 0 ||
		CompareTimeStamps (pTimes->configTime, 
				   pScrPriv->lastConfigTime) != 0)
	    {
		DeliverScreenEvent (client, pWin, pScreen);
	    }
	}
    }
    else if (stuff->enable == 0) 
    {
	/* delete the interest */
	if (pHead) {
	    RREventPtr pNewRREvent = 0;
	    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
		if (pRREvent->client == client)
		    break;
		pNewRREvent = pRREvent;
	    }
	    if (pRREvent) {
		FreeResource (pRREvent->clientResource, ClientType);
		if (pNewRREvent)
		    pNewRREvent->next = pRREvent->next;
		else
		    *pHead = pRREvent->next;
		xfree (pRREvent);
	    }
	}
    }
    else 
    {
	client->errorValue = stuff->enable;
	return BadValue;
    }
    return Success;
}

/*
 * Retrieve valid screen size range
 */
static int ProcRRGetScreenSizeRange (ClientPtr client)
{
    REQUEST(xRRGetScreenSizeRangeReq);
    xRRGetScreenSizeRangeReply	rep;
    WindowPtr			pWin;
    ScreenPtr			pScreen;
    rrScrPrivPtr		pScrPriv;
    
    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    
    rep.type = X_Reply;
    rep.pad = 0;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    
    if (pScrPriv) 
    {
	RRGetInfo (pScreen);
	rep.minWidth  = pScrPriv->minWidth;
	rep.minHeight = pScrPriv->minHeight;
	rep.maxWidth  = pScrPriv->maxWidth;
	rep.maxHeight = pScrPriv->maxHeight;
    }
    else
    {
	rep.maxWidth  = rep.minWidth  = pScreen->width;
	rep.maxHeight = rep.minHeight = pScreen->height;
    }
    if (client->swapped) 
    {
	int n;
	
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swaps(&rep.minWidth, n);
	swaps(&rep.minHeight, n);
	swaps(&rep.maxWidth, n);
	swaps(&rep.maxHeight, n);
    }
    WriteToClient(client, sizeof(xRRGetScreenSizeRangeReply), (char *)&rep);
    return (client->noClientException);
}

static int ProcRRSetScreenSize (ClientPtr client)
{
    REQUEST(xRRSetScreenSizeReq);
    WindowPtr		pWin;
    ScreenPtr		pScreen;
    rrScrPrivPtr	pScrPriv;
    RRCrtcPtr		crtc;
    int			i;
    
    REQUEST_SIZE_MATCH(xRRSetScreenSizeReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    if (stuff->width < pScrPriv->minWidth || pScrPriv->maxWidth < stuff->width)
    {
	client->errorValue = stuff->width;
	return BadValue;
    }
    if (stuff->height < pScrPriv->minHeight || 
	pScrPriv->maxHeight < stuff->height)
    {
	client->errorValue = stuff->height;
	return BadValue;
    }
    for (i = 0; i < pScrPriv->numCrtcs; i++) {
	crtc = pScrPriv->crtcs[i];
	if (crtc->mode &&
	    (crtc->x + crtc->mode->mode.width > stuff->width ||
	     crtc->y + crtc->mode->mode.height > stuff->height))
	    return BadMatch;
    }
    if (stuff->widthInMillimeters == 0 || stuff->heightInMillimeters == 0)
    {
	client->errorValue = 0;
	return BadValue;
    }
    if (!RRSetScreenSize (pScreen, 
			  stuff->width, stuff->height,
			  stuff->widthInMillimeters,
			  stuff->heightInMillimeters))
    {
	return BadMatch;
    }
    return Success;
}

#if 0
static int ProcRRGetMonitorInfo (ClientPtr client)
{
    REQUEST(xRRGetMonitorInfoReq);
    xRRGetMonitorInfoReply	rep;
    WindowPtr			pWin;
    ScreenPtr			pScreen;
    rrScrPrivPtr		pScrPriv;
    RRMonitorPtr		pMonitor;
    RRModePtr			pMode;
    int				extraLen;
    CARD8			*extra;
    xRRMonitorInfo		*monitor;
    xRRMonitorMode		*mode;
    CARD8			*names;
    
    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    rep.type = X_Reply;
    rep.pad = 0;
    rep.sequenceNumber = client->sequence;
    rep.numMonitors = 0;
    rep.numModes = 0;
    rep.sizeNames = 0;
    if (!pScrPriv)
    {
	extraLen = 0;
	extra = NULL;
    }
    else
    {
	int i, m, b;
	for (pMonitor = pScrPriv->pMonitors; pMonitor; pMonitor = pMonitor->next)
	{
	    rep.numMonitors++;
	    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
	    {
		rep.numModes++;
		rep.sizeNames += (1 + pMode->mode.nameLength);
	    }
	}
	extraLen = (rep.numMonitors * sizeof (xRRMonitorInfo) +
		    rep.numModes * sizeof (xRRMonitorMode) +
		    rep.sizeNames + 3) & ~3;
	extra = (CARD8 *) xalloc (extraLen);
	if (!extra)
	    return BadAlloc;
	monitor = (xRRMonitorInfo *) extra;
	mode = (xRRMonitorMode *) (monitor + rep.numMonitors);
	names = (CARD8 *) (mode + rep.numModes);
	i = 0;
	m = 0;
	b = 0;
	for (pMonitor = pScrPriv->pMonitors; pMonitor; pMonitor = pMonitor->next)
	{
	    monitor[i].timestamp = pScrPriv->lastSetTime;
	    monitor[i].configTimestamp = pScrPriv->lastConfigTime;
	    monitor[i].x = pMonitor->x;
	    monitor[i].y = pMonitor->y;
	    monitor[i].rotation = pMonitor->rotation;
	    monitor[i].mode = pMonitor->pMode->id;
	    monitor[i].defaultMode = 0;	/* XXX */
	    monitor[i].rotations = pMonitor->rotations;
	    monitor[i].firstMode = m;
	    monitor[i].numModes = 0;
	    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
	    {
		monitor[i].numModes++;
		mode[m] = pMode->mode;
		names[b] = pMode->mode.nameLength;
		b++;
		memcpy (names + b, (char *) (pMode + 1), 
			pMode->mode.nameLength);
		b += pMode->mode.nameLength;
		m++;
	    }
	    i++;
	}
	if ((char *) (names + ((b + 3) & ~3)) != (char *) extra + extraLen)
	    FatalError ("RRGetMonitorInfo length mismatch\n");
    }
    rep.length = extraLen >> 2;
    
    WriteToClient(client, sizeof(xRRGetMonitorInfoReply), (char *)&rep);
    if (extraLen)
    {
	WriteToClient (client, extraLen, (char *) extra);
	xfree (extra);
    }
    
    if (extra)
	xfree (extra);
    return (client->noClientException);
}

static int ProcRRAddMonitorMode (ClientPtr client)
{
    return BadImplementation;
}

static int ProcRRDeleteMonitorMode (ClientPtr client)
{
    return BadImplementation;
}

static int ProcRRSetMonitorConfig (ClientPtr client)
{
    REQUEST(xRRSetMonitorConfigReq);
    xRRSetMonitorConfigReply	rep;
    WindowPtr			pWin;
    ScreenPtr			pScreen;
    rrScrPrivPtr		pScrPriv;
    RRMonitorPtr		pMonitor;
    RRModePtr			pMode;
    TimeStamp		    configTime;
    TimeStamp		    time;
    Rotation		    rotation;
    
    REQUEST_SIZE_MATCH(xRRSetScreenConfigReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    
    time = ClientTimeToServerTime(stuff->timestamp);
    configTime = ClientTimeToServerTime(stuff->configTimestamp);
    
    if (!pScrPriv)
    {
	time = currentTime;
	rep.status = RRSetConfigFailed;
	goto sendReply;
    }
    if (!RRGetInfo (pScreen))
	return BadAlloc;
    
    /*
     * if the client's config timestamp is not the same as the last config
     * timestamp, then the config information isn't up-to-date and
     * can't even be validated
     */
    if (CompareTimeStamps (configTime, pScrPriv->lastConfigTime) != 0)
    {
	rep.status = RRSetConfigInvalidConfigTime;
	goto sendReply;
    }
    
    for (pMonitor = pScrPriv->pMonitors; pMonitor; pMonitor = pMonitor->next)
    {
	if (pMonitor->id == stuff->monitorIndex)
	    break;
    }
    if (!pMonitor)
    {
	client->errorValue = stuff->monitorIndex;
	return BadValue;
    }
    
    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
    {
	if (pMode->id == stuff->modeIndex)
	    break;
    }
    if (!pMode)
    {
	client->errorValue = stuff->modeIndex;
	return BadValue;
    }
    
    /*
     * Validate requested rotation
     */
    rotation = (Rotation) stuff->rotation;

    /* test the rotation bits only! */
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_90:
    case RR_Rotate_180:
    case RR_Rotate_270:
	break;
    default:
	/*
	 * Invalid rotation
	 */
	client->errorValue = stuff->rotation;
	return BadValue;
    }

    if ((~pMonitor->rotations) & rotation)
    {
	/*
	 * requested rotation or reflection not supported by screen
	 */
	client->errorValue = stuff->rotation;
	return BadMatch;
    }

    if (stuff->x + pMode->mode.width > pScreen->width)
    {
	client->errorValue = stufff
	stuff->y + pMode->mode.height > pScreen
    /*
     * Make sure the requested set-time is not older than
     * the last set-time
     */
    if (CompareTimeStamps (time, pScrPriv->lastSetTime) < 0)
    {
	rep.status = RRSetConfigInvalidTime;
	goto sendReply;
    }

    rep.status = RRMonitorSetMode (pScreen, pMonitor, 
				   pMode, stuff->x, stuff->y, rotation, time);
    
    return client->noClientException;
}
#endif

int (*ProcRandrVector[RRNumberRequests])(ClientPtr) = {
    ProcRRQueryVersion,	/* 0 */
/* we skip 1 to make old clients fail pretty immediately */
    NULL,			/* 1 ProcRandrOldGetScreenInfo */
/* V1.0 apps share the same set screen config request id */
    ProcRRSetScreenConfig,	/* 2 */
    NULL,			/* 3 ProcRandrOldScreenChangeSelectInput */
/* 3 used to be ScreenChangeSelectInput; deprecated */
    ProcRRSelectInput,		/* 4 */
    ProcRRGetScreenInfo,    	/* 5 */
/* V1.2 additions */
#if 0
    ProcRRGetScreenSizeRange,	/* 6 */
    ProcRRSetScreenSize,	/* 7 */
    ProcRRGetMonitorInfo,	/* 8 */
    ProcRRAddMonitorMode,	/* 9 */
    ProcRRDeleteMonitorMode,	/* 10 */
    ProcRRSetMonitorConfig,	/* 11 */
#endif
};


static int
ProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !ProcRandrVector[stuff->data])
	return BadRequest;
    return (*ProcRandrVector[stuff->data]) (client);
}

static int
SProcRRQueryVersion (ClientPtr client)
{
    register int n;
    REQUEST(xRRQueryVersionReq);

    swaps(&stuff->length, n);
    swapl(&stuff->majorVersion, n);
    swapl(&stuff->minorVersion, n);
    return ProcRRQueryVersion(client);
}

static int
SProcRRGetScreenInfo (ClientPtr client)
{
    register int n;
    REQUEST(xRRGetScreenInfoReq);

    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return ProcRRGetScreenInfo(client);
}

static int
SProcRRSetScreenConfig (ClientPtr client)
{
    register int n;
    REQUEST(xRRSetScreenConfigReq);

    if (RRClientKnowsRates (client))
    {
	REQUEST_SIZE_MATCH (xRRSetScreenConfigReq);
	swaps (&stuff->rate, n);
    }
    else
    {
	REQUEST_SIZE_MATCH (xRR1_0SetScreenConfigReq);
    }
    
    swaps(&stuff->length, n);
    swapl(&stuff->drawable, n);
    swapl(&stuff->timestamp, n);
    swaps(&stuff->sizeID, n);
    swaps(&stuff->rotation, n);
    return ProcRRSetScreenConfig(client);
}

static int
SProcRRSelectInput (ClientPtr client)
{
    register int n;
    REQUEST(xRRSelectInputReq);

    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return ProcRRSelectInput(client);
}


static int
SProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_RRQueryVersion:
	return SProcRRQueryVersion(client);
    case X_RRSetScreenConfig:
        return SProcRRSetScreenConfig(client);
    case X_RRSelectInput:
        return SProcRRSelectInput(client);
    case X_RRGetScreenInfo:
        return SProcRRGetScreenInfo(client);
    default:
	return BadRequest;
    }
}

/*
 * Register the range of sizes for the screen
 */
void
RRScreenSetSizeRange (ScreenPtr	pScreen,
		      CARD16	minWidth,
		      CARD16	minHeight,
		      CARD16	maxWidth,
		      CARD16	maxHeight)
{
    rrScrPriv (pScreen);

    if (!pScrPriv)
	return;
    pScrPriv->minWidth  = minWidth;
    pScrPriv->minHeight = minHeight;
    pScrPriv->maxWidth  = maxWidth;
    pScrPriv->maxHeight = maxHeight;
}

#ifdef RANDR_SCREEN_INTERFACE

static Bool
RRScreenSizeMatches (RRScreenSizePtr  a,
		   RRScreenSizePtr  b)
{
    if (a->width != b->width)
	return FALSE;
    if (a->height != b->height)
	return FALSE;
    if (a->mmWidth != b->mmWidth)
	return FALSE;
    if (a->mmHeight != b->mmHeight)
	return FALSE;
    return TRUE;
}

RRScreenSizePtr
RRRegisterSize (ScreenPtr	    pScreen,
		short		    width, 
		short		    height,
		short		    mmWidth,
		short		    mmHeight)
{
    rrScrPriv (pScreen);
    int		    i;
    RRScreenSize    tmp;
    RRScreenSizePtr pNew;

    if (!pScrPriv)
	return 0;
    
    tmp.width = width;
    tmp.height= height;
    tmp.mmWidth = mmWidth;
    tmp.mmHeight = mmHeight;
    tmp.pRates = 0;
    tmp.nRates = 0;
    for (i = 0; i < pScrPriv->nSizes; i++)
	if (RRScreenSizeMatches (&tmp, &pScrPriv->pSizes[i]))
	    return &pScrPriv->pSizes[i];
    pNew = xrealloc (pScrPriv->pSizes,
		     (pScrPriv->nSizes + 1) * sizeof (RRScreenSize));
    if (!pNew)
	return 0;
    pNew[pScrPriv->nSizes++] = tmp;
    pScrPriv->pSizes = pNew;
    return &pNew[pScrPriv->nSizes-1];
}

Bool RRRegisterRate (ScreenPtr		pScreen,
		     RRScreenSizePtr	pSize,
		     int		rate)
{
    rrScrPriv(pScreen);
    int		    i;
    RRScreenRatePtr pNew, pRate;

    if (!pScrPriv)
	return FALSE;
    
    for (i = 0; i < pSize->nRates; i++)
	if (pSize->pRates[i].rate == rate)
	    return TRUE;

    pNew = xrealloc (pSize->pRates,
		     (pSize->nRates + 1) * sizeof (RRScreenRate));
    if (!pNew)
	return FALSE;
    pRate = &pNew[pSize->nRates++];
    pRate->rate = rate;
    pSize->pRates = pNew;
    return TRUE;
}

Rotation
RRGetRotation(ScreenPtr pScreen)
{
    RROutputPtr	output = RRFirstOutput (pScreen);

    if (!output)
	return RR_Rotate_0;

    return output->crtc->rotation;
}

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize)
{
    rrScrPriv (pScreen);

    if (!pScrPriv)
	return;
    pScrPriv->size = pSize - pScrPriv->pSizes;
    pScrPriv->rotation = rotation;
    pScrPriv->rate = rate;
}
#endif
