/*
 * Copyright © 2005 Novell, Inc.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"
#include "gcstruct.h"
#include "damage.h"

#ifdef RENDER

#define XGL_TRAP_FALLBACK_PROLOGUE(pPicture, func) \
    xglSyncDamageBoxBits (pPicture->pDrawable);	   \
    XGL_PICTURE_SCREEN_UNWRAP (func)

#define XGL_TRAP_FALLBACK_EPILOGUE(pPicture, func, xglfunc) \
    XGL_PICTURE_SCREEN_WRAP (func, xglfunc);		    \
    xglAddCurrentSurfaceDamage (pPicture->pDrawable)

/* just a guess */
#define SMOOTH_TRAPS_ESTIMATE_RECTS(nTrap) (30 * nTrap)

static PicturePtr
xglCreateMaskPicture (ScreenPtr     pScreen, 
		      PicturePtr    pDst,
		      PictFormatPtr pPictFormat,
		      CARD16	    width,
		      CARD16	    height,
		      Bool	    accelerate)
{
    PixmapPtr	 pPixmap;
    PicturePtr	 pPicture;
    GCPtr	 pGC;
    int		 error;
    xRectangle	 rect;

    if (width > 32767 || height > 32767)
	return 0;

    pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height, 
					pPictFormat->depth);
    if (!pPixmap)
	return 0;

    if (!accelerate)
    {
	XGL_PIXMAP_PRIV (pPixmap);
	
	if (!xglAllocatePixmapBits (pPixmap, XGL_PIXMAP_USAGE_HINT_DEFAULT))
	{
	    (*pScreen->DestroyPixmap) (pPixmap);
	    return 0;
	}

	pPixmapPriv->target = xglPixmapTargetNo;
    }
    
    pGC = GetScratchGC (pPixmap->drawable.depth, pScreen);
    if (!pGC)
    {
	(*pScreen->DestroyPixmap) (pPixmap);
	return 0;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;
    
    ValidateGC (&pPixmap->drawable, pGC);
    (*pGC->ops->PolyFillRect) (&pPixmap->drawable, pGC, 1, &rect);
    FreeScratchGC (pGC);

    pPicture = CreatePicture (0, &pPixmap->drawable, pPictFormat,
			      0, 0, serverClient, &error);
    (*pScreen->DestroyPixmap) (pPixmap);
    
    return pPicture;
}

#define LINE_FIXED_X(l, _y, v)			 \
    dx = (l)->p2.x - (l)->p1.x;			 \
    ex = (xFixed_32_32) ((_y) - (l)->p1.y) * dx; \
    dy = (l)->p2.y - (l)->p1.y;			 \
    (v) = (l)->p1.x + (xFixed) (ex / dy)

#define LINE_FIXED_X_CEIL(l, _y, v)		      \
    dx = (l)->p2.x - (l)->p1.x;			      \
    ex = (xFixed_32_32) ((_y) - (l)->p1.y) * dx;      \
    dy = (l)->p2.y - (l)->p1.y;			      \
    (v) = (l)->p1.x + (xFixed) ((ex + (dy - 1)) / dy)

