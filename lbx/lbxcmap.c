/* $Xorg: lbxcmap.c,v 1.4 2001/02/09 02:05:16 xorgcvs Exp $ */

/*
Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/
/* $XFree86: xc/programs/Xserver/lbx/lbxcmap.c,v 1.10 2001/12/14 19:59:59 dawes Exp $ */

#include <sys/types.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "Xos.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "propertyst.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#include "Xfuncproto.h"
#include <stdio.h>

static int lbxScreenPrivIndex;		/* lbx screen private index */
static int lbxColormapPrivIndex;	/* lbx colormap private index */

typedef struct {			/* lbx screen private */
    CreateColormapProcPtr CreateColormap;
    DestroyColormapProcPtr DestroyColormap;
  CloseScreenProcPtr CloseScreen;
} LbxScreenPriv;

typedef struct _LbxStalled {
    XID id;
    struct _LbxStalled *next;
} LbxStalled;

typedef struct _LbxColormapPriv {	/* lbx colormap private */
    char	grab_status;
    char	smart_grab;
    LbxProxyPtr	grabber;
    int		last_grabber;	/* uid, not pid */
    LbxStalled	*stalled_clients;
    ColormapPtr	next;		/* proxy chain */
} LbxColormapPriv;

#define CMAP_NOT_GRABBED	0
#define CMAP_GRABBED		1
#define CMAP_WAITING_FOR_UNGRAB	2

static int LbxUnstallClient(pointer data, XID id);

static RESTYPE StalledResType;

/*
 * Initialize the fields in the colormap private allocated for LBX.
 */

static LbxColormapPriv *
LbxColormapPrivInit (ColormapPtr pmap)
{
    LbxColormapPriv *cmapPriv;

    cmapPriv = (LbxColormapPriv *) xalloc (sizeof (LbxColormapPriv));
    if (!cmapPriv)
	return cmapPriv;

    pmap->devPrivates[lbxColormapPrivIndex].ptr = (pointer) cmapPriv;

    cmapPriv->grab_status = CMAP_NOT_GRABBED;
    cmapPriv->grabber = NULL;
    cmapPriv->last_grabber = 0;
    cmapPriv->smart_grab = FALSE;
    cmapPriv->stalled_clients = NULL;
    cmapPriv->next = NULL;

    return cmapPriv;
}


static int
LbxDefCmapPrivInit (ColormapPtr pmap)
{
#if 0
    /* BUG: You can't do that. lbxColormapPrivIndex hasn't 
	been initialized yet.  */
    pmap->devPrivates[lbxColormapPrivIndex].ptr = NULL;
#endif
    return 1;
}

static Bool
LbxCreateColormap (ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    Bool ret;

    pScreen->CreateColormap = ((LbxScreenPriv *) (pScreen->devPrivates[
	lbxScreenPrivIndex].ptr))->CreateColormap;

    pmap->devPrivates[lbxColormapPrivIndex].ptr = NULL;
    ret = (*pScreen->CreateColormap) (pmap);
 
    pScreen->CreateColormap = LbxCreateColormap;

    return ret;
}

static void
LbxDestroyColormap (ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;

    LbxReleaseCmap(pmap, FALSE);
    pScreen->DestroyColormap = ((LbxScreenPriv *) (pScreen->devPrivates[
	lbxScreenPrivIndex].ptr))->DestroyColormap;
    (*pScreen->DestroyColormap) (pmap);
    if (pmap->devPrivates && pmap->devPrivates[lbxColormapPrivIndex].ptr)
	xfree(pmap->devPrivates[lbxColormapPrivIndex].ptr);
    pScreen->DestroyColormap = LbxDestroyColormap;
}

static Bool
LbxCloseScreen(int i, ScreenPtr pScreen)
{
    LbxScreenPriv* pLbxScrPriv = ((LbxScreenPriv *) 
			     (pScreen->devPrivates[lbxScreenPrivIndex].ptr));
    
    pScreen->CloseScreen = pLbxScrPriv->CloseScreen;

    xfree(pScreen->devPrivates[lbxScreenPrivIndex].ptr);
    pScreen->devPrivates[lbxScreenPrivIndex].ptr = NULL;

    return pScreen->CloseScreen(i, pScreen);
}

/*
 * Initialize LBX colormap private.
 */

int
LbxCmapInit (void)

