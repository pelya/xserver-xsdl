/************************************************************

Author: Eamon Walsh <ewalsh@epoch.ncsc.mil>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdarg.h>
#include "windowstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "xacestr.h"
#include "modinit.h"

CallbackListPtr XaceHooks[XACE_NUM_HOOKS] = {0};

/* Proc vectors for untrusted clients, swapped and unswapped versions.
 * These are the same as the normal proc vectors except that extensions
 * that haven't declared themselves secure will have ProcBadRequest plugged
 * in for their major opcode dispatcher.  This prevents untrusted clients
 * from guessing extension major opcodes and using the extension even though
 * the extension can't be listed or queried.
 */
static int (*UntrustedProcVector[256])(
    ClientPtr /*client*/
);
static int (*SwappedUntrustedProcVector[256])(
    ClientPtr /*client*/
);

/* Entry point for hook functions.  Called by Xserver.
 */
int XaceHook(int hook, ...)
{
    pointer calldata;	/* data passed to callback */
    int *prv = NULL;	/* points to return value from callback */
    va_list ap;		/* argument list */
    va_start(ap, hook);

    /* Marshal arguments for passing to callback.
     * Each callback has its own case, which sets up a structure to hold
     * the arguments and integer return parameter, or in some cases just
     * sets calldata directly to a single argument (with no return result)
     */
    switch (hook)
    {
	case XACE_CORE_DISPATCH: {
	    XaceCoreDispatchRec rec = {
		va_arg(ap, ClientPtr),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_RESOURCE_ACCESS: {
	    XaceResourceAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, XID),
		va_arg(ap, RESTYPE),
		va_arg(ap, Mask),
		va_arg(ap, pointer),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_DEVICE_ACCESS: {
	    XaceDeviceAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, DeviceIntPtr),
		va_arg(ap, Bool),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_PROPERTY_ACCESS: {
	    XacePropertyAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, WindowPtr),
		va_arg(ap, Atom),
		va_arg(ap, Mask),
		XaceAllowOperation   /* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_DRAWABLE_ACCESS: {
	    XaceDrawableAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, DrawablePtr),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_MAP_ACCESS:
	case XACE_BACKGRND_ACCESS: {
	    XaceMapAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, WindowPtr),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_EXT_DISPATCH:
	case XACE_EXT_ACCESS: {
	    XaceExtAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, ExtensionEntry*),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_HOSTLIST_ACCESS: {
	    XaceHostlistAccessRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, Mask),
		TRUE	/* default allow */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_SITE_POLICY: {
	    XaceSitePolicyRec rec = {
		va_arg(ap, char*),
		va_arg(ap, int),
		FALSE	/* default unrecognized */
	    };
	    calldata = &rec;
	    prv = &rec.rval;
	    break;
	}
	case XACE_DECLARE_EXT_SECURE: {
	    XaceDeclareExtSecureRec rec = {
		va_arg(ap, ExtensionEntry*),
		va_arg(ap, Bool)
	    };
	    calldata = &rec;
	    break;
	}
	case XACE_AUTH_AVAIL: {
	    XaceAuthAvailRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, XID)
	    };
	    calldata = &rec;
	    break;
	}
	case XACE_KEY_AVAIL: {
	    XaceKeyAvailRec rec = {
		va_arg(ap, xEventPtr),
		va_arg(ap, DeviceIntPtr),
		va_arg(ap, int)
	    };
	    calldata = &rec;
	    break;
	}
	case XACE_WINDOW_INIT: {
	    XaceWindowRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, WindowPtr)
	    };
	    calldata = &rec;
	    break;
	}
	case XACE_AUDIT_BEGIN: {
	    XaceAuditRec rec = {
		va_arg(ap, ClientPtr),
		0
	    };
	    calldata = &rec;
	    break;
	}
	case XACE_AUDIT_END: {
	    XaceAuditRec rec = {
		va_arg(ap, ClientPtr),
		va_arg(ap, int)
	    };
	    calldata = &rec;
	    break;
	}
	default: {
	    va_end(ap);
	    return 0;	/* unimplemented hook number */
	}
    }
    va_end(ap);
 
    /* call callbacks and return result, if any. */
    CallCallbacks(&XaceHooks[hook], calldata);
    return prv ? *prv : 0;
}

