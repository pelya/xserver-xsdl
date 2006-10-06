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

RESTYPE	RROutputType;

/*
 * Create an output
 */

RROutputPtr
RROutputCreate (ScreenPtr   pScreen,
		const char  *name,
		int	    nameLength,
		void	    *devPrivate)
{
    rrScrPriv (pScreen);
    RROutputPtr	output;
    RROutputPtr	*outputs;

    output = xalloc (sizeof (RROutputRec) + nameLength + 1);
    if (!output)
	return NULL;
    if (pScrPriv->numOutputs)
	outputs = xrealloc (pScrPriv->outputs, 
			    (pScrPriv->numOutputs + 1) * sizeof (RROutputPtr));
    else
	outputs = xalloc (sizeof (RROutputPtr));
    if (!outputs)
    {
	xfree (output);
	return NULL;
    }
    output->id = FakeClientID (0);
    output->pScreen = pScreen;
    output->name = (char *) (output + 1);
    output->nameLength = nameLength;
    memcpy (output->name, name, nameLength);
    output->name[nameLength] = '\0';
    output->connection = RR_UnknownConnection;
    output->subpixelOrder = SubPixelUnknown;
    output->crtc = NULL;
    output->currentOptions = 0;
    output->possibleOptions = 0;
    output->numCrtcs = 0;
    output->crtcs = NULL;
    output->numClones = 0;
    output->clones = NULL;
    output->numModes = 0;
    output->numPreferred = 0;
    output->modes = NULL;
    output->properties = NULL;
    output->changed = TRUE;
    output->devPrivate = devPrivate;
    
    if (!AddResource (output->id, RROutputType, (pointer) output))
	return NULL;

    pScrPriv->outputs = outputs;
    pScrPriv->outputs[pScrPriv->numOutputs++] = output;
    pScrPriv->changed = TRUE;
    return output;
}

/*
 * Notify extension that output parameters have been changed
 */
Bool
RROutputSetClones (RROutputPtr  output,
		   RROutputPtr  *clones,
		   int		numClones)
{
    RROutputPtr	*newClones;
    int		i;

    if (numClones == output->numClones)
    {
	for (i = 0; i < numClones; i++)
	    if (output->clones[i] != clones[i])
		break;
	if (i == numClones)
	    return TRUE;
    }
    if (numClones)
    {
	newClones = xalloc (numClones * sizeof (RROutputPtr));
	if (!newClones)
	    return FALSE;
    }
    else
	newClones = NULL;
    if (output->clones)
	xfree (output->clones);
    memcpy (newClones, clones, numClones * sizeof (RROutputPtr));
    output->clones = newClones;
    output->numClones = numClones;
    output->changed = TRUE;
    return TRUE;
}

Bool
RROutputSetModes (RROutputPtr	output,
		  RRModePtr	*modes,
		  int		numModes,
		  int		numPreferred)
{
    RRModePtr	*newModes;
    int		i;

    if (numModes == output->numModes && numPreferred == output->numPreferred)
    {
	for (i = 0; i < numModes; i++)
	    if (output->modes[i] != modes[i])
		break;
	if (i == numModes)
	{
	    for (i = 0; i < numModes; i++)
		RRModeDestroy (modes[i]);
	    return TRUE;
	}
    }

    if (numModes)
    {
	newModes = xalloc (numModes * sizeof (RRModePtr));
	if (!newModes)
	    return FALSE;
    }
    else
	newModes = NULL;
    if (output->modes)
    {
	for (i = 0; i < output->numModes; i++)
	    RRModeDestroy (output->modes[i]);
	xfree (output->modes);
    }
    memcpy (newModes, modes, numModes * sizeof (RRModePtr));
    output->modes = newModes;
    output->numModes = numModes;
    output->numPreferred = numPreferred;
    output->changed = TRUE;
    return TRUE;
}

Bool
RROutputSetCrtcs (RROutputPtr	output,
		  RRCrtcPtr	*crtcs,
		  int		numCrtcs)
{
    RRCrtcPtr	*newCrtcs;
    int		i;

    if (numCrtcs == output->numCrtcs)
    {
	for (i = 0; i < numCrtcs; i++)
	    if (output->crtcs[i] != crtcs[i])
		break;
	if (i == numCrtcs)
	    return TRUE;
    }
    if (numCrtcs)
    {
	newCrtcs = xalloc (numCrtcs * sizeof (RRCrtcPtr));
	if (!newCrtcs)
	    return FALSE;
    }
    else
	newCrtcs = NULL;
    if (output->crtcs)
	xfree (output->crtcs);
    memcpy (newCrtcs, crtcs, numCrtcs * sizeof (RRCrtcPtr));
    output->crtcs = newCrtcs;
    output->numCrtcs = numCrtcs;
    output->changed = TRUE;
    return TRUE;
}

