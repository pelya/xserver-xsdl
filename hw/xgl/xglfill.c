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
#include "gcstruct.h"

Bool
xglFill (DrawablePtr	pDrawable,
	 GCPtr		pGC,
	 xglGeometryPtr pGeometry)
{
    XGL_GC_PRIV (pGC);

    switch (pGC->fillStyle) {
    case FillSolid:
	if (xglSolid (pDrawable, pGCPriv->op, &pGCPriv->fg,
		      pGeometry,
		      REGION_RECTS (pGC->pCompositeClip),
		      REGION_NUM_RECTS (pGC->pCompositeClip)))
	    return TRUE;
	break;
    case FillStippled:
    case FillOpaqueStippled:
	break;
    case FillTiled:
	if (xglTile (pDrawable, pGCPriv->op, pGC->tile.pixmap,
		     -pGC->patOrg.x, -pGC->patOrg.y,
		     pGeometry,
		     REGION_RECTS (pGC->pCompositeClip),
		     REGION_NUM_RECTS (pGC->pCompositeClip)))
	    return TRUE;
	break;
    }

    return FALSE;
}

Bool
xglFillRect (DrawablePtr pDrawable, 
	     GCPtr	 pGC, 
	     int	 nrect,
	     xRectangle  *prect)
{
    xglGeometryRec geometry;

    GEOMETRY_INIT (pGC->pScreen, &geometry, nrect << 3);
    GEOMETRY_ADD_RECT (pGC->pScreen, &geometry, prect, nrect);
    GEOMETRY_TRANSLATE (&geometry, pDrawable->x, pDrawable->y);
	
    if (xglFill (pDrawable, pGC, &geometry))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}

Bool
xglFillSpan (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     int	 n,
	     DDXPointPtr ppt,
	     int	 *pwidth)
{
    xglGeometryRec geometry;

    GEOMETRY_INIT (pGC->pScreen, &geometry, n << 2);
    GEOMETRY_ADD_SPAN (pGC->pScreen, &geometry, ppt, pwidth, n);

    /* Spans are treated as lines so they need a 0.5 translate */
    GEOMETRY_TRANSLATE_FIXED (&geometry, 1 << 15, 1 << 15);
    GEOMETRY_SET_PRIMITIVE (pGC->pScreen, &geometry,
			    GLITZ_GEOMETRY_PRIMITIVE_LINES);

    if (xglFill (pDrawable, pGC, &geometry))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}

Bool
xglFillLine (DrawablePtr pDrawable,
	     GCPtr       pGC,
	     int	 mode,
	     int	 npt,
	     DDXPointPtr ppt)
{
    xglGeometryRec geometry;
    Bool	   coincident_endpoints;

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

    GEOMETRY_INIT (pGC->pScreen, &geometry, npt << 1);
    GEOMETRY_ADD_LINE (pGC->pScreen, &geometry,
		       coincident_endpoints, mode, npt, ppt);

    if (coincident_endpoints)
	GEOMETRY_SET_PRIMITIVE (pGC->pScreen, &geometry,
				GLITZ_GEOMETRY_PRIMITIVE_LINE_LOOP);
    else
	GEOMETRY_SET_PRIMITIVE (pGC->pScreen, &geometry,
				GLITZ_GEOMETRY_PRIMITIVE_LINE_STRIP);

    /* Lines need a 0.5 translate */
    GEOMETRY_TRANSLATE_FIXED (&geometry, 1 << 15, 1 << 15);
    
    GEOMETRY_TRANSLATE (&geometry, pDrawable->x, pDrawable->y);
	
    if (xglFill (pDrawable, pGC, &geometry))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}

Bool
xglFillSegment (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    nsegInit,
		xSegment    *pSegInit)
{
    xglGeometryRec geometry;

    GEOMETRY_INIT (pGC->pScreen, &geometry, nsegInit << 2);
    GEOMETRY_ADD_SEGMENT (pGC->pScreen, &geometry, nsegInit, pSegInit);

    /* Line segments need 0.5 translate */
    GEOMETRY_TRANSLATE_FIXED (&geometry, 1 << 15, 1 << 15);
    GEOMETRY_SET_PRIMITIVE (pGC->pScreen, &geometry,
			    GLITZ_GEOMETRY_PRIMITIVE_LINES);
    
    GEOMETRY_TRANSLATE (&geometry, pDrawable->x, pDrawable->y);
	
    if (xglFill (pDrawable, pGC, &geometry))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}
