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

RESTYPE	RRCrtcType;

/*
 * Create a CRTC
 */
RRCrtcPtr
RRCrtcCreate (ScreenPtr	pScreen,
	      void	*devPrivate)
{
    rrScrPriv (pScreen);
    RRCrtcPtr	crtc;
    RRCrtcPtr	*crtcs;
    
    crtc = xalloc (sizeof (RRCrtcRec));
    if (!crtc)
	return NULL;
    if (pScrPriv->numCrtcs)
	crtcs = xrealloc (pScrPriv->crtcs, 
			  (pScrPriv->numCrtcs + 1) * sizeof (RRCrtcPtr));
    else
	crtcs = xalloc (sizeof (RRCrtcPtr));
    if (!crtcs)
    {
	xfree (crtc);
	return NULL;
    }
    crtc->id = FakeClientID (0);
    crtc->pScreen = pScreen;
    crtc->mode = NULL;
    crtc->x = 0;
    crtc->y = 0;
    crtc->rotation = RR_Rotate_0;
    crtc->rotations = RR_Rotate_0;
    crtc->outputs = NULL;
    crtc->numOutputs = 0;
    crtc->changed = TRUE;
    crtc->devPrivate = devPrivate;

    if (!AddResource (crtc->id, RRCrtcType, (pointer) crtc))
	return NULL;

    pScrPriv->crtcs = crtcs;
    pScrPriv->crtcs[pScrPriv->numCrtcs++] = crtc;
    pScrPriv->changed = TRUE;
    return crtc;
}

/*
 * Notify the extension that the Crtc has been reconfigured,
 * the driver calls this whenever it has updated the mode
 */
Bool
RRCrtcNotify (RRCrtcPtr	    crtc,
	      RRModePtr	    mode,
	      int	    x,
	      int	    y,
	      Rotation	    rotation,
	      int	    numOutputs,
	      RROutputPtr   *outputs)
{
    ScreenPtr	pScreen = crtc->pScreen;
    rrScrPriv(pScreen);
    int	    i, j;
    int	    prevNumOutputs = crtc->numOutputs;
    
    if (numOutputs != prevNumOutputs)
    {
	RROutputPtr *outputs;
	
	if (crtc->numOutputs)
	    outputs = xrealloc (crtc->outputs,
				numOutputs * sizeof (RROutputPtr));
	else
	    outputs = xalloc (numOutputs * sizeof (RROutputPtr));
	if (!outputs)
	    return FALSE;
	crtc->outputs = outputs;
    }
    for (i = 0; i < numOutputs; i++)
    {
	for (j = 0; j < crtc->numOutputs; j++)
	    if (outputs[i] == crtc->outputs[j])
		break;
	if (j != crtc->numOutputs)
	{
	    outputs[i]->changed = TRUE;
	    crtc->changed = TRUE;
	}
    }
    for (j = 0; j < crtc->numOutputs; j++)
    {
	for (i = 0; i < numOutputs; i++)
	    if (outputs[i] == crtc->outputs[j])
		break;
	if (i != numOutputs)
	{
	    crtc->outputs[j]->changed = TRUE;
	    crtc->changed = TRUE;
	}
    }
    if (mode != crtc->mode)
    {
	if (crtc->mode)
	    RRModeDestroy (crtc->mode);
	crtc->mode = mode;
	mode->refcnt++;
	crtc->changed = TRUE;
    }
    if (x != crtc->x)
    {
	crtc->x = x;
	crtc->changed = TRUE;
    }
    if (y != crtc->y)
    {
	crtc->y = y;
	crtc->changed = TRUE;
    }
    if (rotation != crtc->rotation)
    {
	crtc->rotation = rotation;
	crtc->changed = TRUE;
    }
    if (crtc->changed)
	pScrPriv->changed = TRUE;
    return TRUE;
}

void
RRDeliverCrtcEvent (ClientPtr client, WindowPtr pWin, RRCrtcPtr crtc)
{
    
}

/*
 * Request that the Crtc be reconfigured
 */
Bool
RRCrtcSet (RRCrtcPtr    crtc,
	   RRModePtr	mode,
	   int		x,
	   int		y,
	   Rotation	rotation,
	   int		numOutputs,
	   RROutputPtr  *outputs)
{
    ScreenPtr	pScreen = crtc->pScreen;
    rrScrPriv(pScreen);

    /* See if nothing changed */
    if (crtc->mode == mode &&
	crtc->x == x &&
	crtc->y == y &&
	crtc->rotation == rotation &&
	crtc->numOutputs == numOutputs &&
	!memcmp (crtc->outputs, outputs, numOutputs * sizeof (RROutputPtr)))
    {
	return TRUE;
    }
#if RANDR_12_INTERFACE
    if (pScrPriv->rrCrtcSet)
    {
	return (*pScrPriv->rrCrtcSet) (pScreen, crtc, mode, x, y, 
				       rotation, numOutputs, outputs);
    }
#endif
#if RANDR_10_INTERFACE
    if (pScrPriv->rrSetConfig)
    {
	RRScreenSize	    size;
	RRScreenRate	    rate;
	Bool		    ret;

	size.width = mode->mode.width;
	size.height = mode->mode.height;
	size.mmWidth = mode->mode.mmWidth;
	size.mmHeight = mode->mode.mmHeight;
	size.nRates = 1;
	rate.rate = RRVerticalRefresh (&mode->mode);
	size.pRates = &rate;
	ret = (*pScrPriv->rrSetConfig) (pScreen, rotation, rate.rate, &size);
	/*
	 * Old 1.0 interface tied screen size to mode size
	 */
	if (ret)
	    RRScreenSizeNotify (pScreen);
	return ret;
    }
#endif
    return FALSE;
}

/*
 * Destroy a Crtc at shutdown
 */
void
RRCrtcDestroy (RRCrtcPtr crtc)
{
    FreeResource (crtc->id, 0);
}

static int
RRCrtcDestroyResource (pointer value, XID pid)
{
    RRCrtcPtr	crtc = (RRCrtcPtr) value;
    ScreenPtr	pScreen = crtc->pScreen;
    rrScrPriv(pScreen);
    int		i;

    for (i = 0; i < pScrPriv->numCrtcs; i++)
    {
	if (pScrPriv->crtcs[i] == crtc)
	{
	    memmove (pScrPriv->crtcs, pScrPriv->crtcs + 1,
		     (pScrPriv->numCrtcs - (i - 1)) * sizeof (RRCrtcPtr));
	    --pScrPriv->numCrtcs;
	    break;
	}
    }
    free (value);
    return 1;
}

/*
 * Initialize crtc type
 */
Bool
RRCrtcInit (void)
{
    RRCrtcType = CreateNewResourceType (RRCrtcDestroyResource);
    if (!RRCrtcType)
	return FALSE;
#ifdef XResExtension
	RegisterResourceName (RRCrtcType, "CRTC");
#endif
    return TRUE;
}
