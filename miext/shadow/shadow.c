/*
 * $XFree86: xc/programs/Xserver/miext/shadow/shadow.c,v 1.15 2003/11/10 18:22:51 tsi Exp $
 *
 * Copyright © 2000 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


#include    "X.h"
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    "font.h"
#include    "dixfontstr.h"
#include    "fontstruct.h"
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"

typedef struct _shadowGCPriv {
    GCOps   *ops;
    GCFuncs *funcs;
} shadowGCPrivRec, *shadowGCPrivPtr;

int shadowScrPrivateIndex;
int shadowGCPrivateIndex;
int shadowGeneration;

#define shadowGetGCPriv(pGC) \
    ((shadowGCPrivPtr) (pGC)->devPrivates[shadowGCPrivateIndex].ptr)
#define shadowGCPriv(pGC) \
    shadowGCPrivPtr  pGCPriv = shadowGetGCPriv(pGC)

#define wrap(priv, real, mem, func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv, real, mem) {\
    real->mem = priv->mem; \
}

static void
shadowRedisplay (ScreenPtr pScreen)
{
    shadowScrPriv(pScreen);
    shadowBufPtr    pBuf;

    for (pBuf = pScrPriv->pBuf; pBuf; pBuf = pBuf->pNext)
    {
	if (REGION_NOTEMPTY (pScreen, &pBuf->damage))
	{
	    REGION_INTERSECT (pScreen, &pBuf->damage, &pBuf->damage,
			      &WindowTable[pScreen->myNum]->borderClip);
	    (*pBuf->update) (pScreen, pBuf);
	    REGION_EMPTY (pScreen, &pBuf->damage);
	}
    }
}

static void
shadowBlockHandler (pointer	data,
		      OSTimePtr pTimeout,
		      pointer pRead)
{
    ScreenPtr	pScreen = (ScreenPtr) data;

    shadowRedisplay (pScreen);
}

static void
shadowWakeupHandler (pointer data, int i, pointer LastSelectMask)
{
}

static void
shadowDamageRegion (WindowPtr pWindow, RegionPtr pRegion)
{
    shadowBufPtr    pBuf = shadowFindBuf (pWindow);

    if (!pBuf)
	abort ();

    REGION_INTERSECT(pWindow->drawable.pScreen, pRegion, pRegion,
		     &pWindow->borderClip);
    REGION_UNION(pWindow->drawable.pScreen, &pBuf->damage, &pBuf->damage,
		 pRegion);
#ifdef ALWAYS_DISPLAY
    shadowRedisplay (pWindow->drawable.pScreen);
#endif
}

static void
shadowDamageBox (WindowPtr pWindow, BoxPtr pBox)
{
    RegionRec	region;

    REGION_INIT (pWindow->drawable.pScreen, &region, pBox, 1);
    shadowDamageRegion (pWindow, &region);
}

static void
shadowDamageRect (WindowPtr pWindow, int x, int y, int w, int h)
{
    BoxRec	box;

    x += pWindow->drawable.x;
    y += pWindow->drawable.y;
    box.x1 = x;
    box.x2 = x + w;
    box.y1 = y;
    box.y2 = y + h;
    shadowDamageBox (pWindow, &box);
}

static void shadowValidateGC(GCPtr, unsigned long, DrawablePtr);
static void shadowChangeGC(GCPtr, unsigned long);
static void shadowCopyGC(GCPtr, unsigned long, GCPtr);
static void shadowDestroyGC(GCPtr);
static void shadowChangeClip(GCPtr, int, pointer, int);
static void shadowDestroyClip(GCPtr);
static void shadowCopyClip(GCPtr, GCPtr);

GCFuncs shadowGCFuncs = {
    shadowValidateGC, shadowChangeGC, shadowCopyGC, shadowDestroyGC,
    shadowChangeClip, shadowDestroyClip, shadowCopyClip
};

extern GCOps shadowGCOps;

static Bool
shadowCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    shadowScrPriv(pScreen);
    shadowGCPriv(pGC);
    Bool ret;

    unwrap (pScrPriv, pScreen, CreateGC);
    if((ret = (*pScreen->CreateGC) (pGC))) {
	pGCPriv->ops = NULL;
	pGCPriv->funcs = pGC->funcs;
	pGC->funcs = &shadowGCFuncs;
    }
    wrap (pScrPriv, pScreen, CreateGC, shadowCreateGC);

    return ret;
}

void
shadowWrapGC (GCPtr pGC)
{
    shadowGCPriv(pGC);

    pGCPriv->ops = NULL;
    pGCPriv->funcs = pGC->funcs;
    pGC->funcs = &shadowGCFuncs;
}

void
shadowUnwrapGC (GCPtr pGC)
{
    shadowGCPriv(pGC);

    pGC->funcs = pGCPriv->funcs;
    if (pGCPriv->ops)
	pGC->ops = pGCPriv->ops;
}

#define SHADOW_GC_OP_PROLOGUE(pGC, pDraw) \
    shadowGCPriv(pGC);  \
    GCFuncs *oldFuncs = pGC->funcs; \
    unwrap(pGCPriv, pGC, funcs);  \
    unwrap(pGCPriv, pGC, ops); \

#define SHADOW_GC_OP_EPILOGUE(pGC, pDraw) \
    wrap(pGCPriv, pGC, funcs, oldFuncs); \
    wrap(pGCPriv, pGC, ops, &shadowGCOps)

#define SHADOW_GC_FUNC_PROLOGUE(pGC) \
    shadowGCPriv(pGC); \
    unwrap(pGCPriv, pGC, funcs); \
    if (pGCPriv->ops) unwrap(pGCPriv, pGC, ops)

#define SHADOW_GC_FUNC_EPILOGUE(pGC) \
    wrap(pGCPriv, pGC, funcs, &shadowGCFuncs);  \
    if (pGCPriv->ops) wrap(pGCPriv, pGC, ops, &shadowGCOps)

static void
shadowValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
){
    SHADOW_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);
    if(pDraw->type == DRAWABLE_WINDOW)
	pGCPriv->ops = pGC->ops;  /* just so it's not NULL */
    else
	pGCPriv->ops = NULL;
    SHADOW_GC_FUNC_EPILOGUE (pGC);
}