{
    LbxScreenPriv *pScreenPriv;
    ColormapPtr defMap;
    ScreenPtr pScreen;
    int i;

    StalledResType = CreateNewResourceType(LbxUnstallClient);

    lbxScreenPrivIndex = AllocateScreenPrivateIndex ();
    if (lbxScreenPrivIndex < 0)
	return 0;

    lbxColormapPrivIndex = AllocateColormapPrivateIndex (LbxDefCmapPrivInit);
    if (lbxColormapPrivIndex < 0)
	return 0;

    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];

        defMap = (ColormapPtr) LookupIDByType(
			pScreen->defColormap, RT_COLORMAP);

	/* now lbxColormapPrivIndex exists */
        defMap->devPrivates[lbxColormapPrivIndex].ptr = NULL;

	pScreenPriv = (LbxScreenPriv *) xalloc (sizeof (LbxScreenPriv));
	if (!pScreenPriv)
	    return 0;

	pScreenPriv->CreateColormap = pScreen->CreateColormap;
	pScreen->CreateColormap = LbxCreateColormap;
	pScreenPriv->DestroyColormap = pScreen->DestroyColormap;
	pScreen->DestroyColormap = LbxDestroyColormap;
	pScreenPriv->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = LbxCloseScreen;
	pScreen->devPrivates[lbxScreenPrivIndex].ptr = (pointer) pScreenPriv;
    }

    return 1;
}

/*
 * Return the number of allocated cells in the PSEUDO colormap.
 */

static int
NumAllocatedCells (EntryPtr pent,
		   int size)
{
    Pixel pixel;
    int count = 0;

    for (pixel = 0; pixel < size; pixel++)
    {
	if (pent[pixel].refcnt != 0 && pent[pixel].refcnt != AllocTemporary)
	    count++;
    }

    return count;
}

#define NUMRED(vis) ((vis->redMask >> vis->offsetRed) + 1)
#define NUMGREEN(vis) ((vis->greenMask >> vis->offsetGreen) + 1)
#define NUMBLUE(vis) ((vis->blueMask >> vis->offsetBlue) + 1)

#define PIX_OUT(ptr,is2,val) \
    if (is2) *ptr++ = (val) >> 8; \
    *ptr++ = (val)
#define RGB_OUT(ptr,is2,shift,val) \
    *ptr++ = (val) >> shift; \
    if (is2) *ptr++ = (val)

/*
 * Return the list of allocated cells in the channel in
 * the format required by LbxGrabCmapReply.
 */

static CARD8 *
OutputChannel(ColormapPtr pmap,
	      EntryPtr chan,
	      int size,
	      CARD8 *ptr,
	      CARD8 flags,
	      CARD8 channels)
{
    Bool	px2;
    Bool	rgb2;
    int		shift;
    int		rgb_sz;
    Pixel	pixel;
    EntryPtr	pent;
    CARD8 *pixel_private_range_ptr = NULL;
    CARD8 *pixel_shared_range_ptr = NULL;
    int		allocpriv;

    px2 = (flags & LBX_2BYTE_PIXELS) != 0;
    rgb2 = (flags & LBX_RGB_BITS_MASK) > 7;
    if (rgb2)
	shift = 8;
    else
	shift = 15 - (flags & LBX_RGB_BITS_MASK);
    rgb_sz = rgb2 + 1;
    if (channels == (DoRed|DoGreen|DoBlue))
	rgb_sz *= 3;
    /* kinda gross, but ddxen use AllocAll on static maps */
    allocpriv = (pmap->pVisual->class & DynamicClass) ? AllocPrivate : 0;
    for (pixel = 0; pixel < size; pixel++)
    {
	pent = (EntryPtr) &chan[pixel];

	if (pent->refcnt == 0 || pent->refcnt == AllocTemporary)
	{
	    /*
	     * A free pixel.  This disrupts all ranges.
	     */

	    pixel_private_range_ptr = pixel_shared_range_ptr = NULL;

	    continue;
	}
	else if (pent->refcnt == allocpriv)
	{
	    /*
	     * A private pixel.  This disrupts any PIXEL_SHARED_RANGE.
	     */

	    pixel_shared_range_ptr = NULL;

	    if (!pixel_private_range_ptr)
	    {
		pixel_private_range_ptr = ptr;

		*ptr++ = LBX_PIXEL_PRIVATE;
		PIX_OUT(ptr, px2, pixel);
	    }
	    else
	    {
		CARD8 *pos = pixel_private_range_ptr + 2 + px2;
		if (*pixel_private_range_ptr == LBX_PIXEL_PRIVATE) {
		    *pixel_private_range_ptr = LBX_PIXEL_RANGE_PRIVATE;
		    ptr += 1 + px2;
		}
		PIX_OUT(pos, px2, pixel);
	    }
	}
	else
	{
	    /*
	     * A shared pixel.  This disrupts any PIXEL_PRIVATE_RANGE.
	     */

	    pixel_private_range_ptr = NULL;

	    if (!pixel_shared_range_ptr)
	    {
		pixel_shared_range_ptr = ptr;

		*ptr++ = LBX_PIXEL_SHARED;
		PIX_OUT(ptr, px2, pixel);
	    }
	    else
	    {
		CARD8 *pos = pixel_shared_range_ptr + 2 + px2;
		if (*pixel_shared_range_ptr == LBX_PIXEL_SHARED)
		{
		    *pixel_shared_range_ptr = LBX_PIXEL_RANGE_SHARED;
		    memmove (pos + 1 + px2, pos, rgb_sz);
		    ptr += 1 + px2;
		}
		PIX_OUT(pos, px2, pixel);
	    }

	    if (channels & DoRed) {
		RGB_OUT(ptr, rgb2, shift, pent->co.local.red);
	    }
	    if (channels & DoGreen) {
		RGB_OUT(ptr, rgb2, shift, pent->co.local.green);
	    }
	    if (channels & DoBlue) {
		RGB_OUT(ptr, rgb2, shift, pent->co.local.blue);
	    }
	}
    }
    return ptr;
}

