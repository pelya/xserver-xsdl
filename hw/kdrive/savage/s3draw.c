/*
 * Id: s3draw.c,v 1.1 1999/11/02 03:54:47 keithp Exp $
 *
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */
/* $XFree86: xc/programs/Xserver/hw/kdrive/savage/s3draw.c,v 1.2 1999/12/30 03:03:11 robin Exp $ */

#include	"s3.h"
#include	"s3draw.h"

#include	"Xmd.h"
#include	"gcstruct.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mistruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"fb.h"
#include	"migc.h"
#include	"miline.h"

/*
 * Map X rops to S3 rops
 */

short s3alu[16] = {
    MIX_0,
    MIX_AND,
    MIX_SRC_AND_NOT_DST,
    MIX_SRC,
    MIX_NOT_SRC_AND_DST,
    MIX_DST,
    MIX_XOR,
    MIX_OR,
    MIX_NOR,
    MIX_XNOR,
    MIX_NOT_DST,
    MIX_SRC_OR_NOT_DST,
    MIX_NOT_SRC,
    MIX_NOT_SRC_OR_DST,
    MIX_NAND,
    MIX_1
};

/*
 * Handle pixel transfers
 */

#define BURST
#ifdef BURST
#define PixTransDeclare	VOL32	*pix_trans_base = (VOL32 *) (s3c->registers),\
				*pix_trans = pix_trans_base
#define PixTransStart(n)	if (pix_trans + (n) > pix_trans_base + 8192) pix_trans = pix_trans_base
#define PixTransStore(t)	*pix_trans++ = (t)
#else
#define PixTransDeclare	VOL32	*pix_trans = &s3->pix_trans
#define PixTransStart(n)	
#define PixTransStore(t)	*pix_trans = (t)
#endif

int	s3GCPrivateIndex;
int	s3WindowPrivateIndex;
int	s3Generation;

/*
  s3DoBitBlt
  =============
  Bit Blit for all window to window blits.
*/

#define sourceInvarient(alu)	(((alu) & 3) == (((alu) >> 2) & 3))

void
s3CopyNtoN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure)
{
    SetupS3(pDstDrawable->pScreen);
    int	    srcX, srcY, dstX, dstY;
    int	    w, h;
    int	    flags;
    
    if (sourceInvarient (pGC->alu))
    {
	s3FillBoxSolid (pDstDrawable, nbox, pbox, 0, pGC->alu, pGC->planemask);
	return;
    }
    
    _s3SetBlt(s3,pGC->alu,pGC->planemask);
    DRAW_DEBUG ((DEBUG_RENDER, "s3CopyNtoN alu %d planemask 0x%x",
		pGC->alu, pGC->planemask));
    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	flags = 0;
	if (reverse)
	{
	    dstX = pbox->x2 - 1;
	}
	else
	{
	    dstX = pbox->x1;
	    flags |= INC_X;
	}
	srcX = dstX + dx;
	
	if (upsidedown)
	{
	    dstY = pbox->y2 - 1;
	}
	else
	{
	    dstY = pbox->y1;
	    flags |= INC_Y;
	}
	srcY = dstY + dy;
	
	_s3Blt (s3, srcX, srcY, dstX, dstY, w, h, flags);
	pbox++;
    }
    MarkSyncS3 (pSrcDrawable->pScreen);
}

RegionPtr
s3CopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	   int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    SetupS3(pDstDrawable->pScreen);
    
    if (pSrcDrawable->type == DRAWABLE_WINDOW &&
	pDstDrawable->type == DRAWABLE_WINDOW)
    {
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, s3CopyNtoN, 0, 0);
    }
    return KdCheckCopyArea (pSrcDrawable, pDstDrawable, pGC, 
			    srcx, srcy, width, height, dstx, dsty);
}

typedef struct _s31toNargs {
    unsigned long	copyPlaneFG, copyPlaneBG;
    Bool		opaque;
} s31toNargs;

void
_s3Stipple (S3CardInfo	*s3c,
	    FbStip	*psrcBase,
	    FbStride	widthSrc,
	    int		srcx,
	    int		srcy,
	    int		dstx,
	    int		dsty,
	    int		width,
	    int		height)
{
    S3Ptr	s3 = s3c->s3;
    FbStip	*psrcLine, *psrc;
    FbStride	widthRest;
    FbStip	bits, tmp, lastTmp;
    int		leftShift, rightShift;
    int		nl, nlMiddle;
    int		r;
    PixTransDeclare;
    
    /* Compute blt address and parameters */
    psrc = psrcBase + srcy * widthSrc + (srcx >> 5);
    nlMiddle = (width + 31) >> 5;
    leftShift = srcx & 0x1f;
    rightShift = 32 - leftShift;
    widthRest = widthSrc - nlMiddle;
    
    _s3PlaneBlt(s3,dstx,dsty,width,height);
    
    if (leftShift == 0)
    {
	while (height--)
	{
	    nl = nlMiddle;
	    PixTransStart(nl);
	    while (nl--)
	    {
		tmp = *psrc++;
		S3AdjustBits32 (tmp);
		PixTransStore (tmp);
	    }
	    psrc += widthRest;
	}
    }
    else
    {
	widthRest--;
	while (height--)
	{
	    bits = *psrc++;
	    nl = nlMiddle;
	    PixTransStart(nl);
	    while (nl--)
	    {
		tmp = FbStipLeft(bits, leftShift);
		bits = *psrc++;
		tmp |= FbStipRight(bits, rightShift);
		S3AdjustBits32(tmp);
		PixTransStore (tmp);
	    }
	    psrc += widthRest;
	}
    }
}
	    
void
s3Copy1toN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure)
{
    SetupS3(pDstDrawable->pScreen);
    
    s31toNargs		*args = closure;
    int			dstx, dsty;
    FbStip		*psrcBase;
    FbStride		widthSrc;
    int			srcBpp;

    if (args->opaque && sourceInvarient (pGC->alu))
    {
	s3FillBoxSolid (pDstDrawable, nbox, pbox,
			pGC->bgPixel, pGC->alu, pGC->planemask);
	return;
    }
    
    fbGetStipDrawable (pSrcDrawable, psrcBase, widthSrc, srcBpp);
    
    if (args->opaque)
    {
	_s3SetOpaquePlaneBlt(s3,pGC->alu,pGC->planemask,args->copyPlaneFG,
			     args->copyPlaneBG);
    }
    else
    {
	_s3SetTransparentPlaneBlt (s3, pGC->alu, 
				   pGC->planemask, args->copyPlaneFG);
    }
    
    while (nbox--)
    {
	dstx = pbox->x1;
	dsty = pbox->y1;
	
	_s3Stipple (s3c,
		    psrcBase, widthSrc, 
		    dstx + dx, dsty + dy,
		    dstx, dsty, 
		    pbox->x2 - dstx, pbox->y2 - dsty);
	pbox++;
    }
    MarkSyncS3 (pDstDrawable->pScreen);
}

RegionPtr
s3CopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	int srcx, int srcy, int width, int height, 
	int dstx, int dsty, unsigned long bitPlane)
{
    SetupS3 (pDstDrawable->pScreen);
    RegionPtr		ret;
    s31toNargs		args;

    if (pDstDrawable->type == DRAWABLE_WINDOW &&
	pSrcDrawable->depth == 1)
    {
	args.copyPlaneFG = pGC->fgPixel;
	args.copyPlaneBG = pGC->bgPixel;
	args.opaque = TRUE;
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, s3Copy1toN, bitPlane, &args);
    }
    return KdCheckCopyPlane(pSrcDrawable, pDstDrawable, pGC, 
			    srcx, srcy, width, height, 
			    dstx, dsty, bitPlane);
}

void
s3PushPixels (GCPtr pGC, PixmapPtr pBitmap,
	      DrawablePtr pDrawable,
	      int w, int h, int x, int y)
{
    SetupS3 (pDrawable->pScreen);
    s31toNargs		args;
    
    if (pDrawable->type == DRAWABLE_WINDOW && pGC->fillStyle == FillSolid)
    {
	args.opaque = FALSE;
	args.copyPlaneFG = pGC->fgPixel;
	(void) fbDoCopy ((DrawablePtr) pBitmap, pDrawable, pGC,
			  0, 0, w, h, x, y, s3Copy1toN, 1, &args);
    }
    else
    {
	KdCheckPushPixels (pGC, pBitmap, pDrawable, w, h, x, y);
    }
}