static void
shadowDestroyGC(GCPtr pGC)
{
    SHADOW_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->DestroyGC)(pGC);
    SHADOW_GC_FUNC_EPILOGUE (pGC);
}

static void
shadowChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
){
    SHADOW_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    SHADOW_GC_FUNC_EPILOGUE (pGC);
}

static void
shadowCopyGC (
    GCPtr	    pGCSrc,
    unsigned long   mask,
    GCPtr	    pGCDst
){
    SHADOW_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    SHADOW_GC_FUNC_EPILOGUE (pGCDst);
}

static void
shadowChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects
){
    SHADOW_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    SHADOW_GC_FUNC_EPILOGUE (pGC);
}

static void
shadowCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    SHADOW_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    SHADOW_GC_FUNC_EPILOGUE (pgcDst);
}

static void
shadowDestroyClip(GCPtr pGC)
{
    SHADOW_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    SHADOW_GC_FUNC_EPILOGUE (pGC);
}

#define IS_VISIBLE(pWin) 1


#define TRIM_BOX(box, pGC) { \
    BoxPtr extents = &pGC->pCompositeClip->extents;\
    if(box.x1 < extents->x1) box.x1 = extents->x1; \
    if(box.x2 > extents->x2) box.x2 = extents->x2; \
    if(box.y1 < extents->y1) box.y1 = extents->y1; \
    if(box.y2 > extents->y2) box.y2 = extents->y2; \
    }

#define TRANSLATE_BOX(box, pDraw) { \
    box.x1 += pDraw->x; \
    box.x2 += pDraw->x; \
    box.y1 += pDraw->y; \
    box.y2 += pDraw->y; \
    }

#define TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC) { \
    TRANSLATE_BOX(box, pDraw); \
    TRIM_BOX(box, pGC); \
    }

#define BOX_NOT_EMPTY(box) \
    (((box.x2 - box.x1) > 0) && ((box.y2 - box.y1) > 0))

#ifdef RENDER
static void
shadowComposite (CARD8      op,
		   PicturePtr pSrc,
		   PicturePtr pMask,
		   PicturePtr pDst,
		   INT16      xSrc,
		   INT16      ySrc,
		   INT16      xMask,
		   INT16      yMask,
		   INT16      xDst,
		   INT16      yDst,
		   CARD16     width,
		   CARD16     height)
{
    ScreenPtr		pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen(pScreen);
    shadowScrPriv(pScreen);

    unwrap (pScrPriv, ps, Composite);
    (*ps->Composite) (op,
		       pSrc,
		       pMask,
		       pDst,
		       xSrc,
		       ySrc,
		       xMask,
		       yMask,
		       xDst,
		       yDst,
		       width,
		       height);
    wrap (pScrPriv, ps, Composite, shadowComposite);
    if (pDst->pDrawable->type == DRAWABLE_WINDOW)
	shadowDamageRect ((WindowPtr) pDst->pDrawable, xDst, yDst,
			    width, height);
}

static void
shadowGlyphs (CARD8		op,
		PicturePtr	pSrc,
		PicturePtr	pDst,
		PictFormatPtr	maskFormat,
		INT16		xSrc,
		INT16		ySrc,
		int		nlist,
		GlyphListPtr	list,
		GlyphPtr	*glyphs)
{
    ScreenPtr		pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen(pScreen);
    shadowScrPriv(pScreen);
    int			x, y;
    int			n;
    GlyphPtr		glyph;

    unwrap (pScrPriv, ps, Glyphs);
    (*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, list, glyphs);
    wrap (pScrPriv, ps, Glyphs, shadowGlyphs);
    if (pDst->pDrawable->type == DRAWABLE_WINDOW)
    {
	x = xSrc;
	y = ySrc;
	while (nlist--)
	{
	    x += list->xOff;
	    y += list->yOff;
	    n = list->len;
	    while (n--)
	    {
		glyph = *glyphs++;
		shadowDamageRect ((WindowPtr) pDst->pDrawable,
				    x - glyph->info.x,
				    y - glyph->info.y,
				    glyph->info.width,
				    glyph->info.height);
		x += glyph->info.xOff;
		y += glyph->info.yOff;
	    }
	    list++;
	}
    }
}
#endif

