/* $XFree86: xc/programs/Xserver/iplan2p4/iplsetsp.c,v 3.0 1996/08/18 01:55:07 dawes Exp $ */
/* $XConsortium: iplsetsp.c,v 5.10 94/04/17 20:29:01 dpw Exp $ */
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

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"

#include "misc.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "ipl.h"

#include "iplmskbits.h"
#include "iplmergerop.h"
#include "iplpack.h"

/* iplSetScanline -- copies the bits from psrc to the drawable starting at
 * (xStart, y) and continuing to (xEnd, y).  xOrigin tells us where psrc 
 * starts on the scanline. (I.e., if this scanline passes through multiple
 * boxes, we may not want to start grabbing bits at psrc but at some offset
 * further on.) 
 */
iplSetScanline(y, xOrigin, xStart, xEnd, psrc, alu, pdstBase, widthDst, planemask)
    int			y;
    int			xOrigin;	/* where this scanline starts */
    int			xStart;		/* first bit to use from scanline */
    int			xEnd;		/* last bit to use from scanline + 1 */
    register unsigned int *psrc;
    register int	alu;		/* raster op */
    INTER_DECLAREG(*pdstBase);		/* start of the drawable */
    int			widthDst;	/* width of drawable in groups */
    unsigned long	planemask;
{
    int			w;		/* width of scanline in bits */
    INTER_DECLAREG(*pdst);		/* where to put the bits */
    INTER_DECLAREGP(tmpSrc);		/* scratch buffer to collect bits in */
    int			dstBit;		/* offset in bits from beginning of 
					 * word */
    register int	nstart; 	/* number of bits from first partial */
    register int	nend; 		/* " " last partial word */
    int			offSrc;
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);
    int nlMiddle, nl, longs;
    INTER_DECLAREG(*ipsrc);
    INTER_DECLAREG(*tmppsrc);
    INTER_DeclareMergeRop()

    INTER_InitializeMergeRop(alu,planemask);

    longs=NUM_LONGS(INTER_PLANES, xOrigin, xEnd);

    tmppsrc = ipsrc = (unsigned short *) 
	    ALLOCATE_LOCAL(NUM_TEMP_BYTES(INTER_PLANES,longs));

    iplUnpackLine(INTER_PLANES, longs, psrc, ipsrc);

    pdst = pdstBase + (y * widthDst) + (xStart >> INTER_PGSH) * INTER_PLANES; 
    offSrc = (xStart - xOrigin) & INTER_PIM;
    w = xEnd - xStart;
    dstBit = xStart & INTER_PIM;

    ipsrc += ((xStart - xOrigin) >> INTER_PGSH) * INTER_PLANES;
    if (dstBit + w <= INTER_PPG) 
    { 
	INTER_maskpartialbits(dstBit, w, startmask);
	endmask = 0;
	nlMiddle = 0;
    } 
    else 
    { 
	INTER_maskbits(xStart, w, startmask, endmask, nlMiddle);
    }
    if (startmask) 
	nstart = INTER_PPG - dstBit; 
    else 
	nstart = 0; 
    if (endmask) 
	nend = xEnd & INTER_PIM; 
    else 
	nend = 0; 
    if (startmask) 
    { 
	INTER_getbits(ipsrc, offSrc, nstart, tmpSrc);
	INTER_putbitsmropshort(tmpSrc, dstBit, nstart, pdst);
	INTER_NEXT_GROUP(pdst); 
	offSrc += nstart;
	if (offSrc > INTER_PLST)
	{
	    INTER_NEXT_GROUP(ipsrc);
	    offSrc -= INTER_PPG;
	}
    } 
    nl = nlMiddle; 
    while (nl--) 
    { 
	INTER_getbits(ipsrc, offSrc, INTER_PPG, tmpSrc);
	INTER_DoMergeRop(tmpSrc, pdst, pdst);
	INTER_NEXT_GROUP(pdst); 
	INTER_NEXT_GROUP(ipsrc); 
    } 
    if (endmask) 
    { 
	INTER_getbits(ipsrc, offSrc, nend, tmpSrc);
	INTER_putbitsmropshort(tmpSrc, 0, nend, pdst);
    } 
    DEALLOCATE_LOCAL(tmppsrc);
}