static void
GetAllocatedCells (ColormapPtr pmap,
		   CARD8 *flags,
		   CARD8 *buf,
		   int *bytes)
{
    CARD8	*ptr;

    *flags = pmap->pVisual->bitsPerRGBValue - 1;
    if (pmap->pVisual->ColormapEntries > 256)
	*flags |= LBX_2BYTE_PIXELS;
    if (!(pmap->pVisual->class & DynamicClass))
	*flags |= LBX_AUTO_RELEASE;
    if ((pmap->pVisual->class | DynamicClass) == DirectColor) {
	*flags |= LBX_3CHANNELS;
	ptr = OutputChannel(pmap, pmap->red, NUMRED(pmap->pVisual),
			    buf, *flags, DoRed);
	*ptr++ = LBX_NEXT_CHANNEL;
	ptr = OutputChannel(pmap, pmap->green, NUMGREEN(pmap->pVisual),
			    ptr, *flags, DoGreen);
	*ptr++ = LBX_NEXT_CHANNEL;
	ptr = OutputChannel(pmap, pmap->blue, NUMBLUE(pmap->pVisual),
			    ptr, *flags, DoBlue);
    } else
	ptr = OutputChannel(pmap, pmap->red, pmap->pVisual->ColormapEntries,
			    buf, *flags, DoRed|DoGreen|DoBlue);
    *ptr++ = LBX_LIST_END;
    *bytes = ptr - buf;
}


/*
 * Send an LbxReleaseCmapEvent to a proxy.
 */

static void
SendReleaseCmapEvent (LbxProxyPtr proxy,
		      Colormap cmap)
{
    xLbxReleaseCmapEvent ev;
    ClientPtr client;
    LbxClientPtr lbxcp;
    int n;

    lbxcp = proxy->lbxClients[0];

    if (lbxcp && (client = lbxcp->client))
    {
	ev.type = LbxEventCode;
	ev.lbxType = LbxReleaseCmapEvent;
	ev.sequenceNumber = client->sequence;
	ev.colormap = cmap;
	ev.pad1 = ev.pad2 = ev.pad3 = ev.pad4 = ev.pad5 = ev.pad6 = 0;

	if (client->swapped)
	{
	    swaps(&ev.sequenceNumber, n);
	    swapl(&ev.colormap, n);
	}

	WriteToClient(client, sz_xLbxReleaseCmapEvent, (char *) &ev);
	LbxForceOutput(proxy);

#ifdef COLOR_DEBUG
	fprintf (stderr,
	    "Sent LbxReleaseCmapEvent to proxy %d, seq = 0x%x, cmap = 0x%x\n",
	    proxy->pid, client->sequence, cmap);
#endif
    }
}