static Bool
xglTrapezoidBounds (int	       ntrap,
		    xTrapezoid *traps,
		    BoxPtr     box)
{
    Bool	 x_overlap, overlap = FALSE;
    xFixed	 dx, dy, top, bottom;
    xFixed_32_32 ex;

    if (!ntrap)
    {
	box->x1 = MAXSHORT;
	box->x2 = MINSHORT;
	box->y1 = MAXSHORT;
	box->y2 = MINSHORT;

	return FALSE;
    }

    box->y1 = xFixedToInt (traps->top);
    box->y2 = xFixedToInt (xFixedCeil (traps->bottom));
    
    LINE_FIXED_X (&traps->left, traps->top, top);
    LINE_FIXED_X (&traps->left, traps->bottom, bottom);
    box->x1 = xFixedToInt (MIN (top, bottom));
    
    LINE_FIXED_X_CEIL (&traps->right, traps->top, top);
    LINE_FIXED_X_CEIL (&traps->right, traps->bottom, bottom);
    box->x2 = xFixedToInt (xFixedCeil (MAX (top, bottom)));
    
    ntrap--;
    traps++;

    for (; ntrap; ntrap--, traps++)
    {
	INT16 x1, y1, x2, y2;

	if (!xTrapezoidValid (traps))
	    continue;

	y1 = xFixedToInt (traps->top);
	y2 = xFixedToInt (xFixedCeil (traps->bottom));
	
	LINE_FIXED_X (&traps->left, traps->top, top);
	LINE_FIXED_X (&traps->left, traps->bottom, bottom);
	x1 = xFixedToInt (MIN (top, bottom));
	
	LINE_FIXED_X_CEIL (&traps->right, traps->top, top);
	LINE_FIXED_X_CEIL (&traps->right, traps->bottom, bottom);
	x2 = xFixedToInt (xFixedCeil (MAX (top, bottom)));
	
	x_overlap = FALSE;
	if (x1 >= box->x2)
	    box->x2 = x2;
	else if (x2 <= box->x1)
	    box->x1 = x1;
	else
	{
	    x_overlap = TRUE;
	    if (x1 < box->x1)
		box->x1 = x1;
	    if (x2 > box->x2)
		box->x2 = x2;
	}
	
	if (y1 >= box->y2)
	    box->y2 = y2;
	else if (y2 <= box->y1)
	    box->y1 = y1;	    
	else
	{
	    if (y1 < box->y1)
		box->y1 = y1;
	    if (y2 > box->y2)
		box->y2 = y2;
	    
	    if (x_overlap)
		overlap = TRUE;
	}
    }

    return overlap;
}