/**********************************************************/


static void
shadowFillSpans(
    DrawablePtr pDraw,
    GC		*pGC,
    int		nInit,
    DDXPointPtr pptInit,
    int 	*pwidthInit,
    int 	fSorted
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nInit) {
	DDXPointPtr ppt = pptInit;
	int *pwidth = pwidthInit;
	int i = nInit;
	BoxRec box;

	box.x1 = ppt->x;
	box.x2 = box.x1 + *pwidth;
	box.y2 = box.y1 = ppt->y;

	while(--i) {
	   ppt++;
	   pwidth++;
	   if(box.x1 > ppt->x) box.x1 = ppt->x;
	   if(box.x2 < (ppt->x + *pwidth))
		box.x2 = ppt->x + *pwidth;
	   if(box.y1 > ppt->y) box.y1 = ppt->y;
	   else if(box.y2 < ppt->y) box.y2 = ppt->y;
	}

	box.y2++;

	(*pGC->ops->FillSpans)(pDraw, pGC, nInit, pptInit, pwidthInit, fSorted);

        if(!pGC->miTranslate) {
           TRANSLATE_BOX(box, pDraw);
        }
        TRIM_BOX(box, pGC); 

	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    } else
	(*pGC->ops->FillSpans)(pDraw, pGC, nInit, pptInit, pwidthInit, fSorted);

    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
shadowSetSpans(
    DrawablePtr		pDraw,
    GCPtr		pGC,
    char		*pcharsrc,
    DDXPointPtr 	pptInit,
    int			*pwidthInit,
    int			nspans,
    int			fSorted
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nspans) {
	DDXPointPtr ppt = pptInit;
	int *pwidth = pwidthInit;
	int i = nspans;
	BoxRec box;

	box.x1 = ppt->x;
	box.x2 = box.x1 + *pwidth;
	box.y2 = box.y1 = ppt->y;

	while(--i) {
	   ppt++;
	   pwidth++;
	   if(box.x1 > ppt->x) box.x1 = ppt->x;
	   if(box.x2 < (ppt->x + *pwidth))
		box.x2 = ppt->x + *pwidth;
	   if(box.y1 > ppt->y) box.y1 = ppt->y;
	   else if(box.y2 < ppt->y) box.y2 = ppt->y;
	}

	box.y2++;

	(*pGC->ops->SetSpans)(pDraw, pGC, pcharsrc, pptInit,
				pwidthInit, nspans, fSorted);

        if(!pGC->miTranslate) {
           TRANSLATE_BOX(box, pDraw);
        }
        TRIM_BOX(box, pGC); 

	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    } else
	(*pGC->ops->SetSpans)(pDraw, pGC, pcharsrc, pptInit,
				pwidthInit, nspans, fSorted);

    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
shadowPutImage(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		depth,
    int x, int y, int w, int h,
    int		leftPad,
    int		format,
    char 	*pImage
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PutImage)(pDraw, pGC, depth, x, y, w, h,
		leftPad, format, pImage);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw)) {
	BoxRec box;

	box.x1 = x + pDraw->x;
	box.x2 = box.x1 + w;
	box.y1 = y + pDraw->y;
	box.y2 = box.y1 + h;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static RegionPtr
shadowCopyArea(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty
){
    RegionPtr ret;
    SHADOW_GC_OP_PROLOGUE(pGC, pDst);
    ret = (*pGC->ops->CopyArea)(pSrc, pDst,
            pGC, srcx, srcy, width, height, dstx, dsty);
    SHADOW_GC_OP_EPILOGUE(pGC, pDst);

    if(IS_VISIBLE(pDst)) {
	BoxRec box;

	box.x1 = dstx + pDst->x;
	box.x2 = box.x1 + width;
	box.y1 = dsty + pDst->y;
	box.y2 = box.y1 + height;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDst, &box);
    }

    return ret;
}

static RegionPtr
shadowCopyPlane(
    DrawablePtr	pSrc,
    DrawablePtr	pDst,
    GCPtr pGC,
    int	srcx, int srcy,
    int	width, int height,
    int	dstx, int dsty,
    unsigned long bitPlane
){
    RegionPtr ret;
    SHADOW_GC_OP_PROLOGUE(pGC, pDst);
    ret = (*pGC->ops->CopyPlane)(pSrc, pDst,
	       pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
    SHADOW_GC_OP_EPILOGUE(pGC, pDst);

    if(IS_VISIBLE(pDst)) {
	BoxRec box;

	box.x1 = dstx + pDst->x;
	box.x2 = box.x1 + width;
	box.y1 = dsty + pDst->y;
	box.y2 = box.y1 + height;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDst, &box);
    }

    return ret;
}

