/* $Xorg: lbxswap.c,v 1.4 2001/02/09 02:05:17 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * Copyright 1992 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of NCD. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCD. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NCD.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/lbx/lbxswap.c,v 1.4 2001/12/14 20:00:01 dawes Exp $ */

#include <sys/types.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "Xos.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "swaprep.h"
#include "propertyst.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#include "Xfuncproto.h"

#include <stdio.h>

static int
SProcLbxQueryVersion(ClientPtr client)
{
    register int n;

    REQUEST(xLbxQueryVersionReq);

    swaps(&stuff->length, n);
    return ProcLbxQueryVersion(client);
}

static int
SProcLbxStartProxy(ClientPtr client)
{
    register int n;

    REQUEST(xLbxStartProxyReq);

    swaps(&stuff->length, n);
    return ProcLbxStartProxy(client);
}

static int
SProcLbxStopProxy(ClientPtr client)
{
    register int n;

    REQUEST(xLbxStopProxyReq);

    swaps(&stuff->length, n);
    return ProcLbxStopProxy(client);
}

int
SProcLbxSwitch(ClientPtr client)
{
    register int n;

    REQUEST(xLbxSwitchReq);

    swaps(&stuff->length, n);
    swapl(&stuff->client, n);
    return ProcLbxSwitch(client);
}

int
SProcLbxBeginLargeRequest (ClientPtr client)
{
    register int n;

    REQUEST(xLbxBeginLargeRequestReq);

    swaps(&stuff->length, n);
    swapl(&stuff->largeReqLength, n);
    return ProcLbxBeginLargeRequest(client);
}

int
SProcLbxLargeRequestData (ClientPtr client)
{
    register int n;

    REQUEST(xLbxLargeRequestDataReq);

    swaps(&stuff->length, n);
    return ProcLbxLargeRequestData(client);
}

int
SProcLbxEndLargeRequest (ClientPtr client)
{
    register int n;

    REQUEST(xLbxEndLargeRequestReq);

    swaps(&stuff->length, n);
    return ProcLbxEndLargeRequest(client);
}

static int
SProcLbxNewClient(ClientPtr client)
{
    register int n;

    REQUEST(xLbxNewClientReq);

    swaps(&stuff->length, n);
    swapl(&stuff->client, n);
    return ProcLbxNewClient(client);
}

static int
SProcLbxCloseClient(ClientPtr client)
{
    register int n;

    REQUEST(xLbxCloseClientReq);

    swaps(&stuff->length, n);
    swapl(&stuff->client, n);
    return ProcLbxCloseClient(client);
}

static int
SProcLbxModifySequence(ClientPtr client)
{
    register int n;

    REQUEST(xLbxModifySequenceReq);

    swaps(&stuff->length, n);
    swapl(&stuff->adjust, n);
    return ProcLbxModifySequence(client);
}

static int
SProcLbxAllowMotion(ClientPtr client)
{
    register int n;

    REQUEST(xLbxAllowMotionReq);

    swaps(&stuff->length, n);
    swapl(&stuff->num, n);
    return ProcLbxAllowMotion(client);
}

static int
SProcLbxIncrementPixel(ClientPtr client)
{
    register int n;

    REQUEST(xLbxIncrementPixelReq);

    swaps(&stuff->length, n);
    swapl(&stuff->cmap, n);
    swapl(&stuff->pixel, n);
    return ProcLbxIncrementPixel(client);
}

static int
SProcLbxGrabCmap(ClientPtr client)
{
    register int n;

    REQUEST(xLbxGrabCmapReq);

    swaps(&stuff->length, n);
    swapl(&stuff->cmap, n);

    return ProcLbxGrabCmap(client);
}

static int
SProcLbxReleaseCmap(ClientPtr client)
{
    register int n;

    REQUEST(xLbxReleaseCmapReq);

    swaps(&stuff->length, n);
    swapl(&stuff->cmap, n);

    return ProcLbxReleaseCmap(client);
}

static int
SProcLbxAllocColor(ClientPtr client)
{
    register int n;

    REQUEST(xLbxAllocColorReq);

    swaps(&stuff->length, n);
    swapl(&stuff->cmap, n);
    swapl(&stuff->pixel, n);
    swaps(&stuff->red, n);
    swaps(&stuff->green, n);
    swaps(&stuff->blue, n);

    return ProcLbxAllocColor(client);
}

