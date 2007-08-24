/*
 * Copyright © 1999 Keith Packard
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
#include "trident.h"
#include "tridentdraw.h"

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
#include	"picturestr.h"

CARD8 tridentRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0x88,         /* src AND dst */
    /* GXandReverse */      0x44,         /* src AND NOT dst */
    /* GXcopy       */      0xcc,         /* src */
    /* GXandInverted*/      0x22,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x66,         /* src XOR dst */
    /* GXor         */      0xee,         /* src OR dst */
    /* GXnor        */      0x11,         /* NOT src AND NOT dst */
    /* GXequiv      */      0x99,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xdd,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x33,         /* NOT src */
    /* GXorInverted */      0xbb,         /* NOT src OR dst */
    /* GXnand       */      0x77,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

#define tridentFillPix(bpp,pixel) {\
    if (bpp == 8) \
    { \
	pixel = pixel & 0xff; \
	pixel = pixel | pixel << 8; \
    } \
    if (bpp <= 16) \
    { \
	pixel = pixel & 0xffff; \
	pixel = pixel | pixel << 16; \
    } \
}

static Cop	*cop;
static CARD32	cmd;

Bool
tridentPrepareSolid (DrawablePtr    pDrawable,
		     int	    alu,
		     Pixel	    pm,
		     Pixel	    fg)
{
    FbBits  depthMask = FbFullMask(pDrawable->depth);
    
    if ((pm & depthMask) != depthMask)
	return FALSE;
    else
    {
	KdScreenPriv(pDrawable->pScreen);
	tridentCardInfo(pScreenPriv);
	cop = tridentc->cop;
	
	tridentFillPix(pDrawable->bitsPerPixel,fg);
	_tridentInit(cop,tridentc);
	_tridentSetSolidRect(cop,fg,alu,cmd);
	return TRUE;
    }
}

void
tridentSolid (int x1, int y1, int x2, int y2)
{
    _tridentRect (cop, x1, y1, x2 - 1, y2 - 1, cmd);
}

void
tridentDoneSolid (void)
{
}

Bool
tridentPrepareCopy (DrawablePtr	pSrcDrawable,
		    DrawablePtr	pDstDrawable,
		    int		dx,
		    int		dy,
		    int		alu,
		    Pixel	pm)
{
    FbBits  depthMask = FbFullMask(pDstDrawable->depth);
    
    if ((pm & depthMask) == depthMask)
    {
	KdScreenPriv(pDstDrawable->pScreen);
	tridentCardInfo(pScreenPriv);
	cop = tridentc->cop;
	_tridentInit(cop,tridentc);
	cop->multi = COP_MULTI_PATTERN;
	cop->multi = COP_MULTI_ROP | tridentRop[alu];
	cmd = COP_OP_BLT | COP_SCL_OPAQUE | COP_OP_ROP | COP_OP_FB;
	if (dx < 0 || dy < 0)
	    cmd |= COP_X_REVERSE;
	return TRUE;
    }
    else
	return FALSE;
}

void
tridentCopy (int srcX,
	     int srcY,
	     int dstX,
	     int dstY,
	     int w,
	     int h)
{
    if (cmd & COP_X_REVERSE)
    {
	cop->src_start_xy = TRI_XY (srcX + w - 1, srcY + h - 1);
	cop->src_end_xy   = TRI_XY (srcX, srcY);
	cop->dst_start_xy = TRI_XY (dstX + w - 1, dstY + h - 1);
	cop->dst_end_xy   = TRI_XY (dstX, dstY);
    }
    else
    {
	cop->src_start_xy   = TRI_XY (srcX, srcY);
	cop->src_end_xy	    = TRI_XY (srcX + w - 1, srcY + h - 1);
	cop->dst_start_xy   = TRI_XY (dstX, dstY);
	cop->dst_end_xy	    = TRI_XY (dstX + w - 1, dstY + h - 1);
    }
    _tridentWaitDone (cop);
    cop->command = cmd;
}

void
tridentDoneCopy (void)
{
}

