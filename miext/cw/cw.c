/*
 * Copyright © 2004 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#include "gcstruct.h"
#include "windowstr.h"
#include "cw.h"

#define CW_DEBUG 1

#if CW_DEBUG
#define CW_ASSERT(x) do {						\
    if (!(x)) {								\
	ErrorF("composite wrapper: assertion failed at %s:%d\n", __FUNC__, \
	    __LINE__);							\
    }									\
} while (0)
#else
#define CW_ASSERT(x) do {} while (0)
#endif

int cwGCIndex;
int cwScreenIndex;
#ifdef RENDER
int cwPictureIndex;
#endif
static unsigned long cwGeneration = 0;
extern GCOps cwGCOps;

static Bool
cwCloseScreen (int i, ScreenPtr pScreen);

static void
cwValidateGC(GCPtr pGC, unsigned long stateChanges, DrawablePtr pDrawable);
static void
cwChangeGC(GCPtr pGC, unsigned long mask);
static void
cwCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
static void
cwDestroyGC(GCPtr pGC);
static void
cwChangeClip(GCPtr pGC, int type, pointer pvalue, int nrects);
static void
cwCopyClip(GCPtr pgcDst, GCPtr pgcSrc);
static void
cwDestroyClip(GCPtr pGC);

static void
cwCheapValidateGC(GCPtr pGC, unsigned long stateChanges, DrawablePtr pDrawable);
static void
cwCheapChangeGC(GCPtr pGC, unsigned long mask);
static void
cwCheapCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
static void
cwCheapDestroyGC(GCPtr pGC);
static void
cwCheapChangeClip(GCPtr pGC, int type, pointer pvalue, int nrects);
static void
cwCheapCopyClip(GCPtr pgcDst, GCPtr pgcSrc);
static void
cwCheapDestroyClip(GCPtr pGC);

static GCFuncs cwGCFuncs = {
    cwValidateGC,
    cwChangeGC,
    cwCopyGC,
    cwDestroyGC,
    cwChangeClip,
    cwDestroyClip,
    cwCopyClip,
};

static GCFuncs cwCheapGCFuncs = {
    cwCheapValidateGC,
    cwCheapChangeGC,
    cwCheapCopyGC,
    cwCheapDestroyGC,
    cwCheapChangeClip,
    cwCheapDestroyClip,
    cwCheapCopyClip,
};

DrawablePtr
cwGetBackingDrawable(DrawablePtr pDrawable, int *x_off, int *y_off)
{
    if (cwDrawableIsRedirWindow(pDrawable)) {
	WindowPtr pWin = (WindowPtr)pDrawable;
	PixmapPtr pPixmap = (*pDrawable->pScreen->GetWindowPixmap)(pWin);
	*x_off = -pPixmap->screen_x;
	*y_off = -pPixmap->screen_y;

	return &pPixmap->drawable;
    } else {
	*x_off = *y_off = 0;

	return pDrawable;
    }
}

/*
 * create the full func/op wrappers for a GC
 */

static Bool
cwCreateGCPrivate(GCPtr pGC, DrawablePtr pDrawable)
{
    cwGCRec *pPriv;
    int status, x_off, y_off;
    XID noexpose = xFalse;
    DrawablePtr pBackingDrawable;

    pPriv = (cwGCRec *)xalloc(sizeof (cwGCRec));
    if (!pPriv)
	return FALSE;
    pBackingDrawable = cwGetBackingDrawable(pDrawable, &x_off, &y_off);
    pPriv->pBackingGC = CreateGC(pBackingDrawable, GCGraphicsExposures,
				 &noexpose, &status);
    if (status != Success) {
	xfree(pPriv);
	return FALSE;
    }
    pPriv->guarantee = GuaranteeNothing;
    pPriv->serialNumber = 0;
    pPriv->stateChanges = (1 << (GCLastBit + 1)) - 1;
    pPriv->wrapOps = pGC->ops;
    pPriv->wrapFuncs = pGC->funcs;
    pGC->funcs = &cwGCFuncs;
    pGC->ops = &cwGCOps;
    setCwGC (pGC, pPriv);
    return TRUE;
}

static void
cwDestroyGCPrivate(GCPtr pGC)
{
    cwGCPtr pPriv;

    pPriv = (cwGCPtr) getCwGC (pGC);
    pGC->funcs = &cwCheapGCFuncs;
    pGC->ops = pPriv->wrapOps;
    if (pPriv->pBackingGC)
	FreeGC(pPriv->pBackingGC, (XID)0);
    setCwGC (pGC, pPriv->wrapFuncs);
    xfree((pointer)pPriv);
}