/*
 * WaitForServerCmapControl checks if the colormap is grabbed by a proxy,
 * and if so, sends an LbxReleaseCmapEvent to the proxy.  It then suspends
 * the current request until the server gets the ReleaseCmap message from
 * the proxy.
 */

static Bool
WaitForServerCmapControl (ClientPtr client,
			  ColormapPtr pmap)
{
    LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);
    LbxStalled *stalled;

    if (cmapPriv->grab_status == CMAP_GRABBED)
    {
	/*
	 * Send an LbxReleaseCmapEvent to the proxy that has the grab.
	 */

	SendReleaseCmapEvent (cmapPriv->grabber, pmap->mid);
	cmapPriv->grab_status = CMAP_WAITING_FOR_UNGRAB;
    }

    stalled = (LbxStalled *)xalloc(sizeof(LbxStalled));
    if (!stalled)
	return FALSE;
    stalled->id = FakeClientID(client->index);
    stalled->next = cmapPriv->stalled_clients;
    cmapPriv->stalled_clients = stalled;
    return AddResource(stalled->id, StalledResType, (pointer)cmapPriv);
}


/*
 * When the X server gets any of the requests that allocate color cells,
 * it calls LbxCheckColorRequest on the request.  This function will check
 * if the colormap is grabbed by a proxy, and if so, will suspend the
 * current request and wait for the proxy to release the colormap.
 */

Bool
LbxCheckColorRequest (ClientPtr client,
		      ColormapPtr pmap,
		      xReq *req)
{
    LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);

    if (!cmapPriv)
	return FALSE;

    if (cmapPriv->grab_status != CMAP_NOT_GRABBED)
    {
	/*
	 * The colormap is grabbed by a proxy.  Reset this request, and
	 * process it after the server gets back control of the colormap.
	 * Before we reset the request, we must put it back in the
	 * client's byte order.
	 */

	if (!WaitForServerCmapControl (client, pmap))
	    return FALSE;

	if (client->swapped)
	{
	    register int n;

	    switch (req->reqType)
	    {
	    case X_AllocColor:
	    {
		xAllocColorReq *stuff = (xAllocColorReq *) req;
		swaps(&stuff->length, n);
		swapl(&stuff->cmap, n);
		swaps(&stuff->red, n);
		swaps(&stuff->green, n);
		swaps(&stuff->blue, n);
		break;
	    }
	    case X_AllocNamedColor:
	    {
		xAllocNamedColorReq *stuff = (xAllocNamedColorReq *) req;
		swaps(&stuff->length, n);
		swapl(&stuff->cmap, n);
		swaps(&stuff->nbytes, n);
		break;
	    }
	    case X_AllocColorCells:
	    {
		xAllocColorCellsReq *stuff = (xAllocColorCellsReq *) req;
		swaps(&stuff->length, n);
		swapl(&stuff->cmap, n);
		swaps(&stuff->colors, n);
		swaps(&stuff->planes, n);
		break;
	    }
	    case X_AllocColorPlanes:
	    {
		xAllocColorPlanesReq *stuff = (xAllocColorPlanesReq *) req;
		swaps(&stuff->length, n);
		swapl(&stuff->cmap, n);
		swaps(&stuff->colors, n);
		swaps(&stuff->red, n);
		swaps(&stuff->green, n);
		swaps(&stuff->blue, n);
		break;
	    }
	    default:
		break;
	    }
	}

	ResetCurrentRequest(client);
	client->sequence--;
	IgnoreClient(client);

	return TRUE;
    }

    if (!LbxClient(client) ||
	LbxProxy(client)->uid != cmapPriv->last_grabber)
    {
	/*
	 * Next time the proxy for this client does a colormap grab, it
	 * will have to get the colormap state (a non-smart grab).
	 */

	cmapPriv->smart_grab = FALSE;
    }

    return FALSE;
}

static Bool
LbxGrabbedByClient (ClientPtr client,
		    ColormapPtr pmap)
{
    LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);
    return (cmapPriv &&
	    (cmapPriv->grab_status != CMAP_NOT_GRABBED) &&
	    (cmapPriv->grabber == LbxProxy(client)));
}

/*
 * Check if a colormap is grabbed by a proxy.
 */

int
LbxCheckCmapGrabbed (ColormapPtr pmap)
{
    LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);

    return (cmapPriv && (cmapPriv->grab_status == CMAP_GRABBED));
}