static int
SProcLbxGetModifierMapping(ClientPtr client)
{
    register int n;

    REQUEST(xLbxGetModifierMappingReq);

    swaps(&stuff->length, n);
    return ProcLbxGetModifierMapping(client);
}

static int
SProcLbxGetKeyboardMapping(ClientPtr client)
{
    register int n;

    REQUEST(xLbxGetKeyboardMappingReq);

    swaps(&stuff->length, n);
    return ProcLbxGetKeyboardMapping(client);
}

static int
SProcLbxQueryFont(ClientPtr client)
{
    register int n;

    REQUEST(xLbxQueryFontReq);

    swaps(&stuff->length, n);
    swapl(&stuff->fid, n);
    return ProcLbxQueryFont(client);
}

static int
SProcLbxChangeProperty(ClientPtr client)
{
    register int n;

    REQUEST(xLbxChangePropertyReq);

    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xLbxChangePropertyReq);
    swapl(&stuff->window, n);
    swapl(&stuff->property, n);
    swapl(&stuff->type, n);
    swapl(&stuff->nUnits, n);
    switch (stuff->format) {
    case 8:
	break;
    case 16:
	SwapRestS(stuff);
	break;
    case 32:
	SwapRestL(stuff);
	break;
    }
    return ProcLbxChangeProperty(client);
}

static int
SProcLbxGetProperty(ClientPtr client)
{
    register int n;

    REQUEST(xLbxGetPropertyReq);

    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    swapl(&stuff->property, n);
    swapl(&stuff->type, n);
    swapl(&stuff->longOffset, n);
    swapl(&stuff->longLength, n);
    return ProcLbxGetProperty(client);
}

static int
SProcLbxTagData(ClientPtr client)
{
    register int n;

    REQUEST(xLbxTagDataReq);

    swaps(&stuff->length, n);
    swapl(&stuff->tag, n);
    swapl(&stuff->real_length, n);
    return ProcLbxTagData(client);
}

static int
SProcLbxInvalidateTag(ClientPtr client)
{
    register int n;

    REQUEST(xLbxInvalidateTagReq);

    swaps(&stuff->length, n);
    swapl(&stuff->tag, n);
    return ProcLbxInvalidateTag(client);
}

static int
SProcLbxPoly(ClientPtr client)
{
    register int n;

    REQUEST(xLbxPolyPointReq);
    char *after;

    swaps(&stuff->length, n);
    after = ((char *) stuff) + SIZEOF(xLbxPolyPointReq);
    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	swapl (((GContext *) after), n);
    return ProcLbxDispatch(client);
}

static int
SProcLbxFillPoly(ClientPtr client)
{
    register int n;

    REQUEST(xLbxFillPolyReq);
    char *after;

    swaps(&stuff->length, n);
    after = ((char *) stuff) + SIZEOF(xLbxFillPolyReq);
    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	swapl (((GContext *) after), n);
    return ProcLbxFillPoly(client);
}