static void
shadowPolyPoint(
    DrawablePtr pDraw,
    GCPtr pGC,
    int mode,
    int npt,
    xPoint *pptInit
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyPoint)(pDraw, pGC, mode, npt, pptInit);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && npt) {
	BoxRec box;

	box.x2 = box.x1 = pptInit->x;
	box.y2 = box.y1 = pptInit->y;

	/* this could be slow if the points were spread out */

	while(--npt) {
	   pptInit++;
	   if(box.x1 > pptInit->x) box.x1 = pptInit->x;
	   else if(box.x2 < pptInit->x) box.x2 = pptInit->x;
	   if(box.y1 > pptInit->y) box.y1 = pptInit->y;
	   else if(box.y2 < pptInit->y) box.y2 = pptInit->y;
	}

	box.x2++;
	box.y2++;

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static void
shadowPolylines(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		mode,
    int		npt,
    DDXPointPtr pptInit
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->Polylines)(pDraw, pGC, mode, npt, pptInit);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);


    if(IS_VISIBLE(pDraw) && npt) {
	BoxRec box;
	int extra = pGC->lineWidth >> 1;

	box.x2 = box.x1 = pptInit->x;
	box.y2 = box.y1 = pptInit->y;

	if(npt > 1) {
	   if(pGC->joinStyle == JoinMiter)
		extra = 6 * pGC->lineWidth;
	   else if(pGC->capStyle == CapProjecting)
		extra = pGC->lineWidth;
        }

	if(mode == CoordModePrevious) {
	   int x = box.x1;
	   int y = box.y1;
	   while(--npt) {
		pptInit++;
		x += pptInit->x;
		y += pptInit->y;
		if(box.x1 > x) box.x1 = x;
		else if(box.x2 < x) box.x2 = x;
		if(box.y1 > y) box.y1 = y;
		else if(box.y2 < y) box.y2 = y;
	    }
	} else {
	   while(--npt) {
		pptInit++;
		if(box.x1 > pptInit->x) box.x1 = pptInit->x;
		else if(box.x2 < pptInit->x) box.x2 = pptInit->x;
		if(box.y1 > pptInit->y) box.y1 = pptInit->y;
		else if(box.y2 < pptInit->y) box.y2 = pptInit->y;
	    }
	}

	box.x2++;
	box.y2++;

	if(extra) {
	   box.x1 -= extra;
	   box.x2 += extra;
	   box.y1 -= extra;
	   box.y2 += extra;
        }

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static void
shadowPolySegment(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nseg,
    xSegment	*pSeg
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolySegment)(pDraw, pGC, nseg, pSeg);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nseg) {
	BoxRec box;
	int extra = pGC->lineWidth;

        if(pGC->capStyle != CapProjecting)
	   extra >>= 1;

	if(pSeg->x2 > pSeg->x1) {
	    box.x1 = pSeg->x1;
	    box.x2 = pSeg->x2;
	} else {
	    box.x2 = pSeg->x1;
	    box.x1 = pSeg->x2;
	}

	if(pSeg->y2 > pSeg->y1) {
	    box.y1 = pSeg->y1;
	    box.y2 = pSeg->y2;
	} else {
	    box.y2 = pSeg->y1;
	    box.y1 = pSeg->y2;
	}

	while(--nseg) {
	    pSeg++;
	    if(pSeg->x2 > pSeg->x1) {
		if(pSeg->x1 < box.x1) box.x1 = pSeg->x1;
		if(pSeg->x2 > box.x2) box.x2 = pSeg->x2;
	    } else {
		if(pSeg->x2 < box.x1) box.x1 = pSeg->x2;
		if(pSeg->x1 > box.x2) box.x2 = pSeg->x1;
	    }
	    if(pSeg->y2 > pSeg->y1) {
		if(pSeg->y1 < box.y1) box.y1 = pSeg->y1;
		if(pSeg->y2 > box.y2) box.y2 = pSeg->y2;
	    } else {
		if(pSeg->y2 < box.y1) box.y1 = pSeg->y2;
		if(pSeg->y1 > box.y2) box.y2 = pSeg->y1;
	    }
	}

	box.x2++;
	box.y2++;

	if(extra) {
	   box.x1 -= extra;
	   box.x2 += extra;
	   box.y1 -= extra;
	   box.y2 += extra;
        }

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static void
shadowPolyRectangle(
    DrawablePtr  pDraw,
    GCPtr        pGC,
    int	         nRects,
    xRectangle  *pRects
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyRectangle)(pDraw, pGC, nRects, pRects);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nRects)
    {
	BoxRec box;
	int offset1, offset2, offset3;

	offset2 = pGC->lineWidth;
	if(!offset2) offset2 = 1;
	offset1 = offset2 >> 1;
	offset3 = offset2 - offset1;

	while(nRects--)
	{
	    box.x1 = pRects->x - offset1;
	    box.y1 = pRects->y - offset1;
	    box.x2 = box.x1 + pRects->width + offset2;
	    box.y2 = box.y1 + offset2;
	    TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	    if(BOX_NOT_EMPTY(box))
		shadowDamageBox ((WindowPtr) pDraw, &box);

	    box.x1 = pRects->x - offset1;
	    box.y1 = pRects->y + offset3;
	    box.x2 = box.x1 + offset2;
	    box.y2 = box.y1 + pRects->height - offset2;
	    TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	    if(BOX_NOT_EMPTY(box))
		shadowDamageBox ((WindowPtr) pDraw, &box);

	    box.x1 = pRects->x + pRects->width - offset1;
	    box.y1 = pRects->y + offset3;
	    box.x2 = box.x1 + offset2;
	    box.y2 = box.y1 + pRects->height - offset2;
	    TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	    if(BOX_NOT_EMPTY(box))
		shadowDamageBox ((WindowPtr) pDraw, &box);

	    box.x1 = pRects->x - offset1;
	    box.y1 = pRects->y + pRects->height - offset1;
	    box.x2 = box.x1 + pRects->width + offset2;
	    box.y2 = box.y1 + offset2;
	    TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	    if(BOX_NOT_EMPTY(box))
		shadowDamageBox ((WindowPtr) pDraw, &box);

	    pRects++;
	}
    }
 }