/* GCFuncs wrappers.  These only get used when the drawable is a window with a
 * backing pixmap, to avoid the overhead in the non-window-backing-pixmap case.
 */

#define FUNC_PROLOGUE(pGC, pPriv) \
    ((pGC)->funcs = pPriv->wrapFuncs), \
    ((pGC)->ops = pPriv->wrapOps)

#define FUNC_EPILOGUE(pGC, pPriv) \
    ((pGC)->funcs = &cwGCFuncs), \
    ((pGC)->ops = &cwGCOps)

static void
cwValidateGC(GCPtr pGC, unsigned long stateChanges, DrawablePtr pDrawable)
{
    GCPtr   	  	pBackingGC;
    cwGCPtr		pPriv;
    DrawablePtr		pBackingDrawable;
    int			x_off, y_off;

    pPriv = (cwGCPtr) getCwGC (pGC);

    FUNC_PROLOGUE(pGC, pPriv);

    if (pDrawable->serialNumber != pPriv->serialNumber &&
	!cwDrawableIsRedirWindow(pDrawable))
    {
	/* The drawable is no longer a window with backing store, so kill the
	 * private and go back to cheap functions.
	 */
	cwDestroyGCPrivate(pGC);
	(*pGC->funcs->ValidateGC)(pGC, stateChanges, pDrawable);
	return;
    }
    (*pGC->funcs->ValidateGC)(pGC, stateChanges, pDrawable);

    /*
     * rewrap funcs and ops as Validate may have changed them
     */
    pPriv->wrapFuncs = pGC->funcs;
    pPriv->wrapOps = pGC->ops;

    pBackingGC = pPriv->pBackingGC;
    pBackingDrawable = cwGetBackingDrawable(pDrawable, &x_off, &y_off);

    pPriv->stateChanges |= stateChanges;

    if (pPriv->stateChanges) {
	CopyGC(pGC, pBackingGC, pPriv->stateChanges);
	pPriv->stateChanges = 0;
    }

    if ((pGC->patOrg.x + x_off) != pBackingGC->patOrg.x ||
	(pGC->patOrg.y + y_off) != pBackingGC->patOrg.y)
    {
	XID vals[2];
	vals[0] = pGC->patOrg.x + x_off;
	vals[1] = pGC->patOrg.y + y_off;
	DoChangeGC(pBackingGC, GCTileStipXOrigin|GCTileStipYOrigin, vals, 0);
    }

    if (pDrawable->serialNumber != pPriv->serialNumber) {
	XID vals[2];

	/* Either the drawable has changed, or the clip list in the drawable has
	 * changed.  Copy the new clip list over and set the new translated
	 * offset for it.
	 */

	(*pBackingGC->funcs->DestroyClip)(pBackingGC);
	(*pBackingGC->funcs->CopyClip)(pBackingGC, pGC);
	vals[0] = pGC->clipOrg.x + x_off;
	vals[1] = pGC->clipOrg.y + y_off;
	DoChangeGC(pBackingGC, GCClipXOrigin|GCClipYOrigin, vals, 0);

	ValidateGC(pBackingDrawable, pBackingGC);
	pPriv->serialNumber = pDrawable->serialNumber;
    }

    FUNC_EPILOGUE(pGC, pPriv);
}

static void
cwChangeGC(GCPtr pGC, unsigned long mask)
{
    cwGCPtr	pPriv = (cwGCPtr)(pGC)->devPrivates[cwGCIndex].ptr;

    FUNC_PROLOGUE(pGC, pPriv);

    (*pGC->funcs->ChangeGC) (pGC, mask);

    FUNC_EPILOGUE(pGC, pPriv);
}

static void
cwCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    cwGCPtr	pPriv = (cwGCPtr)(pGCDst)->devPrivates[cwGCIndex].ptr;

    FUNC_PROLOGUE(pGCDst, pPriv);

    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);

    FUNC_EPILOGUE(pGCDst, pPriv);
}

static void
cwDestroyGC(GCPtr pGC)
{
    cwGCPtr	pPriv = (cwGCPtr)(pGC)->devPrivates[cwGCIndex].ptr;

    FUNC_PROLOGUE(pGC, pPriv);

    cwDestroyGCPrivate(pGC);

    (*pGC->funcs->DestroyGC) (pGC);

    /* leave it unwrapped */
}