static int
SProcLbxCopyArea(ClientPtr client)
{
    int    n;

    REQUEST(xLbxCopyAreaReq);
    char *after;

    swaps(&stuff->length, n);
    after = ((char *) stuff) + SIZEOF(xLbxCopyAreaReq);
    if (GFXdCacheEnt (stuff->srcCache) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	swapl (((GContext *) after), n);
    return ProcLbxCopyArea(client);
}

static int
SProcLbxCopyPlane(ClientPtr client)
{
    int    n;

    REQUEST(xLbxCopyPlaneReq);
    char *after;

    swaps(&stuff->length, n);
    swapl(&stuff->bitPlane, n);
    after = ((char *) stuff) + SIZEOF(xLbxCopyPlaneReq);
    if (GFXdCacheEnt (stuff->srcCache) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	swapl (((GContext *) after), n);
    return ProcLbxCopyPlane(client);
}

static int
SProcLbxPolyText(ClientPtr client)
{
    register int n;

    REQUEST(xLbxPolyTextReq);
    char *after;

    swaps(&stuff->length, n);
    after = ((char *) stuff) + SIZEOF(xLbxPolyTextReq);
    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	swapl (((GContext *) after), n);
    return ProcLbxDispatch(client);
}

static int
SProcLbxImageText(ClientPtr client)
{
    register int n;

    REQUEST(xLbxImageTextReq);
    char *after;

    swaps(&stuff->length, n);
    after = ((char *) stuff) + SIZEOF(xLbxImageTextReq);
    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone)
    {
	swapl (((Drawable *) after), n);
	after += sizeof (Drawable);
    }
    if (GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	swapl (((GContext *) after), n);
    return ProcLbxDispatch(client);
}


static int
SProcLbxPutImage(ClientPtr client)
{
    register int n;

    REQUEST(xLbxPutImageReq);

    swaps (&stuff->length, n);
    return ProcLbxPutImage(client);
}

static int
SProcLbxGetImage(ClientPtr client)
{
    register int n;

    REQUEST(xLbxGetImageReq);

    swaps (&stuff->length, n);
    swapl (&stuff->drawable, n);
    swaps (&stuff->x, n);
    swaps (&stuff->y, n);
    swaps (&stuff->width, n);
    swaps (&stuff->height, n);
    swapl (&stuff->planeMask, n);
    return ProcLbxGetImage(client);
}

static int
SProcLbxInternAtoms(ClientPtr client)
{
    register int n;
    char *ptr;
    char lenbuf[2];
    CARD16 len;
    int i;

    REQUEST(xLbxInternAtomsReq);

    swaps (&stuff->length, n);
    swaps (&stuff->num, n);

    ptr = (char *) stuff + sz_xLbxInternAtomsReq;
    for (i = 0; i < stuff->num; i++)
    {
	swaps (ptr, n);
	lenbuf[0] = ptr[0];
	lenbuf[1] = ptr[1];
	len = *((CARD16 *) lenbuf);
	ptr += (len + 2);
    }

    return ProcLbxInternAtoms(client);
}


static int
SProcLbxGetWinAttrAndGeom(ClientPtr client)
{
    int	n;

    REQUEST(xLbxGetWinAttrAndGeomReq);

    swaps(&stuff->length, n);
    swapl(&stuff->id, n);

    return ProcLbxGetWinAttrAndGeom(client);
}



static int
SProcLbxQueryExtension(ClientPtr client)
{
    int	n;

    REQUEST(xLbxQueryExtensionReq);

    swaps(&stuff->length, n);
    swapl(&stuff->nbytes, n);
    return ProcLbxQueryExtension(client);
}

int
SProcLbxDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_LbxQueryVersion:
	return SProcLbxQueryVersion(client);
    case X_LbxStartProxy:
	return SProcLbxStartProxy(client);
    case X_LbxStopProxy:
	return SProcLbxStopProxy(client);
    case X_LbxNewClient:
	return SProcLbxNewClient(client);
    case X_LbxCloseClient:
	return SProcLbxCloseClient(client);
    case X_LbxModifySequence:
	return SProcLbxModifySequence(client);
    case X_LbxAllowMotion:
	return SProcLbxAllowMotion(client);
    case X_LbxIncrementPixel:
	return SProcLbxIncrementPixel(client);
    case X_LbxGrabCmap:
	return SProcLbxGrabCmap(client);
    case X_LbxReleaseCmap:
	return SProcLbxReleaseCmap(client);
    case X_LbxAllocColor:
	return SProcLbxAllocColor(client);
    case X_LbxGetModifierMapping:
	return SProcLbxGetModifierMapping(client);
    case X_LbxGetKeyboardMapping:
	return SProcLbxGetKeyboardMapping(client);
    case X_LbxInvalidateTag:
	return SProcLbxInvalidateTag(client);
    case X_LbxPolyPoint:
    case X_LbxPolyLine:
    case X_LbxPolySegment:
    case X_LbxPolyRectangle:
    case X_LbxPolyArc:
    case X_LbxPolyFillRectangle:
    case X_LbxPolyFillArc:
	return SProcLbxPoly(client);
    case X_LbxFillPoly:
	return SProcLbxFillPoly(client);
    case X_LbxQueryFont:
	return SProcLbxQueryFont(client);
    case X_LbxChangeProperty:
	return SProcLbxChangeProperty(client);
    case X_LbxGetProperty:
	return SProcLbxGetProperty(client);
    case X_LbxTagData:
	return SProcLbxTagData(client);
    case X_LbxCopyArea:
	return SProcLbxCopyArea(client);
    case X_LbxCopyPlane:
	return SProcLbxCopyPlane(client);
    case X_LbxPolyText8:
    case X_LbxPolyText16:
	return SProcLbxPolyText(client);
    case X_LbxImageText8:
    case X_LbxImageText16:
	return SProcLbxImageText (client);
    case X_LbxQueryExtension:
	return SProcLbxQueryExtension(client);
    case X_LbxPutImage:
	return SProcLbxPutImage(client);
    case X_LbxGetImage:
	return SProcLbxGetImage(client);
    case X_LbxInternAtoms:
	return SProcLbxInternAtoms(client);
    case X_LbxGetWinAttrAndGeom:
	return SProcLbxGetWinAttrAndGeom(client);
    case X_LbxSync:
	return ProcLbxSync(client);		/* nothing to swap */
    case X_LbxBeginLargeRequest:
	return SProcLbxBeginLargeRequest(client);
    case X_LbxLargeRequestData:
	return SProcLbxLargeRequestData(client);
    default:
	return BadRequest;
    }
}

