/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@freedesktop.org>
 */

#include "xgl.h"
#include "fb.h"
#include "gcstruct.h"
#include "migc.h"

#define XGL_GC_OP_FALLBACK_PROLOGUE(pDrawable) \
    xglSyncDamageBoxBits (pDrawable);	       \
    XGL_GC_UNWRAP (funcs);		       \
    XGL_GC_UNWRAP (ops)

#define XGL_GC_OP_FALLBACK_EPILOGUE(pDrawable)	  \
    XGL_GC_WRAP (funcs, (GCFuncs *) &xglGCFuncs); \
    XGL_GC_WRAP (ops, (GCOps *) &xglGCOps);	  \
    xglAddSurfaceDamage (pDrawable)

#define XGL_GC_FILL_OP_FALLBACK_PROLOGUE(pDrawable)		 \
    switch (pGC->fillStyle) {					 \
    case FillSolid:						 \
	break;							 \
    case FillStippled:						 \
    case FillOpaqueStippled:					 \
	if (!xglSyncBits (&pGC->stipple->drawable, NullBox))	 \
	    FatalError (XGL_SW_FAILURE_STRING);			 \
	break;							 \
    case FillTiled:						 \
	if (!xglSyncBits (&pGC->tile.pixmap->drawable, NullBox)) \
	    FatalError (XGL_SW_FAILURE_STRING);			 \
	break;							 \
    }								 \
    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable)

static const GCFuncs xglGCFuncs = {
    xglValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

static const GCOps xglGCOps = {
    xglFillSpans,
    xglSetSpans,
    xglPutImage,
    xglCopyArea,
    xglCopyPlane,
    xglPolyPoint,
    xglPolylines,
    xglPolySegment,
    miPolyRectangle,
    xglPolyArc,
    miFillPolygon,
    xglPolyFillRect,
    xglPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    xglImageGlyphBlt,
    xglPolyGlyphBlt,
    xglPushPixels
#ifdef NEED_LINEHELPER
    , NULL
#endif
};

void
xglFillSpans (DrawablePtr pDrawable,
	      GCPtr	  pGC,
	      int	  nspans,
	      DDXPointPtr ppt,
	      int	  *pwidth,
	      int	  fSorted)
{
    XGL_GC_PRIV (pGC);

    if (!pGCPriv->flags)
    {
	if (xglFillSpan (pDrawable, pGC, nspans, ppt, pwidth))
	{
	    xglAddBitDamage (pDrawable);
	    return;
	}
    }

    XGL_GC_FILL_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->FillSpans) (pDrawable, pGC, nspans, ppt, pwidth, fSorted);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglSetSpans (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     char	 *psrc,
	     DDXPointPtr ppt,
	     int	 *pwidth,
	     int	 nspans,
	     int	 fSorted)
{
    XGL_GC_PRIV (pGC);
    
    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->SetSpans) (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglPutImage (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     int	 depth,
	     int	 x,
	     int	 y,
	     int	 w,
	     int	 h,
	     int	 leftPad,
	     int	 format,
	     char	 *bits)
{
    XGL_GC_PRIV (pGC);

    switch (format) {
    case XYBitmap:
	break;
    case XYPixmap:
    case ZPixmap:
	if (!pGCPriv->flags)
	{
	    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

	    if (!pPixmapPriv->allBits &&
		pPixmapPriv->target == xglPixmapTargetIn)
	    {
		if (xglSetPixels (pDrawable,
				  bits,
				  PixmapBytePad (w, pDrawable->depth),
				  FALSE,
				  x + pDrawable->x, y + pDrawable->y,
				  w, h,
				  REGION_RECTS (pGC->pCompositeClip),
				  REGION_NUM_RECTS (pGC->pCompositeClip)))
		{
		    xglAddBitDamage (pDrawable);
		    return;
		}
	    }
	}
	break;
    }
    
    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->PutImage) (pDrawable, pGC, depth,
			   x, y, w, h, leftPad, format, bits);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

RegionPtr
xglCopyArea (DrawablePtr pSrc,
	     DrawablePtr pDst,
	     GCPtr	 pGC,
	     int	 srcX,
	     int	 srcY,
	     int	 w,
	     int	 h,
	     int	 dstX,
	     int	 dstY)
{
    unsigned long flags;
    RegionPtr	  pRegion;
    BoxRec	  box;
    
    XGL_GC_PRIV (pGC);

    flags = pGCPriv->flags;

    if (XGL_GET_DRAWABLE_PIXMAP_PRIV (pSrc)->target == xglPixmapTargetIn)
	flags &= ~xglGCReadOnlyDrawableFlag;

    if (!flags)
    {
	Bool ret;

	ret = TRUE;
	pRegion = fbDoCopy (pSrc, pDst, pGC,
			    srcX, srcY,
			    w, h,
			    dstX, dstY,
			    xglCopyProc, 0,
			    (void *) &ret);
	if (ret)
	{
	    xglAddBitDamage (pDst);
	    return pRegion;
	}
	
	if (pRegion)
	    REGION_DESTROY (pDst->pScreen, pRegion);
    }

    box.x1 = pSrc->x + srcX;
    box.y1 = pSrc->y + srcY;
    box.x2 = box.x1 + w;
    box.y2 = box.y1 + h;
    
    if (!xglSyncBits (pSrc, &box))
	FatalError (XGL_SW_FAILURE_STRING);

    XGL_GC_OP_FALLBACK_PROLOGUE (pDst);
    pRegion = (*pGC->ops->CopyArea) (pSrc, pDst, pGC,
				     srcX, srcY, w, h, dstX, dstY);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDst);

    return pRegion;
}

RegionPtr
xglCopyPlane (DrawablePtr   pSrc,
	      DrawablePtr   pDst,
	      GCPtr	    pGC,
	      int	    srcX,
	      int	    srcY,
	      int	    w,
	      int	    h,
	      int	    dstX,
	      int	    dstY,
	      unsigned long bitPlane)
{
    RegionPtr pRegion;
    BoxRec    box;
    
    XGL_GC_PRIV (pGC);

    box.x1 = pSrc->x + srcX;
    box.y1 = pSrc->y + srcY;
    box.x2 = box.x1 + w;
    box.y2 = box.y1 + h;
    
    if (!xglSyncBits (pSrc, &box))
	FatalError (XGL_SW_FAILURE_STRING);

    XGL_GC_OP_FALLBACK_PROLOGUE (pDst);
    pRegion = (*pGC->ops->CopyPlane) (pSrc, pDst, pGC,
				      srcX, srcY, w, h, dstX, dstY,
				      bitPlane);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDst);

    return pRegion;
}

