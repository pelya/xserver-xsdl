/*
 * $XFree86: xc/programs/Xserver/miext/layer/layerwin.c,v 1.7 2002/11/08 22:19:42 keithp Exp $
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

static LayerListPtr
NewLayerList (ScreenPtr pScreen, LayerPtr pLayer)
{
    LayerListPtr    pLayList;

    pLayList = (LayerListPtr) xalloc (sizeof (LayerListRec));
    if (!pLayList)
	return 0;
    pLayList->pNext = 0;
    pLayList->pLayer = pLayer;
    pLayList->inheritClip = TRUE;
    REGION_INIT (pScreen, &pLayList->clipList, NullBox, 0);
    REGION_INIT (pScreen, &pLayList->borderClip, NullBox, 0);
    return pLayList;
}

static void
FreeLayerList (ScreenPtr pScreen, LayerListPtr pLayList)
{
    REGION_UNINIT (&pScreen, &pLayList->clipList);
    REGION_UNINIT (&pScreen, &pLayList->borderClip);
    xfree (pLayList);
}

/*
 * Create pixmap for a layer
 */

Bool
LayerCreatePixmap (ScreenPtr pScreen, LayerPtr pLayer)
{
    LayerKindPtr    pKind = pLayer->pKind;
    
    LayerUnwrap (pScreen, pKind, CreatePixmap);
    /* XXX create full-screen sized layers all around */
    pLayer->pPixmap = (*pScreen->CreatePixmap) (pScreen, pScreen->width, 
						pScreen->height, pLayer->depth);
    LayerWrap (pScreen, pKind, CreatePixmap, layerCreatePixmap);
    if (!pLayer->pPixmap)
	return FALSE;
    if (pLayer->pKind->kind == LAYER_SHADOW)
    {
	if (!shadowAdd (pScreen, pLayer->pPixmap, pLayer->update, 
			pLayer->window, pLayer->randr, 
			pLayer->closure))
	    return FALSE;
    }
    return TRUE;
}

/*
 * Destroy pixmap for a layer
 */

void
LayerDestroyPixmap (ScreenPtr pScreen, LayerPtr pLayer)
{
    if (pLayer->pPixmap)
    {
	if (pLayer->pKind->kind == LAYER_SHADOW)
	    shadowRemove (pScreen, pLayer->pPixmap);
	if (pLayer->freePixmap)
	{
	    LayerKindPtr    pKind = pLayer->pKind;

	    LayerUnwrap (pScreen, pKind, DestroyPixmap);
	    (*pScreen->DestroyPixmap) (pLayer->pPixmap);
	    LayerWrap (pScreen, pKind, DestroyPixmap, layerDestroyPixmap);
	}
	pLayer->pPixmap = 0;
    }
}
    
/*
 * Add a window to a layer
 */
Bool
LayerWindowAdd (ScreenPtr pScreen, LayerPtr pLayer, WindowPtr pWin)
{
    layerWinPriv(pWin);

    if (pLayer->pPixmap == LAYER_SCREEN_PIXMAP)
	pLayer->pPixmap = (*pScreen->GetScreenPixmap) (pScreen);
    else if (!pLayer->pPixmap && !LayerCreatePixmap (pScreen, pLayer))
	return FALSE;
    /*
     * Add a new layer list if needed
     */
    if (pLayWin->isList || pLayWin->u.pLayer)
    {
	LayerListPtr	pPrev;
	LayerListPtr    pLayList;
	
	if (!pLayWin->isList)
	{
	    pPrev = NewLayerList (pScreen, pLayWin->u.pLayer);
	    if (!pPrev)
		return FALSE;
	}
	else
	{
	    for (pPrev = pLayWin->u.pLayList; pPrev->pNext; pPrev = pPrev->pNext)
		;
	}
	pLayList = NewLayerList (pScreen, pLayer);
	if (!pLayList)
	{
	    if (!pLayWin->isList)
		FreeLayerList (pScreen, pPrev);
	    return FALSE;
	}
	pPrev->pNext = pLayList;
	if (!pLayWin->isList)
	{
	    pLayWin->isList = TRUE;
	    pLayWin->u.pLayList = pPrev;
	}
    }
    else
	pLayWin->u.pLayer = pLayer;
    /*
     * XXX only one layer supported for drawing, last one wins
     */
    (*pScreen->SetWindowPixmap) (pWin, pLayer->pPixmap);
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pLayer->refcnt++;
    pLayer->windows++;
    return TRUE;
}

/*
 * Remove a window from a layer
 */