void
s3FillBoxSolid (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		unsigned long pixel, int alu, unsigned long planemask)
{
    SetupS3(pDrawable->pScreen);
    register int	r;

    _s3SetSolidFill(s3,pixel,alu,planemask);
    
    while (nBox--) {
	_s3SolidRect(s3,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1);
	pBox++;
    }
    MarkSyncS3 (pDrawable->pScreen);
}

void
_s3SetPattern (ScreenPtr pScreen,
	      int alu, unsigned long planemask, s3PatternPtr pPattern)
{
    SetupS3(pScreen);
    S3PatternCache  *cache;
    
    _s3LoadPattern (pScreen, pPattern);
    cache = pPattern->cache;
    
    switch (pPattern->fillStyle) {
    case FillTiled:
	_s3SetTile(s3,alu,planemask);
	break;
    case FillStippled:
	_s3SetStipple(s3,alu,planemask,pPattern->fore);
	break;
    case FillOpaqueStippled:
	_s3SetOpaqueStipple(s3,alu,planemask,pPattern->fore,pPattern->back);
	break;
    }
}

void
s3FillBoxPattern (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		  int alu, unsigned long planemask, s3PatternPtr pPattern)
{
    SetupS3(pDrawable->pScreen);
    S3PatternCache    	*cache;
    int			patx, paty;

    _s3SetPattern (pDrawable->pScreen, alu, planemask, pPattern);
    cache = pPattern->cache;
    while (nBox--) 
    {
	_s3PatRect(s3,cache->x, cache->y,
		   pBox->x1, pBox->y1, 
		   pBox->x2-pBox->x1, pBox->y2-pBox->y1);
	pBox++;
    }
    MarkSyncS3 (pDrawable->pScreen);
}

void
s3FillBoxLargeStipple (DrawablePtr pDrawable, GCPtr pGC,
		       int nBox, BoxPtr pBox)
{
    SetupS3(pDrawable->pScreen);
    DrawablePtr	pStipple = &pGC->stipple->drawable;
    int		xRot = pGC->patOrg.x + pDrawable->x;
    int		yRot = pGC->patOrg.y + pDrawable->y;
    FbStip	*stip;
    FbStride	stipStride;
    int		stipBpp;
    int		stipWidth, stipHeight;
    int		dstX, dstY, width, height;
    
    stipWidth = pStipple->width;
    stipHeight = pStipple->height;
    fbGetStipDrawable (pStipple, stip, stipStride, stipBpp);

    if (pGC->fillStyle == FillOpaqueStippled)
    {
	_s3SetOpaquePlaneBlt(s3,pGC->alu,pGC->planemask,
			     pGC->fgPixel, pGC->bgPixel);
    
    }
    else
    {
	_s3SetTransparentPlaneBlt(s3,pGC->alu,pGC->planemask, pGC->fgPixel);
    }
    
    while (nBox--)
    {
	int		stipX, stipY, sx;
	int		widthTmp;
	int		h, w;
	int		x, y;
    
	dstX = pBox->x1;
	dstY = pBox->y1;
	width = pBox->x2 - pBox->x1;
	height = pBox->y2 - pBox->y1;
	pBox++;
	modulus (dstY - yRot, stipHeight, stipY);
	modulus (dstX - xRot, stipWidth, stipX);
	y = dstY;
	while (height)
	{
	    h = stipHeight - stipY;
	    if (h > height)
		h = height;
	    height -= h;
	    widthTmp = width;
	    x = dstX;
	    sx = stipX;
	    while (widthTmp)
	    {
		w = (stipWidth - sx);
		if (w > widthTmp)
		    w = widthTmp;
		widthTmp -= w;
		_s3Stipple (s3c,
			    stip,
			    stipStride,
			    sx, stipY,
			    x, y,
			    w, h);
		x += w;
		sx = 0;
	    }
	    y += h;
	    stipY = 0;
	}
    }
    MarkSyncS3 (pDrawable->pScreen);
}

#define NUM_STACK_RECTS	1024

void
s3PolyFillRect (DrawablePtr pDrawable, GCPtr pGC, 
		int nrectFill, xRectangle *prectInit)
{
    s3GCPrivate(pGC);
    xRectangle	    *prect;
    RegionPtr	    prgnClip;
    register BoxPtr pbox;
    register BoxPtr pboxClipped;
    BoxPtr	    pboxClippedBase;
    BoxPtr	    pextent;
    BoxRec	    stackRects[NUM_STACK_RECTS];
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate (pGC);
    int		    numRects;
    int		    n;
    int		    xorg, yorg;
    int		    x, y;

    prgnClip = fbGetCompositeClip(pGC);
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }
    
    prect = prectInit;

    numRects = REGION_NUM_RECTS(prgnClip) * nrectFill;
    if (numRects > NUM_STACK_RECTS)
    {
	pboxClippedBase = (BoxPtr)ALLOCATE_LOCAL(numRects * sizeof(BoxRec));
	if (!pboxClippedBase)
	    return;
    }
    else
	pboxClippedBase = stackRects;

    pboxClipped = pboxClippedBase;
	
    if (REGION_NUM_RECTS(prgnClip) == 1)
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_RECTS(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    if ((pboxClipped->x1 = prect->x) < x1)
		pboxClipped->x1 = x1;
    
	    if ((pboxClipped->y1 = prect->y) < y1)
		pboxClipped->y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    pboxClipped->x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    pboxClipped->y2 = by2;

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2))
	    {
		pboxClipped++;
	    }
    	}
    }
    else
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_EXTENTS(pGC->pScreen, prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    BoxRec box;
    
	    if ((box.x1 = prect->x) < x1)
		box.x1 = x1;
    
	    if ((box.y1 = prect->y) < y1)
		box.y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    box.x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    box.y2 = by2;
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
    	}
    }
    if (pboxClipped != pboxClippedBase)
    {
	if (pGC->fillStyle == FillSolid)
	    s3FillBoxSolid(pDrawable,
			   pboxClipped-pboxClippedBase, pboxClippedBase,
			   pGC->fgPixel, pGC->alu, pGC->planemask);
	else if (s3Priv->pPattern)
	    s3FillBoxPattern (pDrawable,
			      pboxClipped-pboxClippedBase, pboxClippedBase,
			      pGC->alu, pGC->planemask,
			      s3Priv->pPattern);
	else
	    s3FillBoxLargeStipple (pDrawable, pGC,
				   pboxClipped-pboxClippedBase, 
				   pboxClippedBase);
    }
    if (pboxClippedBase != stackRects)
    	DEALLOCATE_LOCAL(pboxClippedBase);
}

void
_s3FillSpanLargeStipple (DrawablePtr pDrawable, GCPtr pGC,
			 int n, DDXPointPtr ppt, int *pwidth)
{
    SetupS3 (pDrawable->pScreen);
    DrawablePtr	pStipple = &pGC->stipple->drawable;
    int		xRot = pGC->patOrg.x + pDrawable->x;
    int		yRot = pGC->patOrg.y + pDrawable->y;
    FbStip	*stip;
    FbStride	stipStride;
    int		stipBpp;
    int		stipWidth, stipHeight;
    int		dstX, dstY, width, height;
    
    stipWidth = pStipple->width;
    stipHeight = pStipple->height;
    fbGetStipDrawable (pStipple, stip, stipStride, stipBpp);
    if (pGC->fillStyle == FillOpaqueStippled)
    {
	_s3SetOpaquePlaneBlt(s3,pGC->alu,pGC->planemask,
			     pGC->fgPixel, pGC->bgPixel);
    
    }
    else
    {
	_s3SetTransparentPlaneBlt(s3,pGC->alu,pGC->planemask, pGC->fgPixel);
    }
    while (n--)
    {
	int		stipX, stipY, sx;
	int		w;
	int		x, y;
    
	dstX = ppt->x;
	dstY = ppt->y;
	ppt++;
	width = *pwidth++;
	modulus (dstY - yRot, stipHeight, stipY);
	modulus (dstX - xRot, stipWidth, stipX);
	y = dstY;
	x = dstX;
	sx = stipX;
	while (width)
	{
	    w = (stipWidth - sx);
	    if (w > width)
		w = width;
	    width -= w;
	    _s3Stipple (s3c,
			stip,
			stipStride,
			sx, stipY,
			x, y,
			w, 1);
	    x += w;
	    sx = 0;
	}
    }
}