static void
cwChangeClip(GCPtr pGC, int type, pointer pvalue, int nrects)
{
    cwGCPtr	pPriv = (cwGCPtr)(pGC)->devPrivates[cwGCIndex].ptr;

    FUNC_PROLOGUE(pGC, pPriv);

    (*pGC->funcs->ChangeClip)(pGC, type, pvalue, nrects);

    FUNC_EPILOGUE(pGC, pPriv);
}

static void
cwCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    cwGCPtr	pPriv = (cwGCPtr)(pgcDst)->devPrivates[cwGCIndex].ptr;

    FUNC_PROLOGUE(pgcDst, pPriv);

    (*pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);

    FUNC_EPILOGUE(pgcDst, pPriv);
}

static void
cwDestroyClip(GCPtr pGC)
{
    cwGCPtr	pPriv = (cwGCPtr)(pGC)->devPrivates[cwGCIndex].ptr;

    FUNC_PROLOGUE(pGC, pPriv);

    (*pGC->funcs->DestroyClip)(pGC);

    FUNC_EPILOGUE(pGC, pPriv);
}

/*
 * Cheap GC func wrappers.  Pass everything through unless we find a window with
 * a backing pixmap, then turn on the real wrappers.
 */

#define CHEAP_FUNC_PROLOGUE(pGC) \
    ((pGC)->funcs = (GCFuncs *)(pGC)->devPrivates[cwGCIndex].ptr)

#define CHEAP_FUNC_EPILOGUE(pGC) \
    ((pGC)->funcs = &cwCheapGCFuncs)

static void
cwCheapValidateGC(GCPtr pGC, unsigned long stateChanges, DrawablePtr pDrawable)
{
    CHEAP_FUNC_PROLOGUE(pGC);

    /* Check if the drawable is a window with backing pixmap.  If so,
     * cwCreateGCPrivate will wrap with the backing-pixmap GC funcs and we won't
     * re-wrap on return.
     */
    if (pDrawable->type == DRAWABLE_WINDOW &&
cwDrawableIsRedirWindow(pDrawable) &&
	cwCreateGCPrivate(pGC, pDrawable))
    {
	(*pGC->funcs->ValidateGC)(pGC, stateChanges, pDrawable);
    }
    else
    {
	(*pGC->funcs->ValidateGC)(pGC, stateChanges, pDrawable);

	/* rewrap funcs as Validate may have changed them */
	pGC->devPrivates[cwGCIndex].ptr = (pointer) pGC->funcs;

	CHEAP_FUNC_EPILOGUE(pGC);
    }
}

static void
cwCheapChangeGC(GCPtr pGC, unsigned long mask)
{
    CHEAP_FUNC_PROLOGUE(pGC);

    (*pGC->funcs->ChangeGC)(pGC, mask);

    CHEAP_FUNC_EPILOGUE(pGC);
}

static void
cwCheapCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    CHEAP_FUNC_PROLOGUE(pGCDst);

    (*pGCDst->funcs->CopyGC)(pGCSrc, mask, pGCDst);

    CHEAP_FUNC_EPILOGUE(pGCDst);
}

static void
cwCheapDestroyGC(GCPtr pGC)
{
    CHEAP_FUNC_PROLOGUE(pGC);

    (*pGC->funcs->DestroyGC)(pGC);

    /* leave it unwrapped */
}

static void
cwCheapChangeClip(GCPtr pGC, int type, pointer pvalue, int nrects)
{
    CHEAP_FUNC_PROLOGUE(pGC);

    (*pGC->funcs->ChangeClip)(pGC, type, pvalue, nrects);

    CHEAP_FUNC_EPILOGUE(pGC);
}

static void
cwCheapCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    CHEAP_FUNC_PROLOGUE(pgcDst);

    (*pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);

    CHEAP_FUNC_EPILOGUE(pgcDst);
}

static void
cwCheapDestroyClip(GCPtr pGC)
{
    CHEAP_FUNC_PROLOGUE(pGC);

    (*pGC->funcs->DestroyClip)(pGC);

    CHEAP_FUNC_EPILOGUE(pGC);
}

/*
 * GC Create wrapper.  Set up the cheap GC func wrappers to track
 * GC validation on BackingStore windows.
 */

#define SCREEN_PROLOGUE(pScreen, field)\
  ((pScreen)->field = ((cwScreenPtr) \
    (pScreen)->devPrivates[cwScreenIndex].ptr)->field)

#define SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)

