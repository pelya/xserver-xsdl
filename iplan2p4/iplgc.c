/* $XFree86: xc/programs/Xserver/iplan2p4/iplgc.c,v 3.1 1998/03/20 21:08:08 hohndel Exp $ */
/***********************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* $XConsortium: iplgc.c,v 5.62 94/04/17 20:28:49 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "ipl.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"

#include "mistruct.h"
#include "mibstore.h"
#include "migc.h"

#include "iplmskbits.h"

# ifdef WriteBitGroup
#  define useTEGlyphBlt	iplImageGlyphBlt8
# else
#  define useTEGlyphBlt	iplTEGlyphBlt
# endif

#ifdef WriteBitGroup
# define useImageGlyphBlt	iplImageGlyphBlt8
# define usePolyGlyphBlt	iplPolyGlyphBlt8
#else
# define useImageGlyphBlt	miImageGlyphBlt
# define usePolyGlyphBlt	miPolyGlyphBlt
#endif

#ifdef FOUR_BIT_CODE
# define usePushPixels	iplPushPixels8
#else
# define usePushPixels	mfbPushPixels
#endif

#ifdef PIXEL_ADDR
# define ZeroPolyArc	iplZeroPolyArcSS8Copy
#else
# define ZeroPolyArc	miZeroPolyArc
#endif

GCFuncs iplGCFuncs = {
    iplValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip,
};

GCOps	iplTEOps1Rect = {
    iplSolidSpansCopy,
    iplSetSpans,
    iplPutImage,
    iplCopyArea,
    iplCopyPlane,
    iplPolyPoint,
#ifdef PIXEL_ADDR
    ipl8LineSS1Rect,
    ipl8SegmentSS1Rect,
#else
    iplLineSS,
    iplSegmentSS,
#endif
    miPolyRectangle,
    ZeroPolyArc,
    iplFillPoly1RectCopy,
    iplPolyFillRect,
    iplPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    useTEGlyphBlt,
    usePolyGlyphBlt,
    usePushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

GCOps	iplNonTEOps1Rect = {
    iplSolidSpansCopy,
    iplSetSpans,
    iplPutImage,
    iplCopyArea,
    iplCopyPlane,
    iplPolyPoint,
#ifdef PIXEL_ADDR
    ipl8LineSS1Rect,
    ipl8SegmentSS1Rect,
#else
    iplLineSS,
    iplSegmentSS,
#endif
    miPolyRectangle,
    ZeroPolyArc,
    iplFillPoly1RectCopy,
    iplPolyFillRect,
    iplPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    useImageGlyphBlt,
    usePolyGlyphBlt,
    usePushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

GCOps	iplTEOps = {
    iplSolidSpansCopy,
    iplSetSpans,
    iplPutImage,
    iplCopyArea,
    iplCopyPlane,
    iplPolyPoint,
    iplLineSS,
    iplSegmentSS,
    miPolyRectangle,
    ZeroPolyArc,
    miFillPolygon,
    iplPolyFillRect,
    iplPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    useTEGlyphBlt,
    usePolyGlyphBlt,
    usePushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

GCOps	iplNonTEOps = {
    iplSolidSpansCopy,
    iplSetSpans,
    iplPutImage,
    iplCopyArea,
    iplCopyPlane,
    iplPolyPoint,
    iplLineSS,
    iplSegmentSS,
    miPolyRectangle,
#ifdef PIXEL_ADDR
    iplZeroPolyArcSS8Copy,
#else
    miZeroPolyArc,
#endif
    miFillPolygon,
    iplPolyFillRect,
    iplPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    useImageGlyphBlt,
    usePolyGlyphBlt,
    usePushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

GCOps *
iplMatchCommon (pGC, devPriv)
    GCPtr	    pGC;
    iplPrivGCPtr    devPriv;
{
    if (pGC->lineWidth != 0)
	return 0;
    if (pGC->lineStyle != LineSolid)
	return 0;
    if (pGC->fillStyle != FillSolid)
	return 0;
    if (devPriv->rop != GXcopy)
	return 0;
    if (pGC->font &&
	FONTMAXBOUNDS(pGC->font,rightSideBearing) -
        FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32 &&
	FONTMINBOUNDS(pGC->font,characterWidth) >= 0)
    {
	if (TERMINALFONT(pGC->font)
#ifdef FOUR_BIT_CODE
	    && FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
	)
#ifdef NO_ONE_RECT
            return &iplTEOps1Rect;
#else
	    if (devPriv->oneRect)
		return &iplTEOps1Rect;
	    else
		return &iplTEOps;
#endif
	else
#ifdef NO_ONE_RECT
	    return &iplNonTEOps1Rect;
#else
	    if (devPriv->oneRect)
		return &iplNonTEOps1Rect;
	    else
		return &iplNonTEOps;
#endif
    }
    return 0;
}

Bool
iplCreateGC(pGC)
    register GCPtr pGC;
{
    iplPrivGC  *pPriv;

    if (PixmapWidthPaddingInfo[pGC->depth].padPixelsLog2 == LOG2_BITMAP_PAD)
	return (mfbCreateGC(pGC));
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;

    /*
     * some of the output primitives aren't really necessary, since they
     * will be filled in ValidateGC because of dix/CreateGC() setting all
     * the change bits.  Others are necessary because although they depend
     * on being a color frame buffer, they don't change 
     */

    pGC->ops = &iplNonTEOps;
    pGC->funcs = &iplGCFuncs;

    /* ipl wants to translate before scan conversion */
    pGC->miTranslate = 1;

    pPriv = iplGetGCPrivate(pGC);
    pPriv->rop = pGC->alu;
    pPriv->oneRect = FALSE;
    pGC->fExpose = TRUE;
    pGC->freeCompClip = FALSE;
    pGC->pRotatedPixmap = (PixmapPtr) NULL;
    return TRUE;
}