#ifdef notyet
void
LbxWriteSConnectionInfo(ClientPtr	pClient,
			unsigned long	size,
			char 		*pInfo)
{
    int		i, j, k;
    ScreenPtr	pScreen;
    DepthPtr	pDepth;
    char	*pInfoT, *pInfoTBase;
    xConnSetup	*pConnSetup = (xConnSetup *)pInfo;

    pInfoT = pInfoTBase = (char *) ALLOCATE_LOCAL(size);
    if (!pInfoTBase)
    {
	pClient->noClientException = -1;
	return;
    }
    SwapConnSetup(pConnSetup, (xConnSetup *)pInfoT);
    pInfo += sizeof(xConnSetup);
    pInfoT += sizeof(xConnSetup);

    /* Copy the vendor string */
    i = (pConnSetup->nbytesVendor + 3) & ~3;
    memmove(pInfoT, pInfo, i);
    pInfo += i;
    pInfoT += i;

    /* The Pixmap formats don't need to be swapped, just copied. */
    i = sizeof(xPixmapFormat) * screenInfo.numPixmapFormats;
    memmove(pInfoT, pInfo, i);
    pInfo += i;
    pInfoT += i;

    for(i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	SwapWinRoot((xWindowRoot *)pInfo, (xWindowRoot *)pInfoT);
	pInfo += sizeof(xWindowRoot);
	pInfoT += sizeof(xWindowRoot);
	pDepth = pScreen->allowedDepths;
	for(j = 0; j < pScreen->numDepths; j++, pDepth++)
	{
            ((xDepth *)pInfoT)->depth = ((xDepth *)pInfo)->depth;
	    cpswaps(((xDepth *)pInfo)->nVisuals, ((xDepth *)pInfoT)->nVisuals);
	    pInfo += sizeof(xDepth);
	    pInfoT += sizeof(xDepth);
	    for(k = 0; k < pDepth->numVids; k++)
	    {
		SwapVisual((xVisualType *)pInfo, (xVisualType *)pInfoT);
		pInfo += sizeof(xVisualType);
		pInfoT += sizeof(xVisualType);
	    }
	}
    }
    (void)WriteToClient(pClient, (int)size, (char *) pInfoTBase);
    DEALLOCATE_LOCAL(pInfoTBase);
}

void
SwapConnSetup(xConnSetup *pConnSetup, 
	      xConnSetup *pConnSetupT)
{
    cpswapl(pConnSetup->release, pConnSetupT->release);
    cpswapl(pConnSetup->ridBase, pConnSetupT->ridBase);
    cpswapl(pConnSetup->ridMask, pConnSetupT->ridMask);
    cpswapl(pConnSetup->motionBufferSize, pConnSetupT->motionBufferSize);
    cpswaps(pConnSetup->nbytesVendor, pConnSetupT->nbytesVendor);
    cpswaps(pConnSetup->maxRequestSize, pConnSetupT->maxRequestSize);
    pConnSetupT->minKeyCode = pConnSetup->minKeyCode;
    pConnSetupT->maxKeyCode = pConnSetup->maxKeyCode;
    pConnSetupT->numRoots = pConnSetup->numRoots;
    pConnSetupT->numFormats = pConnSetup->numFormats;
    pConnSetupT->imageByteOrder = pConnSetup->imageByteOrder;
    pConnSetupT->bitmapBitOrder = pConnSetup->bitmapBitOrder;
    pConnSetupT->bitmapScanlineUnit = pConnSetup->bitmapScanlineUnit;
    pConnSetupT->bitmapScanlinePad = pConnSetup->bitmapScanlinePad;
}