/*
 * Disable a smart grab on the specified colormap.
 */

void
LbxDisableSmartGrab (ColormapPtr pmap)
{
    LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);

    if (cmapPriv)
	cmapPriv->smart_grab = FALSE;
}


/*
 * Send an LbxFreeCellsEvent to the specified proxy.
 */

static void
SendFreeCellsEvent (LbxProxyPtr proxy,
		    Colormap cmap,
		    Pixel pixel_start,
		    Pixel pixel_end)
{
    xLbxFreeCellsEvent ev;
    ClientPtr client;
    LbxClientPtr lbxcp;
    int n;

    lbxcp = proxy->lbxClients[0];

    if (lbxcp && (client = lbxcp->client))
    {
	ev.type = LbxEventCode;
	ev.lbxType =  LbxFreeCellsEvent;
	ev.sequenceNumber = client->sequence;
	ev.colormap = cmap;
	ev.pixelStart = pixel_start;
	ev.pixelEnd = pixel_end;
	ev.pad1 = ev.pad2 = ev.pad3 = ev.pad4 = 0;

	if (client->swapped)
	{
	    swaps(&ev.sequenceNumber, n);
	    swapl(&ev.colormap, n);
	    swapl(&ev.pixelStart, n);
	    swapl(&ev.pixelEnd, n);
	}

	WriteToClient(client, sz_xLbxFreeCellsEvent, (char *) &ev);
	LbxForceOutput(proxy);
#ifdef COLOR_DEBUG
	fprintf (stderr, "Sent LbxFreeCellsEvent to proxy %d, seq = 0x%x\n",
	    proxy->pid, client->sequence);
	fprintf (stderr, "   cmap = 0x%x, pixelStart = %d, pixelEnd = %d\n",
	    cmap, pixel_start, pixel_end);
#endif
    }
}

/* XXX use of globals like this is gross */
static long pixel_start;
static long pixel_end;


/*
 * LbxFreeCellsEvent generation functions.
 */

/*ARGSUSED*/
void
LbxBeginFreeCellsEvent (ColormapPtr pmap)
{
    pixel_start = -1;
    pixel_end = -1;
}


void
LbxAddFreeCellToEvent (ColormapPtr pmap,
		       Pixel pixel)
{
    /*
     * We must notify the proxy that has this colormap
     * grabbed which cells are being freed (their refcount
     * has reached zero).
     */

    if (pixel_start == -1)
	pixel_start = pixel;
    else
    {
	if (pixel_end == -1)
	    pixel_end = pixel;
	else
	{
	    if (pixel_end + 1 == pixel)
		pixel_end = pixel;
	    else if (pixel > pixel_end + 1)
	    {
		LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
		    (pmap->devPrivates[lbxColormapPrivIndex].ptr);

		SendFreeCellsEvent (cmapPriv->grabber,
				    pmap->mid, pixel_start, pixel_end);

		pixel_start = pixel;
		pixel_end = -1;
	    }
	}
    }
}

void
LbxEndFreeCellsEvent (ColormapPtr pmap)
{
    /*
     * Check if there is an LbxFreeCellEvent we need to write.
     */

    if (pixel_start != -1)
    {
	LbxColormapPriv *cmapPriv = (LbxColormapPriv *)
	    (pmap->devPrivates[lbxColormapPrivIndex].ptr);

	SendFreeCellsEvent (cmapPriv->grabber,
			    pmap->mid, pixel_start,
			    pixel_end == -1 ? pixel_start : pixel_end);
    }
}


/*
 * Sort the specified pixel list.  This optimizes generation
 * of LbxFreeCellsEvent.
 */

void
LbxSortPixelList (Pixel *pixels,
		  int count)
{
     int i, j;

     for (i = 0; i <= count - 2; i++)
	 for (j = count - 1; j > i; j--)
	    if (pixels[j - 1] > pixels[j])
	    {
		Pixel temp = pixels[j - 1];
		pixels[j - 1] = pixels[j];
		pixels[j] = temp;
	    }
}


/*
 * Handle a colormap grab request from a proxy.
 */