/* Clipping conventions
	if the drawable is a window
	    CT_REGION ==> pCompositeClip really is the composite
	    CT_other ==> pCompositeClip is the window clip region
	if the drawable is a pixmap
	    CT_REGION ==> pCompositeClip is the translated client region
		clipped to the pixmap boundary
	    CT_other ==> pCompositeClip is the pixmap bounding box
*/

void
iplValidateGC(pGC, changes, pDrawable)
    register GCPtr  pGC;
    unsigned long   changes;
    DrawablePtr	    pDrawable;
{
    int         mask;		/* stateChanges */
    int         index;		/* used for stepping through bitfields */
    int		new_rrop;
    int         new_line, new_text, new_fillspans, new_fillarea;
    int		new_rotate;
    int		xrot, yrot;
    /* flags for changing the proc vector */
    iplPrivGCPtr devPriv;
    int		oneRect;

    new_rotate = pGC->lastWinOrg.x != pDrawable->x ||
		 pGC->lastWinOrg.y != pDrawable->y;

    pGC->lastWinOrg.x = pDrawable->x;
    pGC->lastWinOrg.y = pDrawable->y;
    devPriv = iplGetGCPrivate(pGC);

    new_rrop = FALSE;
    new_line = FALSE;
    new_text = FALSE;
    new_fillspans = FALSE;
    new_fillarea = FALSE;

    /*
     * if the client clip is different or moved OR the subwindowMode has
     * changed OR the window's clip has changed since the last validation
     * we need to recompute the composite clip 
     */

    if ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode)) ||
	(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
	)
    {
	miComputeCompositeClip (pGC, pDrawable);
#ifdef NO_ONE_RECT
	devPriv->oneRect = FALSE;
#else
	oneRect = REGION_NUM_RECTS(pGC->pCompositeClip) == 1;
	if (oneRect != devPriv->oneRect)
	    new_line = TRUE;
	devPriv->oneRect = oneRect;
#endif
    }

    mask = changes;
    while (mask) {
	index = lowbit (mask);
	mask &= ~index;

	/*
	 * this switch acculmulates a list of which procedures might have
	 * to change due to changes in the GC.  in some cases (e.g.
	 * changing one 16 bit tile for another) we might not really need
	 * a change, but the code is being paranoid. this sort of batching
	 * wins if, for example, the alu and the font have been changed,
	 * or any other pair of items that both change the same thing. 
	 */
	switch (index) {
	case GCFunction:
	case GCForeground:
	    new_rrop = TRUE;
	    break;
	case GCPlaneMask:
	    new_rrop = TRUE;
	    new_text = TRUE;
	    break;
	case GCBackground:
	    break;
	case GCLineStyle:
	case GCLineWidth:
	    new_line = TRUE;
	    break;
	case GCJoinStyle:
	case GCCapStyle:
	    break;
	case GCFillStyle:
	    new_text = TRUE;
	    new_fillspans = TRUE;
	    new_line = TRUE;
	    new_fillarea = TRUE;
	    break;
	case GCFillRule:
	    break;
	case GCTile:
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	    break;

	case GCStipple:
	    if (pGC->stipple)
	    {
		int width = pGC->stipple->drawable.width;
		PixmapPtr nstipple;

		if ((width <= INTER_PGSZ) && !(width & (width - 1)) &&
		    (nstipple = iplCopyPixmap(pGC->stipple)))
		{
		    iplPadPixmap(nstipple);
		    (*pGC->pScreen->DestroyPixmap)(pGC->stipple);
		    pGC->stipple = nstipple;
		}
	    }
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	    break;

	case GCTileStipXOrigin:
	    new_rotate = TRUE;
	    break;

	case GCTileStipYOrigin:
	    new_rotate = TRUE;
	    break;

	case GCFont:
	    new_text = TRUE;
	    break;
	case GCSubwindowMode:
	    break;
	case GCGraphicsExposures:
	    break;
	case GCClipXOrigin:
	    break;
	case GCClipYOrigin:
	    break;
	case GCClipMask:
	    break;
	case GCDashOffset:
	    break;
	case GCDashList:
	    break;
	case GCArcMode:
	    break;
	default:
	    break;
	}
    }

    /*
     * If the drawable has changed,  ensure suitable
     * entries are in the proc vector. 
     */
    if (pDrawable->serialNumber != (pGC->serialNumber & (DRAWABLE_SERIAL_BITS))) {
	new_fillspans = TRUE;	/* deal with FillSpans later */
    }

    if (new_rotate || new_fillspans)
    {
	Bool new_pix = FALSE;

	xrot = pGC->patOrg.x + pDrawable->x;
	yrot = pGC->patOrg.y + pDrawable->y;

	switch (pGC->fillStyle)
	{
	case FillTiled:
	    if (!pGC->tileIsPixel)
	    {
		int width = pGC->tile.pixmap->drawable.width;

		if ((width <= INTER_PGSZ) && !(width & (width - 1)))
		{
		    iplCopyRotatePixmap(pGC->tile.pixmap,
					&pGC->pRotatedPixmap,
					xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
#ifdef FOUR_BIT_CODE
	case FillStippled:
	case FillOpaqueStippled:
	    {
		int width = pGC->stipple->drawable.width;

		if ((width <= INTER_PGSZ) && !(width & (width - 1)))
		{
		    mfbCopyRotatePixmap(pGC->stipple,
					&pGC->pRotatedPixmap, xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
#endif
	}
	if (!new_pix && pGC->pRotatedPixmap)
	{
	    (*pGC->pScreen->DestroyPixmap)(pGC->pRotatedPixmap);
	    pGC->pRotatedPixmap = (PixmapPtr) NULL;
	}
    }

    if (new_rrop)
    {
	int old_rrop;

	old_rrop = devPriv->rop;
	devPriv->rop = iplReduceRasterOp(pGC->alu, pGC->fgPixel, 
		pGC->planemask, devPriv->andg, devPriv->xorg);
	if (old_rrop == devPriv->rop)
	    new_rrop = FALSE;
	else
	{
#ifdef PIXEL_ADDR
	    new_line = TRUE;
#endif
#ifdef WriteBitGroup
	    new_text = TRUE;
#endif
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	}
    }

    if (new_rrop || new_fillspans || new_text || new_fillarea || new_line)
    {
	GCOps	*newops;

	if (newops = iplMatchCommon (pGC, devPriv))
 	{
	    if (pGC->ops->devPrivate.val)
		miDestroyGCOps (pGC->ops);
	    pGC->ops = newops;
	    new_rrop = new_line = new_fillspans = new_text = new_fillarea = 0;
	}
 	else
 	{
	    if (!pGC->ops->devPrivate.val)
	    {
		pGC->ops = miCreateGCOps (pGC->ops);
		pGC->ops->devPrivate.val = 1;
	    }
	}
    }

    /* deal with the changes we've collected */
    if (new_line)
    {
	pGC->ops->FillPolygon = miFillPolygon;
#ifdef NO_ONE_RECT
	if (pGC->fillStyle == FillSolid)
	{
	    switch (devPriv->rop) {
	    case GXcopy:
		pGC->ops->FillPolygon = iplFillPoly1RectCopy;
		break;
	    default:
		pGC->ops->FillPolygon = iplFillPoly1RectGeneral;
		break;
	    }
	}
#else
	if (devPriv->oneRect && pGC->fillStyle == FillSolid)
	{
	    switch (devPriv->rop) {
	    case GXcopy:
		pGC->ops->FillPolygon = iplFillPoly1RectCopy;
		break;
	    default:
		pGC->ops->FillPolygon = iplFillPoly1RectGeneral;
		break;
	    }
	}
#endif
	if (pGC->lineWidth == 0)
	{
#ifdef PIXEL_ADDR
	    if ((pGC->lineStyle == LineSolid) && (pGC->fillStyle == FillSolid))
	    {
		switch (devPriv->rop)
		{
		case GXxor:
		    pGC->ops->PolyArc = iplZeroPolyArcSS8Xor;
		    break;
		case GXcopy:
		    pGC->ops->PolyArc = iplZeroPolyArcSS8Copy;
		    break;
		default:
		    pGC->ops->PolyArc = iplZeroPolyArcSS8General;
		    break;
		}
	    }
	    else
#endif
		pGC->ops->PolyArc = miZeroPolyArc;
	}
	else
	    pGC->ops->PolyArc = miPolyArc;
	pGC->ops->PolySegment = miPolySegment;
	switch (pGC->lineStyle)
	{
	case LineSolid:
	    if(pGC->lineWidth == 0)
	    {
		if (pGC->fillStyle == FillSolid)
		{
#if defined(PIXEL_ADDR) && !defined(NO_ONE_RECT)
		    if (devPriv->oneRect &&
			((pDrawable->x >= pGC->pScreen->width - 32768) &&
			 (pDrawable->y >= pGC->pScreen->height - 32768)))
		    {
			pGC->ops->Polylines = ipl8LineSS1Rect;
			pGC->ops->PolySegment = ipl8SegmentSS1Rect;
		    } else
#endif
#ifdef NO_ONE_RECT
		    {
			pGC->ops->Polylines = ipl8LineSS1Rect;
			pGC->ops->PolySegment = ipl8SegmentSS1Rect;
		    }
#else
		    {
		    	pGC->ops->Polylines = iplLineSS;
		    	pGC->ops->PolySegment = iplSegmentSS;
		    }
#endif
		}
 		else
		    pGC->ops->Polylines = miZeroLine;
	    }
	    else
		pGC->ops->Polylines = miWideLine;
	    break;
	case LineOnOffDash:
	case LineDoubleDash:
	    if (pGC->lineWidth == 0 && pGC->fillStyle == FillSolid)
	    {
		pGC->ops->Polylines = iplLineSD;
		pGC->ops->PolySegment = iplSegmentSD;
	    } else
		pGC->ops->Polylines = miWideDash;
	    break;
	}
    }

    if (new_text && (pGC->font))
    {
        if (FONTMAXBOUNDS(pGC->font,rightSideBearing) -
            FONTMINBOUNDS(pGC->font,leftSideBearing) > 32 ||
	    FONTMINBOUNDS(pGC->font,characterWidth) < 0)
        {
            pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
            pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
        }
        else
        {
#ifdef WriteBitGroup
	    if (pGC->fillStyle == FillSolid)
	    {
		if (devPriv->rop == GXcopy)
		    pGC->ops->PolyGlyphBlt = iplPolyGlyphBlt8;
		else
#ifdef FOUR_BIT_CODE
		    pGC->ops->PolyGlyphBlt = iplPolyGlyphRop8;
#else
		    pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
#endif
	    }
	    else
#endif
		pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
            /* special case ImageGlyphBlt for terminal emulator fonts */
#if !defined(WriteBitGroup)
	    if (TERMINALFONT(pGC->font) &&
		(pGC->planemask & INTER_PMSK) == INTER_PMSK
#ifdef FOUR_BIT_CODE
		&& FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
		)
	    {
		pGC->ops->ImageGlyphBlt = useTEGlyphBlt;
	    }
            else
#endif
	    {
#ifdef WriteBitGroup
		if (devPriv->rop == GXcopy &&
		    pGC->fillStyle == FillSolid &&
		    (pGC->planemask & INTER_PMSK) == INTER_PMSK)
		    pGC->ops->ImageGlyphBlt = iplImageGlyphBlt8;
		else
#endif
		    pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
	    }
        }
    }    


    if (new_fillspans) {
	switch (pGC->fillStyle) {
	case FillSolid:
	    switch (devPriv->rop) {
	    case GXcopy:
		pGC->ops->FillSpans = iplSolidSpansCopy;
		break;
	    case GXxor:
		pGC->ops->FillSpans = iplSolidSpansXor;
		break;
	    default:
		pGC->ops->FillSpans = iplSolidSpansGeneral;
		break;
	    }
	    break;
	case FillTiled:
	    if (pGC->pRotatedPixmap)
	    {
		if (pGC->alu == GXcopy && (pGC->planemask & INTER_PMSK) == INTER_PMSK)
		    pGC->ops->FillSpans = iplTile32FSCopy;
		else
		    pGC->ops->FillSpans = iplTile32FSGeneral;
	    }
	    else
		pGC->ops->FillSpans = iplUnnaturalTileFS;
	    break;
	case FillStippled:
#ifdef FOUR_BIT_CODE
	    if (pGC->pRotatedPixmap)
		pGC->ops->FillSpans = ipl8Stipple32FS;
	    else
#endif
		pGC->ops->FillSpans = iplUnnaturalStippleFS;
	    break;
	case FillOpaqueStippled:
#ifdef FOUR_BIT_CODE
	    if (pGC->pRotatedPixmap)
		pGC->ops->FillSpans = ipl8OpaqueStipple32FS;
	    else
#endif
		pGC->ops->FillSpans = iplUnnaturalStippleFS;
	    break;
	default:
	    FatalError("iplValidateGC: illegal fillStyle\n");
	}
    } /* end of new_fillspans */

    if (new_fillarea) {
#ifndef FOUR_BIT_CODE
	pGC->ops->PolyFillRect = miPolyFillRect;
	if (pGC->fillStyle == FillSolid || pGC->fillStyle == FillTiled)
	{
	    pGC->ops->PolyFillRect = iplPolyFillRect;
	}
#endif
#ifdef FOUR_BIT_CODE
	pGC->ops->PushPixels = mfbPushPixels;
	if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy)
	    pGC->ops->PushPixels = iplPushPixels8;
#endif
	pGC->ops->PolyFillArc = miPolyFillArc;
	if (pGC->fillStyle == FillSolid)
	{
	    switch (devPriv->rop)
	    {
	    case GXcopy:
		pGC->ops->PolyFillArc = iplPolyFillArcSolidCopy;
		break;
	    default:
		pGC->ops->PolyFillArc = iplPolyFillArcSolidGeneral;
		break;
	    }
	}
    }
}