void
xglPolyPoint (DrawablePtr pDrawable,
	      GCPtr       pGC,
	      int	  mode,
	      int	  npt,
	      DDXPointPtr pptInit)
{
    XGL_GC_PRIV (pGC);

    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pptInit);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglPolylines (DrawablePtr pDrawable,
	      GCPtr       pGC,
	      int	  mode,
	      int	  npt,
	      DDXPointPtr ppt)
{
    if (pGC->lineWidth == 0)
    {
	XGL_GC_PRIV (pGC);

	if (!pGCPriv->flags)
	{
	    if (pGC->lineStyle == LineSolid)
	    {
		if (xglFillLine (pDrawable, pGC, mode, npt, ppt))
		{
		    xglAddBitDamage (pDrawable);
		    return;
		}
	    }
	}
	
	XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
	(*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, ppt);
	XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
    }
    else
    {
	if (pGC->lineStyle != LineSolid)
	    miWideDash (pDrawable, pGC, mode, npt, ppt);
	else
	    miWideLine (pDrawable, pGC, mode, npt, ppt);
    }
}

void
xglPolySegment (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    nsegInit,
		xSegment    *pSegInit)
{
    if (pGC->lineWidth == 0)
    {
	XGL_GC_PRIV (pGC);

	if (!pGCPriv->flags)
	{
	    if (pGC->lineStyle == LineSolid)
	    {
		if (xglFillSegment (pDrawable, pGC, nsegInit, pSegInit))
		{
		    xglAddBitDamage (pDrawable);
		    return;
		}
	    }
	}

	XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
	(*pGC->ops->PolySegment) (pDrawable, pGC, nsegInit, pSegInit);
	XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
    } else
	miPolySegment (pDrawable, pGC, nsegInit, pSegInit);
}

void
xglPolyArc (DrawablePtr pDrawable,
	    GCPtr	pGC, 
	    int		narcs,
	    xArc	*pArcs)
{
    if (pGC->lineWidth == 0)
    {
	XGL_GC_PRIV (pGC);
	
	XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
	(*pGC->ops->PolyArc) (pDrawable, pGC, narcs, pArcs);
	XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
    } else
	miPolyArc (pDrawable, pGC, narcs, pArcs);
}