static void
shadowPolyArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyArc)(pDraw, pGC, narcs, parcs);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && narcs) {
	int extra = pGC->lineWidth >> 1;
	BoxRec box;

	box.x1 = parcs->x;
	box.x2 = box.x1 + parcs->width;
	box.y1 = parcs->y;
	box.y2 = box.y1 + parcs->height;

	/* should I break these up instead ? */

	while(--narcs) {
	   parcs++;
	   if(box.x1 > parcs->x) box.x1 = parcs->x;
	   if(box.x2 < (parcs->x + parcs->width))
		box.x2 = parcs->x + parcs->width;
	   if(box.y1 > parcs->y) box.y1 = parcs->y;
	   if(box.y2 < (parcs->y + parcs->height))
		box.y2 = parcs->y + parcs->height;
        }

	if(extra) {
	   box.x1 -= extra;
	   box.x2 += extra;
	   box.y1 -= extra;
	   box.y2 += extra;
        }

	box.x2++;
	box.y2++;

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static void
shadowFillPolygon(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		shape,
    int		mode,
    int		count,
    DDXPointPtr	pptInit
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && (count > 2)) {
	DDXPointPtr ppt = pptInit;
	int i = count;
	BoxRec box;

	box.x2 = box.x1 = ppt->x;
	box.y2 = box.y1 = ppt->y;

	if(mode != CoordModeOrigin) {
	   int x = box.x1;
	   int y = box.y1;
	   while(--i) {
		ppt++;
		x += ppt->x;
		y += ppt->y;
		if(box.x1 > x) box.x1 = x;
		else if(box.x2 < x) box.x2 = x;
		if(box.y1 > y) box.y1 = y;
		else if(box.y2 < y) box.y2 = y;
	    }
	} else {
	   while(--i) {
		ppt++;
		if(box.x1 > ppt->x) box.x1 = ppt->x;
		else if(box.x2 < ppt->x) box.x2 = ppt->x;
		if(box.y1 > ppt->y) box.y1 = ppt->y;
		else if(box.y2 < ppt->y) box.y2 = ppt->y;
	    }
	}

	box.x2++;
	box.y2++;

	(*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, count, pptInit);

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    } else
	(*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, count, pptInit);

    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);
}


static void
shadowPolyFillRect(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nRectsInit,
    xRectangle	*pRectsInit
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nRectsInit) {
	BoxRec box;
	xRectangle *pRects = pRectsInit;
	int nRects = nRectsInit;

	box.x1 = pRects->x;
	box.x2 = box.x1 + pRects->width;
	box.y1 = pRects->y;
	box.y2 = box.y1 + pRects->height;

	while(--nRects) {
	    pRects++;
	    if(box.x1 > pRects->x) box.x1 = pRects->x;
	    if(box.x2 < (pRects->x + pRects->width))
		box.x2 = pRects->x + pRects->width;
	    if(box.y1 > pRects->y) box.y1 = pRects->y;
	    if(box.y2 < (pRects->y + pRects->height))
		box.y2 = pRects->y + pRects->height;
	}

	/* cfb messes with the pRectsInit so we have to do our
	   calculations first */

	(*pGC->ops->PolyFillRect)(pDraw, pGC, nRectsInit, pRectsInit);

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	    shadowDamageBox ((WindowPtr) pDraw, &box);
    } else
	(*pGC->ops->PolyFillRect)(pDraw, pGC, nRectsInit, pRectsInit);

    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);
}


static void
shadowPolyFillArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyFillArc)(pDraw, pGC, narcs, parcs);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && narcs) {
	BoxRec box;

	box.x1 = parcs->x;
	box.x2 = box.x1 + parcs->width;
	box.y1 = parcs->y;
	box.y2 = box.y1 + parcs->height;

	/* should I break these up instead ? */

	while(--narcs) {
	   parcs++;
	   if(box.x1 > parcs->x) box.x1 = parcs->x;
	   if(box.x2 < (parcs->x + parcs->width))
		box.x2 = parcs->x + parcs->width;
	   if(box.y1 > parcs->y) box.y1 = parcs->y;
	   if(box.y2 < (parcs->y + parcs->height))
		box.y2 = parcs->y + parcs->height;
        }

	TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }

}