void
s3FillSpans (DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    s3GCPrivate(pGC);
    SetupS3(pDrawable->pScreen);
    int		    x, y, x1, y1, x2, y2;
    int		    width;
				/* next three parameters are post-clip */
    int		    nTmp;
    int		    *pwidthFree;/* copies of the pointers to free */
    DDXPointPtr	    pptFree;
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate(pGC);
    BoxPtr	    extents;
    S3PatternCache  *cache;
    RegionPtr	    pClip = fbGetCompositeClip (pGC);

    if (REGION_NUM_RECTS(pClip) == 1 && 
	(pGC->fillStyle == FillSolid  || s3Priv->pPattern))
    {
	extents = REGION_RECTS(pClip);
	x1 = extents->x1;
	x2 = extents->x2;
	y1 = extents->y1;
	y2 = extents->y2;
	if (pGC->fillStyle == FillSolid)
	{
	    _s3SetSolidFill(s3,pGC->fgPixel,pGC->alu,pGC->planemask);
	    cache = 0;
	}
	else
	{
	    _s3SetPattern (pDrawable->pScreen, pGC->alu, pGC->planemask,
			   s3Priv->pPattern);
	    cache = s3Priv->pPattern->cache;
	}
	while (n--)
	{
	    y = ppt->y;
	    if (y1 <= y && y < y2)
	    {
		x = ppt->x;
		width = *pwidth;
		if (x < x1)
		{
		    width -= (x1 - x);
		    x = x1;
		}
		if (x2 < x + width)
		    width = x2 - x;
		if (width > 0)
		{
		    if (cache)
		    {
			_s3PatRect(s3, cache->x, cache->y, x, y, width, 1);
		    }
		    else
		    {
			_s3SolidRect(s3,x,y,width,1);
		    }
		}
	    }
	    ppt++;
	    pwidth++;
	}
    }
    else
    {
	nTmp = n * miFindMaxBand(pClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(nTmp * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(nTmp * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree)
	{
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	n = miClipSpans(fbGetCompositeClip(pGC),
			ppt, pwidth, n,
			pptFree, pwidthFree, fSorted);
	pwidth = pwidthFree;
	ppt = pptFree;
	if (pGC->fillStyle == FillSolid)
	{
	    _s3SetSolidFill(s3,pGC->fgPixel,pGC->alu,pGC->planemask);
	    while (n--)
	    {
		x = ppt->x;
		y = ppt->y;
		ppt++;
		width = *pwidth++;
		if (width)
		{
		    _s3SolidRect(s3,x,y,width,1);
		}
	    }
	}
	else if (s3Priv->pPattern)
	{
	    _s3SetPattern (pDrawable->pScreen, pGC->alu, pGC->planemask,
			   s3Priv->pPattern);
	    cache = s3Priv->pPattern->cache;
	    while (n--)
	    {
		x = ppt->x;
		y = ppt->y;
		ppt++;
		width = *pwidth++;
		if (width)
		{
		    _s3PatRect(s3, cache->x, cache->y, x, y, width, 1);
		}
	    }
	}
	else
	{
	    _s3FillSpanLargeStipple (pDrawable, pGC, n, ppt, pwidth);
	}
	DEALLOCATE_LOCAL(pptFree);
	DEALLOCATE_LOCAL(pwidthFree);
    }
    MarkSyncS3 (pDrawable->pScreen);
}

#include "mifillarc.h"

#define FILLSPAN(s3,y,__x1,__x2) {\
    DRAW_DEBUG ((DEBUG_ARCS, "FILLSPAN %d: %d->%d", y, __x1, __x2)); \
    if ((__x2) >= (__x1)) {\
	_s3SolidRect(s3,(__x1),(y),(__x2)-(__x1)+1,1); \
    } \
}

#define FILLSLICESPANS(flip,__y) \
    if (!flip) \
    { \
	FILLSPAN(s3,__y,xl,xr) \
    } \
    else \
    { \
	xc = xorg - x; \
	FILLSPAN(s3, __y, xc, xr) \
	xc += slw - 1; \
	FILLSPAN(s3, __y, xl, xc) \
    }

static void
_s3FillEllipse (DrawablePtr pDraw, S3Ptr s3, xArc *arc)
{
    int x, y, e;
    int yk, xk, ym, xm, dx, dy, xorg, yorg;
    int	y_top, y_bot;
    miFillArcRec info;
    register int xpos;
    int	slw;

    miFillArcSetup(arc, &info);
    MIFILLARCSETUP();
    y_top = pDraw->y + yorg - y;
    y_bot = pDraw->y + yorg + y + dy;
    xorg += pDraw->x;
    while (y)
    {
	y_top++;
	y_bot--;
	MIFILLARCSTEP(slw);
	if (!slw)
	    continue;
	xpos = xorg - x;
	_s3SolidRect (s3,xpos,y_top,slw,1);
	if (miFillArcLower(slw))
	    _s3SolidRect (s3,xpos,y_bot,slw,1);
    }
}


static void
_s3FillArcSlice (DrawablePtr pDraw, GCPtr pGC, S3Ptr s3, xArc *arc)
{
    int yk, xk, ym, xm, dx, dy, xorg, yorg, slw;
    register int x, y, e;
    miFillArcRec info;
    miArcSliceRec slice;
    int xl, xr, xc;
    int	y_top, y_bot;

    DRAW_DEBUG ((DEBUG_ARCS, "slice %dx%d+%d+%d %d->%d",
		 arc->width, arc->height, arc->x, arc->y,
		 arc->angle1, arc->angle2));
    miFillArcSetup(arc, &info);
    miFillArcSliceSetup(arc, &slice, pGC);
    DRAW_DEBUG ((DEBUG_ARCS, "edge1.x %d edge2.x %d", 
		slice.edge1.x, slice.edge2.x));
    MIFILLARCSETUP();
    DRAW_DEBUG ((DEBUG_ARCS, "xorg %d yorg %d",
		xorg, yorg));
    xorg += pDraw->x;
    yorg += pDraw->y;
    y_top = yorg - y;
    y_bot = yorg + y + dy;
    slice.edge1.x += pDraw->x;
    slice.edge2.x += pDraw->x;
    DRAW_DEBUG ((DEBUG_ARCS, "xorg %d y_top %d y_bot %d",
		 xorg, y_top, y_bot));
    while (y > 0)
    {
	y_top++;
	y_bot--;
	MIFILLARCSTEP(slw);
	MIARCSLICESTEP(slice.edge1);
	MIARCSLICESTEP(slice.edge2);
	if (miFillSliceUpper(slice))
	{
	    MIARCSLICEUPPER(xl, xr, slice, slw);
	    FILLSLICESPANS(slice.flip_top, y_top);
	}
	if (miFillSliceLower(slice))
	{
	    MIARCSLICELOWER(xl, xr, slice, slw);
	    FILLSLICESPANS(slice.flip_bot, y_bot);
	}
    }
}

void
s3PolyFillArcSolid (DrawablePtr pDraw, GCPtr pGC, int narcs, xArc *parcs)
{
    SetupS3(pDraw->pScreen);
    xArc	    *arc;
    int		    i;
    int		    x, y;
    BoxRec	    box;
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    BOOL	    set;

    set = FALSE;
    for (; --narcs >= 0; parcs++)
    {
	if (miFillArcEmpty(parcs))
	    continue;
	if (miCanFillArc(parcs))
	{
	    box.x1 = parcs->x + pDraw->x;
	    box.y1 = parcs->y + pDraw->y;
	    box.x2 = box.x1 + (int)parcs->width + 1;
	    box.y2 = box.y1 + (int)parcs->height + 1;
	    switch (RECT_IN_REGION(pDraw->pScreen, pClip, &box))
	    {
	    case rgnIN:
		if (!set)
		{
		    _s3SetSolidFill (s3, pGC->fgPixel, pGC->alu, pGC->planemask);
		    set = TRUE;
		}
		if ((parcs->angle2 >= FULLCIRCLE) ||
		    (parcs->angle2 <= -FULLCIRCLE))
		{
		    DRAW_DEBUG ((DEBUG_ARCS, "Full circle ellipse %dx%d",
				 parcs->width, parcs->height));
		    _s3FillEllipse (pDraw, s3, parcs);
		}
		else
		{
		    DRAW_DEBUG ((DEBUG_ARCS, "Partial ellipse %dx%d",
				 parcs->width, parcs->height));
		    _s3FillArcSlice (pDraw, pGC, s3, parcs);
		}
		/* fall through ... */
	    case rgnOUT:
		continue;
	    case rgnPART:
		break;
	    }
	}
	if (set)
	{
	    MarkSyncS3 (pDraw->pScreen);
	    set = FALSE;
	}
	KdCheckPolyFillArc(pDraw, pGC, 1, parcs);
    }
    if (set)
    {
	MarkSyncS3 (pDraw->pScreen);
	set = FALSE;
    }
}

void
s3FillPoly1Rect (DrawablePtr pDrawable, GCPtr pGC, int shape, 
		 int mode, int countInit, DDXPointPtr ptsIn)
{
    SetupS3(pDrawable->pScreen);
    FbGCPrivPtr	    fbPriv;
    int		    nwidth;
    int		    maxy;
    int		    count;
    register int    vertex1, vertex2;
    int		    c;
    BoxPtr	    extents;
    int		    y, sy;
    int		    *vertex1p, *vertex2p;
    int		    *endp;
    int		    x1, x2, sx;
    int		    dx1, dx2;
    int		    dy1, dy2;
    int		    e1, e2;
    int		    step1, step2;
    int		    sign1, sign2;
    int		    h;
    int		    l, r;
    int		    nmiddle;

    if (mode == CoordModePrevious)
    {
	KdCheckFillPolygon (pDrawable, pGC, shape, mode, countInit, ptsIn);
	return;
    }
    
    fbPriv = fbGetGCPrivate(pGC);
    sy = pDrawable->y;
    sx = pDrawable->x;
    extents = &fbGetCompositeClip(pGC)->extents;
    
    y = 32767;
    maxy = 0;
    vertex2p = (int *) ptsIn;
    endp = vertex2p + countInit;
    if (shape == Convex)
    {
	count = countInit;
    	while (count--)
    	{
	    c = *vertex2p;
	    /*
	     * Check for negative or over S3 limits
	     */
	    if (c & 0xe000e000)
	    {
		KdCheckFillPolygon (pDrawable, pGC, shape, mode, countInit, ptsIn);
		return;
	    }
	    c = intToY(c);
	    DRAW_DEBUG ((DEBUG_POLYGON, "Y coordinate %d", c));
	    if (c < y) 
	    {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
    	}
    }
    else
    {
	int yFlip = 0;
	dx1 = 1;
	x2 = -1;
	x1 = -1;
	count = countInit;
    	while (count--)
    	{
	    c = *vertex2p;
	    /*
	     * Check for negative or over S3 limits
	     */
	    if (c & 0xe000e000)
	    {
		KdCheckFillPolygon (pDrawable, pGC, shape, mode, countInit, ptsIn);
		return;
	    }
	    c = intToY(c);
	    DRAW_DEBUG ((DEBUG_POLYGON, "Y coordinate %d", c));
	    if (c < y) 
	    {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
	    if (c == x1)
		continue;
	    if (dx1 > 0)
	    {
		if (x2 < 0)
		    x2 = c;
		else
		    dx2 = dx1 = (c - x1) >> 31;
	    }
	    else
		if ((c - x1) >> 31 != dx1) 
		{
		    dx1 = ~dx1;
		    yFlip++;
		}
	    x1 = c;
       	}
	x1 = (x2 - c) >> 31;
	if (x1 != dx1)
	    yFlip++;
	if (x1 != dx2)
	    yFlip++;
	if (yFlip != 2) 
	{
	    KdCheckFillPolygon (pDrawable, pGC, shape, mode, countInit, ptsIn);
	    return;
	}
    }
    if (y == maxy)
	return;

    _s3SetSolidFill(s3,pGC->fgPixel,pGC->alu,pGC->planemask);
    _s3SetClip(s3,extents);
    
    vertex2p = vertex1p;
    vertex2 = vertex1 = *vertex2p++;
    if (vertex2p == endp)
	vertex2p = (int *) ptsIn;
#define Setup(c,x,vertex,dx,dy,e,sign,step) {\
    x = intToX(vertex); \
    if (dy = intToY(c) - y) { \
    	dx = intToX(c) - x; \
	step = 0; \
    	if (dx >= 0) \
    	{ \
	    e = 0; \
	    sign = 1; \
	    if (dx >= dy) {\
	    	step = dx / dy; \
	    	dx = dx % dy; \
	    } \
    	} \
    	else \
    	{ \
	    e = 1 - dy; \
	    sign = -1; \
	    dx = -dx; \
	    if (dx >= dy) { \
		step = - (dx / dy); \
		dx = dx % dy; \
	    } \
    	} \
    } \
    x += sx; \
    vertex = c; \
}

#define Step(x,dx,dy,e,sign,step) {\
    x += step; \
    if ((e += dx) > 0) \
    { \
	x += sign; \
	e -= dy; \
    } \
}
    sy += y;
    DRAW_DEBUG ((DEBUG_POLYGON, "Starting polygon at %d", sy));
    for (;;)
    {
	DRAW_DEBUG ((DEBUG_POLYGON, "vertex1 0x%x vertex2 0x%x y %d vy1 %d vy2 %d",
		     vertex1, vertex2,
		     y, intToY(vertex1), intToY (vertex2)));
	if (y == intToY(vertex1))
	{
	    DRAW_DEBUG ((DEBUG_POLYGON, "Find next -- vertext"));
	    do
	    {
	    	if (vertex1p == (int *) ptsIn)
		    vertex1p = endp;
	    	c = *--vertex1p;
	    	Setup (c,x1,vertex1,dx1,dy1,e1,sign1,step1);
		DRAW_DEBUG ((DEBUG_POLYGON, "-- vertex 0x%x y %d",
			     vertex1, intToY(vertex1)));
	    } while (y >= intToY(vertex1));
	    h = dy1;
	}
	else
	{
	    Step(x1,dx1,dy1,e1,sign1,step1)
	    h = intToY(vertex1) - y;
	}
	if (y == intToY(vertex2))
	{
	    DRAW_DEBUG ((DEBUG_POLYGON, "Find next ++ vertext"));
	    do
	    {
	    	c = *vertex2p++;
	    	if (vertex2p == endp)
		    vertex2p = (int *) ptsIn;
	    	Setup (c,x2,vertex2,dx2,dy2,e2,sign2,step2)
		DRAW_DEBUG ((DEBUG_POLYGON, "++ vertex 0x%x y %d",
			     vertex1, intToY(vertex1)));
	    } while (y >= intToY(vertex2));
	    if (dy2 < h)
		h = dy2;
	}
	else
	{
	    Step(x2,dx2,dy2,e2,sign2,step2)
	    if ((c = (intToY(vertex2) - y)) < h)
		h = c;
	}
	DRAW_DEBUG ((DEBUG_POLYGON, "This band %d", h));
	/* fill spans for this segment */
	for (;;)
	{
	    nmiddle = x2 - x1;
	    DRAW_DEBUG ((DEBUG_POLYGON, "This span %d->%d", x1, x2));
	    if (nmiddle)
	    {
		l = x1;
		if (nmiddle < 0)
		{
		    nmiddle = -nmiddle;
		    l = x2;
		}
		_s3SolidRect(s3,l,sy,nmiddle,1);
	    }
	    y++;
	    sy++;
	    if (!--h)
		break;
	    Step(x1,dx1,dy1,e1,sign1,step1)
	    Step(x2,dx2,dy2,e2,sign2,step2)
	}
	if (y == maxy)
	    break;
    }
    _s3ResetClip (s3, pDrawable->pScreen);
    MarkSyncS3 (pDrawable->pScreen);
}

void
s3PolyGlyphBltClipped (DrawablePtr pDrawable,
		       GCPtr pGC,
		       int x, int y, 
		       unsigned int nglyph,
		       CharInfoPtr *ppciInit, 
		       pointer pglyphBase)
{
    SetupS3(pDrawable->pScreen);
    int		    h;
    int		    w;
    int		    xBack, yBack;
    int		    hBack, wBack;
    int		    lw;
    FontPtr	    pfont = pGC->font;
    CharInfoPtr	    pci;
    unsigned long   *bits;
    BoxPtr	    extents;
    BoxRec	    bbox;
    CARD32	    b;
    CharInfoPtr	    *ppci;
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate(pGC);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    BoxPtr	    pBox;
    int		    nbox;
    int		    x1, y1, x2, y2;
    unsigned char   alu;
    Bool	    set;
    PixTransDeclare;

    x += pDrawable->x;
    y += pDrawable->y;

    if (pglyphBase == (pointer) 1)
    {
	xBack = x;
	yBack = y - FONTASCENT(pGC->font);
	wBack = 0;
	hBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
	if (hBack)
	{
	    h = nglyph;
	    ppci = ppciInit;
	    while (h--)
		wBack += (*ppci++)->metrics.characterWidth;
	}
	if (wBack < 0)
	{
	    xBack = xBack + wBack;
	    wBack = -wBack;
	}
	if (hBack < 0)
	{
	    yBack = yBack + hBack;
	    hBack = -hBack;
	}
	alu = GXcopy;
    }
    else
    {
	wBack = 0;
	alu = pGC->alu;
    }
    
    if (wBack)
    {
	_s3SetSolidFill (s3, pGC->bgPixel, GXcopy, pGC->planemask);
	for (nbox = REGION_NUM_RECTS (pClip),
	     pBox = REGION_RECTS (pClip);
	     nbox--;
	     pBox++)
	{
	    x1 = xBack;
	    x2 = xBack + wBack;
	    y1 = yBack;
	    y2 = yBack + hBack;
	    if (x1 < pBox->x1) x1 = pBox->x1;
	    if (x2 > pBox->x2) x2 = pBox->x2;
	    if (y1 < pBox->y1) y1 = pBox->y1;
	    if (y2 > pBox->y2) y2 = pBox->y2;
	    if (x1 < x2 && y1 < y2)
	    {
		_s3SolidRect (s3, x1, y1, x2 - x1, y2 - y1);
	    }
	}
	MarkSyncS3 (pDrawable->pScreen);
    }
    ppci = ppciInit;
    set = FALSE;
    while (nglyph--)
    {
	pci = *ppci++;
	h = pci->metrics.ascent + pci->metrics.descent;
	w = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	x1 = x + pci->metrics.leftSideBearing;
	y1 = y - pci->metrics.ascent;
	bbox.x1 = x1;
	bbox.y1 = y1;
	bbox.x2 = x1 + w;
	bbox.y2 = y1 + h;
	switch (RECT_IN_REGION(pGC->pScreen, pClip, &bbox))
	{
	case rgnIN:
#if 1
	    lw = h * ((w + 31) >> 5);
	    if (lw)
	    {
		if (!set)
		{
		    _s3SetTransparentPlaneBlt (s3, alu, pGC->planemask, pGC->fgPixel);
		    set = TRUE;
		}
		_s3PlaneBlt(s3,
			    x + pci->metrics.leftSideBearing,
			    y - pci->metrics.ascent,
			    w, h);
		bits = (unsigned long *) pci->bits;
		PixTransStart (lw);
		while (lw--) 
		{
		    b = *bits++;
		    S3AdjustBits32 (b);
		    PixTransStore(b);
		}
		MarkSyncS3 (pDrawable->pScreen);
	    }
	    break;
#endif
	case rgnPART:
	    set = FALSE;
	    CheckSyncS3 (pDrawable->pScreen);
	    fbPutXYImage (pDrawable,
			  pClip,
			  fbPriv->fg,
			  fbPriv->bg,
			  fbPriv->pm,
			  alu,
			  FALSE,
			  x1, y1,
			  w, h,
			  (FbStip *) pci->bits,
			  (w + 31) >> 5,
			  0);
	    break;
	case rgnOUT:
	    break;
	}
	x += pci->metrics.characterWidth;
    }
}
		       
/*
 * Blt glyphs using S3 image transfer register, this does both
 * poly glyph blt and image glyph blt (when pglyphBase == 1)
 */

void
s3PolyGlyphBlt (DrawablePtr pDrawable, 
		GCPtr pGC, 
		int x, int y, 
		unsigned int nglyph,
		CharInfoPtr *ppciInit, 
		pointer pglyphBase)
{
    SetupS3(pDrawable->pScreen);
    int		    h;
    int		    w;
    int		    xBack, yBack;
    int		    hBack, wBack;
    int		    lw;
    FontPtr	    pfont = pGC->font;
    CharInfoPtr	    pci;
    unsigned long   *bits;
    BoxPtr	    extents;
    BoxRec	    bbox;
    CARD32	    b;
    CharInfoPtr	    *ppci;
    unsigned char   alu;
    PixTransDeclare;

    x += pDrawable->x;
    y += pDrawable->y;

    /* compute an approximate (but covering) bounding box */
    ppci = ppciInit;
    w = 0;
    h = nglyph;
    while (h--)
	w += (*ppci++)->metrics.characterWidth;
    if (w < 0)
    {
	bbox.x1 = x + w;
	bbox.x2 = x;
    }
    else
    {
	bbox.x1 = x;
	bbox.x2 = x + w;
    }
    w = FONTMINBOUNDS(pfont,leftSideBearing);
    if (w < 0)
	bbox.x1 += w;
    w = FONTMAXBOUNDS(pfont, rightSideBearing) - FONTMINBOUNDS(pfont, characterWidth);
    if (w > 0)
	bbox.x2 += w;
    bbox.y1 = y - FONTMAXBOUNDS(pfont,ascent);
    bbox.y2 = y + FONTMAXBOUNDS(pfont,descent);
    
    DRAW_DEBUG ((DEBUG_TEXT, "PolyGlyphBlt %d box is %d %d", nglyph,
		 bbox.x1, bbox.x2));
    switch (RECT_IN_REGION(pGC->pScreen, fbGetCompositeClip(pGC), &bbox))
    {
    case rgnIN:
	break;
    case rgnPART:
	s3PolyGlyphBltClipped(pDrawable, pGC, x - pDrawable->x,
			      y - pDrawable->y,
			      nglyph, ppciInit, pglyphBase);
    case rgnOUT:
	return;
    }
    
    if (pglyphBase == (pointer) 1)
    {
	xBack = x;
	yBack = y - FONTASCENT(pGC->font);
	wBack = 0;
	hBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
	if (hBack)
	{
	    h = nglyph;
	    ppci = ppciInit;
	    while (h--)
		wBack += (*ppci++)->metrics.characterWidth;
	}
	if (wBack < 0)
	{
	    xBack = xBack + wBack;
	    wBack = -wBack;
	}
	if (hBack < 0)
	{
	    yBack = yBack + hBack;
	    hBack = -hBack;
	}
	alu = GXcopy;
    }
    else
    {
	wBack = 0;
	alu = pGC->alu;
    }
    
    if (wBack)
    {
	_s3SetSolidFill (s3, pGC->bgPixel, GXcopy, pGC->planemask);
	_s3SolidRect (s3, xBack, yBack, wBack, hBack);
    }
    _s3SetTransparentPlaneBlt (s3, alu, pGC->planemask, pGC->fgPixel);
    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	h = pci->metrics.ascent + pci->metrics.descent;
	w = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	lw = h * ((w + 31) >> 5);
	if (lw)
	{
	    _s3PlaneBlt(s3,
			x + pci->metrics.leftSideBearing,
			y - pci->metrics.ascent,
			w, h);
	    bits = (unsigned long *) pci->bits;
	    PixTransStart(lw);
	    while (lw--) 
	    {
		b = *bits++;
		S3AdjustBits32 (b);
		PixTransStore(b);
	    }
	}
	x += pci->metrics.characterWidth;
    }
    MarkSyncS3 (pDrawable->pScreen);
}

void
s3ImageGlyphBlt (DrawablePtr pDrawable, 
		GCPtr pGC, 
		int x, int y, 
		unsigned int nglyph, 
		CharInfoPtr *ppci, 
		pointer pglyphBase)
{
    s3PolyGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, (pointer) 1);
}

