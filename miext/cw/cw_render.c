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

#define cwPictureDecl						\
    PicturePtr	    pBackingPicture =				\
	((pPicture)->devPrivates[cwPictureIndex].ptr)

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

static PicturePtr
cwCreateBackingPicture (PicturePtr pPicture)
{
    ScreenPtr	    pScreen = pPicture->pDrawable->pScreen;
    WindowPtr	    pWindow = (WindowPtr) pPicture->pDrawable;
    PixmapPtr	    pPixmap = (*pScreen->GetWindowPixmap) (pWindow);
    int		    error;
    PicturePtr	    pBackingPicture;

    pBackingPicture = CreatePicture (0, &pPixmap->drawable, pPicture->pFormat,
				     0, 0, serverClient, &error);

    pPicture->devPrivates[cwPictureIndex].ptr = pBackingPicture;

    CopyPicture(pPicture, (1 << (CPLastBit + 1)) - 1, pBackingPicture);

    return pBackingPicture;
}

static void
cwDestroyBackingPicture (PicturePtr pPicture)
{
    cwPictureDecl;

    if (pBackingPicture)
    {
	FreePicture (pBackingPicture, 0);
	pPicture->devPrivates[cwPictureIndex].ptr = NULL;
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

	*x_off = pPixmap->drawable.x - pPixmap->screen_x;
	*y_off = pPixmap->drawable.y - pPixmap->screen_y;

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

	pBackingDrawable = cwGetBackingDrawable (&pWin->drawable, &x_off,&y_off);

	SetPictureTransform(pBackingPicture, pPicture->transform);
	/* XXX Set filters */

	if (mask & (CPClipXOrigin || CPClipYOrigin)) {
	    XID vals[2];

	    vals[0] = pPicture->clipOrigin.x + x_off;
	    vals[1] = pPicture->clipOrigin.y + y_off;

	    ChangePicture(pBackingPicture, CPClipXOrigin | CPClipYOrigin,
			  vals, NULL, NullClient);
	    mask &= ~(CPClipXOrigin | CPClipYOrigin);
	}

	CopyPicture(pPicture, mask, pBackingPicture);

	(*ps->ValidatePicture) (pBackingPicture, mask);
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
    if (dst_picture_x_off || dst_picture_y_off) {
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
    }
    (*ps->Trapezoids) (op, pBackingSrcPicture, pBackingDstPicture, maskFormat,
		       xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		       ntrap, traps);
    cwPsWrap(Trapezoids, cwTrapezoids);
}

static void
cwTriangles (CARD8	    op,
	     PicturePtr	    pSrcPicture,
	     PicturePtr	    pDstPicture,
	     PictFormatPtr  maskFormat,
	     INT16	    xSrc,
	     INT16	    ySrc,
	     int	    ntri,
	     xTriangle	   *tris)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwSrcPictureDecl;
    cwDstPictureDecl;
    int i;
    
    cwPsUnwrap(Triangles);
    if (dst_picture_x_off || dst_picture_y_off) {
	for (i = 0; i < ntri; i++)
	{
	    tris[i].p1.x += dst_picture_x_off << 16;
	    tris[i].p1.y += dst_picture_y_off << 16;
	    tris[i].p2.x += dst_picture_x_off << 16;
	    tris[i].p2.y += dst_picture_y_off << 16;
	    tris[i].p3.x += dst_picture_x_off << 16;
	    tris[i].p3.y += dst_picture_y_off << 16;
	}
    }
    (*ps->Triangles) (op, pBackingSrcPicture, pBackingDstPicture, maskFormat,
		      xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		      ntri, tris);
    cwPsWrap(Triangles, cwTriangles);
}

static void
cwTriStrip (CARD8	    op,
	    PicturePtr	    pSrcPicture,
	    PicturePtr	    pDstPicture,
	    PictFormatPtr   maskFormat,
	    INT16	    xSrc,
	    INT16	    ySrc,
	    int		    npoint,
	    xPointFixed    *points)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwSrcPictureDecl;
    cwDstPictureDecl;
    int i;

    cwPsUnwrap(TriStrip);
    if (dst_picture_x_off || dst_picture_y_off) {
	for (i = 0; i < npoint; i++)
	{
	    points[i].x += dst_picture_x_off << 16;
	    points[i].y += dst_picture_y_off << 16;
	}
    }
    (*ps->TriStrip) (op, pBackingSrcPicture, pBackingDstPicture, maskFormat,
		     xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		     npoint, points);
    cwPsWrap(TriStrip, cwTriStrip);
}

static void
cwTriFan (CARD8		 op,
	  PicturePtr	 pSrcPicture,
	  PicturePtr	 pDstPicture,
	  PictFormatPtr  maskFormat,
	  INT16		 xSrc,
	  INT16		 ySrc,
	  int		 npoint,
	  xPointFixed   *points)
{
    ScreenPtr	pScreen = pDstPicture->pDrawable->pScreen;
    cwPsDecl(pScreen);
    cwSrcPictureDecl;
    cwDstPictureDecl;
    int i;

    cwPsUnwrap(TriFan);
    if (dst_picture_x_off || dst_picture_y_off) {
	for (i = 0; i < npoint; i++)
	{
	    points[i].x += dst_picture_x_off << 16;
	    points[i].y += dst_picture_y_off << 16;
	}
    }
    (*ps->TriFan) (op, pBackingSrcPicture, pBackingDstPicture, maskFormat,
		   xSrc + src_picture_x_off, ySrc + src_picture_y_off,
		   npoint, points);
    cwPsWrap(TriFan, cwTriFan);
}

void
cwInitializeRender (ScreenPtr pScreen)
{
    cwPsDecl (pScreen);

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
}

void
cwFiniRender (ScreenPtr pScreen)
{
    cwPsDecl (pScreen);

    cwPsUnwrap(CreatePicture);
    cwPsUnwrap(DestroyPicture);
    cwPsUnwrap(ChangePicture);
    cwPsUnwrap(ValidatePicture);
    cwPsUnwrap(Composite);
    cwPsUnwrap(Glyphs);
    cwPsUnwrap(CompositeRects);
    cwPsUnwrap(Trapezoids);
    cwPsUnwrap(Triangles);
    cwPsUnwrap(TriStrip);
    cwPsUnwrap(TriFan);
}

#endif /* RENDER */