static int
shadowPolyText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int 	y,
    int 	count,
    char	*chars
){
    int width;

    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    width = (*pGC->ops->PolyText8)(pDraw, pGC, x, y, count, chars);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    width -= x;

    if(IS_VISIBLE(pDraw) && (width > 0)) {
	BoxRec box;

	/* ugh */
	box.x1 = pDraw->x + x + FONTMINBOUNDS(pGC->font, leftSideBearing);
	box.x2 = pDraw->x + x + FONTMAXBOUNDS(pGC->font, rightSideBearing);

	if(count > 1) {
	   if(width > 0) box.x2 += width;
	   else box.x1 += width;
	}

	box.y1 = pDraw->y + y - FONTMAXBOUNDS(pGC->font, ascent);
	box.y2 = pDraw->y + y + FONTMAXBOUNDS(pGC->font, descent);

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }

    return (width + x);
}

static int
shadowPolyText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars
){
    int width;

    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    width = (*pGC->ops->PolyText16)(pDraw, pGC, x, y, count, chars);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    width -= x;

    if(IS_VISIBLE(pDraw) && (width > 0)) {
	BoxRec box;

	/* ugh */
	box.x1 = pDraw->x + x + FONTMINBOUNDS(pGC->font, leftSideBearing);
	box.x2 = pDraw->x + x + FONTMAXBOUNDS(pGC->font, rightSideBearing);

	if(count > 1) {
	   if(width > 0) box.x2 += width;
	   else box.x1 += width;
	}

	box.y1 = pDraw->y + y - FONTMAXBOUNDS(pGC->font, ascent);
	box.y2 = pDraw->y + y + FONTMAXBOUNDS(pGC->font, descent);

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }

    return (width + x);
}

static void
shadowImageText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    char	*chars
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->ImageText8)(pDraw, pGC, x, y, count, chars);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && count) {
	int top, bot, Min, Max;
	BoxRec box;

	top = max(FONTMAXBOUNDS(pGC->font, ascent), FONTASCENT(pGC->font));
	bot = max(FONTMAXBOUNDS(pGC->font, descent), FONTDESCENT(pGC->font));

	Min = count * FONTMINBOUNDS(pGC->font, characterWidth);
	if(Min > 0) Min = 0;
	Max = count * FONTMAXBOUNDS(pGC->font, characterWidth);
	if(Max < 0) Max = 0;

	/* ugh */
	box.x1 = pDraw->x + x + Min +
		FONTMINBOUNDS(pGC->font, leftSideBearing);
	box.x2 = pDraw->x + x + Max +
		FONTMAXBOUNDS(pGC->font, rightSideBearing);

	box.y1 = pDraw->y + y - top;
	box.y2 = pDraw->y + y + bot;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}
static void
shadowImageText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->ImageText16)(pDraw, pGC, x, y, count, chars);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && count) {
	int top, bot, Min, Max;
	BoxRec box;

	top = max(FONTMAXBOUNDS(pGC->font, ascent), FONTASCENT(pGC->font));
	bot = max(FONTMAXBOUNDS(pGC->font, descent), FONTDESCENT(pGC->font));

	Min = count * FONTMINBOUNDS(pGC->font, characterWidth);
	if(Min > 0) Min = 0;
	Max = count * FONTMAXBOUNDS(pGC->font, characterWidth);
	if(Max < 0) Max = 0;

	/* ugh */
	box.x1 = pDraw->x + x + Min +
		FONTMINBOUNDS(pGC->font, leftSideBearing);
	box.x2 = pDraw->x + x + Max +
		FONTMAXBOUNDS(pGC->font, rightSideBearing);

	box.y1 = pDraw->y + y - top;
	box.y2 = pDraw->y + y + bot;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}


static void
shadowImageGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int x, int y,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->ImageGlyphBlt)(pDraw, pGC, x, y, nglyph,
					ppci, pglyphBase);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nglyph) {
	int top, bot, width = 0;
	BoxRec box;

	top = max(FONTMAXBOUNDS(pGC->font, ascent), FONTASCENT(pGC->font));
	bot = max(FONTMAXBOUNDS(pGC->font, descent), FONTDESCENT(pGC->font));

	box.x1 = ppci[0]->metrics.leftSideBearing;
	if(box.x1 > 0) box.x1 = 0;
	box.x2 = ppci[nglyph - 1]->metrics.rightSideBearing -
		ppci[nglyph - 1]->metrics.characterWidth;
	if(box.x2 < 0) box.x2 = 0;

	box.x2 += pDraw->x + x;
	box.x1 += pDraw->x + x;

	while(nglyph--) {
	    width += (*ppci)->metrics.characterWidth;
	    ppci++;
	}

	if(width > 0)
	   box.x2 += width;
	else
	   box.x1 += width;

	box.y1 = pDraw->y + y - top;
	box.y2 = pDraw->y + y + bot;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static void
shadowPolyGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int x, int y,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyGlyphBlt)(pDraw, pGC, x, y, nglyph,
				ppci, pglyphBase);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw) && nglyph) {
	BoxRec box;

	/* ugh */
	box.x1 = pDraw->x + x + ppci[0]->metrics.leftSideBearing;
	box.x2 = pDraw->x + x + ppci[nglyph - 1]->metrics.rightSideBearing;

	if(nglyph > 1) {
	    int width = 0;

	    while(--nglyph) {
		width += (*ppci)->metrics.characterWidth;
		ppci++;
	    }

	    if(width > 0) box.x2 += width;
	    else box.x1 += width;
	}

	box.y1 = pDraw->y + y - FONTMAXBOUNDS(pGC->font, ascent);
	box.y2 = pDraw->y + y + FONTMAXBOUNDS(pGC->font, descent);

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}

