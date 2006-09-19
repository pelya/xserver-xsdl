/*
 * Copyright © 2006 Keith Packard
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
 */

#include "randrstr.h"

Bool
RRClientKnowsRates (ClientPtr	pClient)
{
    rrClientPriv(pClient);

    return (pRRClient->major_version > 1 ||
	    (pRRClient->major_version == 1 && pRRClient->minor_version >= 1));
}

typedef struct _RR10Data {
    RRScreenSizePtr sizes;
    int		    nsize;
    int		    nrefresh;
    int		    size;
    CARD16	    refresh;
} RR10DataRec, *RR10DataPtr;

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
						 pWin->drawable.id, RREventType,
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
	    if (!AddResource (clientResource, RRClientType, (pointer)pRREvent))
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
		    !AddResource (pWin->drawable.id, RREventType, (pointer)pHead))
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
		RRDeliverScreenEvent (client, pWin, pScreen);
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
		FreeResource (pRREvent->clientResource, RRClientType);
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
static int 
ProcRRGetScreenSizeRange (ClientPtr client)
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
    if (!RRScreenSizeSet (pScreen, 
			  stuff->width, stuff->height,
			  stuff->widthInMillimeters,
			  stuff->heightInMillimeters))
    {
	return BadMatch;
    }
    return Success;
}

static int
ProcRRGetScreenResources (ClientPtr client)
{
    REQUEST(xRRGetScreenResourcesReq);
    xRRGetScreenResourcesReply  rep;
    WindowPtr			pWin;
    ScreenPtr			pScreen;
    rrScrPrivPtr		pScrPriv;
    CARD8			*extra;
    unsigned long		extraLen;
    int				i;
    RRCrtc			*crtcs;
    RROutput			*outputs;
    xRRModeInfo			*modeinfos;
    CARD8			*names;
    int				n;
    
    REQUEST_SIZE_MATCH(xRRGetScreenResourcesReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;
    
    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    rep.pad = 0;
    
    if (pScrPriv)
	RRGetInfo (pScreen);

    if (!pScrPriv)
    {
	rep.type = X_Reply;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.timestamp = currentTime.milliseconds;
	rep.configTimestamp = currentTime.milliseconds;
	rep.nCrtcs = 0;
	rep.nOutputs = 0;
	rep.nModes = 0;
	rep.nbytesNames = 0;
	extra = NULL;
	extraLen = 0;
    }
    else
    {
	rep.type = X_Reply;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.timestamp = currentTime.milliseconds;
	rep.configTimestamp = currentTime.milliseconds;
	rep.nCrtcs = pScrPriv->numCrtcs;
	rep.nOutputs = pScrPriv->numOutputs;
	rep.nModes = pScrPriv->numModes;;
	rep.nbytesNames = 0;

	for (i = 0; i < pScrPriv->numModes; i++)
	    rep.nbytesNames += pScrPriv->modes[i]->mode.nameLength;

	rep.length = (pScrPriv->numCrtcs + 
		      pScrPriv->numOutputs + 
		      pScrPriv->numModes * 10 +
		      ((rep.nbytesNames + 3) >> 2));
	
	extraLen = rep.length << 2;
	extra = xalloc (extraLen);
	if (!extra)
	    return BadAlloc;

	crtcs = (RRCrtc *) extra;
	outputs = (RROutput *) (crtcs + pScrPriv->numCrtcs);
	modeinfos = (xRRModeInfo *) (outputs + pScrPriv->numOutputs);
	names = (CARD8 *) (modeinfos + pScrPriv->numModes);
	
	for (i = 0; i < pScrPriv->numCrtcs; i++)
	{
	    crtcs[i] = pScrPriv->crtcs[i]->id;
	    if (client->swapped)
		swapl (&crtcs[i], n);
	}
	
	for (i = 0; i < pScrPriv->numOutputs; i++)
	{
	    outputs[i] = pScrPriv->outputs[i]->id;
	    if (client->swapped)
		swapl (&outputs[i], n);
	}
	
	for (i = 0; i < pScrPriv->numModes; i++)
	{
	    modeinfos[i] = pScrPriv->modes[i]->mode;
	    if (client->swapped)
	    {
		swapl (&modeinfos[i].id, n);
		swaps (&modeinfos[i].width, n);
		swaps (&modeinfos[i].height, n);
		swapl (&modeinfos[i].mmWidth, n);
		swapl (&modeinfos[i].mmHeight, n);
		swapl (&modeinfos[i].dotClock, n);
		swaps (&modeinfos[i].hSyncStart, n);
		swaps (&modeinfos[i].hSyncEnd, n);
		swaps (&modeinfos[i].hTotal, n);
		swaps (&modeinfos[i].hSkew, n);
		swaps (&modeinfos[i].vSyncStart, n);
		swaps (&modeinfos[i].vSyncEnd, n);
		swaps (&modeinfos[i].vTotal, n);
		swaps (&modeinfos[i].nameLength, n);
		swapl (&modeinfos[i].modeFlags, n);
	    }
	    memcpy (names, pScrPriv->modes[i]->name, 
		    pScrPriv->modes[i]->mode.nameLength);
	    names += pScrPriv->modes[i]->mode.nameLength;
	}
	assert ((names + 3 >> 3) == rep.length);
    }
    
    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.timestamp, n);
	swapl(&rep.configTimestamp, n);
	swaps(&rep.nCrtcs, n);
	swaps(&rep.nOutputs, n);
	swaps(&rep.nModes, n);
	swaps(&rep.nbytesNames, n);
    }
    WriteToClient(client, sizeof(xRRGetScreenResourcesReply), (char *)&rep);
    if (extraLen)
    {
	WriteToClient (client, extraLen, (char *) extra);
	xfree (extra);
    }
    return client->noClientException;
}

