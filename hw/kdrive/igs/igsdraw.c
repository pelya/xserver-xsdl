/*
 * Copyright © 2000 Keith Packard
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "igs.h"
#include "igsdraw.h"

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

CARD8 igsPatRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0xa0,         /* src AND dst */
    /* GXandReverse */      0x50,         /* src AND NOT dst */
    /* GXcopy       */      0xf0,         /* src */
    /* GXandInverted*/      0x0a,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x5a,         /* src XOR dst */
    /* GXor         */      0xfa,         /* src OR dst */
    /* GXnor        */      0x05,         /* NOT src AND NOT dst */
    /* GXequiv      */      0xa5,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xf5,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x0f,         /* NOT src */
    /* GXorInverted */      0xaf,         /* NOT src OR dst */
    /* GXnand       */      0x5f,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

/*
 * Handle pixel transfers
 */

#define BURST
#ifdef BURST
#define PixTransDeclare	VOL32	*pix_trans_base = igsc->copData,\
				*pix_trans = pix_trans_base
#define PixTransStart(n)	if (pix_trans + (n) > pix_trans_base + 16384) pix_trans = pix_trans_base
#define PixTransStore(t)	*pix_trans++ = (t)
#else
#define PixTransDeclare	VOL32	*pix_trans = igsc->copData
#define PixTransStart(n)	
#define PixTransStore(t)	*pix_trans = (t)
#endif

static IgsPattern *
igsSetPattern (ScreenPtr    pScreen,
	       PixmapPtr    pPixmap,
	       CARD8	    fillStyle,
	       INT32	    xrot,
	       INT32	    yrot)
{
    KdScreenPriv(pScreen);
    igsCardInfo(pScreenPriv);
    igsScreenInfo(pScreenPriv);
    int		i;
    IgsPatternCache *c;
    IgsPattern	*p;

    if (fillStyle == FillTiled)
	c = &igss->tile;
    else
	c = &igss->stipple;
    for (i = 0; i < IGS_NUM_PATTERN; i++)
    {
	p = &c->pattern[i];
	if (p->serial_number == pPixmap->drawable.serialNumber &&
	    p->xrot == xrot &&
	    p->yrot == yrot)
	{
	    return p;
	}
    }
    p = &c->pattern[c->next];
    if (++c->next == IGS_NUM_PATTERN)
	c->next = 0;
    p->serial_number = pPixmap->drawable.serialNumber;
    p->xrot = xrot;
    p->yrot = yrot;

    if (fillStyle != FillTiled)
    {
	FbStip	    *pix;
	FbStride    pixStride;
	int	    pixBpp;
	int	    pixXoff, pixYoff;
	CARD8	    tmp[8];
	CARD32	    *pat;
	int	    stipX, stipY;
	int	    y;
	FbStip	    bits;

	fbGetStipDrawable (&pPixmap->drawable, pix, pixStride, pixBpp, pixXoff, pixYoff);
	
	modulus (-yrot - pixYoff, pPixmap->drawable.height, stipY);
	modulus (-xrot - pixXoff, FB_UNIT, stipX);

	pat = (CARD32 *) p->base;

	for (y = 0; y < 8; y++)
	{
	    bits = pix[stipY * pixStride];
	    FbRotLeft (bits, stipX);
	    tmp[y] = (CARD8) bits;
	    stipY++;
	    if (stipY == pPixmap->drawable.height)
		stipY = 0;
	}
	for (i = 0; i < 2; i++)
	{
	    bits = (tmp[i*4+0] | 
		      (tmp[i*4+1] << 8) | 
		      (tmp[i*4+2] << 16) |
		      (tmp[i*4+3] << 24));
	    IgsAdjustBits32 (bits);
	    *pat++ = bits;
	}
    }
    else
    {
	FbBits	    *pix;
	FbStride    pixStride;
	int	    pixBpp;
	FbBits	    *pat;
	FbStride    patStride;
	int	    patBpp;
	int	    patXoff, patYoff;
	
	fbGetDrawable (&pPixmap->drawable, pix, pixStride, pixBpp, patXoff, patYoff);

	pat = (FbBits *) p->base;
	patBpp = pixBpp;
	patStride = (patBpp * IGS_PATTERN_WIDTH) / (8 * sizeof (FbBits));

	fbTile (pat, patStride, 0,
		patBpp * IGS_PATTERN_WIDTH, IGS_PATTERN_HEIGHT,

		pix, pixStride, 
		pPixmap->drawable.width * pixBpp,
		pPixmap->drawable.height,
		GXcopy, FB_ALLONES, pixBpp,
		(xrot - patXoff) * pixBpp, yrot - patYoff);
    }
    return p;
}