static void
shadowPushPixels(
    GCPtr	pGC,
    PixmapPtr	pBitMap,
    DrawablePtr pDraw,
    int	dx, int dy, int xOrg, int yOrg
){
    SHADOW_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PushPixels)(pGC, pBitMap, pDraw, dx, dy, xOrg, yOrg);
    SHADOW_GC_OP_EPILOGUE(pGC, pDraw);

    if(IS_VISIBLE(pDraw)) {
	BoxRec box;

        box.x1 = xOrg;
        box.y1 = yOrg;

        if(!pGC->miTranslate) {
           box.x1 += pDraw->x;          
           box.y1 += pDraw->y;          
        }

	box.x2 = box.x1 + dx;
	box.y2 = box.y1 + dy;

	TRIM_BOX(box, pGC);
	if(BOX_NOT_EMPTY(box))
	   shadowDamageBox ((WindowPtr) pDraw, &box);
    }
}


static void
shadowPaintWindow(
  WindowPtr pWindow,
  RegionPtr prgn,
  int what
){
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    shadowScrPriv(pScreen);

    if(what == PW_BACKGROUND) {
	unwrap (pScrPriv, pScreen, PaintWindowBackground);
	(*pScreen->PaintWindowBackground) (pWindow, prgn, what);
	wrap (pScrPriv, pScreen, PaintWindowBackground, shadowPaintWindow);
    } else {
	unwrap (pScrPriv, pScreen, PaintWindowBorder);
	(*pScreen->PaintWindowBorder) (pWindow, prgn, what);
	wrap (pScrPriv, pScreen, PaintWindowBorder, shadowPaintWindow);
    }
    shadowDamageRegion (pWindow, prgn);
}


static void
shadowCopyWindow(
   WindowPtr pWindow,
   DDXPointRec ptOldOrg,
   RegionPtr prgn
){
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    shadowScrPriv(pScreen);

    unwrap (pScrPriv, pScreen, CopyWindow);
    (*pScreen->CopyWindow) (pWindow, ptOldOrg, prgn);
    wrap (pScrPriv, pScreen, CopyWindow, shadowCopyWindow);
    shadowDamageRegion (pWindow, prgn);
}

GCOps shadowGCOps = {
    shadowFillSpans, shadowSetSpans,
    shadowPutImage, shadowCopyArea,
    shadowCopyPlane, shadowPolyPoint,
    shadowPolylines, shadowPolySegment,
    shadowPolyRectangle, shadowPolyArc,
    shadowFillPolygon, shadowPolyFillRect,
    shadowPolyFillArc, shadowPolyText8,
    shadowPolyText16, shadowImageText8,
    shadowImageText16, shadowImageGlyphBlt,
    shadowPolyGlyphBlt, shadowPushPixels,
#ifdef NEED_LINEHELPER
    NULL,
#endif
    {NULL}		/* devPrivate */
};

static void
shadowGetImage (DrawablePtr pDrawable,
		int sx,
		int sy,
		int w,
		int h,
		unsigned int format,
		unsigned long planeMask,
		char * pdstLine)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    shadowScrPriv(pScreen);

    /* Many apps use GetImage to sync with the visable frame buffer */
    if (pDrawable->type == DRAWABLE_WINDOW)
	shadowRedisplay (pScreen);
    unwrap (pScrPriv, pScreen, GetImage);
    (*pScreen->GetImage) (pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
    wrap (pScrPriv, pScreen, GetImage, shadowGetImage);
}

static void
shadowRestoreAreas (PixmapPtr pPixmap,
		    RegionPtr prgn,
		    int xorg,
		    int yorg,
		    WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    shadowScrPriv(pScreen);

    unwrap (pScrPriv, pScreen, BackingStoreFuncs.RestoreAreas);
    (*pScreen->BackingStoreFuncs.RestoreAreas) (pPixmap, prgn,
						xorg, yorg, pWin);
    wrap (pScrPriv, pScreen, BackingStoreFuncs.RestoreAreas,
			     shadowRestoreAreas);
    shadowDamageRegion (pWin, prgn);
}

static Bool
shadowCloseScreen (int i, ScreenPtr pScreen)
{
    shadowScrPriv(pScreen);

    unwrap (pScrPriv, pScreen, CreateGC);
    unwrap (pScrPriv, pScreen, PaintWindowBackground);
    unwrap (pScrPriv, pScreen, PaintWindowBorder);
    unwrap (pScrPriv, pScreen, CopyWindow);
    unwrap (pScrPriv, pScreen, CloseScreen);
    unwrap (pScrPriv, pScreen, GetImage);
    unwrap (pScrPriv, pScreen, BackingStoreFuncs.RestoreAreas);
    xfree (pScrPriv);
    return (*pScreen->CloseScreen) (i, pScreen);
}

