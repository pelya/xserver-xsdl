/*
 * $XFree86: xc/programs/Xserver/miext/layer/layergc.c,v 1.5 2001/10/28 03:34:15 tsi Exp $
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

GCFuncs	layerGCFuncs = {
    layerValidateGC, layerChangeGC, layerCopyGC, layerDestroyGC,
    layerChangeClip, layerDestroyClip, layerCopyClip
};

#if 0
/*
 * XXX dont need this until this supports 
 * separate clipping and multiple layers
 */
GCOps layerGCOps = {
    layerFillSpans, layerSetSpans, 
    layerPutImage, layerCopyArea, 
    layerCopyPlane, layerPolyPoint, 
    layerPolylines, layerPolySegment, 
    layerPolyRectangle, layerPolyArc, 
    layerFillPolygon, layerPolyFillRect, 
    layerPolyFillArc, layerPolyText8, 
    layerPolyText16, layerImageText8, 
    layerImageText16, layerImageGlyphBlt, 
    layerPolyGlyphBlt, layerPushPixels,
#ifdef NEED_LINEHELPER
    NULL,
#endif
    {NULL}		/* devPrivate */
};
#endif

Bool
layerCreateGC (GCPtr pGC)
{
    Bool	    ret = TRUE;
    LayerKindPtr    pLayKind;
    ScreenPtr	    pScreen = pGC->pScreen;
    layerScrPriv(pScreen);
    layerGCPriv(pGC);
    
    /*
     * XXX assume the first layer can handle all GCs
     */
    pLayKind = &pLayScr->kinds[0];
    if (pLayScr->pLayers)
	pLayKind = pLayScr->pLayers->pKind;
    pLayGC->pKind = pLayKind;
    LayerUnwrap (pScreen,pLayGC->pKind,CreateGC);
    
    if (!(*pScreen->CreateGC) (pGC))
	ret = FALSE;
    LayerWrap (pScreen,pLayKind,CreateGC,layerCreateGC);

    LayerWrap (pGC,pLayGC,funcs,&layerGCFuncs);

    return ret;
}

void
layerValidateGC(GCPtr         pGC,
		unsigned long changes,
		DrawablePtr   pDraw)
{
    layerGCPriv(pGC);
    LayerKindPtr    pKind;
    
    if (pDraw->type == DRAWABLE_WINDOW)
    {
	layerWinPriv ((WindowPtr) pDraw);
	pKind = layerWinLayer (pLayWin)->pKind;
    }
    else
    {
	/* XXX assume the first layer can handle all pixmaps */
	layerScrPriv (pDraw->pScreen);
	pKind = &pLayScr->kinds[0];
	if (pLayScr->pLayers)
	    pKind = pLayScr->pLayers->pKind;
    }
    
    LayerUnwrap (pGC,pLayGC,funcs);
    if (pKind != pLayGC->pKind)
    {
	/*
	 * Clean up the previous user
	 */
	CreateGCProcPtr	CreateGC;
	(*pGC->funcs->DestroyGC) (pGC);
	
	pGC->serialNumber = GC_CHANGE_SERIAL_BIT;

	pLayGC->pKind = pKind;
	
	/*
	 * Temporarily unwrap Create GC and let
	 * the new code setup the GC 
	 */
	CreateGC = pGC->pScreen->CreateGC;
	LayerUnwrap (pGC->pScreen, pLayGC->pKind, CreateGC);
	(*pGC->pScreen->CreateGC) (pGC);
	LayerWrap (pGC->pScreen, pLayGC->pKind, CreateGC, CreateGC);
    }
    
    (*pGC->funcs->ValidateGC) (pGC, changes, pDraw);
    LayerWrap(pGC,pLayGC,funcs,&layerGCFuncs);
}

void
layerDestroyGC(GCPtr pGC)
{
    layerGCPriv(pGC);
    LayerUnwrap (pGC,pLayGC,funcs);
    (*pGC->funcs->DestroyGC)(pGC);
    LayerWrap(pGC,pLayGC,funcs,&layerGCFuncs);
}

void
layerChangeGC (GCPtr		pGC,
	       unsigned long	mask)
{
    layerGCPriv(pGC);
    LayerUnwrap (pGC,pLayGC,funcs);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    LayerWrap(pGC,pLayGC,funcs,&layerGCFuncs);
}

void
layerCopyGC (GCPtr	    pGCSrc, 
	     unsigned long  mask,
	     GCPtr	    pGCDst)
{
    layerGCPriv(pGCDst);
    LayerUnwrap (pGCDst,pLayGC,funcs);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    LayerWrap(pGCDst,pLayGC,funcs,&layerGCFuncs);
}

void
layerChangeClip (GCPtr	    pGC,
		 int	    type,
		 pointer    pvalue,
		 int	    nrects)
{
    layerGCPriv(pGC);
    LayerUnwrap (pGC,pLayGC,funcs);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    LayerWrap(pGC,pLayGC,funcs,&layerGCFuncs);
}

void
layerCopyClip(GCPtr pGCDst, GCPtr pGCSrc)
{
    layerGCPriv(pGCDst);
    LayerUnwrap (pGCDst,pLayGC,funcs);
    (*pGCDst->funcs->CopyClip) (pGCDst, pGCSrc);
    LayerWrap(pGCDst,pLayGC,funcs,&layerGCFuncs);
}

void
layerDestroyClip(GCPtr pGC)
{
    layerGCPriv(pGC);
    LayerUnwrap (pGC,pLayGC,funcs);
    (*pGC->funcs->DestroyClip) (pGC);
    LayerWrap(pGC,pLayGC,funcs,&layerGCFuncs);
}

