/* $XFree86: xc/programs/Xserver/iplan2p4/ipltile32.c,v 3.1 1998/03/20 21:08:09 hohndel Exp $ */
/*
 * Fill 32 bit tiled rectangles.  Used by both PolyFillRect and PaintWindow.
 * no depth dependencies.
 */

/*

Copyright (c) 1989  X Consortium

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
*/

/* $XConsortium: ipltile32.c,v 1.8 94/04/17 20:29:05 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "ipl.h"

#include "mi.h"
#include "mispans.h"

#include "iplmskbits.h"
#include "iplmergerop.h"

#define STORE(p)    INTER_MROP_PREBUILT_SOLID(srcpix, p, p)

#define Expand(left,right) {\
    while (h--)	{ \
	INTER_COPY(psrc+srcy*INTER_PLANES, srcpix); \
	INTER_MROP_PREBUILD(srcpix); \
	++srcy; \
	if (srcy == tileHeight) \
	    srcy = 0; \
	left \
	ngw = ngwMiddle; \
	while (ngw--) \
	{ \
	    STORE(p); \
	    INTER_NEXT_GROUP(p); \
	} \
	right \
	p += ngwExtra; \
    } \
}

void
INTER_MROP_NAME(iplFillRectTile32) (pDrawable, pGC, nBox, pBox)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    nBox;	/* number of boxes to fill */
    BoxPtr 	    pBox;	/* pointer to list of boxes to fill */
{
    INTER_DECLAREGP(srcpix);
    INTER_DECLAREG(*psrc);	/* pointer to bits in tile, if needed */
    int tileHeight;	/* height of the tile */

    int ngwDst;		/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask); /* masks for reggedy bits at either end of line */
    int ngwMiddle;	/* number of longwords between sides of boxes */
    int ngwExtra;	/* to get from right of box to left of next span */
    register int ngw;	/* loop version of ngwMiddle */
    INTER_DECLAREG(*p);	/* pointer to bits we're writing */
    int y;		/* current scan line */
    int srcy;		/* current tile position */

    INTER_DECLAREG(*pbits); /* pointer to start of pixmap */
    PixmapPtr	    tile;	/* rotated, expanded tile */
    INTER_MROP_DECLARE_REG()
    INTER_MROP_PREBUILT_DECLARE()

    tile = pGC->pRotatedPixmap;
    tileHeight = tile->drawable.height;
    psrc = (unsigned short *)tile->devPrivate.ptr;

    INTER_MROP_INITIALIZE(pGC->alu, pGC->planemask);

    iplGetGroupWidthAndPointer (pDrawable, ngwDst, pbits)

    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;
	y = pBox->y1;
	p = pbits + (y * ngwDst) + (pBox->x1 >> INTER_PGSH) * INTER_PLANES;
	srcy = y % tileHeight;

	if ( ((pBox->x1 & INTER_PIM) + w) <= INTER_PPG)
	{
	    INTER_maskpartialbits(pBox->x1, w, startmask);
	    ngwExtra = ngwDst;
	    while (h--)
	    {
		INTER_COPY(psrc+srcy*INTER_PLANES, srcpix);
		INTER_MROP_PREBUILD(srcpix);
		++srcy;
		if (srcy == tileHeight)
		    srcy = 0;
		INTER_MROP_PREBUILT_MASK(srcpix, p, startmask, p);
		p += ngwExtra;
	    }
	}
	else
	{
	    INTER_maskbits(pBox->x1, w, startmask, endmask, ngwMiddle);
	    ngwExtra = ngwDst - ngwMiddle * INTER_PLANES;

	    if (startmask)
	    {
		ngwExtra -= INTER_PLANES;
		if (endmask)
		{
		    Expand(
		    INTER_MROP_PREBUILT_MASK(srcpix, p, startmask, p);
		    INTER_NEXT_GROUP(p);,
		    INTER_MROP_PREBUILT_MASK(srcpix, p, endmask, p));
		}
		else
		{
		    Expand(
		    INTER_MROP_PREBUILT_MASK(srcpix, p, startmask, p);
		    INTER_NEXT_GROUP(p);,
			   ;)
		}
	    }
	    else
	    {
		if (endmask)
		{
		    Expand(;,
		    INTER_MROP_PREBUILT_MASK(srcpix, p, endmask, p));
		}
		else
		{
		    Expand(;,
			   ;)
		}
	    }
	}
        pBox++;
    }
}

void
INTER_MROP_NAME(iplTile32FS)(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int			n;	/* number of spans to fill */
    DDXPointPtr		ppt;	/* pointer to list of start points */
    int			*pwidth;/* pointer to list of n widths */
    INTER_DECLAREG(*pbits);	/* pointer to start of bitmap */
    int			ngwDst;	/* width in longwords of bitmap */
    INTER_DECLAREG(*p);		/* pointer to current longword in bitmap */
    register int	w;	/* current span width */
    register int	ngw;
    register int	x;
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);
    INTER_DECLAREGP(srcpix);
    int			y;
    int			*pwidthFree;/* copies of the pointers to free */
    DDXPointPtr		pptFree;
    PixmapPtr		tile;
    INTER_DECLAREG(*psrc);	/* pointer to bits in tile */
    int			tileHeight;/* height of the tile */
    INTER_MROP_DECLARE_REG ()
    INTER_MROP_PREBUILT_DECLARE()

    n = nInit * miFindMaxBand( iplGetCompositeClip(pGC) );
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans( iplGetCompositeClip(pGC),
		     pptInit, pwidthInit, nInit,
		     ppt, pwidth, fSorted);

    tile = pGC->pRotatedPixmap;
    tileHeight = tile->drawable.height;
    psrc = (unsigned short *)tile->devPrivate.ptr;

    INTER_MROP_INITIALIZE(pGC->alu, pGC->planemask);

    iplGetGroupWidthAndPointer (pDrawable, ngwDst, pbits)

    {
    	while (n--)
    	{
	    x = ppt->x;
	    y = ppt->y;
	    ++ppt;
	    w = *pwidth++;
	    p = pbits + (y * ngwDst) + (x >> INTER_PGSH) * INTER_PLANES;
	    INTER_COPY(psrc +(y % tileHeight)*INTER_PLANES,srcpix);
	    INTER_MROP_PREBUILD(srcpix);
    
	    if ((x & INTER_PIM) + w < INTER_PPG)
	    {
	    	INTER_maskpartialbits(x, w, startmask);
		INTER_MROP_PREBUILT_MASK(srcpix, p, startmask, p);
	    }
	    else
	    {
	    	INTER_maskbits(x, w, startmask, endmask, ngw);
	    	if (startmask)
	    	{
		    INTER_MROP_PREBUILT_MASK(srcpix, p, startmask, p);
		    INTER_NEXT_GROUP(p);
	    	}
	    	while (ngw--)
	    	{
		    STORE(p);
		    INTER_NEXT_GROUP(p);
	    	}
	    	if (endmask)
	    	{
		    INTER_MROP_PREBUILT_MASK(srcpix, p, endmask, p);
	    	}
	    }
    	}
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