/*
 * Blt TE fonts using S3 image transfer.  Differs from
 * above in that it doesn't need to fill a solid rect for
 * the background and it can draw multiple characters at a time
 */

void
s3ImageTEGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		   int xInit, int yInit,
		   unsigned int nglyph,
		   CharInfoPtr *ppci,
		   pointer pglyphBase)
{
    SetupS3(pDrawable->pScreen);
    int		    x, y;
    int		    h, lw, lwTmp;
    int		    w;
    FontPtr	    pfont = pGC->font;
    unsigned long   *char1, *char2, *char3, *char4;
    int		    widthGlyphs, widthGlyph;
    BoxRec	    bbox;
    CARD32	    tmp;
    PixTransDeclare;

    widthGlyph = FONTMAXBOUNDS(pfont,characterWidth);
    if (!widthGlyph)
	return;
    
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);
    if (!h)
	return;
    
    DRAW_DEBUG ((DEBUG_TEXT, "ImageTEGlyphBlt chars are %d %d",
		 widthGlyph, h));
    
    x = xInit + FONTMAXBOUNDS(pfont,leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    
    bbox.x1 = x;
    bbox.x2 = x + (widthGlyph * nglyph);
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch (RECT_IN_REGION(pGC->pScreen, fbGetCompositeClip(pGC), &bbox))
    {
      case rgnIN:
	break;
      case rgnPART:
	if (pglyphBase == (pointer) 1)
	    pglyphBase = 0;
	else
	    pglyphBase = (pointer) 1;
	s3PolyGlyphBltClipped(pDrawable, pGC, 
			      xInit,
			      yInit,
			      nglyph, ppci, 
			      pglyphBase);
      case rgnOUT:
	return;
    }

    if (pglyphBase == (pointer) 1)
    {
	_s3SetTransparentPlaneBlt (s3, pGC->alu, pGC->planemask, pGC->fgPixel);
    }
    else
    {
	_s3SetOpaquePlaneBlt (s3, GXcopy, pGC->planemask, pGC->fgPixel, pGC->bgPixel);
    }

#if BITMAP_BIT_ORDER == LSBFirst
#define SHIFT	<<
#else
#define SHIFT	>>
#endif
    
#define LoopIt(count, w, loadup, fetch) \
    while (nglyph >= count) \
    { \
	nglyph -= count; \
	_s3PlaneBlt (s3, x, y, w, h); \
	x += w; \
	loadup \
	lwTmp = h; \
	PixTransStart(h); \
	while (lwTmp--) { \
	    tmp = fetch; \
	    S3AdjustBits32(tmp); \
	    PixTransStore(tmp); \
	} \
    }

    if (widthGlyph <= 8)
    {
	widthGlyphs = widthGlyph << 2;
	LoopIt(4, widthGlyphs,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;
	       char3 = (unsigned long *) (*ppci++)->bits;
	       char4 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | ((*char2++ | ((*char3++ | (*char4++
						      SHIFT widthGlyph))
					 SHIFT widthGlyph))
			    SHIFT widthGlyph)))
    }
    else if (widthGlyph <= 10)
    {
	widthGlyphs = (widthGlyph << 1) + widthGlyph;
	LoopIt(3, widthGlyphs,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;
	       char3 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | ((*char2++ | (*char3++ SHIFT widthGlyph)) SHIFT widthGlyph)))
    }
    else if (widthGlyph <= 16)
    {
	widthGlyphs = widthGlyph << 1;
	LoopIt(2, widthGlyphs,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | (*char2++ SHIFT widthGlyph)))
    }
    lw = h * ((widthGlyph + 31) >> 5);
    while (nglyph--) 
    {
	_s3PlaneBlt (s3, x, y, widthGlyph, h);
	x += widthGlyph;
	char1 = (unsigned long *) (*ppci++)->bits;
	lwTmp = lw;
	PixTransStart(lw);
	while (lwTmp--)
	{
	    tmp = *char1++;
	    S3AdjustBits32(tmp);
	    PixTransStore(tmp);
	}
    }
    MarkSyncS3 (pDrawable->pScreen);
}