void
igsFillBoxSolid (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		 unsigned long pixel, int alu, unsigned long planemask)
{
    SetupIgs(pDrawable->pScreen);
    CARD32	cmd;

    _igsSetSolidRect(cop,alu,planemask,pixel,cmd);
    while (nBox--) 
    {
	_igsRect(cop,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1,cmd);
	pBox++;
    }
    KdMarkSync (pDrawable->pScreen);
}

void
igsFillBoxTiled (DrawablePtr pDrawable, int nBox, BoxPtr pBox,
		 PixmapPtr pPixmap, int xrot, int yrot, int alu)
{
    SetupIgs(pDrawable->pScreen);
    CARD32      cmd;
    IgsPattern	*p = igsSetPattern (pDrawable->pScreen,
				    pPixmap,
				    FillTiled,
				    xrot, yrot);
    
    _igsSetTiledRect(cop,alu,planemask,p->offset,cmd);
    while (nBox--) 
    {
	_igsPatRect(cop,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1,cmd);
	pBox++;
    }
    KdMarkSync (pDrawable->pScreen);
}

void
igsFillBoxStippled (DrawablePtr pDrawable, GCPtr pGC,
		    int nBox, BoxPtr pBox)
{
    SetupIgs(pDrawable->pScreen);
    CARD32      cmd;
    int		xrot = pGC->patOrg.x + pDrawable->x;
    int		yrot = pGC->patOrg.y + pDrawable->y;
    IgsPattern	*p = igsSetPattern (pDrawable->pScreen,
				    pGC->stipple,
				    pGC->fillStyle,
				    xrot, yrot);
    if (pGC->fillStyle == FillStippled)
    {
	_igsSetStippledRect (cop,pGC->alu,planemask,pGC->fgPixel,p->offset,cmd);
    }
    else
    {
	_igsSetOpaqueStippledRect (cop,pGC->alu,planemask,
				   pGC->fgPixel,pGC->bgPixel,p->offset,cmd);
    }
    while (nBox--) 
    {
	_igsPatRect(cop,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1,cmd);
	pBox++;
    }
    KdMarkSync (pDrawable->pScreen);
}


void
igsStipple (ScreenPtr	pScreen,
	    CARD32	cmd,
	    FbStip	*psrcBase,
	    FbStride	widthSrc,
	    int		srcx,
	    int		srcy,
	    int		dstx,
	    int		dsty,
	    int		width,
	    int		height)
{
    SetupIgs(pScreen);
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
    
    _igsPlaneBlt(cop,dstx,dsty,width,height,cmd);
    
    if (leftShift == 0)
    {
	while (height--)
	{
	    nl = nlMiddle;
	    PixTransStart(nl);
	    while (nl--)
	    {
		tmp = *psrc++;
		IgsAdjustBits32 (tmp);
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
		IgsAdjustBits32(tmp);
		PixTransStore (tmp);
	    }
	    psrc += widthRest;
	}
    }
}
	    
void
igsCopyNtoN (DrawablePtr	pSrcDrawable,
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
    SetupIgs(pDstDrawable->pScreen);
    int	    srcX, srcY, dstX, dstY;
    int	    w, h;
    CARD32  flags;
    CARD32  cmd;
    CARD8   alu;
    
    if (pGC)
    {
	alu = pGC->alu;
	if (sourceInvarient (pGC->alu))
	{
	    igsFillBoxSolid (pDstDrawable, nbox, pbox, 0, pGC->alu, pGC->planemask);
	    return;
	}
    }
    else
	alu = GXcopy;
    
    _igsSetBlt(cop,alu,pGC->planemask,reverse,upsidedown,cmd);
    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	if (reverse)
	    dstX = pbox->x2 - 1;
	else
	    dstX = pbox->x1;
	srcX = dstX + dx;
	
	if (upsidedown)
	    dstY = pbox->y2 - 1;
	else
	    dstY = pbox->y1;
	
	srcY = dstY + dy;
	
	_igsBlt (cop, srcX, srcY, dstX, dstY, w, h, cmd);
	pbox++;
    }
    KdMarkSync (pDstDrawable->pScreen);
}

