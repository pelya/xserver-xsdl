/*
 * Support for RENDER extension while protecting the alpha channel
 */
/*
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Apple Computer, Inc. All Rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* This file is largely based on fbcompose.c and fbpict.c, which contain
 * the following copyright:
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 */
 /* $XFree86: xc/programs/Xserver/miext/rootless/safeAlpha/safeAlphaPicture.c,v 1.4 2004/01/19 01:22:48 torrey Exp $ */

#ifdef RENDER

#include "fb.h"
#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"
#include "safeAlpha.h"
#include "rootlessCommon.h"

# define mod(a,b)	((b) == 1 ? 0 : (a) >= 0 ? (a) % (b) : (b) - (-a) % (b))


// Replacement for fbStore_x8r8g8b8 that sets the alpha channel
void
SafeAlphaStore_x8r8g8b8 (FbCompositeOperand *op, CARD32 value)
{
    FbBits  *line = op->u.drawable.line; CARD32 offset = op->u.drawable.offset;
    ((CARD32 *)line)[offset >> 5] = (value & ~RootlessAlphaMask(32)) |
                                    RootlessAlphaMask(32);
}


// Defined in fbcompose.c
extern FbCombineFunc fbCombineFuncU[];
extern FbCombineFunc fbCombineFuncC[];

/* A bunch of macros from fbpict.c */
#define cvt0565to8888(s)    (((((s) << 3) & 0xf8) | (((s) >> 2) & 0x7)) | \
			     ((((s) << 5) & 0xfc00) | (((s) >> 1) & 0x300)) | \
			     ((((s) << 8) & 0xf80000) | (((s) << 3) & 0x70000)))

#if IMAGE_BYTE_ORDER == MSBFirst
#define Fetch24(a)  ((unsigned long) (a) & 1 ? \
		     ((*(a) << 16) | *((CARD16 *) ((a)+1))) : \
		     ((*((CARD16 *) (a)) << 8) | *((a)+2)))
#define Store24(a,v) ((unsigned long) (a) & 1 ? \
		      ((*(a) = (CARD8) ((v) >> 16)), \
		       (*((CARD16 *) ((a)+1)) = (CARD16) (v))) : \
		      ((*((CARD16 *) (a)) = (CARD16) ((v) >> 8)), \
		       (*((a)+2) = (CARD8) (v))))
#else
#define Fetch24(a)  ((unsigned long) (a) & 1 ? \
		     ((*(a)) | (*((CARD16 *) ((a)+1)) << 8)) : \
		     ((*((CARD16 *) (a))) | (*((a)+2) << 16)))
#define Store24(a,v) ((unsigned long) (a) & 1 ? \
		      ((*(a) = (CARD8) (v)), \
		       (*((CARD16 *) ((a)+1)) = (CARD16) ((v) >> 8))) : \
		      ((*((CARD16 *) (a)) = (CARD16) (v)),\
		       (*((a)+2) = (CARD8) ((v) >> 16))))
#endif
		      
#define fbComposeGetSolid(pict, bits) { \
    FbBits	*__bits__; \
    FbStride	__stride__; \
    int		__bpp__; \
    int		__xoff__,__yoff__; \
\
    fbGetDrawable((pict)->pDrawable,__bits__,__stride__,__bpp__,__xoff__,__yoff__); \
    switch (__bpp__) { \
    case 32: \
	(bits) = *(CARD32 *) __bits__; \
	break; \
    case 24: \
	(bits) = Fetch24 ((CARD8 *) __bits__); \
	break; \
    case 16: \
	(bits) = *(CARD16 *) __bits__; \
	(bits) = cvt0565to8888(bits); \
	break; \
    default: \
	return; \
    } \
    /* manage missing src alpha */ \
    if ((pict)->pFormat->direct.alphaMask == 0) \
	(bits) |= 0xff000000; \
}

#define fbComposeGetStart(pict,x,y,type,stride,line,mul) {\
    FbBits	*__bits__; \
    FbStride	__stride__; \
    int		__bpp__; \
    int		__xoff__,__yoff__; \
\
    fbGetDrawable((pict)->pDrawable,__bits__,__stride__,__bpp__,__xoff__,__yoff__); \
    (stride) = __stride__ * sizeof (FbBits) / sizeof (type); \
    (line) = ((type *) __bits__) + (stride) * ((y) - __yoff__) + (mul) * ((x) - __xoff__); \
}


