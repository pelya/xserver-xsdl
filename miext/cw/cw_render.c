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

#ifdef RENDER

#define cwPsDecl(pScreen)	\
    PictureScreenPtr	ps = GetPictureScreen (pScreen);	\
    cwScreenPtr		pCwScreen = getCwScreen (pScreen)

#define cwBackingPicture(pCwPicture, pPicture) \
    ((pCwPicture && pCwPicture->pBackingPicture) ? \
     pCwPicture->pBackingPicture : pPicture)

#define cwPictureDecl								\
    cwPicturePtr    pCwPicture = getCwPicture(pPicture);			\
    PicturePtr	    pBackingPicture = pCwPicture ? pCwPicture->pBackingPicture : 0

#define cwSrcPictureDecl							\
    int		    src_picture_x_off, src_picture_y_off;			\
    PicturePtr	    pBackingSrcPicture = cwGetBackingPicture(pSrcPicture,	\
							     &src_picture_x_off,\
							     &src_picture_y_off)

#define cwDstPictureDecl							\
    int		    dst_picture_x_off, dst_picture_y_off;			\
    PicturePtr	    pBackingDstPicture = cwGetBackingPicture(pDstPicture,	\
							     &dst_picture_x_off,\
							     &dst_picture_y_off)

#define cwMskPictureDecl							\
    int		    msk_picture_x_off = 0, msk_picture_y_off = 0;		\
    PicturePtr	    pBackingMskPicture = (!pMskPicture ? 0 :	    		\
					  cwGetBackingPicture(pMskPicture,	\
							      &msk_picture_x_off,\
							      &msk_picture_y_off))

#define cwPsUnwrap(elt) {	\
    ps->elt = pCwScreen->elt;	\
}

#define cwPsWrap(elt,func) {	\
    pCwScreen->elt = ps->elt;	\
    ps->elt = func;		\
}

static VisualPtr
cwFindVisualById (ScreenPtr pScreen, VisualID visual)
{
    int		i;
    VisualPtr	pVisual;
    for (i = 0, pVisual = pScreen->visuals;
	 i < pScreen->numVisuals;
	 i++, pVisual++)
    {
	if (pVisual->vid == visual)
	    return pVisual;
    }
    return 0;
}

static PicturePtr
cwCreateBackingPicture (PicturePtr pPicture)
{
    ScreenPtr	    pScreen = pPicture->pDrawable->pScreen;
    WindowPtr	    pWindow = (WindowPtr) pPicture->pDrawable;
    PixmapPtr	    pPixmap = (*pScreen->GetWindowPixmap) (pWindow);
    VisualPtr	    pVisual = cwFindVisualById (pScreen, wVisual (pWindow));
    PictFormatPtr   pFormat = PictureMatchVisual (pScreen, pWindow->drawable.depth,
						  pVisual);
    int		    error;
    PicturePtr	    pBackingPicture = CreatePicture (0, &pPixmap->drawable, pFormat,
						     0, 0, serverClient, &error);
    cwPicturePtr    pCwPicture = getCwPicture (pPicture);

    return pCwPicture->pBackingPicture = pBackingPicture;
}

static void
cwDestroyBackingPicture (PicturePtr pPicture)
{
    cwPictureDecl;

    if (pBackingPicture)
    {
	FreePicture (pBackingPicture, 0);
	pCwPicture->pBackingPicture = 0;
    }
}

static PicturePtr
cwGetBackingPicture (PicturePtr pPicture, int *x_off, int *y_off)
{
    cwPictureDecl;

    if (pBackingPicture)
    {
	DrawablePtr pDrawable = pPicture->pDrawable;
	ScreenPtr   pScreen = pDrawable->pScreen;
	WindowPtr   pWin = (WindowPtr) pDrawable;
	PixmapPtr   pPixmap = (*pScreen->GetWindowPixmap) (pWin);

	*x_off = -pPixmap->screen_x;
	*y_off = -pPixmap->screen_y;

	return pBackingPicture;
    }
    else
    {
	*x_off = *y_off = 0;
	return pPicture;
    }
}

static int
cwCreatePicture (PicturePtr pPicture)
{
    int			ret;
    ScreenPtr		pScreen = pPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    
    cwPsUnwrap (CreatePicture);
    setCwPicture(pPicture, 0);
    ret = (*ps->CreatePicture) (pPicture);
    cwPsWrap (CreatePicture, cwCreatePicture);
    return ret;
}
    
static void
cwDestroyPicture (PicturePtr pPicture)
{
    ScreenPtr		pScreen = pPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    
    cwPsUnwrap(DestroyPicture);
    cwDestroyBackingPicture (pPicture);
    (*ps->DestroyPicture) (pPicture);
    cwPsWrap(DestroyPicture, cwDestroyPicture);
}