RegionPtr
igsCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	   int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    KdScreenPriv(pDstDrawable->pScreen);
    FbBits	    depthMask;
    
    depthMask = FbFullMask (pDstDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pSrcDrawable->type == DRAWABLE_WINDOW &&
	pDstDrawable->type == DRAWABLE_WINDOW)
    {
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, igsCopyNtoN, 0, 0);
    }
    return KdCheckCopyArea (pSrcDrawable, pDstDrawable, pGC, 
		       srcx, srcy, width, height, dstx, dsty);
}

typedef struct _igs1toNargs {
    unsigned long	copyPlaneFG, copyPlaneBG;
    Bool		opaque;
} igs1toNargs;

void
igsCopy1toN (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     BoxPtr	    pbox,
	     int	    nbox,
	     int	    dx,
	     int	    dy,
	     Bool	    reverse,
	     Bool	    upsidedown,
	     Pixel	    bitplane,
	     void	    *closure)
{
    SetupIgs(pDstDrawable->pScreen);
    
    igs1toNargs		*args = closure;
    int			dstx, dsty;
    FbStip		*psrcBase;
    FbStride		widthSrc;
    int			srcBpp;
    int			srcXoff, srcYoff;
    CARD32		cmd;

    if (args->opaque && sourceInvarient (pGC->alu))
    {
	igsFillBoxSolid (pDstDrawable, nbox, pbox,
			 pGC->bgPixel, pGC->alu, pGC->planemask);
	return;
    }
    
    fbGetStipDrawable (pSrcDrawable, psrcBase, widthSrc, srcBpp, srcXoff, srcYoff);
    
    if (args->opaque)
    {
	_igsSetOpaquePlaneBlt (cop, pGC->alu, pGC->planemask, args->copyPlaneFG,
			       args->copyPlaneBG, cmd);
    }
    else
    {
	_igsSetTransparentPlaneBlt (cop, pGC->alu, pGC->planemask,
				    args->copyPlaneFG, cmd);
    }
    
    while (nbox--)
    {
	dstx = pbox->x1;
	dsty = pbox->y1;
	
	igsStipple (pDstDrawable->pScreen, cmd,
		    psrcBase, widthSrc, 
		    dstx + dx - srcXoff, dsty + dy - srcYoff,
		    dstx, dsty, 
		    pbox->x2 - dstx, pbox->y2 - dsty);
	pbox++;
    }
    KdMarkSync (pDstDrawable->pScreen);
}

RegionPtr
igsCopyPlane (DrawablePtr   pSrcDrawable,
	      DrawablePtr   pDstDrawable,
	      GCPtr	    pGC,
	      int	    srcx,
	      int	    srcy,
	      int	    width,
	      int	    height, 
	      int	    dstx,
	      int	    dsty,
	      unsigned long bitPlane)
{
    RegionPtr	    ret;
    igs1toNargs	    args;
    FbBits	    depthMask;

    depthMask = FbFullMask (pDstDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pDstDrawable->type == DRAWABLE_WINDOW &&
	pSrcDrawable->depth == 1)
    {
	args.copyPlaneFG = pGC->fgPixel;
	args.copyPlaneBG = pGC->bgPixel;
	args.opaque = TRUE;
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, igsCopy1toN, bitPlane, &args);
    }
    return KdCheckCopyPlane(pSrcDrawable, pDstDrawable, pGC, 
			    srcx, srcy, width, height, 
			    dstx, dsty, bitPlane);
}

#if 0
/* would you believe this is slower than fb? */
void
igsPushPixels (GCPtr	    pGC,
	       PixmapPtr    pBitmap,
	       DrawablePtr  pDrawable,
	       int	    w,
	       int	    h,
	       int	    x,
	       int	    y)
{
    igs1toNargs		args;
    FbBits	    depthMask;

    depthMask = FbFullMask (pDstDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pDrawable->type == DRAWABLE_WINDOW && 
	pGC->fillStyle == FillSolid)
    {
	args.opaque = FALSE;
	args.copyPlaneFG = pGC->fgPixel;
	(void) fbDoCopy ((DrawablePtr) pBitmap, pDrawable, pGC,
			  0, 0, w, h, x, y, igsCopy1toN, 1, &args);
    }
    else
    {
	KdCheckPushPixels (pGC, pBitmap, pDrawable, w, h, x, y);
    }
}
#else
#define igsPushPixels KdCheckPushPixels
#endif