int
ProcLbxGrabCmap(ClientPtr client)
{
    REQUEST(xLbxGrabCmapReq);
    xLbxGrabCmapReply *reply;
    Bool smartGrab;
    LbxColormapPriv *cmapPriv;
    ColormapPtr pmap;
    int bytes, n;
    LbxProxyPtr proxy = LbxProxy(client);

    client->sequence--;		/* not a counted request */

    pmap = (ColormapPtr) SecurityLookupIDByType (client, stuff->cmap,
						 RT_COLORMAP,
						 SecurityWriteAccess);

    if (!pmap)
    {
        client->errorValue = stuff->cmap;
	return BadColor;
    }

    cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);
    if (!cmapPriv)
    {
	cmapPriv = LbxColormapPrivInit (pmap);
	if (!cmapPriv)
	    return BadAlloc;
    }


    /*
     * We have a SMART GRAB if since this proxy last ungrabbed the
     * colormap, no color cell was alloc'd by an entity other than
     * this proxy (this includes other proxies as well as clients
     * directly connected to the X server without a proxy).
     *
     * We want to optimize this special case because a proxy may give
     * up a grab because it got a request that it could not handle
     * (e.g. AllocNamedColor or LookupColor).  When it asks back for
     * the grab, there is no need for the server to send the colormap
     * state, because the proxy is already up to date on the state of
     * the colormap.
     *
     * In order for this to work, the following assumptions are made
     * about the proxy:
     *
     * - the proxy is kept up to date on all cell allocations made on its
     *   behalf resulting from the following requests: AllocNamedColor, 
     *   AllocColorCells, AllocColorPlanes
     * - the proxy is kept up to date on all cells freed by any client
     *   via the LbxFreeCell event.
     */

    /* if proxy is this confused, give it full info */
    if (cmapPriv->grab_status == CMAP_GRABBED && cmapPriv->grabber == proxy)
	LbxReleaseCmap(pmap, FALSE);

    if (proxy->uid != cmapPriv->last_grabber)
	cmapPriv->smart_grab = FALSE;
    smartGrab = cmapPriv->smart_grab;

#ifdef COLOR_DEBUG
    fprintf (stderr, "\nGot colormap grab request, ");
    fprintf (stderr, "seq = 0x%x, proxy = %d, client = %d, cmap = 0x%x\n",
	client->sequence, proxy->pid, client->index, stuff->cmap);

    if (cmapPriv->grab_status == CMAP_NOT_GRABBED)
    {
	fprintf (stderr, "cmap 0x%x is not grabbed by any proxy\n",
	    stuff->cmap);
	if (smartGrab)
	    fprintf (stderr, "This is a smart grab\n");
    }
    else if (cmapPriv->grab_status == CMAP_GRABBED)
    {
	if (cmapPriv->grabber == proxy)
	{
	    fprintf (stderr, "cmap 0x%x is already grabbed by proxy %d\n",
	        stuff->cmap, proxy->pid);
	}
	else
	{
	    fprintf (stderr, "cmap 0x%x is currently grabbed by proxy %d\n",
	        stuff->cmap, cmapPriv->grabber->pid);
	}
    }
    else if (cmapPriv->grab_status == CMAP_WAITING_FOR_UNGRAB)
    {
	fprintf (stderr,
	    "Already waiting for cmap 0x%x to be ungrabbed by proxy %d\n",
	    stuff->cmap, cmapPriv->grabber->pid);
    }