static int
ProcRRGetOutputInfo (ClientPtr client)
{
    REQUEST(xRRGetOutputInfoReq);;
    xRRGetOutputInfoReply	rep;
    RROutputPtr			output;
    CARD8			*extra;
    unsigned long		extraLen;
    ScreenPtr			pScreen;
    rrScrPrivPtr		pScrPriv;
    RRCrtc			*crtcs;
    RRMode			*modes;
    RROutput			*clones;
    char			*name;
    int				i, n;
    
    REQUEST_SIZE_MATCH(xRRGetOutputInfoReq);
    output = (RROutputPtr)SecurityLookupIDByType(client, stuff->output, 
						 RROutputType, 
						 SecurityReadAccess);

    if (!output)
	return RRErrorBase + BadRROutput;

    pScreen = output->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.timestamp = pScrPriv->lastSetTime.milliseconds;
    rep.crtc = output->crtc ? output->crtc->id : None;
    rep.connection = output->connection;
    rep.subpixelOrder = output->subpixelOrder;
    rep.nCrtcs = output->numCrtcs;
    rep.nModes = output->numModes;
    rep.nClones = output->numClones;
    rep.nameLength = output->nameLength;
    
    rep.length = (output->numCrtcs + 
		  output->numModes + 
		  output->numClones +
		  ((rep.nameLength + 3) >> 2));

    extraLen = rep.length << 2;
    extra = xalloc (extraLen);
    if (!extra)
	return BadAlloc;

    crtcs = (RRCrtc *) extra;
    modes = (RRMode *) (crtcs + output->numCrtcs);
    clones = (RROutput *) (modes + output->numModes);
    name = (char *) (clones + output->numClones);
    
    for (i = 0; i < output->numCrtcs; i++)
    {
	crtcs[i] = output->crtcs[i]->id;
	if (client->swapped)
	    swapl (&crtcs[i], n);
    }
    for (i = 0; i < output->numModes; i++)
    {
	modes[i] = output->modes[i]->mode.id;
	if (client->swapped)
	    swapl (&modes[i], n);
    }
    for (i = 0; i < output->numClones; i++)
    {
	clones[i] = output->clones[i]->id;
	if (client->swapped)
	    swapl (&clones[i], n);
    }
    memcpy (name, output->name, output->nameLength);
    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.timestamp, n);
	swapl(&rep.crtc, n);
	swaps(&rep.nCrtcs, n);
	swaps(&rep.nModes, n);
	swaps(&rep.nClones, n);
	swaps(&rep.nameLength, n);
    }
    WriteToClient(client, sizeof(xRRGetOutputInfoReply), (char *)&rep);
    if (extraLen)
    {
	WriteToClient (client, extraLen, (char *) extra);
	xfree (extra);
    }
    
    return client->noClientException;
}