void
xglTrapezoids (CARD8	     op,
	       PicturePtr    pSrc,
	       PicturePtr    pDst,
	       PictFormatPtr maskFormat,
	       INT16	     xSrc,
	       INT16	     ySrc,
	       int	     nTrap,
	       xTrapezoid    *traps)
{
    ScreenPtr	    pScreen = pDst->pDrawable->pScreen;
    PicturePtr	    pMask = NULL, pSrcPicture, pDstPicture;
    xglGeometryPtr  pGeometry = NULL;
    glitz_surface_t *mask = NULL;
    unsigned int    polyEdge = pDst->polyEdge;
    INT16	    xDst, yDst;
    INT16	    xOff, yOff;
    BoxRec	    bounds;
    Bool	    overlap;
    
    XGL_SCREEN_PRIV (pScreen);

    xDst = traps[0].left.p1.x >> 16;
    yDst = traps[0].left.p1.y >> 16;
    
    overlap = xglTrapezoidBounds (nTrap, traps, &bounds);
    if (bounds.y1 >= bounds.y2 || bounds.x1 >= bounds.x2)
	return;

    if (nTrap > 1 && op != PictOpAdd && maskFormat &&
	(overlap || op != PictOpOver))
    {
	xglPixmapPtr pPixmapPriv;
	int	     width, height;
	Bool	     accelerate;
	
	if (!pScreenPriv->pSolidAlpha)
	{
	    xglCreateSolidAlphaPicture (pScreen);
	    if (!pScreenPriv->pSolidAlpha)
		return;
	}

	accelerate = TRUE;
	width  = bounds.x2 - bounds.x1;
	height = bounds.y2 - bounds.y1;
	if (maskFormat->depth > 1)
	{
	    /* Avoid acceleration if the estimated amount of vertex data
	       is likely to exceed the size of the mask. */
	    if ((SMOOTH_TRAPS_ESTIMATE_RECTS (nTrap) * 4) > (width * height))
		accelerate = FALSE;
	} else
	    accelerate = FALSE;

	pMask = xglCreateMaskPicture (pScreen, pDst, maskFormat,
				      width, height, accelerate);
	if (!pMask)
	    return;

	ValidatePicture (pMask);

	/* all will be damaged */
	pPixmapPriv = XGL_GET_DRAWABLE_PIXMAP_PRIV (pMask->pDrawable);
	pPixmapPriv->damageBox.x1 = 0;
	pPixmapPriv->damageBox.y1 = 0;
	pPixmapPriv->damageBox.x2 = pMask->pDrawable->width;
	pPixmapPriv->damageBox.y2 = pMask->pDrawable->height;

	xOff = -bounds.x1;
	yOff = -bounds.y1;
	pSrcPicture = pScreenPriv->pSolidAlpha;
	pDstPicture = pMask;
    }
    else
    {
	if (maskFormat)
	{
	    if (maskFormat->depth == 1)
		polyEdge = PolyEdgeSharp;
	    else
		polyEdge = PolyEdgeSmooth;
	}
	
	xOff = 0;
	yOff = 0;
	pSrcPicture = pSrc;
	pDstPicture = pDst;
    }

    if (xglPrepareTarget (pDstPicture->pDrawable))
    {
	if (maskFormat || polyEdge == PolyEdgeSmooth)
	{
	    glitz_vertex_format_t *format;
	    xTrapezoid		  *pTrap = traps;
	    int			  nAddedTrap, n = nTrap;
	    int			  offset = 0;
	    int			  size = SMOOTH_TRAPS_ESTIMATE_RECTS (n);

	    mask = pScreenPriv->trapInfo.mask;
	    format = &pScreenPriv->trapInfo.format.vertex;
	    
	    size *= format->bytes_per_vertex;
	    pGeometry = xglGetScratchGeometryWithSize (pScreen, size);
	    
	    while (n)
	    {
		if (pGeometry->size < size)
		    GEOMETRY_RESIZE (pScreen, pGeometry, size);
		
		if (!pGeometry->buffer)
		{
		    if (pMask)
			FreePicture (pMask, 0);
		    
		    return;
		}
		
		offset +=
		    glitz_add_trapezoids (pGeometry->buffer,
					  offset, size - offset, format->type,
					  mask, (glitz_trapezoid_t *) pTrap, n,
					  &nAddedTrap);
		
		n     -= nAddedTrap;
		pTrap += nAddedTrap;
		size  *= 2;
	    }

	    pGeometry->f     = pScreenPriv->trapInfo.format;
	    pGeometry->count = offset / format->bytes_per_vertex;
	}
	else
	{
	    pGeometry =
		xglGetScratchVertexGeometryWithType (pScreen,
						     GEOMETRY_DATA_TYPE_FLOAT,
						     4 * nTrap);
	    if (!pGeometry->buffer)
	    {
		if (pMask)
		    FreePicture (pMask, 0);
		
		return;
	    }
	    
	    GEOMETRY_ADD_TRAPEZOID (pScreen, pGeometry, traps, nTrap);
	}

	GEOMETRY_TRANSLATE (pGeometry,
			    pDstPicture->pDrawable->x + xOff,
			    pDstPicture->pDrawable->y + yOff);
    }

    if (pGeometry &&
	xglComp (pMask ? PictOpAdd : op,
		 pSrcPicture,
		 NULL,
		 pDstPicture,
		 bounds.x1 + xOff + xSrc - xDst,
		 bounds.y1 + yOff + ySrc - yDst,
		 0, 0,
		 pDstPicture->pDrawable->x + bounds.x1 + xOff,
		 pDstPicture->pDrawable->y + bounds.y1 + yOff,
		 bounds.x2 - bounds.x1,
		 bounds.y2 - bounds.y1,
		 pGeometry,
		 mask))
    {
	/* no intermediate mask? we need to register damage from here as
	   CompositePicture will never be called. */
	if (!pMask)
	{
	    RegionRec region;

	    REGION_INIT (pScreen, &region, &bounds, 1);
	    REGION_TRANSLATE (pScreen, &region,
			      pDst->pDrawable->x, pDst->pDrawable->y);
	    
	    DamageDamageRegion (pDst->pDrawable, &region);

	    REGION_UNINIT (pScreen, &region);
	}
	
	xglAddCurrentBitDamage (pDstPicture->pDrawable);
    }
    else
    {
	XGL_DRAWABLE_PIXMAP_PRIV (pDstPicture->pDrawable);

	pPixmapPriv->damageBox.x1 = bounds.x1 + xOff;
	pPixmapPriv->damageBox.y1 = bounds.y1 + yOff;
	pPixmapPriv->damageBox.x2 = bounds.x2 + xOff;
	pPixmapPriv->damageBox.y2 = bounds.y2 + yOff;

	xglSyncDamageBoxBits (pDstPicture->pDrawable);

	if (pMask || (polyEdge == PolyEdgeSmooth &&
		      op == PictOpAdd && miIsSolidAlpha (pSrc)))
	{
	    PictureScreenPtr ps = GetPictureScreen (pScreen);
	    
	    for (; nTrap; nTrap--, traps++)
		(*ps->RasterizeTrapezoid) (pDstPicture, traps, xOff, yOff);

	    xglAddCurrentSurfaceDamage (pDstPicture->pDrawable);
	} else
	    miTrapezoids (op, pSrc, pDstPicture, NULL,
			  xSrc, ySrc, nTrap, traps);
    }
    
    if (pMask)
    {
	xglLeaveOffscreenArea ((PixmapPtr) pMask->pDrawable);
	
	CompositePicture (op, pSrc, pMask, pDst,
			  bounds.x1 + xSrc - xDst,
			  bounds.y1 + ySrc - yDst,
			  0, 0,
			  bounds.x1, bounds.y1,
			  bounds.x2 - bounds.x1,
			  bounds.y2 - bounds.y1);
	
	FreePicture (pMask, 0);
    }
}