void
s3PolyTEGlyphBlt (DrawablePtr pDrawable, GCPtr pGC, 
		  int x, int y, 
		  unsigned int nglyph, CharInfoPtr *ppci, 
		  pointer pglyphBase)
{
    s3ImageTEGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, (pointer) 1);
}

void
_s3Segment (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		x1,
	    int		y1,
	    int		x2,
	    int		y2,
	    Bool	drawLast)
{
    SetupS3(pDrawable->pScreen);
    FbGCPrivPtr	pPriv = fbGetGCPrivate(pGC);
    RegionPtr	pClip = fbGetCompositeClip(pGC);
    BoxPtr	pBox;
    int		nBox;
    int		adx;		/* abs values of dx and dy */
    int		ady;
    int		signdx;		/* sign of dx and dy */
    int		signdy;
    int		e, e1, e2;		/* bresenham error and increments */
    int		len;			/* length of segment */
    int		axis;			/* major axis */
    int		octant;
    int		cmd;
    unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);
    unsigned int oc1;	/* outcode of point 1 */
    unsigned int oc2;	/* outcode of point 2 */

    nBox = REGION_NUM_RECTS (pClip);
    pBox = REGION_RECTS (pClip);
    CalcLineDeltas(x1, y1, x2, y2, adx, ady, signdx, signdy,
		   1, 1, octant);

    cmd = LASTPIX;
    
    if (signdx > 0)
	cmd |= INC_X;
    if (signdy > 0)
	cmd |= INC_Y;
	
    if (adx > ady)
    {
	axis = X_AXIS;
	e1 = ady << 1;
	e2 = e1 - (adx << 1);
	e = e1 - adx;
	len = adx;
    }
    else
    {
	cmd |= YMAJAXIS;
	axis = Y_AXIS;
	e1 = adx << 1;
	e2 = e1 - (ady << 1);
	e = e1 - ady;
	SetYMajorOctant(octant);
	len = ady;
    }

    FIXUP_ERROR (e, octant, bias);
    
    /* we have bresenham parameters and two points.
       all we have to do now is clip and draw.
    */

    if (drawLast)
	len++;
    while(nBox--)
    {
	oc1 = 0;
	oc2 = 0;
	OUTCODES(oc1, x1, y1, pBox);
	OUTCODES(oc2, x2, y2, pBox);
	if ((oc1 | oc2) == 0)
	{
	    _s3SetCur (s3, x1, y1);
	    _s3ClipLine (s3, cmd, e1, e2, e, len);
	    break;
	}
	else if (oc1 & oc2)
	{
	    pBox++;
	}
	else
	{
	    int new_x1 = x1, new_y1 = y1, new_x2 = x2, new_y2 = y2;
	    int clip1 = 0, clip2 = 0;
	    int clipdx, clipdy;
	    int err;
	    
	    if (miZeroClipLine(pBox->x1, pBox->y1, pBox->x2-1,
			       pBox->y2-1,
			       &new_x1, &new_y1, &new_x2, &new_y2,
			       adx, ady, &clip1, &clip2,
			       octant, bias, oc1, oc2) == -1)
	    {
		pBox++;
		continue;
	    }

	    if (axis == X_AXIS)
		len = abs(new_x2 - new_x1);
	    else
		len = abs(new_y2 - new_y1);
	    if (clip2 != 0 || drawLast)
		len++;
	    if (len)
	    {
		/* unwind bresenham error term to first point */
		err = e;
		if (clip1)
		{
		    clipdx = abs(new_x1 - x1);
		    clipdy = abs(new_y1 - y1);
		    if (axis == X_AXIS)
			err  += (e2 - e1) * clipdy + e1 * clipdx;
		    else
			err  += (e2 - e1) * clipdx + e1 * clipdy;
		}
		_s3SetCur (s3, new_x1, new_y1);
		_s3ClipLine (s3, cmd, e1, e2, err, len);
	    }
	    pBox++;
	}
    } /* while (nBox--) */
}