#endif

    if (cmapPriv->grab_status != CMAP_NOT_GRABBED &&
	cmapPriv->grabber != proxy)
    {
	/*
	 * The colormap is grabbed by a proxy other than the one that
	 * is requesting this grab.  Reset this grab request, and process
         * it after the server gets back control of the colormap.  Before
	 * we reset the request, we must put it back in the client's byte
	 * order.
	 */

	if (!WaitForServerCmapControl (client, pmap))
	    return BadAlloc;

	if (client->swapped)
	{
	    swaps(&stuff->length, n);
	    swapl(&stuff->cmap, n);
	}

	ResetCurrentRequest(client);
	IgnoreClient(client);

	return Success;
    }

    if (pmap->pVisual->class & DynamicClass) {
	cmapPriv->grabber = proxy;
	cmapPriv->grab_status = CMAP_GRABBED;
	cmapPriv->next = proxy->grabbedCmaps;
	proxy->grabbedCmaps = pmap;
    } else
	smartGrab = FALSE;

    /*
     * For an smart grab (see comments above), there is no information
     * sent about the colormap cells because the proxy is all up to date.
     * Otherwise, the server sends the proxy the state of all allocated
     * cells in the colormap.  All cells not specified are assumed free.
     */

    bytes = 0;
    if (!smartGrab) {
	if ((pmap->pVisual->class | DynamicClass) == DirectColor) {
	    bytes = NumAllocatedCells(pmap->red,
				      NUMRED(pmap->pVisual)) * 9;
	    bytes += NumAllocatedCells(pmap->green,
				       NUMGREEN(pmap->pVisual)) * 9;
	    bytes += NumAllocatedCells(pmap->blue,
				       NUMBLUE(pmap->pVisual)) * 9;
	    bytes += 2;
	} else
	    bytes = NumAllocatedCells(pmap->red,
				      pmap->pVisual->ColormapEntries) * 9;
    }
    bytes += sz_xLbxGrabCmapReply + 1;
    reply = (xLbxGrabCmapReply *) xalloc (bytes);
    bzero (reply, sz_xLbxGrabCmapReply);

    if (smartGrab)
    {
	reply->flags = LBX_SMART_GRAB;
	reply->length = 0;
    }
    else
    {
	GetAllocatedCells (pmap, &reply->flags,
			   (CARD8 *) reply + sz_xLbxGrabCmapReplyHdr, &bytes);
	if (bytes <= (sz_xLbxGrabCmapReply - sz_xLbxGrabCmapReplyHdr))
	    reply->length = 0;
	else
	    reply->length = (sz_xLbxGrabCmapReplyHdr +
		bytes - sz_xLbxGrabCmapReply + 3) >> 2;
    }

    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;

    bytes = sz_xLbxGrabCmapReply + (reply->length << 2);

    if (client->swapped)
    {
	register char n;

	swaps (&reply->sequenceNumber, n);
	swapl (&reply->length, n);
    }

    WriteToClient (client, bytes, (char *)reply);
    
    xfree (reply);

    return (client->noClientException);
}

static int
LbxUnstallClient(pointer     data,
		 XID         id)
{
    LbxColormapPriv *cmapPriv = (LbxColormapPriv *)data;
    LbxStalled **prev;
    ClientPtr client;

    for (prev = &cmapPriv->stalled_clients; *prev && (*prev)->id != id; )
	prev = &(*prev)->next;
    *prev = (*prev)->next;
    client = clients[CLIENT_ID(id)];
    if (!client->clientGone)
	AttendClient(client);
    return 0;
}

void
LbxReleaseCmap(ColormapPtr pmap,
	       Bool smart)
{
    LbxColormapPriv *cmapPriv;
    ColormapPtr *prev;

    if (!pmap->devPrivates)
	return;
    cmapPriv = (LbxColormapPriv *)
	(pmap->devPrivates[lbxColormapPrivIndex].ptr);
    if (!cmapPriv || (cmapPriv->grab_status == CMAP_NOT_GRABBED))
	return;

    for (prev = &cmapPriv->grabber->grabbedCmaps; *prev && *prev != pmap; )
	prev = &((LbxColormapPriv *)
		 (*prev)->devPrivates[lbxColormapPrivIndex].ptr)->next;
    if (*prev == pmap)
	*prev = cmapPriv->next;

    while (cmapPriv->stalled_clients)
	FreeResource(cmapPriv->stalled_clients->id, 0);

    cmapPriv->grab_status = CMAP_NOT_GRABBED;
    cmapPriv->last_grabber = smart ? cmapPriv->grabber->uid : 0;
    cmapPriv->grabber = NULL;
    cmapPriv->smart_grab = smart;
}

/*
 * Handle a colormap release request from a proxy.
 */

int
ProcLbxReleaseCmap(ClientPtr	client)
{
    REQUEST(xLbxReleaseCmapReq);
    ColormapPtr pmap;

#ifdef COLOR_DEBUG
    fprintf (stderr, "Got colormap release request, ");
    fprintf (stderr, "seq = 0x%x, proxy = %d, client = %d, cmap = 0x%x\n",
	client->sequence, LbxProxyID(client), client->index, stuff->cmap);
#endif

    pmap = (ColormapPtr) SecurityLookupIDByType (client, stuff->cmap,
						 RT_COLORMAP,
						 SecurityWriteAccess);

    if (!pmap)
    {
        client->errorValue = stuff->cmap;
	return BadColor;
    }

    if (LbxGrabbedByClient(client, pmap))
	LbxReleaseCmap(pmap, TRUE);
    return Success;
}