static int
ProcRRListOutputProperties (ClientPtr client)
{
    REQUEST(xRRListOutputPropertiesReq);
    
    REQUEST_SIZE_MATCH(xRRListOutputPropertiesReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRChangeOutputProperty (ClientPtr client)
{
    REQUEST(xRRChangeOutputPropertyReq);
    
    REQUEST_SIZE_MATCH(xRRChangeOutputPropertyReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRDeleteOutputProperty (ClientPtr client)
{
    REQUEST(xRRDeleteOutputPropertyReq);
    
    REQUEST_SIZE_MATCH(xRRDeleteOutputPropertyReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRGetOutputProperty (ClientPtr client)
{
    REQUEST(xRRGetOutputPropertyReq);
    
    REQUEST_SIZE_MATCH(xRRGetOutputPropertyReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRCreateMode (ClientPtr client)
{
    REQUEST(xRRCreateModeReq);
    
    REQUEST_SIZE_MATCH(xRRCreateModeReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRDestroyMode (ClientPtr client)
{
    REQUEST(xRRDestroyModeReq);
    
    REQUEST_SIZE_MATCH(xRRDestroyModeReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRAddOutputMode (ClientPtr client)
{
    REQUEST(xRRAddOutputModeReq);
    
    REQUEST_SIZE_MATCH(xRRAddOutputModeReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRDeleteOutputMode (ClientPtr client)
{
    REQUEST(xRRDeleteOutputModeReq);
    
    REQUEST_SIZE_MATCH(xRRDeleteOutputModeReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRGetCrtcInfo (ClientPtr client)
{
    REQUEST(xRRGetCrtcInfoReq);
    
    REQUEST_SIZE_MATCH(xRRGetCrtcInfoReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRSetCrtcConfig (ClientPtr client)
{
    REQUEST(xRRSetCrtcConfigReq);
    xRRSetCrtcConfigReply   rep;
    ScreenPtr		    pScreen;
    rrScrPrivPtr	    pScrPriv;
    RRCrtcPtr		    crtc;
    RRModePtr		    mode;
    int			    numOutputs;
    RROutputPtr		    *outputs = NULL;
    RROutput		    *outputIds;
    TimeStamp		    configTime;
    TimeStamp		    time;
    Rotation		    rotation;
    int			    i, j;
    
    REQUEST_AT_LEAST_SIZE(xRRSetCrtcConfigReq);
    numOutputs = stuff->length - (SIZEOF (xRRSetCrtcConfigReq) >> 2);
    
    crtc = LookupIDByType (stuff->crtc, RRCrtcType);
    if (!crtc)
    {
	client->errorValue = stuff->crtc;
	return RRErrorBase + BadRRCrtc;
    }
    if (stuff->mode == None)
    {
	mode = NULL;
	if (numOutputs > 0)
	    return BadMatch;
    }
    else
    {
	mode = LookupIDByType (stuff->mode, RRModeType);
	if (!mode)
	{
	    client->errorValue = stuff->mode;
	    return RRErrorBase + BadRRMode;
	}
	if (numOutputs == 0)
	    return BadMatch;
    }
    outputs = xalloc (numOutputs * sizeof (RROutputPtr));
    if (!outputs)
	return BadAlloc;
    
    outputIds = (RROutput *) (stuff + 1);
    for (i = 0; i < numOutputs; i++)
    {
	outputs[i] = LookupIDByType (outputIds[i], RROutputType);
	if (!outputs[i])
	{
	    client->errorValue = outputIds[i];
	    return RRErrorBase + BadRROutput;
	}
	/* validate crtc for this output */
	for (j = 0; j < outputs[i]->numCrtcs; j++)
	    if (outputs[i]->crtcs[j] == crtc)
		break;
	if (j == outputs[j]->numCrtcs)
	    return BadMatch;
	/* validate mode for this output */
	for (j = 0; j < outputs[i]->numModes; j++)
	    if (outputs[i]->modes[j] == mode)
		break;
	if (j == outputs[j]->numModes)
	    return BadMatch;
    }

    pScreen = crtc->pScreen;
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

    if ((~crtc->rotations) & rotation)
    {
	/*
	 * requested rotation or reflection not supported by screen
	 */
	client->errorValue = stuff->rotation;
	return BadMatch;
    }

    if (stuff->x + mode->mode.width > pScreen->width)
    {
	client->errorValue = stuff->x;
	return BadValue;
    }
    
    if (stuff->y + mode->mode.height > pScreen->height)
    {
	client->errorValue = stuff->y;
	return BadValue;
    }
    
    /*
     * Make sure the requested set-time is not older than
     * the last set-time
     */
    if (CompareTimeStamps (time, pScrPriv->lastSetTime) < 0)
    {
	rep.status = RRSetConfigInvalidTime;
	goto sendReply;
    }

    rep.status = RRCrtcSet (crtc, mode, stuff->x, stuff->y,
			    rotation, numOutputs, outputs);
    
sendReply:
    if (outputs)
	xfree (outputs);
    
    rep.type = X_Reply;
    /* rep.status has already been filled in */
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.newTimestamp = pScrPriv->lastConfigTime.milliseconds;

    if (client->swapped) 
    {
	int n;
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.newTimestamp, n);
    }
    WriteToClient(client, sizeof(xRRSetCrtcConfigReply), (char *)&rep);
    
    return client->noClientException;
}

static int
ProcRRGetCrtcGammaSize (ClientPtr client)
{
    REQUEST(xRRGetCrtcGammaSizeReq);
    
    REQUEST_SIZE_MATCH(xRRGetCrtcGammaSizeReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRGetCrtcGamma (ClientPtr client)
{
    REQUEST(xRRGetCrtcGammaReq);
    
    REQUEST_SIZE_MATCH(xRRGetCrtcGammaReq);
    (void) stuff;
    return BadImplementation; 
}

static int
ProcRRSetCrtcGamma (ClientPtr client)
{
    REQUEST(xRRSetCrtcGammaReq);
    
    REQUEST_SIZE_MATCH(xRRSetCrtcGammaReq);
    (void) stuff;
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
    ProcRRGetScreenResources,	/* 8 */
    ProcRRGetOutputInfo,	/* 9 */
    ProcRRListOutputProperties,	/* 10 */
    ProcRRChangeOutputProperty,	/* 11 */
    ProcRRDeleteOutputProperty,	/* 12 */
    ProcRRGetOutputProperty,	/* 13 */
    ProcRRCreateMode,		/* 14 */
    ProcRRDestroyMode,		/* 15 */
    ProcRRAddOutputMode,	/* 16 */
    ProcRRDeleteOutputMode,	/* 17 */
    ProcRRGetCrtcInfo,		/* 18 */
    ProcRRSetCrtcConfig,	/* 19 */
    ProcRRGetCrtcGammaSize,	/* 20 */
    ProcRRGetCrtcGamma,		/* 21 */
    ProcRRSetCrtcGamma,		/* 22 */
};