void
LayerWindowRemove (ScreenPtr pScreen, LayerPtr pLayer, WindowPtr pWin)
{
    layerWinPriv(pWin);

    if (pLayWin->isList)
    {
	LayerListPtr	*pPrev;
	LayerListPtr	pLayList;

	for (pPrev = &pLayWin->u.pLayList; (pLayList = *pPrev); pPrev = &pLayList->pNext)
	{
	    if (pLayList->pLayer == pLayer)
	    {
		*pPrev = pLayList->pNext;
		FreeLayerList (pScreen, pLayList);
		--pLayer->windows;
		if (pLayer->windows <= 0)
		    LayerDestroyPixmap (pScreen, pLayer);
		LayerDestroy (pScreen, pLayer);
		break;
	    }
	}
	pLayList = pLayWin->u.pLayList;
	if (!pLayList)
	{
	    /*
	     * List is empty, set isList back to false
	     */
	    pLayWin->isList = FALSE;
	    pLayWin->u.pLayer = 0;
	}
	else if (!pLayList->pNext && pLayList->inheritClip)
	{
	    /*
	     * List contains a single element using the
	     * window clip, free the list structure and
	     * host the layer back to the window private
	     */
	    pLayer = pLayList->pLayer;
	    FreeLayerList (pScreen, pLayList);
	    pLayWin->isList = FALSE;
	    pLayWin->u.pLayer = pLayer;
	}
    }
    else
    {
	if (pLayWin->u.pLayer == pLayer)
	{
	    --pLayer->windows;
	    if (pLayer->windows <= 0)
		LayerDestroyPixmap (pScreen, pLayer);
	    LayerDestroy (pScreen, pLayer);
	    pLayWin->u.pLayer = 0;
	}
    }
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
}

/*
 * Looping primitives for window layering.  Usage:
 *
 *  for (pLayer = LayerWindowFirst (pWin, &loop); 
 *	 pLayer; 
 *	 pLayer = LayerWindowNext (&loop))
 *  {
 *	...
 *  }
 *  LayerWindowDone (pWin, &loop);
 */

LayerPtr
LayerWindowFirst (WindowPtr pWin, LayerWinLoopPtr pLoop)
{
    layerWinPriv (pWin);

    pLoop->pLayWin = pLayWin;
    if (!pLayWin->isList)
	return pLayWin->u.pLayer;

    /*
     * Preserve original state
     */
    pLoop->clipList = pWin->clipList;
    pLoop->borderClip = pWin->borderClip;
    pLoop->pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);

    /*
     * Set initial list element 
     */
    pLoop->pLayList = pLayWin->u.pLayList;

    /*
     * Return first layer
     */
    return LayerWindowNext (pWin, pLoop);
}

LayerPtr
LayerWindowNext (WindowPtr pWin, LayerWinLoopPtr pLoop)
{
    LayerPtr	    pLayer;
    LayerWinPtr	    pLayWin = pLoop->pLayWin;
    LayerListPtr    pLayList;

    if (!pLayWin->isList)
	return 0;
    
    pLayList = pLoop->pLayList;
    pLayer = pLayList->pLayer;
    /*
     * Configure window for this layer
     */
    (*pWin->drawable.pScreen->SetWindowPixmap) (pWin, pLayer->pPixmap);
    if (!pLayList->inheritClip)
    {
	pWin->clipList = pLayList->clipList;
	pWin->borderClip = pLayList->borderClip;
    }
    /*
     * Step to next layer list
     */
    pLoop->pLayList = pLayList->pNext;
    /*
     * Return layer
     */
    return pLayer;
}

void
LayerWindowDone (WindowPtr pWin, LayerWinLoopPtr pLoop)
{
    LayerWinPtr	pLayWin = pLoop->pLayWin;

    if (!pLayWin->isList)
	return;
    /*
     * clean up after the loop
     */
    pWin->clipList = pLoop->clipList;
    pWin->borderClip = pLoop->clipList;
    (*pWin->drawable.pScreen->SetWindowPixmap) (pWin, pLoop->pPixmap);
}

Bool
layerCreateWindow (WindowPtr pWin)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    layerWinPriv(pWin);
    layerScrPriv(pScreen);
    LayerPtr	pLayer;
    Bool	ret;

    pLayWin->isList = FALSE;
    pLayWin->u.pLayer = 0;
    
    /*
     * input only windows don't live in any layer
     */
    if (pWin->drawable.type == UNDRAWABLE_WINDOW)
	return TRUE;
    /*
     * Use a reasonable default layer -- the first
     * layer matching the windows depth.  Subsystems needing
     * alternative layering semantics can override this by
     * replacing this function.  Perhaps a new screen function
     * could be used to select the correct initial window
     * layer instead.
     */
    for (pLayer = pLayScr->pLayers; pLayer; pLayer = pLayer->pNext)
	if (pLayer->depth == pWin->drawable.depth)
	    break;
    ret = TRUE;
    if (pLayer)
    {
	pScreen->CreateWindow = pLayer->pKind->CreateWindow;
	ret = (*pScreen->CreateWindow) (pWin);
	pLayer->pKind->CreateWindow = pScreen->CreateWindow;
	pScreen->CreateWindow = layerCreateWindow;
	LayerWindowAdd (pScreen, pLayer, pWin);
    }
    return ret;
}