Bool
RROutputSetPossibleOptions (RROutputPtr	output,
			    CARD32	possibleOptions)
{
    if (output->possibleOptions == possibleOptions)
	return TRUE;
    output->possibleOptions = possibleOptions;
    output->changed = TRUE;
    return TRUE;
}

void
RROutputSetCrtc (RROutputPtr output, RRCrtcPtr crtc)
{
    if (output->crtc == crtc)
	return;
    output->crtc = crtc;
    output->changed = TRUE;
}

Bool
RROutputSetConnection (RROutputPtr  output,
		       CARD8	    connection)
{
    if (output->connection == connection)
	return TRUE;
    output->connection = connection;
    output->changed = TRUE;
    return TRUE;
}

Bool
RROutputSetSubpixelOrder (RROutputPtr output,
			  int	      subpixelOrder)
{
    if (output->subpixelOrder == subpixelOrder)
	return TRUE;

    output->subpixelOrder = subpixelOrder;
    output->changed = TRUE;
    return TRUE;
}

Bool
RROutputSetCurrentOptions (RROutputPtr output,
			   CARD32      currentOptions)
{
    if (output->currentOptions == currentOptions)
	return TRUE;
    output->currentOptions = currentOptions;
    output->changed = TRUE;
    return TRUE;
}

void
RRDeliverOutputEvent(ClientPtr client, WindowPtr pWin, RROutputPtr output)
{
}

/*
 * Destroy a Output at shutdown
 */
void
RROutputDestroy (RROutputPtr crtc)
{
    FreeResource (crtc->id, 0);
}

static int
RROutputDestroyResource (pointer value, XID pid)
{
    RROutputPtr	output = (RROutputPtr) value;
    ScreenPtr	pScreen = output->pScreen;
    rrScrPriv(pScreen);
    int		i;

    for (i = 0; i < pScrPriv->numOutputs; i++)
    {
	if (pScrPriv->outputs[i] == output)
	{
	    memmove (pScrPriv->outputs + i, pScrPriv->outputs + i + 1,
		     (pScrPriv->numOutputs - (i - 1)) * sizeof (RROutputPtr));
	    --pScrPriv->numOutputs;
	    break;
	}
    }
    if (output->modes)
	xfree (output->modes);
    if (output->crtcs)
	xfree (output->crtcs);
    if (output->clones)
	xfree (output->clones);
    RRDeleteAllOutputProperties (output);
    xfree (output);
    return 1;
}

/*
 * Initialize output type
 */
Bool
RROutputInit (void)
{
    RROutputType = CreateNewResourceType (RROutputDestroyResource);
    if (!RROutputType)
	return FALSE;
#ifdef XResExtension
	RegisterResourceName (RROutputType, "OUTPUT");
#endif
    return TRUE;
}

#define OutputInfoExtra	(SIZEOF(xRRGetOutputInfoReply) - 32)
				
int
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
    output = LookupOutput(client, stuff->output, SecurityReadAccess);

    if (!output)
	return RRErrorBase + BadRROutput;

    pScreen = output->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = OutputInfoExtra >> 2;
    rep.timestamp = pScrPriv->lastSetTime.milliseconds;
    rep.crtc = output->crtc ? output->crtc->id : None;
    rep.currentOptions = output->currentOptions;
    rep.connection = output->connection;
    rep.subpixelOrder = output->subpixelOrder;
    rep.nCrtcs = output->numCrtcs;
    rep.nModes = output->numModes;
    rep.nPreferred = output->numPreferred;
    rep.nClones = output->numClones;
    rep.nameLength = output->nameLength;
    rep.possibleOptions = output->possibleOptions;
    
    extraLen = ((output->numCrtcs + 
		 output->numModes + 
		 output->numClones +
		 ((rep.nameLength + 3) >> 2)) << 2);

    if (extraLen)
    {
	rep.length += extraLen >> 2;
	extra = xalloc (extraLen);
	if (!extra)
	    return BadAlloc;
    }
    else
	extra = NULL;

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

