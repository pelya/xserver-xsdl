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
		char	    *name,
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
    output->numCrtcs = 0;
    output->crtcs = NULL;
    output->numClones = 0;
    output->clones = NULL;
    output->numModes = 0;
    output->modes = NULL;
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

    newClones = xalloc (numClones * sizeof (RROutputPtr));
    if (!newClones)
	return FALSE;
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
		  int		numModes)
{
    RRModePtr	*newModes;

    newModes = xalloc (numModes * sizeof (RRModePtr));
    if (!newModes)
	return FALSE;
    if (output->modes)
	xfree (output->modes);
    memcpy (newModes, modes, numModes * sizeof (RRModePtr));
    output->modes = newModes;
    output->numModes = numModes;
    output->changed = TRUE;
    return TRUE;
}

Bool
RROutputSetCrtcs (RROutputPtr	output,
		  RRCrtcPtr	*crtcs,
		  int		numCrtcs)
{
    RRCrtcPtr	*newCrtcs;

    newCrtcs = xalloc (numCrtcs * sizeof (RRCrtcPtr));
    if (!newCrtcs)
	return FALSE;
    if (output->crtcs)
	xfree (output->crtcs);
    memcpy (newCrtcs, crtcs, numCrtcs * sizeof (RRCrtcPtr));
    output->crtcs = newCrtcs;
    output->numCrtcs = numCrtcs;
    output->changed = TRUE;
    return TRUE;
}

void
RROutputSetCrtc (RROutputPtr output, RRCrtcPtr crtc)
{
    output->crtc = crtc;
    output->changed = TRUE;
}

Bool
RROutputSetConnection (RROutputPtr  output,
		       CARD8	    connection)
{
    output->connection = connection;
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
	    memmove (pScrPriv->outputs, pScrPriv->outputs + 1,
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
    xfree (output);
    return 1;
}

/*
 * Initialize crtc type
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