static Bool
cwCreateGC(GCPtr pGC)
{
    ScreenPtr	pScreen = pGC->pScreen;
    Bool	ret;

    SCREEN_PROLOGUE(pScreen, CreateGC);
    
    if ( (ret = (*pScreen->CreateGC)(pGC)) )
    {
    	pGC->devPrivates[cwGCIndex].ptr = (pointer)pGC->funcs;
    	pGC->funcs = &cwCheapGCFuncs;
    }

    SCREEN_EPILOGUE(pScreen, CreateGC, cwCreateGC);

    return ret;
}

static void
cwGetImage(DrawablePtr pSrc, int sx, int sy, int w, int h, unsigned int format,
	   unsigned long planemask, char *pdstLine)
{
    ScreenPtr pScreen = pSrc->pScreen;
    DrawablePtr pBackingDrawable;
    int x_off, y_off;
    
    SCREEN_PROLOGUE(pScreen, GetImage);

    pBackingDrawable = cwGetBackingDrawable(pSrc, &x_off, &y_off);

    sx += x_off;
    sy += y_off;

    (*pScreen->GetImage)(pBackingDrawable, sx, sy, w, h, format, planemask, pdstLine);

    SCREEN_EPILOGUE(pScreen, GetImage, cwGetImage);
}

static void
cwGetSpans(DrawablePtr pSrc, int wMax, DDXPointPtr ppt, int *pwidth,
	   int nspans, char *pdstStart)
{
    ScreenPtr pScreen = pSrc->pScreen;
    DrawablePtr pBackingDrawable;
    int i;
    int x_off, y_off;
    DDXPointPtr ppt_trans;
    
    SCREEN_PROLOGUE(pScreen, GetSpans);

    pBackingDrawable = cwGetBackingDrawable(pSrc, &x_off, &y_off);

    ppt_trans = (DDXPointPtr)ALLOCATE_LOCAL(nspans * sizeof(DDXPointRec));
    if (ppt_trans) {
	for (i = 0; i < nspans; i++) {
	    ppt_trans[i].x = ppt[i].x + x_off;
	    ppt_trans[i].y = ppt[i].y + y_off;
	}

	(*pScreen->GetSpans)(pBackingDrawable, wMax, ppt, pwidth, nspans,
			     pdstStart);
	DEALLOCATE_LOCAL(ppt_trans);
     }

    SCREEN_EPILOGUE(pScreen, GetSpans, cwGetSpans);
}

/* Screen initialization/teardown */
void
miInitializeCompositeWrapper(ScreenPtr pScreen)
{
    cwScreenPtr pScreenPriv;

    if (cwGeneration != serverGeneration)
    {
	cwScreenIndex = AllocateScreenPrivateIndex();
	if (cwScreenIndex < 0)
	    return;
	cwGCIndex = AllocateGCPrivateIndex();
#ifdef RENDER
	cwPictureIndex = AllocatePicturePrivateIndex();
#endif
	cwGeneration = serverGeneration;
    }
    if (!AllocateGCPrivate(pScreen, cwGCIndex, 0))
	return;
    pScreenPriv = (cwScreenPtr)xalloc(sizeof(cwScreenRec));
    if (!pScreenPriv)
	return;

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->GetImage = pScreen->GetImage;
    pScreenPriv->GetSpans = pScreen->GetSpans;
    pScreenPriv->CreateGC = pScreen->CreateGC;

    pScreen->CloseScreen = cwCloseScreen;
    pScreen->GetImage = cwGetImage;
    pScreen->GetSpans = cwGetSpans;
    pScreen->CreateGC = cwCreateGC;

    pScreen->devPrivates[cwScreenIndex].ptr = (pointer)pScreenPriv;

#ifdef RENDER
    if (GetPictureScreen (pScreen))
    {
	if (!cwInitializeRender (pScreen))
	    /* FIXME */;
    }
#endif

    ErrorF("Initialized composite wrapper\n");
}

static Bool
cwCloseScreen (int i, ScreenPtr pScreen)
{
    cwScreenPtr   pScreenPriv;
#ifdef RENDER
    PictureScreenPtr ps = GetPictureScreenIfSet(pScreen);
#endif

    pScreenPriv = (cwScreenPtr)pScreen->devPrivates[cwScreenIndex].ptr;

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->GetImage = pScreenPriv->GetImage;
    pScreen->GetSpans = pScreenPriv->GetSpans;
    pScreen->CreateGC = pScreenPriv->CreateGC;

#ifdef RENDER
    if (ps) {
	ps->Composite = pScreenPriv->Composite;
	ps->Glyphs = pScreenPriv->Glyphs;
    }
#endif

    xfree((pointer)pScreenPriv);

    return (*pScreen->CloseScreen)(i, pScreen);
}
