/*
 * $Id$
 *
 * Copyright © 2004 Keith Packard
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

#include "xgl.h"

static void
xglValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
    pGC->lastWinOrg.x = pDrawable->x;
    pGC->lastWinOrg.y = pDrawable->y;

    if ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode)) ||
	(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
	)
    {
	miComputeCompositeClip (pGC, pDrawable);
    }
}

static const GCFuncs	xglGCFuncs = {
    xglValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip,
};

static void
xglPolyLine (DrawablePtr	pDrawable,
	     GCPtr	pGC,
	     int		mode,
	     int		npt,
	     DDXPointPtr	ppt)
{
    void	(*line) (DrawablePtr, GCPtr, int mode, int npt, DDXPointPtr ppt);
    
    if (pGC->lineWidth == 0)
    {
	line = miZeroLine;
    }
    else
    {
	if (pGC->lineStyle != LineSolid)
	    line = miWideDash;
	else
	    line = miWideLine;
    }
    (*line) (pDrawable, pGC, mode, npt, ppt);
}

static const GCOps	xglGCOps = {
    xglFillSpans,
    xglSetSpans,
    miPutImage,
    miCopyArea,
    miCopyPlane,
    miPolyPoint,
    xglPolyLine,
    miPolySegment,
    miPolyRectangle,
    miPolyArc,
    miFillPolygon,
    miPolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    miImageGlyphBlt,
    miPolyGlyphBlt,
    xglPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

Bool
xglCreateGC (GCPtr pGC)
{
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
    
    pGC->funcs = (GCFuncs *) &xglGCFuncs;
    pGC->ops = (GCOps *) &xglGCOps;
    pGC->pCompositeClip = 0;
    pGC->fExpose = 1;
    pGC->freeCompClip = FALSE;
    pGC->pRotatedPixmap = 0;
    return TRUE;
}
