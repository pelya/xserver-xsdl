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

#include "randrstr.h"

/* From render.h */
#ifndef SubPixelUnknown
#define SubPixelUnknown 0
#endif

#define RR_VALIDATE
int	RRGeneration;
int	RRNScreens;

static int ProcRRDispatch (ClientPtr pClient);
static int SProcRRDispatch (ClientPtr pClient);

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

int	RREventBase;
int	RRErrorBase;
RESTYPE RRClientType, RREventType; /* resource types for event masks */
int	RRClientPrivateIndex;

int	rrPrivIndex = -1;

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

Bool RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr   pScrPriv;

    if (RRGeneration != serverGeneration)
    {
	if (!RRModeInit ())
	    return FALSE;
	if (!RRCrtcInit ())
	    return FALSE;
	if (!RROutputInit ())
	    return FALSE;
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
    pScrPriv->rrGetInfo = 0;
    pScrPriv->maxWidth = pScrPriv->minWidth = pScreen->width;
    pScrPriv->maxHeight = pScrPriv->minHeight = pScreen->height;
    
#if RANDR_12_INTERFACE
    pScrPriv->rrCrtcSet = 0;
#endif
#if RANDR_10_INTERFACE    
    pScrPriv->rrSetConfig = 0;
    pScrPriv->rotations = RR_Rotate_0;
    pScrPriv->reqWidth = pScreen->width;
    pScrPriv->reqHeight = pScreen->height;
    pScrPriv->nSizes = 0;
    pScrPriv->pSizes = NULL;
    pScrPriv->rotation = RR_Rotate_0;
    pScrPriv->rate = 0;
    pScrPriv->size = 0;
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
    pHead = (RREventPtr *) LookupIDByType(pWin->drawable.id, RREventType);
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
	FreeResource (pCur->clientResource, RRClientType);
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

    RRClientType = CreateNewResourceType(RRFreeClient);
    if (!RRClientType)
	return;
    RREventType = CreateNewResourceType(RRFreeEvents);
    if (!RREventType)
	return;
    extEntry = AddExtension (RANDR_NAME, RRNumberEvents, RRNumberErrors,
			     ProcRRDispatch, SProcRRDispatch,
			     RRResetProc, StandardMinorOpcode);
    if (!extEntry)
	return;
    RRErrorBase = extEntry->errorBase;
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
    ScreenPtr			pScreen = pWin->drawable.pScreen;
    rrScrPriv(pScreen);
    int				i;

    pHead = (RREventPtr *) LookupIDByType (pWin->drawable.id, RREventType);
    if (!pHead)
	return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) 
    {
	client = pRREvent->client;
	if (client == serverClient || client->clientGone)
	    continue;

	if (pRREvent->mask & RRScreenChangeNotifyMask)
	    RRDeliverScreenEvent (client, pWin, pScreen);
	
	if (pRREvent->mask & RRCrtcChangeNotifyMask)
	{
	    for (i = 0; i < pScrPriv->numCrtcs; i++)
	    {
		RRCrtcPtr   crtc = pScrPriv->crtcs[i];
		if (crtc->changed)
		    RRDeliverCrtcEvent (client, pWin, crtc);
	    }
	}
	
	if (pRREvent->mask & RROutputChangeNotifyMask)
	{
	    for (i = 0; i < pScrPriv->numOutputs; i++)
	    {
		RROutputPtr   output = pScrPriv->outputs[i];
		if (output->changed)
		    RRDeliverOutputEvent (client, pWin, output);
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

/*
 * Return the first output which is connected to an active CRTC
 * Used in emulating 1.0 behaviour
 */
RROutputPtr
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

#ifdef RANDR_10_INTERFACE
static RRModePtr
RROldModeAdd (RROutputPtr output, RRScreenSizePtr size, int refresh)
{
    ScreenPtr	pScreen = output->pScreen;
    rrScrPriv(pScreen);
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
    mode = RRModeGet (pScreen, &modeInfo, name);
    if (!mode)
	return NULL;
    for (i = 0; i < output->numModes; i++)
	if (output->modes[i] == mode)
	{
	    RRModeDestroy (mode);
	    return mode;
	}
    
    if (output->numModes)
	modes = xrealloc (output->modes, 
			  (output->numModes + 1) * sizeof (RRModePtr));
    else
	modes = xalloc (sizeof (RRModePtr));
    if (!modes)
    {
	RRModeDestroy (mode);
	FreeResource (mode->id, 0);
	return NULL;
    }
    modes[output->numModes++] = mode;
    output->modes = modes;
    output->changed = TRUE;
    pScrPriv->changed = TRUE;
    return mode;
}

static void
RRScanOldConfig (ScreenPtr pScreen, Rotation rotations)
{
    rrScrPriv(pScreen);
    RROutputPtr	output;
    RRCrtcPtr	crtc;
    RRModePtr	mode, newMode = NULL;
    int		i;
    CARD16	minWidth = MAXSHORT, minHeight = MAXSHORT;
    CARD16	maxWidth = 0, maxHeight = 0;
    
    /*
     * First time through, create a crtc and output and hook
     * them together
     */
    if (pScrPriv->numOutputs == 0 &&
	pScrPriv->numCrtcs == 0)
    {
	crtc = RRCrtcCreate (pScreen, NULL);
	if (!crtc)
	    return;
	output = RROutputCreate (pScreen, "default", 7, NULL);
	if (!output)
	    return;
	RROutputSetCrtcs (output, &crtc, 1);
	RROutputSetConnection (output, RR_Connected);
    }

    output = RRFirstOutput (pScreen);
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
	{
	    for (r = 0; r < size->nRates; r++)
	    {
		mode = RROldModeAdd (output, size, size->pRates[r].rate);
		if (i == pScrPriv->size && 
		    size->pRates[r].rate == pScrPriv->rate)
		{
		    newMode = mode;
		}
	    }
	    xfree (size->pRates);
	}
	else
	{
	    mode = RROldModeAdd (output, size, 0);
	    if (i == pScrPriv->size)
		newMode = mode;
	}
    }
    if (pScrPriv->nSizes)
	xfree (pScrPriv->pSizes);
    pScrPriv->pSizes = NULL;
    pScrPriv->nSizes = 0;
	    
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

    /* notice current mode */
    if (newMode)
	RRCrtcSet (output->crtc, newMode, 0, 0, pScrPriv->rotation,
		   1, &output);
}
#endif

/*
 * Poll the driver for changed information
 */
Bool
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

#if RANDR_10_INTERFACE
    if (pScrPriv->nSizes)
	RRScanOldConfig (pScreen, rotations);
#endif
    RRTellChanged (pScreen);
    return TRUE;
}

CARD16
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

static int
ProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !ProcRandrVector[stuff->data])
	return BadRequest;
    return (*ProcRandrVector[stuff->data]) (client);
}

static int
SProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !ProcRandrVector[stuff->data])
	return BadRequest;
    return (*SProcRandrVector[stuff->data]) (client);
}

#if RANDR_12_INTERFACE
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
#endif

#ifdef RANDR_10_INTERFACE
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
    
    tmp.id = 0;
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