static int
ProcXaceDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data)
    {
	default:
	    return BadRequest;
    }
} /* ProcXaceDispatch */

static int
SProcXaceDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data)
    {
	default:
	    return BadRequest;
    }
} /* SProcXaceDispatch */


/* XaceResetProc
 *
 * Arguments:
 *	extEntry is the extension information for the XACE extension.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Performs any cleanup needed by XACE at server shutdown time.
 */
static void
XaceResetProc(ExtensionEntry *extEntry)
{
    int i;

    for (i=0; i<XACE_NUM_HOOKS; i++)
    {
	DeleteCallbackList(&XaceHooks[i]);
	XaceHooks[i] = NULL;
    }
} /* XaceResetProc */


static int
XaceCatchDispatchProc(ClientPtr client)
{
    REQUEST(xReq);
    int major = stuff->reqType;

    if (!ProcVector[major])
	return (BadRequest);

    if (!XaceHook(XACE_CORE_DISPATCH, client))
	return (BadAccess);

    return client->swapped ? 
	(* SwappedProcVector[major])(client) :
	(* ProcVector[major])(client);
}

static int
XaceCatchExtProc(ClientPtr client)
{
    REQUEST(xReq);
    int major = stuff->reqType;
    ExtensionEntry *ext = GetExtensionEntry(major);

    if (!ext || !ProcVector[major])
	return (BadRequest);

    if (!XaceHook(XACE_EXT_DISPATCH, client, ext))
	return (BadRequest); /* pretend extension doesn't exist */

    return client->swapped ?
	(* SwappedProcVector[major])(client) :
	(* ProcVector[major])(client);
}

	
/* SecurityClientStateCallback
 *
 * Arguments:
 *	pcbl is &ClientStateCallback.
 *	nullata is NULL.
 *	calldata is a pointer to a NewClientInfoRec (include/dixstruct.h)
 *	which contains information about client state changes.
 *
 * Returns: nothing.
 *
 * Side Effects:
 * 
 * If a new client is connecting, its authorization ID is copied to
 * client->authID.  If this is a generated authorization, its reference
 * count is bumped, its timer is cancelled if it was running, and its
 * trustlevel is copied to TRUSTLEVEL(client).
 * 
 * If a client is disconnecting and the client was using a generated
 * authorization, the authorization's reference count is decremented, and
 * if it is now zero, the timer for this authorization is started.
 */

static void
XaceClientStateCallback(
    CallbackListPtr *pcbl,
    pointer nulldata,
    pointer calldata)
{
    NewClientInfoRec *pci = (NewClientInfoRec *)calldata;
    ClientPtr client = pci->client;

    switch (client->clientState)
    {
	case ClientStateRunning:
	{ 
	    client->requestVector = client->swapped ?
		SwappedUntrustedProcVector : UntrustedProcVector;
	    break;
	}
	default: break; 
    }
} /* XaceClientStateCallback */

/* XaceExtensionInit
 *
 * Initialize the XACE Extension
 */
void XaceExtensionInit(INITARGS)
{
    ExtensionEntry	*extEntry;
    int i;

    if (!AddCallback(&ClientStateCallback, XaceClientStateCallback, NULL))
	return;

    extEntry = AddExtension(XACE_EXTENSION_NAME,
			    XaceNumberEvents, XaceNumberErrors,
			    ProcXaceDispatch, SProcXaceDispatch,
			    XaceResetProc, StandardMinorOpcode);

    /* initialize dispatching intercept functions */
    for (i = 0; i < 128; i++)
    {
	UntrustedProcVector[i] = XaceCatchDispatchProc;
	SwappedUntrustedProcVector[i] = XaceCatchDispatchProc;
    }
    for (i = 128; i < 256; i++)
    {
	UntrustedProcVector[i] = XaceCatchExtProc;
	SwappedUntrustedProcVector[i] = XaceCatchExtProc;
    }
}

