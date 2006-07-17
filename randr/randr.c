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
    RRMonitorPtr    pMonitor;

    unwrap (pScrPriv, pScreen, CloseScreen);
    while ((pMonitor = pScrPriv->pMonitors))
    {
	RRModePtr   pMode;
	
	pScrPriv->pMonitors = pMonitor->next;
	while ((pMode = pMonitor->pModes))
	{
	    pMonitor->pModes = pMode->next;
	    xfree (pMode);
	}
	xfree (pMonitor);
    }
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

static void
SRRNotifyEvent (xEvent *from,
		xEvent *to)
{
    switch (from->u.u.detail) {
    case RRNotify_MonitorChange:
	SRRMonitorChangeNotifyEvent ((xRRMonitorChangeNotifyEvent *) from,
				     (xRRMonitorChangeNotifyEvent *) to);
	break;
    default:
	break;
    }
}

Bool RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr   pScrPriv;

    if (RRGeneration != serverGeneration)
    {
	if ((rrPrivIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	RRGeneration = serverGeneration;
    }

    pScrPriv = (rrScrPrivPtr) xalloc (sizeof (rrScrPrivRec));
    if (!pScrPriv)
	return FALSE;

    SetRRScreen(pScreen, pScrPriv);

    /*
     * Calling function best set these function vectors
     */
    pScrPriv->rrSetMode = 0;
    pScrPriv->rrGetInfo = 0;
#ifdef RANDR_SCREEN_INTERFACE    
    pScrPriv->rrSetConfig = 0;
#endif
    
    /*
     * This value doesn't really matter -- any client must call
     * GetScreenInfo before reading it which will automatically update
     * the time
     */
    pScrPriv->lastSetTime = currentTime;
    pScrPriv->lastConfigTime = currentTime;
    
    wrap (pScrPriv, pScreen, CloseScreen, RRCloseScreen);

    pScrPriv->pMonitors = NULL;
    
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
		
static int
TellChanged (WindowPtr pWin, pointer value)
{
    RREventPtr			*pHead, pRREvent;
    ClientPtr			client;
    xRRScreenChangeNotifyEvent	se;
    xRRMonitorChangeNotifyEvent	me;
    ScreenPtr			pScreen = pWin->drawable.pScreen;
    rrScrPriv(pScreen);
    RRModePtr			pMode;
    RRMonitorPtr		pMonitor;
    WindowPtr			pRoot = WindowTable[pScreen->myNum];
    int				i;

    pHead = (RREventPtr *) LookupIDByType (pWin->drawable.id, EventType);
    if (!pHead)
	return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) 
    {
	client = pRREvent->client;
	if (client == serverClient || client->clientGone)
	    continue;

	if (pRREvent->mask & RRMonitorChangeNotifyMask)
	{
	    me.type = RRNotify + RREventBase;
	    me.subCode = RRNotify_MonitorChange;
	    me.timestamp = pScrPriv->lastSetTime.milliseconds;
	    me.configTimestamp = pScrPriv->lastConfigTime.milliseconds;
	    me.root =  pRoot->drawable.id;
	    me.window = pWin->drawable.id;
#ifdef RENDER
	    me.subpixelOrder = PictureGetSubpixelOrder (pScreen);
#else
	    me.subpixelOrder = SubPixelUnknown;
#endif
	    for (pMonitor = pScrPriv->pMonitors, i = 0; 
		 pMonitor; 
		 pMonitor = pMonitor->next, i++)
	    {
		me.monitor = i;
		if (pMonitor->pMode) {
		    me.modeID = pMonitor->pMode->id;
		    me.rotation = pMonitor->rotation;
		    me.x = pMonitor->x;
		    me.y = pMonitor->y;
		} else {
		    me.modeID = 0xffff;
		    me.rotation = RR_Rotate_0;
		    me.x = 0;
		    me.y = 0;
		}
		WriteEventsToClient (client, 1, (xEvent *) &me);
	    }
	}
	if ((pRREvent->mask & RRScreenChangeNotifyMask) &&
	    (pMonitor = pScrPriv->pMonitors))
	{
	    se.type = RRScreenChangeNotify + RREventBase;
	    se.rotation = (CARD8) pMonitor->rotation;
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

	    pMonitor = &pScrPriv->pMonitors[0];
	    se.sequenceNumber = client->sequence;
	    if (pMonitor->pMode) 
	    {
		pMode = pMonitor->pMode;
		se.sizeID = pMode->id;
		se.widthInPixels = pMode->mode.width;
		se.heightInPixels = pMode->mode.height;
		se.widthInMillimeters = pMode->mode.widthInMillimeters;
		se.heightInMillimeters = pMode->mode.heightInMillimeters;
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
    }
    return WT_WALKCHILDREN;
}

static void
RRFreeMode (RRModePtr pMode)
{
    xfree (pMode);
}

static void
RRFreeModes (RRModePtr pHead)
{
    RRModePtr	pMode;
    while ((pMode = pHead)) 
    {
	pHead = pMode->next;
	RRFreeMode (pMode);
    }
}

static void
RRFreeMonitor (RRMonitorPtr pMonitor)
{
    RRFreeModes (pMonitor->pModes);
    xfree (pMonitor);
}


static Bool
RRGetInfo (ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    Bool	    changed;
    Rotation	    rotations;
    RRMonitorPtr    pMonitor, *pPrevMon;
    RRModePtr	    pMode, *pPrevMode;
    int		    monitorid;

    for (pMonitor = pScrPriv->pMonitors; pMonitor; pMonitor = pMonitor->next)
    {
	pMonitor->oldReferenced = TRUE;
	pMonitor->referenced = FALSE;
	pMonitor->pMode = NULL;
	for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
	{
	    pMode->oldReferenced = TRUE;
	    pMode->referenced = FALSE;
	}
    }
    rotations = 0;
    if (!(*pScrPriv->rrGetInfo) (pScreen, &rotations))
	return FALSE;

    changed = FALSE;
    
    /* Old GetInfo clients return rotations here */
    if (rotations && pScrPriv->pMonitors) {
	/*
	 * Check whether anything changed and simultaneously generate
	 * the protocol id values for the objects
	 */
	if (rotations != pScrPriv->pMonitors->rotations)
	{
	    pScrPriv->pMonitors->rotations = rotations;
	    changed = TRUE;
	}
    }
	
    /*
     * Walk monitor and mode lists looking for updates
     */
    monitorid = 0;
    for (pPrevMon = &pScrPriv->pMonitors; (pMonitor = *pPrevMon);)
    {
	int modeid = 0;
	
	if (pMonitor->referenced)
	{
	    pMonitor->id = monitorid++;
	    if (!pMonitor->oldReferenced)
		changed = TRUE;
	    for (pPrevMode = &pMonitor->pModes; (pMode = *pPrevMode);)
	    {
		if (pMode->referenced)
		{
		    pMode->id = modeid++;
		    if (!pMode->oldReferenced)
			changed = TRUE;
		    pPrevMode = &pMode->next;
		}
		else
		{
		    *pPrevMode = pMode->next;
		    changed = TRUE;
		    RRFreeMode (pMode);
		}
	    }
	    pPrevMon = &pMonitor->next;
	}
	else
	{
	    *pPrevMon = pMonitor->next;
	    changed = TRUE;
	    RRFreeMonitor (pMonitor);
	}
    }
    if (changed)
    {
	UpdateCurrentTime ();
	pScrPriv->lastConfigTime = currentTime;
	WalkTree (pScreen, TellChanged, (pointer) pScreen);
    }
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

static int
RRNumModes (RRMonitorPtr pMonitor)
{
    int	n = 0;
    RRModePtr	pMode;

    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
	n++;
    return n;
}

typedef struct _RR10Data {
    RRScreenSizePtr sizes;
    int		    nsize;
    int		    nrefresh;
    int		    size;
    CARD16	    refresh;
} RR10DataRec, *RR10DataPtr;

static CARD16
RRVerticalRefresh (xRRMonitorMode *mode)
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
RR10GetData (ScreenPtr pScreen, RRMonitorPtr pMonitor)
{
    RR10DataPtr	    data;
    RRScreenSizePtr size;
    int		    nmode = RRNumModes (pMonitor);
    int		    i;
    int		    j;
    RRRefreshPtr    refresh;
    CARD16	    vRefresh;
    RRModePtr	    pMode;

    /* Make sure there is plenty of space for any combination */
    data = malloc (sizeof (RR10DataRec) + 
		   sizeof (RRScreenSizeRec) * nmode + 
		   sizeof (RRRefreshRec) * nmode);
    if (!data)
	return NULL;
    size = (RRScreenSizePtr) (data + 1);
    refresh = (RRRefreshPtr) (size + nmode);
    data->sizes = size;
    data->nsize = 0;
    data->nrefresh = 0;
    data->size = 0;
    data->refresh = 0;
    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
    {
	for (i = 0; i < data->nsize; i++)
	    if (pMode->mode.width == size[i].width &&
		pMode->mode.height == size[i].height)
		break;
	if (i == data->nsize)
	{
	    size[i].id = i;
	    size[i].width = pMode->mode.width;
	    size[i].height = pMode->mode.height;
	    size[i].mmWidth = pMode->mode.widthInMillimeters;
	    size[i].mmHeight = pMode->mode.heightInMillimeters;
	    size[i].nrefresh = 0;
	    size[i].refresh = &refresh[data->nrefresh];
	    data->nsize++;
	}
	vRefresh = RRVerticalRefresh (&pMode->mode);
	for (j = 0; j < size[i].nrefresh; j++)
	    if (vRefresh == size[i].refresh[j].refresh)
		break;
	if (j == size[i].nrefresh)
	{
	    size[i].refresh[j].refresh = vRefresh;
	    size[i].refresh[j].pMode = pMode;
	    size[i].nrefresh++;
	    data->nrefresh++;
	}
	if (pMode == pMonitor->pMode)
	{
	    data->size = i;
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

    if (!pScrPriv && !pScrPriv->pMonitors)
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
	RRMonitorPtr		pMonitor = pScrPriv->pMonitors;
	int			i, j;
	xScreenSizes		*size;
	CARD16			*rates;
	CARD8			*data8;
	Bool			has_rate = RRClientKnowsRates (client);
	RR10DataPtr		pData;
	RRScreenSizePtr		pSize;
    
	pData = RR10GetData (pScreen, pMonitor);
	if (!pData)
	    return BadAlloc;
	
	rep.type = X_Reply;
	rep.setOfRotations = pMonitor->rotations;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.root = WindowTable[pWin->drawable.pScreen->myNum]->drawable.id;
	rep.timestamp = pScrPriv->lastSetTime.milliseconds;
	rep.configTimestamp = pScrPriv->lastConfigTime.milliseconds;
	rep.rotation = pMonitor->rotation;
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
		*rates = pSize->nrefresh;
		if (client->swapped)
		{
		    swaps (rates, n);
		}
		rates++;
		for (j = 0; j < pSize->nrefresh; j++)
		{
		    *rates = pSize->refresh[j].refresh;
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
    RRMonitorPtr	    pMonitor;
    RRModePtr		    pMode;
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
    
    pMonitor = pScrPriv->pMonitors;
    if (!pMonitor)
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
    
    pData = RR10GetData (pScreen, pMonitor);
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

    if ((~pMonitor->rotations) & rotation)
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
	for (i = 0; i < pSize->nrefresh; i++)
	{
	    if (pSize->refresh[i].refresh == rate)
		break;
	}
	if (i == pSize->nrefresh)
	{
	    /*
	     * Invalid rate
	     */
	    client->errorValue = rate;
	    xfree (pData);
	    return BadValue;
	}
	pMode = pSize->refresh[i].pMode;
    }
    else
	pMode = pSize->refresh[0].pMode;
    
    /*
     * Make sure the requested set-time is not older than
     * the last set-time
     */
    if (CompareTimeStamps (time, pScrPriv->lastSetTime) < 0)
    {
	rep.status = RRSetConfigInvalidTime;
	goto sendReply;
    }

    rep.status = RRMonitorSetMode (pScreen, pMonitor, pMode, 0, 0, rotation, time);
    
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

static int
ProcRRSelectInput (ClientPtr client)
{
    REQUEST(xRRSelectInputReq);
    rrClientPriv(client);
    RRTimesPtr	pTimes;
    WindowPtr	pWin;
    RREventPtr	pRREvent, pNewRREvent, *pHead;
    XID		clientResource;

    REQUEST_SIZE_MATCH(xRRSelectInputReq);
    pWin = SecurityLookupWindow (stuff->window, client, SecurityWriteAccess);
    if (!pWin)
	return BadWindow;
    pHead = (RREventPtr *)SecurityLookupIDByType(client,
						 pWin->drawable.id, EventType,
						 SecurityWriteAccess);

    if (stuff->enable & (RRScreenChangeNotifyMask)) 
    {
	ScreenPtr	pScreen = pWin->drawable.pScreen;
	rrScrPriv	(pScreen);

	if (pHead) 
	{
	    /* check for existing entry. */
	    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next)
		if (pRREvent->client == client)
		    return Success;
	}

	/* build the entry */
	pNewRREvent = (RREventPtr) xalloc (sizeof (RREventRec));
	if (!pNewRREvent)
	    return BadAlloc;
	pNewRREvent->next = 0;
	pNewRREvent->client = client;
	pNewRREvent->window = pWin;
	pNewRREvent->mask = stuff->enable;
	/*
	 * add a resource that will be deleted when
	 * the client goes away
	 */
	clientResource = FakeClientID (client->index);
	pNewRREvent->clientResource = clientResource;
	if (!AddResource (clientResource, ClientType, (pointer)pNewRREvent))
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
	pNewRREvent->next = *pHead;
	*pHead = pNewRREvent;
	/*
	 * Now see if the client needs an event
	 */
	if (pScrPriv)
	{
	    pTimes = &((RRTimesPtr) (pRRClient + 1))[pScreen->myNum];
	    if (CompareTimeStamps (pTimes->setTime, 
				   pScrPriv->lastSetTime) != 0 ||
		CompareTimeStamps (pTimes->configTime, 
				   pScrPriv->lastConfigTime) != 0)
	    {
		TellChanged (pWin, (pointer) pScreen);
	    }
	}
    }
    else if (stuff->enable == xFalse) 
    {
	/* delete the interest */
	if (pHead) {
	    pNewRREvent = 0;
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


static int ProcRRGetScreenSizeRange (ClientPtr pClient)
{
    return BadImplementation;
}

static int ProcRRSetScreenSize (ClientPtr pClient)
{
    return BadImplementation;
}

static int ProcRRGetMonitorInfo (ClientPtr pClient)
{
    return BadImplementation;
}

static int ProcRRAddMonitorMode (ClientPtr pClient)
{
    return BadImplementation;
}

static int ProcRRDeleteMonitorMode (ClientPtr pClient)
{
    return BadImplementation;
}

static int ProcRRSetMonitorConfig (ClientPtr pClient)
{
    return BadImplementation;
}

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
    ProcRRGetScreenSizeRange,	/* 6 */
    ProcRRSetScreenSize,	/* 7 */
    ProcRRGetMonitorInfo,	/* 8 */
    ProcRRAddMonitorMode,	/* 9 */
    ProcRRDeleteMonitorMode,	/* 10 */
    ProcRRSetMonitorConfig,	/* 11 */
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
 * Register a monitor for a screen. identifier is a unique identifier
 * for the monitors of a particular screen.
 */
RRMonitorPtr
RRRegisterMonitor (ScreenPtr		pScreen,
		   void			*identifier,
		   Rotation		rotations)
{
    rrScrPriv (pScreen);
    RRMonitorPtr    pMonitor, *pPrev, *pInsert;

    pInsert = NULL;
    for (pPrev = &pScrPriv->pMonitors; (pMonitor = *pPrev); 
	 pPrev = &(pMonitor)->next)
    {
	if (pMonitor->identifier == identifier) 
	{
	    pMonitor->referenced = TRUE;
	    return pMonitor;
	}
	if (!pMonitor->referenced)
	    pInsert = pPrev;
    }
    if (!pInsert)
	pInsert = pPrev;
    
    /*
     * New monitor, add before the first unreferenced monitor
     */
    pMonitor = xalloc (sizeof (RRMonitor));
    if (!pMonitor)
	return NULL;
    pMonitor->pScreen = pScreen;
    pMonitor->pModes = NULL;
    pMonitor->identifier = identifier;
    pMonitor->referenced = TRUE;
    pMonitor->oldReferenced = FALSE;
    pMonitor->rotations = RR_Rotate_0;
    
    pMonitor->pMode = NULL;
    pMonitor->x = 0;
    pMonitor->y = 0;
    pMonitor->rotation = RR_Rotate_0;
    pMonitor->next = *pInsert;
    *pInsert = pMonitor;
    return pMonitor;
}

/*
 * Register a mode for a monitor
 */

RRModePtr
RRRegisterMode (RRMonitorPtr	pMonitor,
		xRRMonitorMode	*pModeline,
		char		*name)
{
    RRModePtr	pMode, *pPrev, *pInsert = NULL;

    /*
     * Find existing mode with same modeline and name
     */
    for (pPrev = &pMonitor->pModes; (pMode = *pPrev); pPrev = &(pMode->next))
    {
	if (!memcmp (&pMode->mode, pModeline, sizeof (xRRMonitorMode)) &&
	    pMode->mode.nameLength == pModeline->nameLength &&
	    !memcmp (RRModeName(pMode), name, pModeline->nameLength))
	{
	    pMode->referenced = TRUE;
	    return pMode;
	}
	if (!pMode->referenced)
	    pInsert = pPrev;
    }

    if (!pInsert)
	pInsert = pPrev;
    
    /*
     * Create a new mode, inserting before the first unreferenced mode
     */
    pMode = xalloc (sizeof (RRMode) + pModeline->nameLength + 1);
    pMode->referenced = TRUE;
    pMode->oldReferenced = FALSE;
    pMode->mode = *pModeline;
    memcpy (RRModeName (pMode), name, pModeline->nameLength);
    RRModeName(pMode)[pModeline->nameLength] = '\0';
    pMode->next = *pInsert;
    *pInsert = pMode;
    return pMode;
}

void
RRSetCurrentMode (RRMonitorPtr	pMonitor,
		  RRModePtr	pMode,
		  int		x,
		  int		y,
		  Rotation	rotation)
{
    pMonitor->pMode = pMode;
    pMonitor->x = x;
    pMonitor->y = y;
    pMonitor->rotation = rotation;
}

#ifdef RANDR_SCREEN_INTERFACE

RRScreenSizePtr
RRRegisterSize (ScreenPtr	    pScreen,
		short		    width, 
		short		    height,
		short		    mmWidth,
		short		    mmHeight)
{
    rrScrPriv (pScreen);
    xRRMonitorMode  tmp;
    RRMonitorPtr    pMonitor;
    RRModePtr	    pMode, *pPrev;
    char	    name[100];

    if (!pScrPriv)
	return NULL;
    pMonitor = pScrPriv->pMonitors;
    if (!pMonitor)
    {
	pMonitor = RRRegisterMonitor (pScreen, NULL, RR_Rotate_0);
	if (!pMonitor)
	    return NULL;
    }
    pMonitor->referenced = TRUE;
    
    for (pPrev = &pMonitor->pModes; (pMode = *pPrev); pPrev = &(pMode->next))
	if (pMode->mode.width == width &&
	    pMode->mode.height == height &&
	    pMode->mode.widthInMillimeters == mmWidth &&
	    pMode->mode.heightInMillimeters == mmHeight)
	{
	    pMode->referenced =TRUE;
	    return (void *) pMode;
	}
    memset (&tmp, '\0', sizeof (xRRMonitorMode));
    memset (name, '\0', sizeof (name));
    sprintf (name, "%dx%d", width, height);
    tmp.width = width;
    tmp.height= height;
    tmp.widthInMillimeters = mmWidth;
    tmp.heightInMillimeters = mmHeight;
    tmp.nameLength = strlen (name) + 10;    /* leave space for refresh */
    pMode = RRRegisterMode (pMonitor, &tmp, name);
    return (void *) pMode;
}

static Bool
RRModesMatchSize (RRModePtr a, RRModePtr b)
{
    return (a->mode.width == a->mode.width &&
	    a->mode.height == b->mode.height &&
	    a->mode.widthInMillimeters == b->mode.widthInMillimeters &&
	    a->mode.heightInMillimeters == b->mode.heightInMillimeters);
}

Bool RRRegisterRate (ScreenPtr		pScreen,
		     RRScreenSizePtr	pSize,
		     int		rate)
{
    rrScrPriv(pScreen);
    RRMonitorPtr    pMonitor;
    RRModePtr	    pSizeMode = (RRModePtr) pSize;
    RRModePtr	    pMode, *pPrev;
    CARD16	    width = pSizeMode->mode.width;
    CARD16	    height = pSizeMode->mode.height;
    char	    name[100];
    xRRMonitorMode  modeLine;

    if (!pScrPriv)
	return FALSE;
    
    pMonitor = pScrPriv->pMonitors;
    if (!pMonitor)
	return FALSE;

    for (pPrev = &pMonitor->pModes; (pMode = *pPrev); pPrev = &pMode->next)
    {
	if (RRModesMatchSize (pMode, pSizeMode))
	{
	    if (pMode->mode.dotClock == 0)
	    {
		/*
		 * First refresh for this size; reprogram this mode
		 * with the right refresh.
		 */
		pMode->mode.hSyncStart = width;
		pMode->mode.hSyncEnd = width;
		pMode->mode.hTotal = width;
		pMode->mode.hSkew = 0;
		pMode->mode.vSyncStart = height;
		pMode->mode.vSyncEnd = height;
		pMode->mode.vTotal = height;
		pMode->mode.dotClock = width * height * rate;
		sprintf ((char *) (pMode + 1), "%dx%d@%d", width, height, rate);
		pMode->mode.modeFlags = 0;
		pMode->mode.nameLength = strlen ((char *) (pMode + 1));
		pMode->referenced = TRUE;
		return TRUE;
	    }
	    else if (rate == RRVerticalRefresh (&pMode->mode))
	    {
		pMode->referenced = TRUE;
		return TRUE;
	    }
	}
    }
    
    sprintf (name, "%dx%d@%d", pSizeMode->mode.width, pSizeMode->mode.height,
	     rate);
    modeLine = pSizeMode->mode;
    modeLine.dotClock = rate * modeLine.vTotal * modeLine.hTotal;
    modeLine.nameLength = strlen (name);
    pMode = RRRegisterMode (pMonitor, &modeLine, name);
    if (!pMode)
	return FALSE;
    return TRUE;
}

Rotation
RRGetRotation(ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    RRMonitorPtr    pMonitor;

    if (!pScrPriv)
	return RR_Rotate_0;

    pMonitor = pScrPriv->pMonitors;
    if (!pMonitor)
	return RR_Rotate_0;
    return pMonitor->rotation;
}

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize)
{
    rrScrPriv (pScreen);
    RRMonitorPtr    pMonitor;
    RRModePtr	    pMode;
    RRModePtr	    pSizeMode = (RRModePtr) pSize;

    if (!pScrPriv)
	return;
    pMonitor = pScrPriv->pMonitors;
    if (!pMonitor)
	return;

    for (pMode = pMonitor->pModes; pMode; pMode = pMode->next)
    {
	if (RRModesMatchSize (pMode, pSizeMode) && 
	    RRVerticalRefresh (&pMode->mode) == rate)
	    break;
    }
    if (!pMode)
	return;
    
    RRSetCurrentMode (pMonitor, pMode, 0, 0, rotation);
}
#endif