void
s3Polylines (DrawablePtr pDrawable, GCPtr pGC,
	     int mode, int npt, DDXPointPtr ppt)
{
    SetupS3(pDrawable->pScreen);
    int		x, y, nx, ny;
    int		ox = pDrawable->x, oy = pDrawable->y;
    
    if (!npt)
	return;
    
    _s3SetSolidFill (s3, pGC->fgPixel, pGC->alu, pGC->planemask);
    x = ppt->x + ox;
    y = ppt->y + oy;
    while (--npt)
    {
	++ppt;
	if (mode == CoordModePrevious)
	{
	    nx = x + ppt->x;
	    ny = y + ppt->y;
	}
	else
	{
	    nx = ppt->x + ox;
	    ny = ppt->y + oy;
	}
	_s3Segment (pDrawable, pGC, x, y, nx, ny,
		    npt == 1 && pGC->capStyle != CapNotLast);
	x = nx;
	y = ny;
    }
    MarkSyncS3 (pDrawable->pScreen);
}

void
s3PolySegment (DrawablePtr pDrawable, GCPtr pGC, 
	       int nsegInit, xSegment *pSegInit)
{
    SetupS3(pDrawable->pScreen);
    int		x, y;
    int		ox = pDrawable->x, oy = pDrawable->y;
    RegionPtr	pClip = fbGetCompositeClip (pGC);
    BoxPtr	pBox;
    int		nbox;
    int		nseg;
    xSegment	*pSeg;
    int		dx, dy;
    int		maj, min, len, inc;
    int		t;
    CARD32	cmd;
    CARD32	init_cmd;
    Bool	drawLast;
    
    drawLast = pGC->capStyle != CapNotLast;
    _s3SetSolidFill (s3, pGC->fgPixel, pGC->alu, pGC->planemask);
    
    for (nseg = nsegInit, pSeg = pSegInit; nseg--; pSeg++)
    {
	_s3Segment (pDrawable, pGC, pSeg->x1 + ox, pSeg->y1 + oy,
		    pSeg->x2 + ox, pSeg->y2 + oy, drawLast);
		    
    }
    MarkSyncS3 (pDrawable->pScreen);
}

/*
 * Check to see if a pattern can be painted with the S3
 */