/* XaceCensorImage
 *
 * Called after pScreen->GetImage to prevent pieces or trusted windows from
 * being returned in image data from an untrusted window.
 *
 * Arguments:
 *	client is the client doing the GetImage.
 *      pVisibleRegion is the visible region of the window.
 *	widthBytesLine is the width in bytes of one horizontal line in pBuf.
 *	pDraw is the source window.
 *	x, y, w, h is the rectangle of image data from pDraw in pBuf.
 *	format is the format of the image data in pBuf: ZPixmap or XYPixmap.
 *	pBuf is the image data.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Any part of the rectangle (x, y, w, h) that is outside the visible
 *	region of the window will be destroyed (overwritten) in pBuf.
 */
void
XaceCensorImage(client, pVisibleRegion, widthBytesLine, pDraw, x, y, w, h,
		format, pBuf)
    ClientPtr client;
    RegionPtr pVisibleRegion;
    long widthBytesLine;
    DrawablePtr pDraw;
    int x, y, w, h;
    unsigned int format;
    char * pBuf;
{
    ScreenPtr pScreen;
    RegionRec imageRegion;  /* region representing x,y,w,h */
    RegionRec censorRegion; /* region to obliterate */
    BoxRec imageBox;
    int nRects;

    pScreen = pDraw->pScreen;

    imageBox.x1 = x;
    imageBox.y1 = y;
    imageBox.x2 = x + w;
    imageBox.y2 = y + h;
    REGION_INIT(pScreen, &imageRegion, &imageBox, 1);
    REGION_NULL(pScreen, &censorRegion);

    /* censorRegion = imageRegion - visibleRegion */
    REGION_SUBTRACT(pScreen, &censorRegion, &imageRegion, pVisibleRegion);
    nRects = REGION_NUM_RECTS(&censorRegion);
    if (nRects > 0)
    { /* we have something to censor */
	GCPtr pScratchGC = NULL;
	PixmapPtr pPix = NULL;
	xRectangle *pRects = NULL;
	Bool failed = FALSE;
	int depth = 1;
	int bitsPerPixel = 1;
	int i;
	BoxPtr pBox;

	/* convert region to list-of-rectangles for PolyFillRect */

	pRects = (xRectangle *)ALLOCATE_LOCAL(nRects * sizeof(xRectangle *));
	if (!pRects)
	{
	    failed = TRUE;
	    goto failSafe;
	}
	for (pBox = REGION_RECTS(&censorRegion), i = 0;
	     i < nRects;
	     i++, pBox++)
	{
	    pRects[i].x = pBox->x1;
	    pRects[i].y = pBox->y1 - imageBox.y1;
	    pRects[i].width  = pBox->x2 - pBox->x1;
	    pRects[i].height = pBox->y2 - pBox->y1;
	}

	/* use pBuf as a fake pixmap */

	if (format == ZPixmap)
	{
	    depth = pDraw->depth;
	    bitsPerPixel = pDraw->bitsPerPixel;
	}

	pPix = GetScratchPixmapHeader(pDraw->pScreen, w, h,
		    depth, bitsPerPixel,
		    widthBytesLine, (pointer)pBuf);
	if (!pPix)
	{
	    failed = TRUE;
	    goto failSafe;
	}

	pScratchGC = GetScratchGC(depth, pPix->drawable.pScreen);
	if (!pScratchGC)
	{
	    failed = TRUE;
	    goto failSafe;
	}

	ValidateGC(&pPix->drawable, pScratchGC);
	(* pScratchGC->ops->PolyFillRect)(&pPix->drawable,
			    pScratchGC, nRects, pRects);

    failSafe:
	if (failed)
	{
	    /* Censoring was not completed above.  To be safe, wipe out
	     * all the image data so that nothing trusted gets out.
	     */
	    bzero(pBuf, (int)(widthBytesLine * h));
	}
	if (pRects)     DEALLOCATE_LOCAL(pRects);
	if (pScratchGC) FreeScratchGC(pScratchGC);
	if (pPix)       FreeScratchPixmapHeader(pPix);
    }
    REGION_UNINIT(pScreen, &imageRegion);
    REGION_UNINIT(pScreen, &censorRegion);
} /* XaceCensorImage */