void
xglPolyFillRect (DrawablePtr pDrawable,
		 GCPtr	     pGC,
		 int	     nrect,
		 xRectangle  *prect)
{
    XGL_GC_PRIV (pGC);

    if (!pGCPriv->flags)
    {
	if (xglFillRect (pDrawable, pGC, nrect, prect))
	{
	    xglAddBitDamage (pDrawable);
	    return;
	}
    }

    XGL_GC_FILL_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->PolyFillRect) (pDrawable, pGC, nrect, prect);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglPolyFillArc (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    narcs,
		xArc	    *pArcs)
{
    XGL_GC_PRIV (pGC);
    
    XGL_GC_FILL_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->PolyFillArc) (pDrawable, pGC, narcs, pArcs);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglImageGlyphBlt (DrawablePtr  pDrawable,
		  GCPtr	       pGC,
		  int	       x,
		  int	       y,
		  unsigned int nglyph,
		  CharInfoPtr  *ppci,
		  pointer      pglyphBase)
{
    XGL_GC_PRIV (pGC);
    
    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->ImageGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci,
				pglyphBase);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglPolyGlyphBlt (DrawablePtr  pDrawable,
		 GCPtr	      pGC,
		 int	      x,
		 int	      y,
		 unsigned int nglyph,
		 CharInfoPtr  *ppci,
		 pointer      pglyphBase)
{
    XGL_GC_PRIV (pGC);
    
    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

void
xglPushPixels (GCPtr	   pGC,
	       PixmapPtr   pBitmap,
	       DrawablePtr pDrawable,
	       int	   w,
	       int	   h,
	       int	   x,
	       int	   y)
{
    XGL_GC_PRIV (pGC);

    if (!xglSyncBits (&pBitmap->drawable, NullBox))
	FatalError (XGL_SW_FAILURE_STRING);
    
    XGL_GC_OP_FALLBACK_PROLOGUE (pDrawable);
    (*pGC->ops->PushPixels) (pGC, pBitmap, pDrawable, w, h, x, y);
    XGL_GC_OP_FALLBACK_EPILOGUE (pDrawable);
}

Bool
xglCreateGC (GCPtr pGC)
{
    static glitz_color_t black = { 0x0, 0x0, 0x0, 0xffff };
    ScreenPtr		 pScreen = pGC->pScreen;
    Bool		 ret;
    
    XGL_SCREEN_PRIV (pScreen);
    XGL_GC_PRIV (pGC);

    XGL_SCREEN_UNWRAP (CreateGC);
    ret = (*pScreen->CreateGC) (pGC);
    XGL_SCREEN_WRAP (CreateGC, xglCreateGC);

    XGL_GC_WRAP (funcs, (GCFuncs *) &xglGCFuncs);
    XGL_GC_WRAP (ops, (GCOps *) &xglGCOps);
    
    pGCPriv->flags = 0;
    pGCPriv->op = GLITZ_OPERATOR_SRC;
    pGCPriv->fg = black;
    pGCPriv->bg = black;
    
    return ret;
}

void
xglValidateGC (GCPtr	     pGC,
	       unsigned long changes,
	       DrawablePtr   pDrawable)
{
    XGL_GC_PRIV (pGC);

    XGL_GC_UNWRAP (funcs);
    XGL_GC_UNWRAP (ops);
    (*pGC->funcs->ValidateGC) (pGC, changes, pDrawable);
    XGL_GC_WRAP (funcs, (GCFuncs *) &xglGCFuncs);
    XGL_GC_WRAP (ops, (GCOps *) &xglGCOps);

    if (pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
    {
	XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);
	
	if (pPixmapPriv->format)
	    pGCPriv->flags &= ~xglGCSoftwareDrawableFlag;
	else
	    pGCPriv->flags |= xglGCSoftwareDrawableFlag;

	if (pPixmapPriv->target)
	    pGCPriv->flags &= ~xglGCReadOnlyDrawableFlag;
	else
	    pGCPriv->flags |= xglGCReadOnlyDrawableFlag;
    }

    if (changes & GCFunction)
    {
	switch (pGC->alu) {
	case GXclear:
	    pGCPriv->op = GLITZ_OPERATOR_CLEAR;
	    pGCPriv->flags &= ~xglGCBadFunctionFlag;
	    break;
	case GXcopy:
	    pGCPriv->op = GLITZ_OPERATOR_SRC;
	    pGCPriv->flags &= ~xglGCBadFunctionFlag;
	    break;
	case GXnoop:
	    pGCPriv->op = GLITZ_OPERATOR_DST;
	    pGCPriv->flags &= ~xglGCBadFunctionFlag;
	    break;
	default:
	    pGCPriv->flags |= xglGCBadFunctionFlag;
	    break;
	}
    }

    if (changes & GCPlaneMask)
    {
	FbBits mask;

	mask = FbFullMask (pDrawable->depth);
	
	if ((pGC->planemask & mask) != mask)
	    pGCPriv->flags |= xglGCPlaneMaskFlag;
	else
	    pGCPriv->flags &= ~xglGCPlaneMaskFlag;
    }

    if (changes & (GCForeground | GCBackground))
    {
	XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

	if (pPixmapPriv->pPixel)
	{
	    xglPixelToColor (pPixmapPriv->pPixel, pGC->fgPixel, &pGCPriv->fg);
	    xglPixelToColor (pPixmapPriv->pPixel, pGC->bgPixel, &pGCPriv->bg);
	}
    }
}