/* SetSpans -- for each span copy pwidth[i] bits from psrc to pDrawable at
 * ppt[i] using the raster op from the GC.  If fSorted is TRUE, the scanlines
 * are in increasing Y order.
 * Source bit lines are server scanline padded so that they always begin
 * on a word boundary.
 */ 
void
iplSetSpans(pDrawable, pGC, pcharsrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    char		*pcharsrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    unsigned int	*psrc = (unsigned int *)pcharsrc;
    INTER_DECLAREG(*pdstBase);		/* start of dst bitmap */
    int 		widthDst;	/* width of bitmap in words */
    register BoxPtr 	pbox, pboxLast, pboxTest;
    register DDXPointPtr pptLast;
    int 		alu;
    RegionPtr 		prgnDst;
    int			xStart, xEnd;
    int			yMax;

    alu = pGC->alu;
    prgnDst = iplGetCompositeClip(pGC);
    pptLast = ppt + nspans;

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pdstBase)

    yMax = (int) pDrawable->y + (int) pDrawable->height;

    pbox = REGION_RECTS(prgnDst);
    pboxLast = pbox + REGION_NUM_RECTS(prgnDst);

    if(fSorted)
    {
    /* scan lines sorted in ascending order. Because they are sorted, we
     * don't have to check each scanline against each clip box.  We can be
     * sure that this scanline only has to be clipped to boxes at or after the
     * beginning of this y-band 
     */
	pboxTest = pbox;
	while(ppt < pptLast)
	{
	    pbox = pboxTest;
	    if(ppt->y >= yMax)
		break;
	    while(pbox < pboxLast)
	    {
		if(pbox->y1 > ppt->y)
		{
		    /* scanline is before clip box */
		    break;
		}
		else if(pbox->y2 <= ppt->y)
		{
		    /* clip box is before scanline */
		    pboxTest = ++pbox;
		    continue;
		}
		else if(pbox->x1 > ppt->x + *pwidth) 
		{
		    /* clip box is to right of scanline */
		    break;
		}
		else if(pbox->x2 <= ppt->x)
		{
		    /* scanline is to right of clip box */
		    pbox++;
		    continue;
		}

		/* at least some of the scanline is in the current clip box */
		xStart = max(pbox->x1, ppt->x);
		xEnd = min(ppt->x + *pwidth, pbox->x2);
		iplSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu,
		    pdstBase, widthDst, pGC->planemask);
		if(ppt->x + *pwidth <= pbox->x2)
		{
		    /* End of the line, as it were */
		    break;
		}
		else
		    pbox++;
	    }
	    /* We've tried this line against every box; it must be outside them
	     * all.  move on to the next point */
	    ppt++;
	    psrc += PixmapWidthInPadUnits(*pwidth, pDrawable->depth);
	    pwidth++;
	}
    }
    else
    {
    /* scan lines not sorted. We must clip each line against all the boxes */
	while(ppt < pptLast)
	{
	    if(ppt->y >= 0 && ppt->y < yMax)
	    {
		
		for(pbox = REGION_RECTS(prgnDst); pbox< pboxLast; pbox++)
		{
		    if(pbox->y1 > ppt->y)
		    {
			/* rest of clip region is above this scanline,
			 * skip it */
			break;
		    }
		    if(pbox->y2 <= ppt->y)
		    {
			/* clip box is below scanline */
			pbox++;
			break;
		    }
		    if(pbox->x1 <= ppt->x + *pwidth &&
		       pbox->x2 > ppt->x)
		    {
			xStart = max(pbox->x1, ppt->x);
			xEnd = min(pbox->x2, ppt->x + *pwidth);
			iplSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu,
			    pdstBase, widthDst, pGC->planemask);
		    }

		}
	    }
	psrc += PixmapWidthInPadUnits(*pwidth, pDrawable->depth);
	ppt++;
	pwidth++;
	}
    }
}