static void
cwChangePicture (PicturePtr pPicture,
		 Mask	mask)
{
    ScreenPtr		pScreen = pPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwPictureDecl;
    
    cwPsUnwrap(ChangePicture);
    if (pBackingPicture)
    {
	(*ps->ChangePicture) (pBackingPicture, mask);
    }
    else
    {
	(*ps->ChangePicture) (pPicture, mask);
    }
    cwPsWrap(ChangePicture, cwChangePicture);
}
		 
static void
cwValidatePicture (PicturePtr pPicture,
		   Mask       mask)
{
    ScreenPtr		pScreen = pPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwPictureDecl;
    
    cwPsUnwrap(ValidatePicture);
    (*ps->ValidatePicture) (pPicture, mask);
    if (!cwDrawableIsRedirWindow (pPicture->pDrawable))
    {
	if (pBackingPicture)
	    cwDestroyBackingPicture (pPicture);
    }
    else
    {
	DrawablePtr pDrawable = pPicture->pDrawable;
	WindowPtr   pWin = (WindowPtr) (pDrawable);
	DrawablePtr pBackingDrawable;
	int	    x_off, y_off;
	
	if (pBackingPicture && pBackingPicture->pDrawable != 
	    &(*pScreen->GetWindowPixmap) ((WindowPtr) pPicture->pDrawable)->drawable)
	{
	    cwDestroyBackingPicture (pPicture);
	    pBackingPicture = 0;
	}

	if (!pBackingPicture)
	{
	    pBackingPicture = cwCreateBackingPicture (pPicture);
	    if (!pBackingPicture)
	    {
		cwPsWrap(ValidatePicture, cwValidatePicture);
		return;
	    }
	}

	pBackingDrawable = cwGetBackingDrawable (&pWin->drawable, &x_off, &y_off);
	
	/* Check to see if a new composite clip must be generated */

	if (pDrawable->serialNumber != pCwPicture->serialNumber ||
	    (mask & (CPClipXOrigin|CPClipYOrigin|CPClipMask|CPSubwindowMode)))
	{
	    RegionPtr	pCompositeClip;

	    pCompositeClip = REGION_CREATE(pScreen, NULL, 1);
	    /* note - CT_PIXMAP "cannot" happen because no DDX supports it*/
	    REGION_COPY (pScreen, pCompositeClip, pPicture->pCompositeClip);
	    SetPictureClipRegion (pBackingPicture, -x_off, -y_off,
				  pCompositeClip);
	    pCwPicture->serialNumber = pDrawable->serialNumber;
	}
	mask |= pCwPicture->stateChanges;
	(*ps->ValidatePicture) (pBackingPicture, mask);
	pCwPicture->stateChanges = 0;
	pBackingPicture->serialNumber = pBackingDrawable->serialNumber;
    }
    cwPsWrap(ValidatePicture, cwValidatePicture);
}

static void
cwComposite (CARD8	op,
	     PicturePtr pSrcPicture,
	     PicturePtr pMskPicture,
	     PicturePtr pDstPicture,
	     INT16	xSrc,
	     INT16	ySrc,
	     INT16	xMsk,
	     INT16	yMsk,
	     INT16	xDst,
	     INT16	yDst,
	     CARD16	width,
	     CARD16	height)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwSrcPictureDecl;
    cwMskPictureDecl;
    cwDstPictureDecl;
    
    cwPsUnwrap(Composite);
    (*ps->Composite) (op, pBackingSrcPicture, pBackingMskPicture, pBackingDstPicture,
		      xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		      xMsk + msk_picture_x_off, yMsk + msk_picture_y_off,
		      xDst + dst_picture_x_off, yDst + dst_picture_y_off,
		      width, height);
    cwPsWrap(Composite, cwComposite);
}

static void
cwGlyphs (CARD8      op,
	  PicturePtr pSrcPicture,
	  PicturePtr pDstPicture,
	  PictFormatPtr  maskFormat,
	  INT16      xSrc,
	  INT16      ySrc,
	  int	nlists,
	  GlyphListPtr   lists,
	  GlyphPtr	*glyphs)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwSrcPictureDecl;
    cwDstPictureDecl;
    
    cwPsUnwrap(Glyphs);
    if (nlists)
    {
	lists->xOff += dst_picture_x_off;
	lists->yOff += dst_picture_y_off;
    }
    (*ps->Glyphs) (op, pBackingSrcPicture, pBackingDstPicture, maskFormat,
		   xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		   nlists, lists, glyphs);
    if (nlists)
    {
	lists->xOff -= dst_picture_x_off;
	lists->yOff -= dst_picture_y_off;
    }
    cwPsWrap(Glyphs, cwGlyphs);
}