/* Optimized version of fbCompositeSolidMask_nx8x8888 */
void
SafeAlphaCompositeSolidMask_nx8x8888(
    CARD8      op,
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
    CARD32	src, srca;
    CARD32	*dstLine, *dst, d, dstMask;
    CARD8	*maskLine, *mask, m;
    FbStride	dstStride, maskStride;
    CARD16	w;

    fbComposeGetSolid(pSrc, src);

    dstMask = FbFullMask (pDst->pDrawable->depth);
    srca = src >> 24;
    if (src == 0)
	return;

    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD8, maskStride, maskLine, 1);

    if (dstMask == FB_ALLONES && pDst->pDrawable->bitsPerPixel == 32 &&
        width * height > rootless_CompositePixels_threshold &&
        SCREENREC(pDst->pDrawable->pScreen)->imp->CompositePixels)
    {
	void *srcp[2], *destp[2];
	unsigned int dest_rowbytes[2];
	unsigned int fn;

	srcp[0] = &src; srcp[1] = &src;
	/* null rowbytes pointer means use first value as a constant */
	destp[0] = dstLine; destp[1] = dstLine;
	dest_rowbytes[0] = dstStride * 4; dest_rowbytes[1] = dest_rowbytes[0];
	fn = RL_COMPOSITE_FUNCTION(RL_COMPOSITE_OVER, RL_DEPTH_ARGB8888,
                                   RL_DEPTH_A8, RL_DEPTH_ARGB8888);

	if (SCREENREC(pDst->pDrawable->pScreen)->imp->CompositePixels(
                width, height, fn, srcp, NULL,
                maskLine, maskStride,
                destp, dest_rowbytes) == Success)
	{
	    return;
	}
    }

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	while (w--)
	{
	    m = *mask++;
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		    *dst = src & dstMask;
		else
		    *dst = fbOver (src, *dst) & dstMask;
	    }
	    else if (m)
	    {
		d = fbIn (src, m);
		*dst = fbOver (d, *dst) & dstMask;
	    }
	    dst++;
	}
    }
}


void
SafeAlphaCompositeGeneral(
    CARD8               op,
    PicturePtr          pSrc,
    PicturePtr          pMask,
    PicturePtr          pDst,
    INT16               xSrc,
    INT16               ySrc,
    INT16               xMask,
    INT16               yMask,
    INT16               xDst,
    INT16               yDst,
    CARD16              width,
    CARD16              height)
{
    FbCompositeOperand	src[4],msk[4],dst[4],*pmsk;
    FbCompositeOperand	*srcPict, *srcAlpha;
    FbCompositeOperand	*dstPict, *dstAlpha;
    FbCompositeOperand	*mskPict = 0, *mskAlpha = 0;
    FbCombineFunc	f;
    int			w;

    if (!fbBuildCompositeOperand (pSrc, src, xSrc, ySrc, TRUE, TRUE))
	return;
    if (!fbBuildCompositeOperand (pDst, dst, xDst, yDst, FALSE, TRUE))
	return;

    // Use SafeAlpha operands for on screen picture formats
    if (pDst->format == PICT_x8r8g8b8) {
        dst[0].store = SafeAlphaStore_x8r8g8b8;
    }

    if (pSrc->alphaMap)
    {
	srcPict = &src[1];
	srcAlpha = &src[2];
    }
    else
    {
	srcPict = &src[0];
	srcAlpha = 0;
    }
    if (pDst->alphaMap)
    {
	dstPict = &dst[1];
	dstAlpha = &dst[2];
    }
    else
    {
	dstPict = &dst[0];
	dstAlpha = 0;
    }
    f = fbCombineFuncU[op];
    if (pMask)
    {
	if (!fbBuildCompositeOperand (pMask, msk, xMask, yMask, TRUE, TRUE))
	    return;
	pmsk = msk;
	if (pMask->componentAlpha)
	    f = fbCombineFuncC[op];
	if (pMask->alphaMap)
	{
	    mskPict = &msk[1];
	    mskAlpha = &msk[2];
	}
	else
	{
	    mskPict = &msk[0];
	    mskAlpha = 0;
	}
    }
    else
	pmsk = 0;
    while (height--)
    {
	w = width;
	
	while (w--)
	{
	    (*f) (src, pmsk, dst);
	    (*src->over) (src);
	    (*dst->over) (dst);
	    if (pmsk)
		(*pmsk->over) (pmsk);
	}
	(*src->down) (src);
	(*dst->down) (dst);
	if (pmsk)
	    (*pmsk->down) (pmsk);
    }
}