BOOL
igsFillOk (GCPtr pGC)
{
    FbBits  depthMask;

    depthMask = FbFullMask(pGC->depth);
    if ((pGC->planemask & depthMask) != depthMask)
	return FALSE;
    switch (pGC->fillStyle) {
    case FillSolid:
	return TRUE;
    case FillTiled:
	return (igsPatternDimOk (pGC->tile.pixmap->drawable.width) &&
		igsPatternDimOk (pGC->tile.pixmap->drawable.height));
    case FillStippled:
    case FillOpaqueStippled:
	return (igsPatternDimOk (pGC->stipple->drawable.width) &&
		igsPatternDimOk (pGC->stipple->drawable.height));
    }
    return FALSE;
}

void
igsFillSpans (DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    SetupIgs(pDrawable->pScreen);
    DDXPointPtr	    pptFree;
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate(pGC);
    int		    *pwidthFree;/* copies of the pointers to free */
    CARD32	    cmd;
    int		    nTmp;
    INT16	    x, y;
    int		    width;
    IgsPattern	    *p;
    
    if (!igsFillOk (pGC))
    {
	KdCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
	return;
    }
    nTmp = n * miFindMaxBand(fbGetCompositeClip(pGC));
    pwidthFree = (int *)xalloc(nTmp * sizeof(int));
    pptFree = (DDXPointRec *)xalloc(nTmp * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) xfree(pptFree);
	if (pwidthFree) xfree(pwidthFree);
	return;
    }
    n = miClipSpans(fbGetCompositeClip(pGC),
		     ppt, pwidth, n,
		     pptFree, pwidthFree, fSorted);
    pwidth = pwidthFree;
    ppt = pptFree;
    switch (pGC->fillStyle) {
    case FillSolid:
	_igsSetSolidRect(cop,pGC->alu,pGC->planemask,pGC->fgPixel,cmd);
	break;
    case FillTiled:
	p = igsSetPattern (pDrawable->pScreen,
			   pGC->tile.pixmap,
			   FillTiled,
			   pGC->patOrg.x + pDrawable->x,
			   pGC->patOrg.y + pDrawable->y);
	_igsSetTiledRect (cop,pGC->alu,pGC->planemask,p->offset,cmd);
	break;
    default:
	p = igsSetPattern (pDrawable->pScreen,
			   pGC->stipple,
			   pGC->fillStyle,
			   pGC->patOrg.x + pDrawable->x,
			   pGC->patOrg.y + pDrawable->y);
	if (pGC->fillStyle == FillStippled)
	{
	    _igsSetStippledRect (cop,pGC->alu,pGC->planemask,
				 pGC->fgPixel,p->offset,cmd);
	}
	else
	{
	    _igsSetOpaqueStippledRect (cop,pGC->alu,pGC->planemask,
				       pGC->fgPixel,pGC->bgPixel,p->offset,cmd);
	}
	break;
    }
    while (n--)
    {
	x = ppt->x;
	y = ppt->y;
	ppt++;
	width = *pwidth++;
	if (width)
	{
	    _igsPatRect(cop,x,y,width,1,cmd);
	}
    }
    xfree(pptFree);
    xfree(pwidthFree);
    KdMarkSync (pDrawable->pScreen);
}

#define NUM_STACK_RECTS	1024