#define _s3CheckPatternSize(s)	((s) <= S3_TILE_SIZE && ((s) & ((s) - 1)) == 0)
#define s3CheckPattern(w,h) (_s3CheckPatternSize(w) && _s3CheckPatternSize(h))
			     
Bool
s3AllocPattern (ScreenPtr pScreen,
		PixmapPtr pPixmap, 
		int xorg, int yorg,
		int fillStyle, Pixel fg, Pixel bg,
		s3PatternPtr *ppPattern)
{
    KdScreenPriv(pScreen);
    s3ScreenInfo(pScreenPriv);
    s3PatternPtr    pPattern;
    
    if (s3s->patterns.cache && fillStyle != FillSolid &&
	s3CheckPattern (pPixmap->drawable.width, pPixmap->drawable.height))
    {
	if (!(pPattern = *ppPattern))
	{
	    pPattern = (s3PatternPtr) xalloc (sizeof (s3PatternRec));
	    if (!pPattern)
		return FALSE;
	    *ppPattern = pPattern;
	}
	
	pPattern->cache = 0;
	pPattern->id = 0;
	pPattern->pPixmap = pPixmap;
	pPattern->fillStyle = fillStyle;
	pPattern->xrot = (-xorg) & (S3_TILE_SIZE-1);
	pPattern->yrot = (-yorg) & (S3_TILE_SIZE-1);
	pPattern->fore = fg;
	pPattern->back = bg;
	return TRUE;
    }
    else
    {
	if (*ppPattern)
	{
	    xfree (*ppPattern);
	    *ppPattern = 0;
	}
	return FALSE;
    }
}

void
s3CheckGCFill (GCPtr pGC)
{
    s3PrivGCPtr	    s3Priv = s3GetGCPrivate (pGC);
    PixmapPtr	    pPixmap;

    switch (pGC->fillStyle) {
    case FillSolid:
	pPixmap = 0;
	break;
    case FillOpaqueStippled:
    case FillStippled:
	pPixmap = pGC->stipple;
	break;
    case FillTiled:
	pPixmap = pGC->tile.pixmap;
	break;
    }
    s3AllocPattern (pGC->pScreen,
		    pPixmap,
		    pGC->patOrg.x + pGC->lastWinOrg.x,
		    pGC->patOrg.y + pGC->lastWinOrg.y,
		    pGC->fillStyle, pGC->fgPixel, pGC->bgPixel,
		    &s3Priv->pPattern);
}

void
s3MoveGCFill (GCPtr pGC)
{
    s3PrivGCPtr	    s3Priv = s3GetGCPrivate (pGC);
    int		    xorg, yorg;
    s3PatternPtr    pPattern;
    
    if (pPattern = s3Priv->pPattern)
    {
	/*
	 * Reset origin
	 */
	xorg = pGC->patOrg.x + pGC->lastWinOrg.x;
	yorg = pGC->patOrg.y + pGC->lastWinOrg.y;
	pPattern->xrot = (-xorg) & (S3_TILE_SIZE - 1);
	pPattern->yrot = (-yorg) & (S3_TILE_SIZE - 1);
	/*
	 * Invalidate cache entry
	 */
	pPattern->id = 0;
	pPattern->cache = 0;
    }
}

/*
 * S3 Patterns.  These are always full-depth images, stored in off-screen
 * memory.
 */

Pixel
s3FetchPatternPixel (s3PatternPtr pPattern, int x, int y)
{
    CARD8	*src;
    CARD16	*src16;
    CARD32	*src32;
    PixmapPtr	pPixmap = pPattern->pPixmap;
    
    x = (x + pPattern->xrot) % pPixmap->drawable.width;
    y = (y + pPattern->yrot) % pPixmap->drawable.height;
    src = (CARD8 *) pPixmap->devPrivate.ptr + y * pPixmap->devKind;
    switch (pPixmap->drawable.bitsPerPixel) {
    case 1:
	return (src[x>>3] >> (x & 7)) & 1 ? 0xffffffff : 0x00;
    case 4:
	if (x & 1)
	    return src[x>>1] >> 4;
	else
	    return src[x>>1] & 0xf;
    case 8:
	return src[x];
    case 16:
	src16 = (CARD16 *) src;
	return src16[x];
    case 32:
	src32 = (CARD32 *) src;
	return src32[x];
    }
}

/*
 * Place pattern image on screen; done with S3 locked
 */
void
_s3PutPattern (ScreenPtr pScreen, s3PatternPtr pPattern)
{
    SetupS3(pScreen);
    int		    x, y;
    CARD8	    *dstLine, *dst8;
    CARD16	    *dst16;
    CARD32	    *dst32;
    S3PatternCache  *cache = pPattern->cache;
    
    DRAW_DEBUG ((DEBUG_PATTERN, "_s3PutPattern 0x%x id %d to %d %d",
		pPattern, pPattern->id, cache->x, cache->y));
    
    dstLine = (pScreenPriv->screen->frameBuffer + 
	       cache->y * pScreenPriv->screen->byteStride + 
	       cache->x * pScreenPriv->bytesPerPixel);
    
    CheckSyncS3 (pScreen);
    
    for (y = 0; y < S3_TILE_SIZE; y++)
    {
	switch (pScreenPriv->screen->bitsPerPixel) {
	case 8:
	    dst8 = dstLine;
	    for (x = 0; x < S3_TILE_SIZE; x++)
		*dst8++ = s3FetchPatternPixel (pPattern, x, y);
	    DRAW_DEBUG ((DEBUG_PATTERN, "%c%c%c%c%c%c%c%c",
			 dstLine[0] ? 'X' : ' ',
			 dstLine[1] ? 'X' : ' ',
			 dstLine[2] ? 'X' : ' ',
			 dstLine[3] ? 'X' : ' ',
			 dstLine[4] ? 'X' : ' ',
			 dstLine[5] ? 'X' : ' ',
			 dstLine[6] ? 'X' : ' ',
			 dstLine[7] ? 'X' : ' '));
	    break;
	case 16:
	    dst16 = (CARD16 *) dstLine;
	    for (x = 0; x < S3_TILE_SIZE; x++)
		*dst16++ = s3FetchPatternPixel (pPattern, x, y);
	    break;
	case 32:
	    dst32 = (CARD32 *) dstLine;
	    for (x = 0; x < S3_TILE_SIZE; x++)
		*dst32++ = s3FetchPatternPixel (pPattern, x, y);
	    break;
	}
	dstLine += pScreenPriv->screen->byteStride;
    }
}

/*
 * Load a stipple to off-screen memory; done with S3 locked
 */
void
_s3LoadPattern (ScreenPtr pScreen, s3PatternPtr pPattern)
{
    SetupS3(pScreen);
    s3ScreenInfo(pScreenPriv);
    S3PatternCache  *cache;

    DRAW_DEBUG((DEBUG_PATTERN,
	       "s3LoadPattern 0x%x id %d cache 0x%x cacheid %d",
	       pPattern, pPattern->id, pPattern->cache, 
	       pPattern->cache ? pPattern->cache->id : -1));
    /*
     * Check to see if its still loaded
     */
    cache = pPattern->cache;
    if (cache && cache->id == pPattern->id)
	return;
    /*
     * Lame replacement strategy; assume we'll have plenty of room.
     */
    cache = &s3s->patterns.cache[s3s->patterns.last_used];
    if (++s3s->patterns.last_used == s3s->patterns.ncache)
	s3s->patterns.last_used = 0;
    cache->id = ++s3s->patterns.last_id;
    pPattern->id = cache->id;
    pPattern->cache = cache;
    _s3PutPattern (pScreen, pPattern);
}

void
s3DestroyGC (GCPtr pGC)
{
    s3PrivGCPtr	    s3Priv = s3GetGCPrivate (pGC);

    if (s3Priv->pPattern)
	xfree (s3Priv->pPattern);
    miDestroyGC (pGC);
}