Bool
layerDestroyWindow (WindowPtr pWin)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    layerWinPriv(pWin);
    LayerPtr	pLayer;
    Bool	ret = TRUE;

    while ((pLayer = layerWinLayer (pLayWin)))
    {
	LayerUnwrap (pScreen, pLayer->pKind, DestroyWindow);
	ret = (*pScreen->DestroyWindow) (pWin);
	LayerWrap (pScreen, pLayer->pKind, DestroyWindow, layerDestroyWindow);
	LayerWindowRemove (pWin->drawable.pScreen, pLayer, pWin);
    }
    return ret;
}

Bool
layerChangeWindowAttributes (WindowPtr pWin, unsigned long mask)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLay;
    LayerWinLoopRec loop;
    Bool	    ret = TRUE;

    for (pLay = LayerWindowFirst (pWin, &loop);
	 pLay;
	 pLay = LayerWindowNext (pWin, &loop))
    {
	LayerUnwrap(pScreen,pLay->pKind,ChangeWindowAttributes);
	if (!(*pScreen->ChangeWindowAttributes) (pWin, mask))
	    ret = FALSE;
	LayerWrap(pScreen,pLay->pKind,ChangeWindowAttributes,layerChangeWindowAttributes);
    }
    LayerWindowDone (pWin, &loop);
    return ret;
}

void
layerPaintWindowBackground (WindowPtr pWin, RegionPtr pRegion, int what)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLay;
    LayerWinLoopRec loop;

    for (pLay = LayerWindowFirst (pWin, &loop);
	 pLay;
	 pLay = LayerWindowNext (pWin, &loop))
    {
	LayerUnwrap(pScreen,pLay->pKind,PaintWindowBackground);
	(*pScreen->PaintWindowBackground) (pWin, pRegion, what);
	LayerWrap(pScreen,pLay->pKind,PaintWindowBackground,layerPaintWindowBackground);
    }
    LayerWindowDone (pWin, &loop);
}

void
layerPaintWindowBorder (WindowPtr pWin, RegionPtr pRegion, int what)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLay;
    LayerWinLoopRec loop;

    for (pLay = LayerWindowFirst (pWin, &loop);
	 pLay;
	 pLay = LayerWindowNext (pWin, &loop))
    {
	LayerUnwrap(pScreen,pLay->pKind,PaintWindowBorder);
	(*pScreen->PaintWindowBorder) (pWin, pRegion, what);
	LayerWrap(pScreen,pLay->pKind,PaintWindowBorder,layerPaintWindowBorder);
    }
    LayerWindowDone (pWin, &loop);
}

void
layerCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLay;
    LayerWinLoopRec loop;
    int		    dx = 0, dy = 0;

    for (pLay = LayerWindowFirst (pWin, &loop);
	 pLay;
	 pLay = LayerWindowNext (pWin, &loop))
    {
	LayerUnwrap(pScreen,pLay->pKind,CopyWindow);
	/*
	 * Undo the translation done within the last CopyWindow proc (sigh)
	 */
	if (dx || dy)
	    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, dx, dy);
	(*pScreen->CopyWindow) (pWin, ptOldOrg, prgnSrc);
	LayerWrap(pScreen,pLay->pKind,CopyWindow,layerCopyWindow);
	/*
	 * Save offset to undo translation next time around
	 */
	dx = ptOldOrg.x - pWin->drawable.x;
	dy = ptOldOrg.y - pWin->drawable.y;
    }
    LayerWindowDone (pWin, &loop);
}

PixmapPtr
layerCreatePixmap (ScreenPtr pScreen, int width, int height, int depth)
{
    /* XXX assume the first layer can handle all pixmaps */
    layerScrPriv (pScreen);
    LayerKindPtr    pKind;
    PixmapPtr	    pPixmap;
    
    pKind = &pLayScr->kinds[0];
    if (pLayScr->pLayers)
	pKind = pLayScr->pLayers->pKind;
    LayerUnwrap (pScreen, pKind, CreatePixmap);
    pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height, depth);
    LayerWrap (pScreen,pKind,CreatePixmap,layerCreatePixmap);
    return pPixmap;
}

Bool
layerDestroyPixmap (PixmapPtr pPixmap)
{
    /* XXX assume the first layer can handle all pixmaps */
    ScreenPtr	    pScreen = pPixmap->drawable.pScreen;
    layerScrPriv (pScreen);
    LayerKindPtr    pKind;
    Bool	    ret;
    
    pKind = &pLayScr->kinds[0];
    if (pLayScr->pLayers)
	pKind = pLayScr->pLayers->pKind;
    LayerUnwrap (pScreen, pKind, DestroyPixmap);
    ret = (*pScreen->DestroyPixmap) (pPixmap);
    LayerWrap (pScreen,pKind,DestroyPixmap,layerDestroyPixmap);
    return ret;
}