void
SwapWinRoot(xWindowRoot	*pRoot, 
	    xWindowRoot *pRootT)
{
    cpswapl(pRoot->windowId, pRootT->windowId);
    cpswapl(pRoot->defaultColormap, pRootT->defaultColormap);
    cpswapl(pRoot->whitePixel, pRootT->whitePixel);
    cpswapl(pRoot->blackPixel, pRootT->blackPixel);
    cpswapl(pRoot->currentInputMask, pRootT->currentInputMask);
    cpswaps(pRoot->pixWidth, pRootT->pixWidth);
    cpswaps(pRoot->pixHeight, pRootT->pixHeight);
    cpswaps(pRoot->mmWidth, pRootT->mmWidth);
    cpswaps(pRoot->mmHeight, pRootT->mmHeight);
    cpswaps(pRoot->minInstalledMaps, pRootT->minInstalledMaps);
    cpswaps(pRoot->maxInstalledMaps, pRootT->maxInstalledMaps);
    cpswapl(pRoot->rootVisualID, pRootT->rootVisualID);
    pRootT->backingStore = pRoot->backingStore;
    pRootT->saveUnders = pRoot->saveUnders;
    pRootT->rootDepth = pRoot->rootDepth;
    pRootT->nDepths = pRoot->nDepths;
}

void
SwapVisual(xVisualType *pVis, 
	   xVisualType *pVisT;
{
    cpswapl(pVis->visualID, pVisT->visualID);
    pVisT->class = pVis->class;
    pVisT->bitsPerRGB = pVis->bitsPerRGB;
    cpswaps(pVis->colormapEntries, pVisT->colormapEntries);
    cpswapl(pVis->redMask, pVisT->redMask);
    cpswapl(pVis->greenMask, pVisT->greenMask);
    cpswapl(pVis->blueMask, pVisT->blueMask);
}
#endif

void
LbxWriteSConnSetupPrefix(ClientPtr		pClient,
			 xLbxConnSetupPrefix	*pcsp)
{
    xLbxConnSetupPrefix	cspT;

    cspT.success = pcsp->success;
    cspT.changeType = pcsp->changeType;
    cspT.length = pcsp->length;
    cpswaps(pcsp->majorVersion, cspT.majorVersion);
    cpswaps(pcsp->minorVersion, cspT.minorVersion);
    cpswapl(pcsp->tag, cspT.tag);

    (void)WriteToClient(pClient, sizeof(cspT), (char *) &cspT);
}

void
LbxSwapFontInfo(xLbxFontInfo	*pr,
		Bool		compressed)
{
    unsigned    i;
    xCharInfo  *pxci;
    unsigned    nchars,
                nprops;
    char       *pby;
    register char n;

    nchars = pr->nCharInfos;
    nprops = pr->nFontProps;
    swaps(&pr->minCharOrByte2, n);
    swaps(&pr->maxCharOrByte2, n);
    swaps(&pr->defaultChar, n);
    swaps(&pr->nFontProps, n);
    swaps(&pr->fontAscent, n);
    swaps(&pr->fontDescent, n);
    SwapCharInfo(&pr->minBounds);
    SwapCharInfo(&pr->maxBounds);
    swapl(&pr->nCharInfos, n);

    pby = (char *) &pr[1];
    /*
     * Font properties are an atom and either an int32 or a CARD32, so they
     * are always 2 4 byte values
     */
    for (i = 0; i < nprops; i++) {
	swapl(pby, n);
	pby += 4;
	swapl(pby, n);
	pby += 4;
    }
    if (!compressed) {
	pxci = (xCharInfo *) pby;
	for (i = 0; i < nchars; i++, pxci++)
	    SwapCharInfo(pxci);
    } else {
    	SwapLongs((CARD32 *) pby, nchars);
    }
}
