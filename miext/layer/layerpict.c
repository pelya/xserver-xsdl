/*
 * $XFree86: xc/programs/Xserver/miext/layer/layerpict.c,v 1.1 2001/05/29 04:54:13 keithp Exp $
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
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

#include "layerstr.h"

void
layerComposite (CARD8      op,
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
    LayerPtr		pLayer;
    LayerWinLoopRec	loop;
    DrawablePtr		pDstDrawable = pDst->pDrawable;
    ScreenPtr		pScreen = pDstDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen (pScreen);

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	WindowPtr   pWin = (WindowPtr) pDstDrawable;
	for (pLayer = LayerWindowFirst (pWin, &loop);
	     pLayer;
	     pLayer = LayerWindowNext (pWin, &loop))
	{
	    LayerUnwrap (ps, pLayer->pKind, Composite);
	    (*ps->Composite) (op, pSrc, pMask, pDst, xSrc, ySrc,
			      xMask, yMask, xDst, yDst, width, height);
	    LayerWrap (ps, pLayer->pKind, Composite, layerComposite);
	}
	LayerWindowDone (pWin, &loop);
    }
    else
    {
	layerScrPriv (pScreen);
	LayerUnwrap (ps, &pLayScr->kinds[LAYER_FB], Composite);
	(*ps->Composite) (op, pSrc, pMask, pDst, xSrc, ySrc,
			  xMask, yMask, xDst, yDst, width, height);
	LayerWrap (ps, &pLayScr->kinds[LAYER_FB], Composite, layerComposite);
    }
}

void
layerGlyphs (CARD8	    op,
	     PicturePtr	    pSrc,
	     PicturePtr	    pDst,
	     PictFormatPtr  maskFormat,
	     INT16	    xSrc,
	     INT16	    ySrc,
	     int	    nlist,
	     GlyphListPtr   list,
	     GlyphPtr	    *glyphs)
{
    LayerPtr		pLayer;
    LayerWinLoopRec	loop;
    DrawablePtr		pDstDrawable = pDst->pDrawable;
    ScreenPtr		pScreen = pDstDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen (pScreen);

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	WindowPtr   pWin = (WindowPtr) pDstDrawable;
	for (pLayer = LayerWindowFirst (pWin, &loop);
	     pLayer;
	     pLayer = LayerWindowNext (pWin, &loop))
	{
	    LayerUnwrap (ps, pLayer->pKind, Glyphs);
	    (*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc,
			   nlist, list, glyphs);
	    LayerWrap (ps, pLayer->pKind, Glyphs, layerGlyphs);
	}
	LayerWindowDone (pWin, &loop);
    }
    else
    {
	layerScrPriv (pScreen);
	LayerUnwrap (ps, &pLayScr->kinds[LAYER_FB], Glyphs);
	(*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc,
		       nlist, list, glyphs);
	LayerWrap (ps, &pLayScr->kinds[LAYER_FB], Glyphs, layerGlyphs);
    }
}

void
layerCompositeRects (CARD8	    op,
		     PicturePtr	    pDst,
		     xRenderColor   *color,
		     int	    nRect,
		     xRectangle	    *rects)
{
    LayerPtr		pLayer;
    LayerWinLoopRec	loop;
    DrawablePtr		pDstDrawable = pDst->pDrawable;
    ScreenPtr		pScreen = pDstDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen (pScreen);

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	WindowPtr   pWin = (WindowPtr) pDstDrawable;
	for (pLayer = LayerWindowFirst (pWin, &loop);
	     pLayer;
	     pLayer = LayerWindowNext (pWin, &loop))
	{
	    LayerUnwrap (ps, pLayer->pKind, CompositeRects);
	    (*ps->CompositeRects) (op, pDst, color, nRect, rects);
	    LayerWrap (ps, pLayer->pKind, CompositeRects, layerCompositeRects);
	}
	LayerWindowDone (pWin, &loop);
    }
    else
    {
	layerScrPriv (pScreen);
	LayerUnwrap (ps, &pLayScr->kinds[LAYER_FB], CompositeRects);
	(*ps->CompositeRects) (op, pDst, color, nRect, rects);
	LayerWrap (ps, &pLayScr->kinds[LAYER_FB], CompositeRects, layerCompositeRects);
    }
}