void
igsPolyFillRect (DrawablePtr pDrawable, GCPtr pGC, 
		int nrectFill, xRectangle *prectInit)
{
    SetupIgs(pDrawable->pScreen);
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
    
    if (!igsFillOk (pGC))
    {
	KdCheckPolyFillRect (pDrawable, pGC, nrectFill, prectInit);
	return;
    }
    prgnClip = fbGetCompositeClip (pGC);
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
	pboxClippedBase = (BoxPtr)xalloc(numRects * sizeof(BoxRec));
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
	switch (pGC->fillStyle) {
	case FillSolid:
	    igsFillBoxSolid(pDrawable,
			   pboxClipped-pboxClippedBase, pboxClippedBase,
			   pGC->fgPixel, pGC->alu, pGC->planemask);
	    break;
	case FillTiled:
	    igsFillBoxTiled(pDrawable,
			    pboxClipped-pboxClippedBase, pboxClippedBase,
			    pGC->tile.pixmap,
			    pGC->patOrg.x + pDrawable->x,
			    pGC->patOrg.y + pDrawable->y,
			    pGC->alu);
	    break;
	case FillStippled:
	case FillOpaqueStippled:
	    igsFillBoxStippled (pDrawable, pGC,
				pboxClipped-pboxClippedBase, pboxClippedBase);
	    break;
	}
    }
    if (pboxClippedBase != stackRects)
    	xfree(pboxClippedBase);
}

int
igsTextInRegion (GCPtr		pGC,
		 int		x,
		 int		y,
		 unsigned int	nglyph,
		 CharInfoPtr	*ppci)
{
    int		    w;
    FontPtr	    pfont = pGC->font;
    BoxRec	    bbox;
    
    if (FONTCONSTMETRICS(pfont))
	w = FONTMAXBOUNDS(pfont,characterWidth) * nglyph;
    else
    {
	w = 0;
	while (nglyph--)
	    w += (*ppci++)->metrics.characterWidth;
    }
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
    
    return RECT_IN_REGION(pGC->pScreen, fbGetCompositeClip(pGC), &bbox);
}
		 
void
igsGlyphBltClipped (DrawablePtr	    pDrawable,
		    GCPtr	    pGC,
		    int		    x,
		    int		    y,
		    unsigned int    nglyph,
		    CharInfoPtr	    *ppciInit,
		    Bool	    image)
{
    SetupIgs(pDrawable->pScreen);
    CARD32	    cmd;
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

    if (image)
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
	if (wBack)
	{
	    _igsSetSolidRect (cop, GXcopy, pGC->planemask, pGC->bgPixel, cmd);
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
		    _igsRect (cop, x1, y1, x2 - x1, y2 - y1, cmd);
		}
	    }
	    KdMarkSync (pDrawable->pScreen);
	}
    }
    else
    {
	wBack = 0;
	alu = pGC->alu;
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
	    lw = h * ((w + 31) >> 5);
	    if (lw)
	    {
		if (!set)
		{
		    _igsSetTransparentPlaneBlt (cop, alu, pGC->planemask, pGC->fgPixel, cmd);
		    set = TRUE;
		}
		_igsPlaneBlt(cop,
			     x + pci->metrics.leftSideBearing,
			     y - pci->metrics.ascent,
			     w, h, cmd);
		bits = (unsigned long *) pci->bits;
		PixTransStart (lw);
		while (lw--) 
		{
		    b = *bits++;
		    IgsAdjustBits32 (b);
		    PixTransStore(b);
		}
		KdMarkSync (pDrawable->pScreen);
	    }
	    break;
	case rgnPART:
	    set = FALSE;
	    KdCheckSync (pDrawable->pScreen);
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
		       
void
igsGlyphBlt (DrawablePtr    pDrawable, 
	     GCPtr	    pGC, 
	     int	    x,
	     int	    y, 
	     unsigned int   nglyph,
	     CharInfoPtr    *ppciInit,
	     Bool	    image)
{
    SetupIgs(pDrawable->pScreen);
    CARD32	    cmd;
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

    /*
     * Paint background for image text
     */
    if (image)
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
	if (wBack)
	{
	    _igsSetSolidRect (cop, GXcopy, pGC->planemask, pGC->bgPixel, cmd);
	    _igsRect (cop, xBack, yBack, wBack, hBack, cmd);
	}
    }
    else
    {
	wBack = 0;
	alu = pGC->alu;
    }
    
    _igsSetTransparentPlaneBlt (cop, alu, pGC->planemask, pGC->fgPixel, cmd);
    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	h = pci->metrics.ascent + pci->metrics.descent;
	w = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	lw = h * ((w + 31) >> 5);
	if (lw)
	{
	    _igsPlaneBlt(cop,
			 x + pci->metrics.leftSideBearing,
			 y - pci->metrics.ascent,
			 w, h, cmd);
	    bits = (unsigned long *) pci->bits;
	    PixTransStart(lw);
	    while (lw--) 
	    {
		b = *bits++;
		IgsAdjustBits32 (b);
		PixTransStore(b);
	    }
	}
	x += pci->metrics.characterWidth;
    }
    KdMarkSync (pDrawable->pScreen);
}