void
xglAddTraps (PicturePtr pDst,
	     INT16	xOff,
	     INT16	yOff,
	     int	nTrap,
	     xTrap	*traps)
{
    PictureScreenPtr pPictureScreen; 
    ScreenPtr	     pScreen = pDst->pDrawable->pScreen;
    
    XGL_SCREEN_PRIV (pScreen);
    XGL_DRAWABLE_PIXMAP_PRIV (pDst->pDrawable);

    if (!pScreenPriv->pSolidAlpha)
    {
	xglCreateSolidAlphaPicture (pScreen);
	if (!pScreenPriv->pSolidAlpha)
	    return;
    }

    pPixmapPriv->damageBox.x1 = 0;
    pPixmapPriv->damageBox.y1 = 0;
    pPixmapPriv->damageBox.x2 = pDst->pDrawable->width;
    pPixmapPriv->damageBox.y2 = pDst->pDrawable->height;

    if (xglPrepareTarget (pDst->pDrawable))
    {
	glitz_vertex_format_t *format;
	xglGeometryPtr	      pGeometry;
	xTrap		      *pTrap = traps;
	int		      nAddedTrap, n = nTrap;
	int		      offset = 0;
	int		      size = SMOOTH_TRAPS_ESTIMATE_RECTS (n);

	format = &pScreenPriv->trapInfo.format.vertex;

	size *= format->bytes_per_vertex;
	pGeometry = xglGetScratchGeometryWithSize (pScreen, size);
	    
	while (n)
	{
	    if (pGeometry->size < size)
		GEOMETRY_RESIZE (pScreen, pGeometry, size);
	    
	    if (!pGeometry->buffer)
		return;
	    
	    offset +=
		glitz_add_traps (pGeometry->buffer,
				 offset, size - offset, format->type,
				 pScreenPriv->trapInfo.mask,
				 (glitz_trap_t *) pTrap, n,
				 &nAddedTrap);
		
	    n     -= nAddedTrap;
	    pTrap += nAddedTrap;
	    size  *= 2;
	}

	pGeometry->f     = pScreenPriv->trapInfo.format;
	pGeometry->count = offset / format->bytes_per_vertex;

	GEOMETRY_TRANSLATE (pGeometry,
			    pDst->pDrawable->x + xOff,
			    pDst->pDrawable->y + yOff);
	
	if (xglComp (PictOpAdd,
		     pScreenPriv->pSolidAlpha,
		     NULL,
		     pDst,
		     0, 0,
		     0, 0,
		     pDst->pDrawable->x, pDst->pDrawable->y,
		     pDst->pDrawable->width, pDst->pDrawable->height,
		     pGeometry,
		     pScreenPriv->trapInfo.mask))
	{
	    xglAddCurrentBitDamage (pDst->pDrawable);
	    return;
	}
    }

    pPictureScreen = GetPictureScreen (pScreen);

    XGL_TRAP_FALLBACK_PROLOGUE (pDst, AddTraps);
    (*pPictureScreen->AddTraps) (pDst, xOff, yOff, nTrap, traps);
    XGL_TRAP_FALLBACK_EPILOGUE (pDst, AddTraps, xglAddTraps);
}

#endif
