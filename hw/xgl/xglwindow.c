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

#define XGL_WINDOW_FALLBACK_PROLOGUE(pWin, func) \
    xglSyncDamageBoxBits (&pWin->drawable);	 \
    XGL_SCREEN_UNWRAP (func)

#define XGL_WINDOW_FALLBACK_EPILOGUE(pWin, func, xglfunc) \
    XGL_SCREEN_WRAP (func, xglfunc);			  \
    xglAddSurfaceDamage (&pWin->drawable)

Bool
xglCreateWindow (WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    Bool      ret;
    
    XGL_SCREEN_PRIV (pScreen);
    XGL_WINDOW_PRIV (pWin);

    XGL_SCREEN_UNWRAP (CreateWindow);
    ret = (*pScreen->CreateWindow) (pWin);
    XGL_SCREEN_WRAP (CreateWindow, xglCreateWindow);

    pWinPriv->pPixmap = pWin->drawable.pScreen->devPrivate;

    return ret;
}

void 
xglCopyWindow (WindowPtr   pWin, 
	       DDXPointRec ptOldOrg,
	       RegionPtr   prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    XGL_SCREEN_PRIV (pScreen);

    if (XGL_GET_DRAWABLE_PIXMAP_PRIV (&pWin->drawable)->target)
    {
	PixmapPtr pPixmap;
	RegionRec rgnDst;
	int	  dx, dy;
	Bool	  ret;

	pPixmap = XGL_GET_WINDOW_PIXMAP (pWin);
	
	dx = ptOldOrg.x - pWin->drawable.x;
	dy = ptOldOrg.y - pWin->drawable.y;
    
	REGION_TRANSLATE (pScreen, prgnSrc, -dx, -dy);
	REGION_INIT (pScreen, &rgnDst, NullBox, 0);
	REGION_INTERSECT (pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

#ifdef COMPOSITE
	if (pPixmap->screen_x || pPixmap->screen_y)
	    REGION_TRANSLATE (pWin->drawable.pScreen, &rgnDst, 
			      -pPixmap->screen_x, -pPixmap->screen_y);
#endif

	ret = TRUE;
	fbCopyRegion (&pWin->drawable, &pWin->drawable,
		      0, &rgnDst, dx, dy, xglCopyProc, 0, (void *) &ret);
	
	REGION_UNINIT (pScreen, &rgnDst);
	
	if (ret)
	{
	    xglAddBitDamage (&pWin->drawable);
	    return;
	}

#ifdef COMPOSITE
	if (pPixmap->screen_x || pPixmap->screen_y)
	    REGION_TRANSLATE (pWin->drawable.pScreen, &rgnDst, 
			      pPixmap->screen_x, pPixmap->screen_y);
#endif
	
	REGION_TRANSLATE (pScreen, prgnSrc, dx, dy);
    }

    if (!xglSyncBits (&pWin->drawable, NullBox))
	FatalError (XGL_SW_FAILURE_STRING);

    XGL_WINDOW_FALLBACK_PROLOGUE (pWin, CopyWindow);
    (*pScreen->CopyWindow) (pWin, ptOldOrg, prgnSrc);
    XGL_WINDOW_FALLBACK_EPILOGUE (pWin, CopyWindow, xglCopyWindow);
}

static Bool
xglFillRegionSolid (DrawablePtr	     pDrawable,
		    RegionPtr	     pRegion,
		    Pixel	     pixel)
{
    ScreenPtr	   pScreen = pDrawable->pScreen;
    xglGeometryRec geometry;
    glitz_color_t  color;

    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

    if (!pPixmapPriv->target)
	return FALSE;

    xglPixelToColor (pPixmapPriv->pPixel, pixel, &color);

    GEOMETRY_INIT (pScreen, &geometry, REGION_NUM_RECTS (pRegion) << 3);
    GEOMETRY_ADD_REGION (pScreen, &geometry, pRegion);
    
    if (xglSolid (pDrawable,
		  GLITZ_OPERATOR_SRC,
		  &color,
		  &geometry,
		  REGION_EXTENTS (pScreen, pRegion), 1))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}

static Bool
xglFillRegionTiled (DrawablePtr	pDrawable,
		    RegionPtr	pRegion,
		    PixmapPtr	pTile,
		    int		tileX,
		    int		tileY)
{
    ScreenPtr	   pScreen = pDrawable->pScreen;
    xglGeometryRec geometry;

    if (!XGL_GET_DRAWABLE_PIXMAP_PRIV (pDrawable)->target)
	return FALSE;

    GEOMETRY_INIT (pScreen, &geometry, REGION_NUM_RECTS (pRegion) << 3);
    GEOMETRY_ADD_REGION (pScreen, &geometry, pRegion);

    if (xglTile (pDrawable,
		 GLITZ_OPERATOR_SRC,
		 pTile,
		 tileX, tileY,
		 &geometry,
		 REGION_EXTENTS (pScreen, pRegion), 1))
    {
	GEOMETRY_UNINIT (&geometry);
	return TRUE;
    }
    
    GEOMETRY_UNINIT (&geometry);
    return FALSE;
}

void
xglPaintWindowBackground (WindowPtr pWin,
			  RegionPtr pRegion,
			  int	    what)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    XGL_SCREEN_PRIV (pScreen);
    
    switch (pWin->backgroundState) {
    case None:
	return;
    case ParentRelative:
	do {
	    pWin = pWin->parent;
	} while (pWin->backgroundState == ParentRelative);
	
	(*pScreen->PaintWindowBackground) (pWin, pRegion, what);
	return;
    case BackgroundPixmap:
	if (xglFillRegionTiled (&pWin->drawable,
				pRegion,
				pWin->background.pixmap,
				-pWin->drawable.x,
				-pWin->drawable.y))
	{
	    xglAddBitDamage (&pWin->drawable);
	    return;
	}
	
	if (!xglSyncBits (&pWin->background.pixmap->drawable, NullBox))
	    FatalError (XGL_SW_FAILURE_STRING);
	break;
    case BackgroundPixel:
	if (xglFillRegionSolid (&pWin->drawable,
				pRegion,
				pWin->background.pixel))
	{
	    xglAddBitDamage (&pWin->drawable);
	    return;
	}
	break;
    }

    XGL_WINDOW_FALLBACK_PROLOGUE (pWin, PaintWindowBackground);
    (*pScreen->PaintWindowBackground) (pWin, pRegion, what);
    XGL_WINDOW_FALLBACK_EPILOGUE (pWin, PaintWindowBackground,
				  xglPaintWindowBackground);
}