void
igsTEGlyphBlt (DrawablePtr  pDrawable,
	       GCPtr	    pGC,
	       int	    xInit,
	       int	    yInit,
	       unsigned int nglyph,
	       CharInfoPtr  *ppci,
	       Bool	    image)
{
    SetupIgs(pDrawable->pScreen);
    CARD32	    cmd;
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
    
    x = xInit + FONTMAXBOUNDS(pfont,leftSideBearing);
    y = yInit - FONTASCENT(pfont);
    
    if (image)
    {
	_igsSetOpaquePlaneBlt (cop, GXcopy, pGC->planemask, pGC->fgPixel, pGC->bgPixel, cmd);
    }
    else
    {
	_igsSetTransparentPlaneBlt (cop, pGC->alu, pGC->planemask, pGC->fgPixel, cmd);
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
	_igsPlaneBlt (cop, x, y, w, h, cmd); \
	x += w; \
	loadup \
	lwTmp = h; \
	PixTransStart(h); \
	while (lwTmp--) { \
	    tmp = fetch; \
	    IgsAdjustBits32(tmp); \
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
	_igsPlaneBlt (cop, x, y, widthGlyph, h, cmd);
	x += widthGlyph;
	char1 = (unsigned long *) (*ppci++)->bits;
	lwTmp = lw;
	PixTransStart(lw);
	while (lwTmp--)
	{
	    tmp = *char1++;
	    IgsAdjustBits32(tmp);
	    PixTransStore(tmp);
	}
    }
    KdMarkSync (pDrawable->pScreen);
}

/*
 * Blt glyphs using image transfer window
 */

void
igsPolyGlyphBlt (DrawablePtr	pDrawable, 
		 GCPtr		pGC, 
		 int		x,
		 int		y, 
		 unsigned int	nglyph,
		 CharInfoPtr	*ppci, 
		 pointer	pglyphBase)
{
    if (pGC->fillStyle != FillSolid ||
	fbGetGCPrivate(pGC)->pm != FB_ALLONES)
    {
	KdCheckPolyGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	return;
    }
    x += pDrawable->x;
    y += pDrawable->y;

    switch (igsTextInRegion (pGC, x, y, nglyph, ppci)) {
    case rgnIN:
	if (TERMINALFONT(pGC->font))
	    igsTEGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, FALSE);
	else
	    igsGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, FALSE);
	break;
    case rgnPART:
	igsGlyphBltClipped (pDrawable, pGC, x, y, nglyph, ppci, FALSE);
	break;
    case rgnOUT:
	break;
    }
}

void
igsImageGlyphBlt (DrawablePtr pDrawable, 
		GCPtr pGC, 
		int x, int y, 
		unsigned int nglyph, 
		CharInfoPtr *ppci, 
		pointer pglyphBase)
{
    if (fbGetGCPrivate(pGC)->pm != FB_ALLONES)
    {
	KdCheckImageGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	return;
    }
    x += pDrawable->x;
    y += pDrawable->y;

    switch (igsTextInRegion (pGC, x, y, nglyph, ppci)) {
    case rgnIN:
	if (TERMINALFONT(pGC->font))
	    igsTEGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, TRUE);
	else
	    igsGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, TRUE);
	break;
    case rgnPART:
	igsGlyphBltClipped (pDrawable, pGC, x, y, nglyph, ppci, TRUE);
	break;
    case rgnOUT:
	break;
    }
}

static void
igsInvalidatePattern (IgsPatternCache	*c,
		      PixmapPtr		pPixmap)
{
    int i;

    if (c->base)
    {
	for (i = 0; i < IGS_NUM_PATTERN; i++)
	{
	    if (c->pattern[i].serial_number == pPixmap->drawable.serialNumber)
		c->pattern[i].serial_number = ~0;
	}
    }    
}

