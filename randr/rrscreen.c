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

extern char	*ConnectionInfo;

static int padlength[4] = {0, 3, 2, 1};

/*
 * Edit connection information block so that new clients
 * see the current screen size on connect
 */
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

void
RRDeliverScreenEvent (ClientPtr client, WindowPtr pWin, ScreenPtr pScreen)
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

/*
 * Notify the extension that the screen size has been changed.
 * The driver is responsible for calling this whenever it has changed
 * the size of the screen
 */
void
RRScreenSizeNotify (ScreenPtr	pScreen)
{
    rrScrPriv(pScreen);
    /*
     * Deliver ConfigureNotify events when root changes
     * pixel size
     */
    if (pScrPriv->width == pScreen->width &&
	pScrPriv->height == pScreen->height)
	return;
    
    pScrPriv->width = pScreen->width;
    pScrPriv->height = pScreen->height;
    pScrPriv->changed = TRUE;

    RRSendConfigNotify (pScreen);
    RREditConnectionInfo (pScreen);
    
    /*
     * Fix pointer bounds and location
     */
    ScreenRestructured (pScreen);
}

/*
 * Request that the screen be resized
 */
Bool
RRScreenSizeSet (ScreenPtr  pScreen,
		 CARD16	    width,
		 CARD16	    height,
		 CARD32	    mmWidth,
		 CARD32	    mmHeight)
{
    rrScrPriv(pScreen);

#if RANDR_12_INTERFACE
    if (pScrPriv->rrScreenSizeSet)
    {
	return (*pScrPriv->rrScreenSizeSet) (pScreen,
					     width, height,
					     mmWidth, mmHeight);
    }
#endif
#if RANDR_10_INTERFACE
    if (pScrPriv->rrSetConfig)
    {
	return TRUE;	/* can't set size separately */
    }
#endif
    return FALSE;
}