void
SafeAlphaComposite(
    CARD8           op,
    PicturePtr      pSrc,
    PicturePtr      pMask,
    PicturePtr      pDst,
    INT16           xSrc,
    INT16           ySrc,
    INT16           xMask,
    INT16           yMask,
    INT16           xDst,
    INT16           yDst,
    CARD16          width,
    CARD16          height)
{
    RegionRec	    region;
    int		    n;
    BoxPtr	    pbox;
    CompositeFunc   func;
    Bool	    srcRepeat = pSrc->repeat;
    Bool	    maskRepeat = FALSE;
    Bool            srcAlphaMap = pSrc->alphaMap != 0;
    Bool	    maskAlphaMap = FALSE;
    Bool            dstAlphaMap = pDst->alphaMap != 0;
    int		    x_msk, y_msk, x_src, y_src, x_dst, y_dst;
    int		    w, h, w_this, h_this;
    int		    dstDepth = pDst->pDrawable->depth;

    xDst += pDst->pDrawable->x;
    yDst += pDst->pDrawable->y;
    xSrc += pSrc->pDrawable->x;
    ySrc += pSrc->pDrawable->y;
    if (pMask)
    {
	xMask += pMask->pDrawable->x;
	yMask += pMask->pDrawable->y;
	maskRepeat = pMask->repeat;
	maskAlphaMap = pMask->alphaMap != 0;
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

    // To preserve the alpha channel we have a special,
    // non-optimzied compositor.
    func = SafeAlphaCompositeGeneral;

    /*
     * We can use the more optimized fbpict code, but it sets bits above
     * the depth to zero. Temporarily adjust destination depth if needed.
     */
    if (pDst->pDrawable->type == DRAWABLE_WINDOW
	&& pDst->pDrawable->depth == 24
	&& pDst->pDrawable->bitsPerPixel == 32)
    {
	pDst->pDrawable->depth = 32;
    }

    if (!pSrc->transform && !(pMask && pMask->transform))
    if (!maskAlphaMap && !srcAlphaMap && !dstAlphaMap)
    switch (op) {
    case PictOpOver:
	if (pMask)
	{
	    if (srcRepeat && 
		pSrc->pDrawable->width == 1 &&
		pSrc->pDrawable->height == 1)
	    {
		srcRepeat = FALSE;
		if (PICT_FORMAT_COLOR(pSrc->format)) {
		    switch (pMask->format) {
		    case PICT_a8:
			switch (pDst->format) {
			case PICT_r5g6b5:
			case PICT_b5g6r5:
			    func = fbCompositeSolidMask_nx8x0565;
			    break;
			case PICT_r8g8b8:
			case PICT_b8g8r8:
			    func = fbCompositeSolidMask_nx8x0888;
			    break;
			case PICT_a8r8g8b8:
			case PICT_x8r8g8b8:
			case PICT_a8b8g8r8:
			case PICT_x8b8g8r8:
			    func = SafeAlphaCompositeSolidMask_nx8x8888;
			    break;
			}
			break;
		    case PICT_a8r8g8b8:
			if (pMask->componentAlpha) {
			    switch (pDst->format) {
			    case PICT_a8r8g8b8:
			    case PICT_x8r8g8b8:
				func = fbCompositeSolidMask_nx8888x8888C;
				break;
			    case PICT_r5g6b5:
				func = fbCompositeSolidMask_nx8888x0565C;
				break;
			    }
			}
			break;
		    case PICT_a8b8g8r8:
			if (pMask->componentAlpha) {
			    switch (pDst->format) {
			    case PICT_a8b8g8r8:
			    case PICT_x8b8g8r8:
				func = fbCompositeSolidMask_nx8888x8888C;
				break;
			    case PICT_b5g6r5:
				func = fbCompositeSolidMask_nx8888x0565C;
				break;
			    }
			}
			break;
		    case PICT_a1:
			switch (pDst->format) {
			case PICT_r5g6b5:
			case PICT_b5g6r5:
			case PICT_r8g8b8:
			case PICT_b8g8r8:
			case PICT_a8r8g8b8:
			case PICT_x8r8g8b8:
			case PICT_a8b8g8r8:
			case PICT_x8b8g8r8:
			    func = fbCompositeSolidMask_nx1xn;
			    break;
			}
		    }
		}
	    }
	}
	else
	{
	    switch (pSrc->format) {
	    case PICT_a8r8g8b8:
	    case PICT_x8r8g8b8:
		switch (pDst->format) {
		case PICT_a8r8g8b8:
		case PICT_x8r8g8b8:
		    func = fbCompositeSrc_8888x8888;
		    break;
		case PICT_r8g8b8:
		    func = fbCompositeSrc_8888x0888;
		    break;
		case PICT_r5g6b5:
		    func = fbCompositeSrc_8888x0565;
		    break;
		}
		break;
	    case PICT_a8b8g8r8:
	    case PICT_x8b8g8r8:
		switch (pDst->format) {
		case PICT_a8b8g8r8:
		case PICT_x8b8g8r8:
		    func = fbCompositeSrc_8888x8888;
		    break;
		case PICT_b8g8r8:
		    func = fbCompositeSrc_8888x0888;
		    break;
		case PICT_b5g6r5:
		    func = fbCompositeSrc_8888x0565;
		    break;
		}
		break;
	    case PICT_r5g6b5:
		switch (pDst->format) {
		case PICT_r5g6b5:
		    func = fbCompositeSrc_0565x0565;
		    break;
		}
		break;
	    case PICT_b5g6r5:
		switch (pDst->format) {
		case PICT_b5g6r5:
		    func = fbCompositeSrc_0565x0565;
		    break;
		}
		break;
	    }
	}
	break;
    case PictOpAdd:
	if (pMask == 0)
	{
	    switch (pSrc->format) {
	    case PICT_a8r8g8b8:
		switch (pDst->format) {
		case PICT_a8r8g8b8:
		    func = fbCompositeSrcAdd_8888x8888;
		    break;
		}
		break;
	    case PICT_a8b8g8r8:
		switch (pDst->format) {
		case PICT_a8b8g8r8:
		    func = fbCompositeSrcAdd_8888x8888;
		    break;
		}
		break;
	    case PICT_a8:
		switch (pDst->format) {
		case PICT_a8:
		    func = fbCompositeSrcAdd_8000x8000;
		    break;
		}
		break;
	    case PICT_a1:
		switch (pDst->format) {
		case PICT_a1:
		    func = fbCompositeSrcAdd_1000x1000;
		    break;
		}
		break;
	    }
	}
	break;
    }
    n = REGION_NUM_RECTS (&region);
    pbox = REGION_RECTS (&region);
    while (n--)
    {
	h = pbox->y2 - pbox->y1;
	y_src = pbox->y1 - yDst + ySrc;
	y_msk = pbox->y1 - yDst + yMask;
	y_dst = pbox->y1;
	while (h)
	{
	    h_this = h;
	    w = pbox->x2 - pbox->x1;
	    x_src = pbox->x1 - xDst + xSrc;
	    x_msk = pbox->x1 - xDst + xMask;
	    x_dst = pbox->x1;
	    if (maskRepeat)
	    {
		y_msk = mod (y_msk, pMask->pDrawable->height);
		if (h_this > pMask->pDrawable->height - y_msk)
		    h_this = pMask->pDrawable->height - y_msk;
	    }
	    if (srcRepeat)
	    {
		y_src = mod (y_src, pSrc->pDrawable->height);
		if (h_this > pSrc->pDrawable->height - y_src)
		    h_this = pSrc->pDrawable->height - y_src;
	    }
	    while (w)
	    {
		w_this = w;
		if (maskRepeat)
		{
		    x_msk = mod (x_msk, pMask->pDrawable->width);
		    if (w_this > pMask->pDrawable->width - x_msk)
			w_this = pMask->pDrawable->width - x_msk;
		}
		if (srcRepeat)
		{
		    x_src = mod (x_src, pSrc->pDrawable->width);
		    if (w_this > pSrc->pDrawable->width - x_src)
			w_this = pSrc->pDrawable->width - x_src;
		}
		(*func) (op, pSrc, pMask, pDst,
			 x_src, y_src, x_msk, y_msk, x_dst, y_dst,
			 w_this, h_this);
		w -= w_this;
		x_src += w_this;
		x_msk += w_this;
		x_dst += w_this;
	    }
	    h -= h_this;
	    y_src += h_this;
	    y_msk += h_this;
	    y_dst += h_this;
	}
	pbox++;
    }
    REGION_UNINIT (pDst->pDrawable->pScreen, &region);

    // Reset destination depth to its true value
    pDst->pDrawable->depth = dstDepth;
}

#endif /* RENDER */