GCFuncs	s3GCFuncs = {
    s3ValidateGC,
    miChangeGC,
    miCopyGC,
    s3DestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

int
s3CreateGC (GCPtr pGC)
{
    s3PrivGCPtr  s3Priv;
    
    if (!fbCreateGC (pGC))
	return FALSE;

    if (pGC->depth != 1)
	pGC->funcs = &s3GCFuncs;
    
    s3Priv = s3GetGCPrivate(pGC);
    s3Priv->type = DRAWABLE_PIXMAP;
    s3Priv->pPattern = 0;
    
    return TRUE;
}

Bool
s3CreateWindow (WindowPtr pWin)
{
    if (!KdCreateWindow (pWin))
	return FALSE;
    pWin->devPrivates[s3WindowPrivateIndex].ptr = 0;
    return TRUE;
}

Bool
s3DestroyWindow (WindowPtr pWin)
{
    s3PatternPtr pPattern;
    if (pPattern = s3GetWindowPrivate(pWin))
	xfree (pPattern);
    return fbDestroyWindow (pWin);
}

Bool
s3ChangeWindowAttributes (WindowPtr pWin, Mask mask)
{
    Bool	    ret;
    s3PatternPtr    pPattern;
    PixmapPtr	    pPixmap;
    int		    fillStyle;

    ret = fbChangeWindowAttributes (pWin, mask);
    if (mask & CWBackPixmap)
    {
	if (pWin->backgroundState == BackgroundPixmap)
	{
	    pPixmap = pWin->background.pixmap;
	    fillStyle = FillTiled;
	}
	else
	{
	    pPixmap = 0;
	    fillStyle = FillSolid;
	}
	pPattern = s3GetWindowPrivate(pWin);
	s3AllocPattern (pWin->drawable.pScreen, pPixmap, 
			pWin->drawable.x, pWin->drawable.y,
			fillStyle, 0, 0, &pPattern);
	DRAW_DEBUG ((DEBUG_PAINT_WINDOW, "Background pattern 0x%x pixmap 0x%x style %d",
		    pPattern, pPixmap, fillStyle));
	s3SetWindowPrivate (pWin, pPattern);
    }
    return ret;
}

void
s3PaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
    SetupS3(pWin->drawable.pScreen);
    s3PatternPtr    pPattern;

    DRAW_DEBUG ((DEBUG_PAINT_WINDOW, "s3PaintWindow 0x%x extents %d %d %d %d n %d",
		 pWin->drawable.id,
		 pRegion->extents.x1, pRegion->extents.y1,
		 pRegion->extents.x2, pRegion->extents.y2,
		 REGION_NUM_RECTS(pRegion)));
    if (!REGION_NUM_RECTS(pRegion)) 
	return;
    switch (what) {
    case PW_BACKGROUND:
	switch (pWin->backgroundState) {
	case None:
	    return;
	case ParentRelative:
	    do {
		pWin = pWin->parent;
	    } while (pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
							     what);
	    return;
	case BackgroundPixmap:
	    pPattern = s3GetWindowPrivate(pWin);
	    if (pPattern)
	    {
		s3FillBoxPattern ((DrawablePtr)pWin,
				  (int)REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion),
				  GXcopy, ~0, pPattern);
		return;
	    }
	    break;
	case BackgroundPixel:
	    s3FillBoxSolid((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel, GXcopy, ~0);
	    return;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel)
	{
	    s3FillBoxSolid((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel, GXcopy, ~0);
	    return;
	}
	break;
    }
    KdCheckPaintWindow (pWin, pRegion, what);
}

void
s3CopyWindowProc (DrawablePtr pSrcDrawable,
		  DrawablePtr pDstDrawable,
		  GCPtr       pGC,
		  BoxPtr      pbox,
		  int         nbox,
		  int         dx,
		  int         dy,
		  Bool        reverse,
		  Bool        upsidedown,
		  Pixel       bitplane,
		  void        *closure)
{
    SetupS3(pDstDrawable->pScreen);
    int	    srcX, srcY, dstX, dstY;
    int	    w, h;
    int	    flags;
    
    _s3SetBlt(s3,GXcopy,~0);
    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	flags = 0;
	if (reverse)
	{
	    dstX = pbox->x2 - 1;
	}
	else
	{
	    dstX = pbox->x1;
	    flags |= INC_X;
	}
	srcX = dstX + dx;
	
	if (upsidedown)
	{
	    dstY = pbox->y2 - 1;
	}
	else
	{
	    dstY = pbox->y1;
	    flags |= INC_Y;
	}
	srcY = dstY + dy;
	
	_s3Blt (s3, srcX, srcY, dstX, dstY, w, h, flags);
	pbox++;
    }
    MarkSyncS3 (pDstDrawable->pScreen);
}

void 
s3CopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    KdScreenPriv(pScreen);
    RegionRec	rgnDst;
    int		dx, dy;
    WindowPtr	pwinRoot;

    pwinRoot = WindowTable[pWin->drawable.pScreen->myNum];

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);

    REGION_INIT (pWin->drawable.pScreen, &rgnDst, NullBox, 0);
    
    REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

    fbCopyRegion ((DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot,
		  0,
		  &rgnDst, dx, dy, s3CopyWindowProc, 0, 0);
    
    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}

Bool
s3DrawInit (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    s3ScreenInfo(pScreenPriv);
    int		    ncache_w, ncache_h, ncache;
    int		    px, py;
    S3PatternCache  *cache;

    switch (pScreenPriv->screen->bitsPerPixel) {
    case 8:
    case 16:
    case 32:
	break;
    default:
	return FALSE;
    }
    if (serverGeneration != s3Generation)
    {
	s3GCPrivateIndex = AllocateGCPrivateIndex ();
	s3WindowPrivateIndex = AllocateWindowPrivateIndex ();
	s3Generation = serverGeneration;
    }
    if (!AllocateWindowPrivate(pScreen, s3WindowPrivateIndex, 0))
	return FALSE;
    if (!AllocateGCPrivate(pScreen, s3GCPrivateIndex, sizeof (s3PrivGCRec)))
	return FALSE;
    /*
     * Hook up asynchronous drawing
     */
    RegisterSync (pScreen);
    /*
     * Replace various fb screen functions
     */
    pScreen->CreateGC = s3CreateGC;
    pScreen->CreateWindow = s3CreateWindow;
    pScreen->ChangeWindowAttributes = s3ChangeWindowAttributes;
    pScreen->DestroyWindow = s3DestroyWindow;
    pScreen->PaintWindowBackground = s3PaintWindow;
    pScreen->PaintWindowBorder = s3PaintWindow;
    pScreen->CopyWindow = s3CopyWindow;
    
    /*
     * Initialize patterns
     */
    ncache_w = s3s->offscreen_width / S3_TILE_SIZE;
    ncache_h = s3s->offscreen_height / S3_TILE_SIZE;
    ncache = ncache_w * ncache_h;
    DRAW_DEBUG ((DEBUG_S3INIT, "ncache_w %d ncache_h %d ncache %d",
		 ncache_w, ncache_h, ncache));
    s3s->patterns.cache = (S3PatternCache *) xalloc (ncache * sizeof (S3PatternCache));
    if (s3s->patterns.cache)
    {
	DRAW_DEBUG ((DEBUG_S3INIT, "Have pattern cache"));
	s3s->patterns.ncache = ncache;
	s3s->patterns.last_used = 0;
	s3s->patterns.last_id = 0;
	cache = s3s->patterns.cache;
	for (py = 0; py < ncache_h; py++)
	    for (px = 0; px < ncache_w; px++)
	    {
		cache->id = 0;
		cache->x = s3s->offscreen_x + px * S3_TILE_SIZE;
		cache->y = s3s->offscreen_y + py * S3_TILE_SIZE;
		cache++;
	    }
    }
    return TRUE;
}

void
s3DrawEnable (ScreenPtr pScreen)
{
    SetupS3(pScreen);
    s3ScreenInfo(pScreenPriv);
    int	    c;
    
    /*
     * Flush pattern cache
     */
    for (c = 0; c < s3s->patterns.ncache; c++)
	s3s->patterns.cache[c].id = 0;
    
    _s3WaitIdleEmpty (s3);
    _s3SetScissorsTl(s3, 0, 0);
    _s3SetScissorsBr(s3, pScreenPriv->screen->width - 1, pScreenPriv->screen->height - 1);
    _s3SetSolidFill(s3, pScreen->blackPixel, GXcopy, ~0);
    _s3SolidRect (s3, 0, 0, pScreenPriv->screen->width, pScreenPriv->screen->height);
    MarkSyncS3 (pScreen);
}

void
s3DrawDisable (ScreenPtr pScreen)
{
    SetupS3 (pScreen);
    _s3WaitIdleEmpty (s3);
}

void
s3DrawFini (ScreenPtr pScreen)
{
    SetupS3(pScreen);
    s3ScreenInfo(pScreenPriv);
    
    if (s3s->patterns.cache)
    {
	xfree (s3s->patterns.cache);
	s3s->patterns.cache = 0;
	s3s->patterns.ncache = 0;
    }
}

void
s3DrawSync (ScreenPtr pScreen)
{
    SetupS3(pScreen);
    
    _s3WaitIdleEmpty(s3c->s3);
}
