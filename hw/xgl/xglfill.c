/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
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
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"
#include "gcstruct.h"
#include "fb.h"

Bool
xglFill (DrawablePtr	pDrawable,
	 GCPtr		pGC,
	 xglGeometryPtr pGeometry,
	 int	  	x,
	 int		y,
	 int		width,
	 int		height,
	 BoxPtr		pBox,
	 int		nBox)
{
    XGL_GC_PRIV (pGC);

    switch (pGC->fillStyle) {
    case FillSolid:
	if (xglSolid (pDrawable,
		      pGCPriv->op, &pGCPriv->fg,
		      pGeometry,
		      x, y,
		      width, height,
		      pBox, nBox))
	    return TRUE;
	break;
    case FillStippled:
    case FillOpaqueStippled:
	break;
    case FillTiled:
	if (xglTile (pDrawable,
		     pGCPriv->op, pGC->tile.pixmap,
		     -(pGC->patOrg.x + pDrawable->x),
		     -(pGC->patOrg.y + pDrawable->y),
		     pGeometry,
		     x, y,
		     width, height,
		     pBox, nBox))
	    return TRUE;
	break;
    }

    return FALSE;
}

#define N_STACK_BOX 1024

static BoxPtr
xglMoreBoxes (BoxPtr stackBox,
	      BoxPtr heapBox,
	      int    nBoxes)
{
    Bool stack = !heapBox;
	
    heapBox = xrealloc (heapBox, sizeof (BoxRec) * nBoxes);
    if (!heapBox)
	return NULL;
    
    if (stack)
	memcpy (heapBox, stackBox, sizeof (BoxRec) * N_STACK_BOX);

    return heapBox;
}

#define ADD_BOX(pBox, nBox, stackBox, heapBox, size, box)	\
    {								\
	if ((nBox) == (size))					\
	{							\
	    (size) *= 2;					\
	    (heapBox) = xglMoreBoxes (stackBox, heapBox, size);	\
	    if (!(heapBox))					\
		return;						\
	    (pBox) = (heapBox) + (nBox);			\
	}							\
	*(pBox)++ = (box);					\
	(nBox)++;						\
    }