static void
igsInitPattern (IgsPatternCache *c, int bsize, int psize)
{
    int	    i;
    int	    boffset;
    int	    poffset;

    for (i = 0; i < IGS_NUM_PATTERN; i++)
    {
	boffset = i * bsize;
	poffset = i * psize;
	c->pattern[i].xrot = -1;
	c->pattern[i].yrot = -1;
	c->pattern[i].serial_number = ~0;
	c->pattern[i].offset = c->offset + poffset;
	c->pattern[i].base = c->base + boffset;
    }
    c->next = 0;
}
	
static const GCOps	igsOps = {
    igsFillSpans,
    KdCheckSetSpans,
    KdCheckPutImage,
    igsCopyArea,
    igsCopyPlane,
    KdCheckPolyPoint,
    KdCheckPolylines,
    KdCheckPolySegment,
    miPolyRectangle,
    KdCheckPolyArc,
    miFillPolygon,
    igsPolyFillRect,
    KdCheckPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    igsImageGlyphBlt,
    igsPolyGlyphBlt,
    igsPushPixels,
};

void
igsValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable)
{
    FbGCPrivPtr fbPriv = fbGetGCPrivate(pGC);
    
    fbValidateGC (pGC, changes, pDrawable);
    
    if (pDrawable->type == DRAWABLE_WINDOW)
	pGC->ops = (GCOps *) &igsOps;
    else
	pGC->ops = (GCOps *) &fbGCOps;
}

GCFuncs	igsGCFuncs = {
    igsValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

int
igsCreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    if (pGC->depth != 1)
	pGC->funcs = &igsGCFuncs;
    
    return TRUE;
}

void
igsCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
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
		  &rgnDst, dx, dy, igsCopyNtoN, 0, 0);
    
    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}


Bool
igsDrawInit (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    igsCardInfo(pScreenPriv);
    igsScreenInfo(pScreenPriv);
    int i;
    int pattern_size;
    int boffset, poffset;
    
    KdScreenInitAsync (pScreen);
    
    /*
     * Replace various fb screen functions
     */
    pScreen->CreateGC = igsCreateGC;
    pScreen->CopyWindow = igsCopyWindow;

    /*
     * Initialize patterns
     */
    if (igss->tile.base)
    {
	pattern_size = IgsTileSize(pScreenPriv->screen->fb[0].bitsPerPixel);
	igsInitPattern (&igss->tile,
			pattern_size,
			pattern_size * 8 / pScreenPriv->screen->fb[0].bitsPerPixel);
	pattern_size = IgsStippleSize(pScreenPriv->screen->fb[0].bitsPerPixel);
	igsInitPattern (&igss->stipple,
			pattern_size,
			pattern_size * 8 / pScreenPriv->screen->fb[0].bitsPerPixel);
    }
    return TRUE;
}

void
igsDrawEnable (ScreenPtr pScreen)
{
    SetupIgs(pScreen);
    CARD32  cmd;
    CARD32  base;
    CARD16  stride;
    CARD32  format;
    
    stride = pScreenPriv->screen->fb[0].pixelStride;
    _igsWaitIdleEmpty(cop);
    _igsReset(cop);
    switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
    case 8:
	format = IGS_FORMAT_8BPP;
	break;
    case 16:
	format = IGS_FORMAT_16BPP;
	break;
    case 24:
	format = IGS_FORMAT_24BPP;
	break;
    case 32:
	format = IGS_FORMAT_32BPP;
	break;
    }
    cop->format = format;
    cop->dst_stride = stride - 1;
    cop->src1_stride = stride - 1;
    cop->src2_stride = stride - 1;
    cop->src1_start = 0;
    cop->src2_start = 0;
    cop->extension |= IGS_BLOCK_COP_REG | IGS_BURST_ENABLE;
    
    _igsSetSolidRect(cop, GXcopy, ~0, pScreen->blackPixel, cmd);
    _igsRect (cop, 0, 0, 
	      pScreenPriv->screen->width, pScreenPriv->screen->height,
	      cmd);
    _igsWaitIdleEmpty (cop);
}

void
igsDrawDisable (ScreenPtr pScreen)
{
}

void
igsDrawFini (ScreenPtr pScreen)
{
}

void
igsDrawSync (ScreenPtr pScreen)
{
    SetupIgs(pScreen);

    _igsWaitIdleEmpty(cop);
}