void
tridentComposite (CARD8      op,
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
    SetupTrident (pDst->pDrawable->pScreen);
    tridentScreenInfo(pScreenPriv);
    RegionRec	region;
    int		n;
    BoxPtr	pbox;
    CARD32	rgb;
    CARD8	*msk, *mskLine;
    FbBits	*mskBits;
    FbStride	mskStride;
    int		mskBpp;
    int		mskXoff, mskYoff;
    CARD32	*src, *srcLine;
    CARD32	*off, *offLine;
    FbBits	*srcBits;
    FbStride	srcStride;
    int		srcXoff, srcYoff;
    FbStride	offStride;
    int		srcBpp;
    int		x_msk, y_msk, x_src, y_src, x_dst, y_dst;
    int		x2;
    int		w, h, w_this, h_this, w_remain;
    CARD32	*off_screen;
    int		off_size = tridents->off_screen_size >> 2;
    int		off_width, off_height;
    int		stride = pScreenPriv->screen->fb[0].pixelStride;
    int		mskExtra;
    CARD32	off_screen_offset = tridents->off_screen - tridents->screen;
    int		mode;
    
#define MODE_NONE   0
#define MODE_IMAGE  1
#define MODE_MASK   2
    
    rgb = *((CARD32 *) ((PixmapPtr) (pSrc->pDrawable))->devPrivate.ptr);
    if (pMask && 
	!pMask->repeat &&
	pMask->format == PICT_a8 &&
        op == PictOpOver &&
	pSrc->repeat &&
	pSrc->pDrawable->width == 1 &&
	pSrc->pDrawable->height == 1 &&
	PICT_FORMAT_BPP(pSrc->format) == 32 &&
	(PICT_FORMAT_A(pSrc->format) == 0 ||
	 (rgb & 0xff000000) == 0xff000000) &&
	pDst->pDrawable->bitsPerPixel == 32 &&
	pDst->pDrawable->type == DRAWABLE_WINDOW)
    {
	mode = MODE_MASK;
    }
    else if (!pMask &&
	     op == PictOpOver &&
	     !pSrc->repeat &&
	     PICT_FORMAT_A(pSrc->format) == 8 &&
	     PICT_FORMAT_BPP(pSrc->format) == 32 &&
	     pDst->pDrawable->bitsPerPixel == 32 &&
	     pDst->pDrawable->type == DRAWABLE_WINDOW)
    {
	mode = MODE_IMAGE;
    }
    else
	mode = MODE_NONE;
    
    if (mode != MODE_NONE)
    {
	xDst += pDst->pDrawable->x;
	yDst += pDst->pDrawable->y;
	xSrc += pSrc->pDrawable->x;
	ySrc += pSrc->pDrawable->y;
	
	fbGetDrawable (pSrc->pDrawable, srcBits, srcStride, srcBpp, srcXoff, srcYoff);
	
	if (pMask)
	{
	    xMask += pMask->pDrawable->x;
	    yMask += pMask->pDrawable->y;
	    fbGetDrawable (pMask->pDrawable, mskBits, mskStride, mskBpp, mskXoff, mskYoff);
	    mskStride = mskStride * sizeof (FbBits) / sizeof (CARD8);
	}
	
	if (!miComputeCompositeRegion (&region,
				       pSrc,
				       pMask,
				       pDst,
				       xSrc,
				       ySrc,
				       xMask,
				       yMask,
				       xDst,
				       yDst,
				       width,
				       height))
	    return;
	
	_tridentInit(cop,tridentc);
	
	cop->multi = COP_MULTI_PATTERN;
	cop->src_offset = off_screen_offset;
	
	if (mode == MODE_IMAGE)
	{
	    cop->multi = (COP_MULTI_ALPHA |
			  COP_ALPHA_BLEND_ENABLE |
			  COP_ALPHA_WRITE_ENABLE |
			  0x7 << 16 |
			  COP_ALPHA_DST_BLEND_1_SRC_A |
			  COP_ALPHA_SRC_BLEND_1);
	}
	else
	{
	    rgb &= 0xffffff;
	    cop->multi = (COP_MULTI_ALPHA |
			  COP_ALPHA_BLEND_ENABLE |
			  COP_ALPHA_WRITE_ENABLE |
			  0x7 << 16 |
			  COP_ALPHA_DST_BLEND_1_SRC_A |
			  COP_ALPHA_SRC_BLEND_SRC_A);
	}
	
	n = REGION_NUM_RECTS (&region);
	pbox = REGION_RECTS (&region);
	
	while (n--)
	{
	    h = pbox->y2 - pbox->y1;
	    w = pbox->x2 - pbox->x1;
	    
	    offStride = (w + 7) & ~7;
	    off_height = off_size / offStride;
	    if (off_height > h)
		off_height = h;
	    
	    cop->multi = COP_MULTI_STRIDE | (stride << 16) | offStride;
	    
	    y_dst = pbox->y1;
	    y_src = y_dst - yDst + ySrc;
	    y_msk = y_dst - yDst + yMask;
	    
	    x_dst = pbox->x1;
	    x_src = x_dst - xDst + xSrc;
	    x_msk = x_dst - xDst + xMask;
	
	    if (mode == MODE_IMAGE)
		srcLine = (CARD32 *) srcBits + (y_src - srcYoff) * srcStride + (x_src - srcXoff);
	    else
		mskLine = (CARD8 *) mskBits + (y_msk - mskYoff) * mskStride + (x_msk - mskXoff);

	    while (h)
	    {
		h_this = h;
		if (h_this > off_height)
		    h_this = off_height;
		h -= h_this;
		
		offLine = (CARD32 *) tridents->off_screen;
		
		_tridentWaitDone(cop);
		
		cop->dst_start_xy = TRI_XY(x_dst, y_dst);
		cop->dst_end_xy = TRI_XY(x_dst + w - 1, y_dst + h_this - 1);
		cop->src_start_xy = TRI_XY(0,0);
		cop->src_end_xy = TRI_XY(w - 1, h_this - 1);
					 
		if (mode == MODE_IMAGE)
		{
		    while (h_this--)
		    {
			w_remain = w;
			src = srcLine;
			srcLine += srcStride;
			off = offLine;
			offLine += offStride;
			while (w_remain--)
			    *off++ = *src++;
		    }
		}
		else
		{
		    while (h_this--)
		    {
			w_remain = w;
			msk = mskLine;
			mskLine += mskStride;
			off = offLine;
			offLine += offStride;
			while (w_remain--)
			    *off++ = rgb | (*msk++ << 24);
		    }
		}
		
		cop->command = (COP_OP_BLT |
				COP_SCL_OPAQUE |
				COP_OP_FB);
	    }
	    pbox++;
	}
	cop->src_offset = 0;
	
	KdMarkSync (pDst->pDrawable->pScreen);
    }
    else
    {
	KdCheckComposite (op,
			  pSrc,
			  pMask,
			  pDst,
			  xSrc,
			  ySrc,
			  xMask,
			  yMask,
			  xDst,
			  yDst,
			  width,
			  height);
    }
}