void
xglFillRect (DrawablePtr pDrawable, 
	     GCPtr	 pGC, 
	     int	 nrect,
	     xRectangle  *prect)
{
    RegionPtr pClip = pGC->pCompositeClip;
    BoxPtr    pClipBox;
    BoxPtr    pExtent = REGION_EXTENTS (pGC->pScreen, pClip);
    BoxRec    full, part;
    BoxPtr    heapBox = NULL;
    BoxRec    stackBox[N_STACK_BOX];
    int       size = N_STACK_BOX;
    BoxPtr    pBox = stackBox;
    int	      n, nBox = 0;

    while (nrect--)
    {
	full.x1 = prect->x + pDrawable->x;
	full.y1 = prect->y + pDrawable->y;
	full.x2 = full.x1 + (int) prect->width;
	full.y2 = full.y1 + (int) prect->height;
	
	prect++;
	
	if (full.x1 < pExtent->x1)
	    full.x1 = pExtent->x1;

	if (full.y1 < pExtent->y1)
	    full.y1 = pExtent->y1;
	
	if (full.x2 > pExtent->x2)
	    full.x2 = pExtent->x2;
	
	if (full.y2 > pExtent->y2)
	    full.y2 = pExtent->y2;
	
	if (full.x1 >= full.x2 || full.y1 >= full.y2)
	    continue;
	
	n = REGION_NUM_RECTS (pClip);

	if (n == 1)
	{
	    ADD_BOX (pBox, nBox, stackBox, heapBox, size, full);
	}
	else
	{
	    pClipBox = REGION_RECTS (pClip);

	    while (n--)
	    {
		part.x1 = pClipBox->x1;
		if (part.x1 < full.x1)
		    part.x1 = full.x1;
		
		part.y1 = pClipBox->y1;
		if (part.y1 < full.y1)
		    part.y1 = full.y1;
		
		part.x2 = pClipBox->x2;
		if (part.x2 > full.x2)
		    part.x2 = full.x2;
		
		part.y2 = pClipBox->y2;
		if (part.y2 > full.y2)
		    part.y2 = full.y2;
    
		pClipBox++;
		
		if (part.x1 < part.x2 && part.y1 < part.y2)
		    ADD_BOX (pBox, nBox, stackBox, heapBox, size, part);
	    }
	}
    }

    if (!nBox)
	return;
    
    pBox = (heapBox) ? heapBox : stackBox;

    if (!xglFill (pDrawable, pGC, NULL,
		  pExtent->x1, pExtent->y1,
		  pExtent->x2 - pExtent->x1, pExtent->y2 - pExtent->y1,
		  pBox, nBox))
    {
	RegionRec region;
	Bool      overlap;

	XGL_DRAWABLE_PIXMAP (pDrawable);

	if (!xglMapPixmapBits (pPixmap))
	    FatalError (XGL_SW_FAILURE_STRING);

	switch (pGC->fillStyle) {
	case FillSolid:
	    break;
	case FillStippled:
	case FillOpaqueStippled:
	    if (!xglSyncBits (&pGC->stipple->drawable, NullBox))
		FatalError (XGL_SW_FAILURE_STRING);
	    break;
	case FillTiled:
	    if (!xglSyncBits (&pGC->tile.pixmap->drawable, NullBox))
		FatalError (XGL_SW_FAILURE_STRING);
	    break;
	}
	
	REGION_INIT (pGC->pScreen, &region, pBox, nBox);
	
	while (nBox--)
	{
	    fbFill (pDrawable, pGC,
		    pBox->x1, pBox->y1,
		    pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	    pBox++;
	}

	/* hmm, overlap can't be good, don't know what to do about that */
	REGION_VALIDATE (pGC->pScreen, &region, &overlap);
	xglAddSurfaceDamage (pDrawable, &region);
	REGION_UNINIT (pGC->pScreen, &region);
    } else
	xglAddCurrentBitDamage (pDrawable);

    if (heapBox)
	xfree (heapBox);
}

Bool
xglFillSpan (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     int	 n,
	     DDXPointPtr ppt,
	     int	 *pwidth)
{
    BoxPtr	   pExtent;
    xglGeometryPtr pGeometry;

    pExtent = REGION_EXTENTS (pDrawable->pScreen, pGC->pCompositeClip);

    pGeometry = xglGetScratchVertexGeometry (pGC->pScreen, 2 * n);
    
    GEOMETRY_ADD_SPAN (pGC->pScreen, pGeometry, ppt, pwidth, n);

    /* Spans are treated as lines so they need a 0.5 translate */
    GEOMETRY_TRANSLATE_FIXED (pGeometry, 1 << 15, 1 << 15);
    GEOMETRY_SET_VERTEX_PRIMITIVE (pGeometry, GLITZ_PRIMITIVE_LINES);

    if (xglFill (pDrawable, pGC, pGeometry,
		 pExtent->x1, pExtent->y1,
		 pExtent->x2 - pExtent->x1, pExtent->y2 - pExtent->y1,
		 REGION_RECTS (pGC->pCompositeClip),
		 REGION_NUM_RECTS (pGC->pCompositeClip)))
	return TRUE;
    
    return FALSE;
}

Bool
xglFillLine (DrawablePtr pDrawable,
	     GCPtr       pGC,
	     int	 mode,
	     int	 npt,
	     DDXPointPtr ppt)
{
    BoxPtr	   pExtent;
    xglGeometryPtr pGeometry;
    Bool	   coincident_endpoints;

    pExtent = REGION_EXTENTS (pDrawable->pScreen, pGC->pCompositeClip);

    coincident_endpoints = FALSE;
    if (mode == CoordModePrevious)
    {
	DDXPointPtr pptTmp;
	int	    nptTmp;
	DDXPointRec pt;

	pt = *ppt;
	
	nptTmp = npt - 1;
	pptTmp = ppt + 1;

	while (nptTmp--)
	{
	    pt.x += pptTmp->x;
	    pt.y += pptTmp->y;
	    
	    pptTmp++;
	}
	
	if (pt.x == ppt->x && pt.y == ppt->y)
	    coincident_endpoints = TRUE;
    }
    else
    {
	if (ppt[npt - 1].x == ppt->x && ppt[npt - 1].y == ppt->y)
	    coincident_endpoints = TRUE;
    }

    if (coincident_endpoints)
	npt--;

    pGeometry = xglGetScratchVertexGeometry (pGC->pScreen, npt);
    
    GEOMETRY_ADD_LINE (pGC->pScreen, pGeometry,
		       coincident_endpoints, mode, npt, ppt);

    if (coincident_endpoints)
	GEOMETRY_SET_VERTEX_PRIMITIVE (pGeometry, GLITZ_PRIMITIVE_LINE_LOOP);
    else
	GEOMETRY_SET_VERTEX_PRIMITIVE (pGeometry, GLITZ_PRIMITIVE_LINE_STRIP);

    /* Lines need a 0.5 translate */
    GEOMETRY_TRANSLATE_FIXED (pGeometry, 1 << 15, 1 << 15);
    
    GEOMETRY_TRANSLATE (pGeometry, pDrawable->x, pDrawable->y);

    pExtent = REGION_EXTENTS (pDrawable->pScreen, pGC->pCompositeClip);
	
    if (xglFill (pDrawable, pGC, pGeometry,
		 pExtent->x1, pExtent->y1,
		 pExtent->x2 - pExtent->x1, pExtent->y2 - pExtent->y1,
		 REGION_RECTS (pGC->pCompositeClip),
		 REGION_NUM_RECTS (pGC->pCompositeClip)))
	return TRUE;
    
    return FALSE;
}

Bool
xglFillSegment (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    nsegInit,
		xSegment    *pSegInit)
{
    BoxPtr	   pExtent;
    xglGeometryPtr pGeometry;

    pExtent = REGION_EXTENTS (pDrawable->pScreen, pGC->pCompositeClip);

    pGeometry = xglGetScratchVertexGeometry (pGC->pScreen, 2 * nsegInit);
	
    GEOMETRY_ADD_SEGMENT (pGC->pScreen, pGeometry, nsegInit, pSegInit);

    /* Line segments need 0.5 translate */
    GEOMETRY_TRANSLATE_FIXED (pGeometry, 1 << 15, 1 << 15);
    GEOMETRY_SET_VERTEX_PRIMITIVE (pGeometry, GLITZ_PRIMITIVE_LINES);
    
    GEOMETRY_TRANSLATE (pGeometry, pDrawable->x, pDrawable->y);
	
    if (xglFill (pDrawable, pGC, pGeometry,
		 pExtent->x1, pExtent->y1,
		 pExtent->x2 - pExtent->x1, pExtent->y2 - pExtent->y1,
		 REGION_RECTS (pGC->pCompositeClip),
		 REGION_NUM_RECTS (pGC->pCompositeClip)))
	return TRUE;
    
    return FALSE;
}

Bool
xglFillGlyph (DrawablePtr  pDrawable,
	      GCPtr	   pGC,
	      int	   x,
	      int	   y,
	      unsigned int nGlyph,
	      CharInfoPtr  *ppci,
	      pointer      pglyphBase)
{
    BoxPtr	   pExtent;
    xglGeometryRec geometry;

    pExtent = REGION_EXTENTS (pDrawable->pScreen, pGC->pCompositeClip);

    x += pDrawable->x;
    y += pDrawable->y;

    GEOMETRY_INIT (pDrawable->pScreen, &geometry,
		   GLITZ_GEOMETRY_TYPE_BITMAP,
		   GEOMETRY_USAGE_SYSMEM, 0);

    GEOMETRY_FOR_GLYPH (pDrawable->pScreen,
			&geometry,
			nGlyph,
			ppci,
			pglyphBase);

    GEOMETRY_TRANSLATE (&geometry, x, y);

    if (xglFill (pDrawable, pGC, &geometry,
		 pExtent->x1, pExtent->y1,
		 pExtent->x2 - pExtent->x1, pExtent->y2 - pExtent->y1,
		 REGION_RECTS (pGC->pCompositeClip),
		 REGION_NUM_RECTS (pGC->pCompositeClip)))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}