void
xglPaintWindowBorder (WindowPtr pWin,
		      RegionPtr pRegion,
		      int	what)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    XGL_SCREEN_PRIV (pScreen);

    if (pWin->borderIsPixel)
    {
	if (xglFillRegionSolid (&pWin->drawable,
				pRegion,
				pWin->border.pixel))
	{
	    xglAddBitDamage (&pWin->drawable);
	    return;
	}
    }
    else
    {
	WindowPtr pBgWin = pWin;
	
	while (pBgWin->backgroundState == ParentRelative)
	    pBgWin = pBgWin->parent;

	if (xglFillRegionTiled (&pBgWin->drawable,
				pRegion,
				pWin->border.pixmap,
				-pBgWin->drawable.x,
				-pBgWin->drawable.y))
	{
	    xglAddBitDamage (&pWin->drawable);
	    return;
	}
	
	if (!xglSyncBits (&pWin->border.pixmap->drawable, NullBox))
	    FatalError (XGL_SW_FAILURE_STRING);
    }

    XGL_WINDOW_FALLBACK_PROLOGUE (pWin, PaintWindowBorder);
    (*pScreen->PaintWindowBorder) (pWin, pRegion, what);
    XGL_WINDOW_FALLBACK_EPILOGUE (pWin, PaintWindowBorder,
				  xglPaintWindowBorder);
}