KaaScreenPrivRec    tridentKaa = {
    tridentPrepareSolid,
    tridentSolid,
    tridentDoneSolid,

    tridentPrepareCopy,
    tridentCopy,
    tridentDoneCopy,
};

Bool
tridentDrawInit (ScreenPtr pScreen)
{
    SetupTrident(pScreen);
    tridentScreenInfo(pScreenPriv);
    PictureScreenPtr    ps = GetPictureScreen(pScreen);
    
    if (!kaaDrawInit (pScreen, &tridentKaa))
	return FALSE;

    if (ps && tridents->off_screen)
	ps->Composite = tridentComposite;
    
    return TRUE;
}

void
tridentDrawEnable (ScreenPtr pScreen)
{
    SetupTrident(pScreen);
    CARD32  cmd;
    CARD32  base;
    CARD16  stride;
    CARD32  format;
    CARD32  alpha;
    int	    tries;
    int	    nwrite;
    
    stride = pScreenPriv->screen->fb[0].pixelStride;
    switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
    case 8:
	format = COP_DEPTH_8;
	break;
    case 16:
	format = COP_DEPTH_16;
	break;
    case 24:
	format = COP_DEPTH_24_32;
	break;
    case 32:
	format = COP_DEPTH_24_32;
	break;
    }
    /* 
     * compute a few things which will be set every time the
     * accelerator is used; this avoids problems with APM
     */
    tridentc->cop_depth = COP_MULTI_DEPTH | format;
    tridentc->cop_stride = COP_MULTI_STRIDE | (stride << 16) | (stride);
    
#define NUM_TRIES   100000
    for (tries = 0; tries < NUM_TRIES; tries++)
	if (!(cop->status & COP_STATUS_BUSY))
	    break;
    if (cop->status & COP_STATUS_BUSY)
	FatalError ("Can't initialize graphics coprocessor");
    cop->multi = COP_MULTI_CLIP_TOP_LEFT;
    cop->multi = COP_MULTI_MASK | 0;
    cop->src_offset = 0;
    cop->dst_offset = 0;
    cop->z_offset = 0;
    cop->clip_bottom_right = 0x0fff0fff;
    
    _tridentInit(cop,tridentc);
    _tridentSetSolidRect(cop, pScreen->blackPixel, GXcopy, cmd);
    _tridentRect (cop, 0, 0, 
		  pScreenPriv->screen->width, pScreenPriv->screen->height,
		  cmd);
    KdMarkSync (pScreen);
}

void
tridentDrawDisable (ScreenPtr pScreen)
{
}

void
tridentDrawFini (ScreenPtr pScreen)
{
}

void
tridentDrawSync (ScreenPtr pScreen)
{
    SetupTrident(pScreen);
    
    _tridentWaitIdleEmpty(cop);
}