Bool
shadowSetup (ScreenPtr pScreen)
{
    shadowScrPrivPtr	pScrPriv;
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(pScreen);
#endif

    if (shadowGeneration != serverGeneration)
    {
	shadowScrPrivateIndex = AllocateScreenPrivateIndex ();
	if (shadowScrPrivateIndex == -1)
	    return FALSE;
	shadowGCPrivateIndex = AllocateGCPrivateIndex ();
	if (shadowGCPrivateIndex == -1)
	    return FALSE;
	shadowGeneration = serverGeneration;
    }
    if (!AllocateGCPrivate (pScreen, shadowGCPrivateIndex, sizeof (shadowGCPrivRec)))
	return FALSE;
    pScrPriv = (shadowScrPrivPtr) xalloc (sizeof (shadowScrPrivRec));
    if (!pScrPriv)
	return FALSE;

    if (!RegisterBlockAndWakeupHandlers (shadowBlockHandler,
					 shadowWakeupHandler,
					 (pointer) pScreen))
	return FALSE;

    wrap (pScrPriv, pScreen, CreateGC, shadowCreateGC);
    wrap (pScrPriv, pScreen, PaintWindowBackground, shadowPaintWindow);
    wrap (pScrPriv, pScreen, PaintWindowBorder, shadowPaintWindow);
    wrap (pScrPriv, pScreen, CopyWindow, shadowCopyWindow);
    wrap (pScrPriv, pScreen, CloseScreen, shadowCloseScreen);
    wrap (pScrPriv, pScreen, GetImage, shadowGetImage);
    wrap (pScrPriv, pScreen, BackingStoreFuncs.RestoreAreas,
			     shadowRestoreAreas);
#ifdef RENDER
    if (ps) {
	wrap (pScrPriv, ps, Glyphs, shadowGlyphs);
	wrap (pScrPriv, ps, Composite, shadowComposite);
    }
#endif
    pScrPriv->pBuf = 0;

    pScreen->devPrivates[shadowScrPrivateIndex].ptr = (pointer) pScrPriv;
    return TRUE;
}

Bool
shadowAdd (ScreenPtr	    pScreen,
	   PixmapPtr	    pPixmap,
	   ShadowUpdateProc update,
	   ShadowWindowProc window,
	   int		    randr,
	   void		    *closure)
{
    shadowScrPriv(pScreen);
    shadowBufPtr    pBuf;

    pBuf = (shadowBufPtr) xalloc (sizeof (shadowBufRec));
    if (!pBuf)
	return FALSE;
    /*
     * Map simple rotation values to bitmasks; fortunately,
     * these are all unique
     */
    switch (randr) {
    case 0:
	randr = SHADOW_ROTATE_0;
	break;
    case 90:
	randr = SHADOW_ROTATE_90;
	break;
    case 180:
	randr = SHADOW_ROTATE_180;
	break;
    case 270:
	randr = SHADOW_ROTATE_270;
	break;
    }
    pBuf->pPixmap = pPixmap;
    pBuf->update = update;
    pBuf->window = window;
    REGION_NULL(pScreen, &pBuf->damage);
    pBuf->pNext = pScrPriv->pBuf;
    pBuf->randr = randr;
    pBuf->closure = 0;
    pScrPriv->pBuf = pBuf;
    return TRUE;
}

void
shadowRemove (ScreenPtr pScreen, PixmapPtr pPixmap)
{
    shadowScrPriv(pScreen);
    shadowBufPtr    pBuf, *pPrev;

    for (pPrev = &pScrPriv->pBuf; (pBuf = *pPrev); pPrev = &pBuf->pNext)
	if (pBuf->pPixmap == pPixmap)
	{
	    REGION_UNINIT (pScreen, &pBuf->damage);
	    *pPrev = pBuf->pNext;
	    xfree (pBuf);
	    break;
	}
}

shadowBufPtr
shadowFindBuf (WindowPtr pWindow)
{
    ScreenPtr	    pScreen = pWindow->drawable.pScreen;
    shadowScrPriv(pScreen);
    shadowBufPtr    pBuf, *pPrev;
    PixmapPtr	    pPixmap = (*pScreen->GetWindowPixmap) (pWindow);

    for (pPrev = &pScrPriv->pBuf; (pBuf = *pPrev); pPrev = &pBuf->pNext)
    {
	if (!pBuf->pPixmap)
	    pBuf->pPixmap = (*pScreen->GetScreenPixmap) (pScreen);
	if (pBuf->pPixmap == pPixmap)
	{
	    /*
	     * Reorder so this one is first next time
	     */
	    if (pPrev != &pScrPriv->pBuf)
	    {
		*pPrev = pBuf->pNext;
		pBuf->pNext = pScrPriv->pBuf;
		pScrPriv->pBuf = pBuf;
	    }
	    return pBuf;
	}
    }
    return 0;
}

Bool
shadowInit (ScreenPtr pScreen, ShadowUpdateProc update, ShadowWindowProc window)
{
    if (!shadowSetup (pScreen))
	return FALSE;

    if (!shadowAdd (pScreen, 0, update, window, SHADOW_ROTATE_0, 0))
	return FALSE;

    return TRUE;
}