/*
 * Handle an LbxAllocColor request.  The proxy did the alloc and
 * is telling the server what rgb and pixel value to use.
 */

int
ProcLbxAllocColor(ClientPtr	client)
{
    REQUEST(xLbxAllocColorReq);
    ColormapPtr pmap;
    CARD32 pixel = stuff->pixel;

    REQUEST_SIZE_MATCH (xLbxAllocColorReq);
    pmap = (ColormapPtr) SecurityLookupIDByType (client, stuff->cmap,
						 RT_COLORMAP,
						 SecurityWriteAccess);

#ifdef COLOR_DEBUG
    fprintf (stderr,
	"Got LBX alloc color: seq = 0x%x, proxy = %d, client = %d\n",
	client->sequence, LbxProxyID(client), client->index);
    fprintf (stderr, "  cmap = 0x%x, pixel = %d, rgb = (%d,%d,%d)\n",
        stuff->cmap, stuff->pixel, stuff->red, stuff->green, stuff->blue);
#endif

    if (pmap)
    {
	int status;
	if (!LbxGrabbedByClient(client, pmap))
	    return BadAccess;

	status = AllocColor (pmap,
	    &stuff->red, &stuff->green, &stuff->blue,
	    &pixel, client->index);

	if (status == Success && pixel != stuff->pixel)
	{
	    /*
	     * Internal error - Proxy allocated different pixel from server
	     */
#ifdef COLOR_DEBUG
	    fprintf(stderr, "got pixel %d (%d, %d, %d), expected %d\n",
		    pixel, stuff->red, stuff->green, stuff->blue, stuff->pixel);
#endif
	    FreeColors (pmap, client->index, 1, &pixel, 0L);
	    return BadAlloc;
	}

	return status;
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}


/*
 * The proxy sends an LbxIncrementPixel request when it short circuits
 * an AllocColor.  The server must bump up the reference count the
 * specified amount.
 */

int
ProcLbxIncrementPixel(ClientPtr	client)
{
    REQUEST(xLbxIncrementPixelReq);
    ColormapPtr pmap;
    EntryPtr    pent;
    Pixel	pixel;
    unsigned	short red, green, blue;
    VisualPtr	pVisual;
    int		status;

#ifdef COLOR_DEBUG
    fprintf (stderr,
	"Got LBX increment pixel: seq = 0x%x, proxy = %d, client = %d\n",
	client->sequence, LbxProxyID(client), client->index);
    fprintf (stderr, "  cmap = 0x%x, pixel = %d\n",
        stuff->cmap, stuff->pixel);
#endif

    /*
     * Looks up the color associated with the pixel, and then call
     * AllocColor() - a bit round-about, but it should work.
     */

    pmap = (ColormapPtr) SecurityLookupIDByType(client, stuff->cmap,
						RT_COLORMAP,
						SecurityWriteAccess);
    if (!pmap) {
        client->errorValue = stuff->cmap;
	return BadColor;
    }

    pixel = stuff->pixel;

    switch (pmap->class) {
    case StaticColor:
    case StaticGray:
	red = pmap->red[pixel].co.local.red;
	green = pmap->red[pixel].co.local.green;
	blue = pmap->red[pixel].co.local.blue;
	break;
    case GrayScale:
    case PseudoColor:
	pent = pmap->red + pixel;
	red = pent->co.local.red;
	green = pent->co.local.green;
	blue = pent->co.local.blue;
	break;
    default:
	pVisual = pmap->pVisual;
	red = pmap->red[(pixel & pVisual->redMask) >> pVisual->offsetRed].co.local.red;
	green = pmap->green[(pixel & pVisual->greenMask) >> pVisual->offsetGreen].co.local.green;
	blue = pmap->blue[(pixel & pVisual->blueMask) >> pVisual->offsetBlue].co.local.blue;
	break;
    }

    status = AllocColor(pmap, &red, &green, &blue, &pixel, client->index);

    if (status == Success && pixel != stuff->pixel)
    {
	/*
	 * Internal error - Proxy allocated different pixel from server
	 */
#ifdef COLOR_DEBUG
	fprintf(stderr, "got pixel %d, expected %d\n", pixel, stuff->pixel);
#endif
	FreeColors (pmap, client->index, 1, &pixel, 0L);
	return BadAlloc;
    }

    return status;
}