static void
cwCompositeRects (CARD8		op,
		  PicturePtr	pDstPicture,
		  xRenderColor  *color,
		  int		nRect,
		  xRectangle	*rects)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwDstPictureDecl;
    int i;
    
    cwPsUnwrap(CompositeRects);
    for (i = 0; i < nRect; i++)
    {
	rects[i].x += dst_picture_x_off;
	rects[i].y += dst_picture_y_off;
    }
    (*ps->CompositeRects) (op, pBackingDstPicture, color, nRect, rects);
    for (i = 0; i < nRect; i++)
    {
	rects[i].x -= dst_picture_x_off;
	rects[i].y -= dst_picture_y_off;
    }
    cwPsWrap(CompositeRects, cwCompositeRects);
}

static void
cwTrapezoids (CARD8	    op,
	      PicturePtr    pSrcPicture,
	      PicturePtr    pDstPicture,
	      PictFormatPtr maskFormat,
	      INT16	    xSrc,
	      INT16	    ySrc,
	      int	    ntrap,
	      xTrapezoid    *traps)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwSrcPictureDecl;
    cwDstPictureDecl;
    int i;
    
    cwPsUnwrap(Trapezoids);
    if (dst_picture_x_off | dst_picture_y_off)
	for (i = 0; i < ntrap; i++)
	{
	    traps[i].top += dst_picture_y_off << 16;
	    traps[i].bottom += dst_picture_y_off << 16;
	    traps[i].left.p1.x += dst_picture_x_off << 16;
	    traps[i].left.p1.y += dst_picture_y_off << 16;
	    traps[i].left.p2.x += dst_picture_x_off << 16;
	    traps[i].left.p2.y += dst_picture_y_off << 16;
	    traps[i].right.p1.x += dst_picture_x_off << 16;
	    traps[i].right.p1.y += dst_picture_y_off << 16;
	    traps[i].right.p2.x += dst_picture_x_off << 16;
	    traps[i].right.p2.y += dst_picture_y_off << 16;
	}
    (*ps->Trapezoids) (op, pBackingSrcPicture, pBackingDstPicture, maskFormat,
		       xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		       ntrap, traps);
    if (dst_picture_x_off | dst_picture_y_off)
	for (i = 0; i < ntrap; i++)
	{
	    traps[i].top -= dst_picture_y_off << 16;
	    traps[i].bottom -= dst_picture_y_off << 16;
	    traps[i].left.p1.x -= dst_picture_x_off << 16;
	    traps[i].left.p1.y -= dst_picture_y_off << 16;
	    traps[i].left.p2.x -= dst_picture_x_off << 16;
	    traps[i].left.p2.y -= dst_picture_y_off << 16;
	    traps[i].right.p1.x -= dst_picture_x_off << 16;
	    traps[i].right.p1.y -= dst_picture_y_off << 16;
	    traps[i].right.p2.x -= dst_picture_x_off << 16;
	    traps[i].right.p2.y -= dst_picture_y_off << 16;
	}
    cwPsWrap(Trapezoids, cwTrapezoids);
}

static void
cwTriangles (CARD8	    op,
	     PicturePtr	    pSrc,
	     PicturePtr	    pDst,
	     PictFormatPtr  maskFormat,
	     INT16	    xSrc,
	     INT16	    ySrc,
	     int	    ntri,
	     xTriangle	    *tris)
{
    /* FIXME */
}

static void
cwTriStrip (CARD8	    op,
					     PicturePtr	    pSrc,
					     PicturePtr	    pDst,
					     PictFormatPtr  maskFormat,
					     INT16	    xSrc,
					     INT16	    ySrc,
					     int	    npoint,
					     xPointFixed    *points)
{
    /* FIXME */
}

static void
cwTriFan (CARD8	    op,
					     PicturePtr	    pSrc,
					     PicturePtr	    pDst,
					     PictFormatPtr  maskFormat,
					     INT16	    xSrc,
					     INT16	    ySrc,
					     int	    npoint,
					     xPointFixed    *points)
{
    /* FIXME */
}

Bool
cwInitializeRender (ScreenPtr pScreen)
{
    cwPsDecl (pScreen);

    if (!AllocatePicturePrivate (pScreen, cwPictureIndex, 0))
	return FALSE;
    cwPsWrap(CreatePicture, cwCreatePicture);
    cwPsWrap(DestroyPicture, cwDestroyPicture);
    cwPsWrap(ChangePicture, cwChangePicture);
    cwPsWrap(ValidatePicture, cwValidatePicture);
    cwPsWrap(Composite, cwComposite);
    cwPsWrap(Glyphs, cwGlyphs);
    cwPsWrap(CompositeRects, cwCompositeRects);
    cwPsWrap(Trapezoids, cwTrapezoids);
    cwPsWrap(Triangles, cwTriangles);
    cwPsWrap(TriStrip, cwTriStrip);
    cwPsWrap(TriFan, cwTriFan);
    return TRUE;
}

#endif /* RENDER */
