/* $XFree86: xc/programs/Xserver/iplan2p4/iplblt.c,v 3.0 1996/08/18 01:54:35 dawes Exp $ */
/*
 * ipl copy area
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

Author: Keith Packard

*/
/* $XConsortium: iplblt.c,v 1.13 94/04/17 20:28:44 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"ipl.h"
#include	"fastblt.h"
#include	"iplmergerop.h"
#include	"iplmskbits.h"

void
INTER_MROP_NAME(iplDoBitblt)(pSrc, pDst, alu, prgnDst, pptSrc, planemask)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned long   planemask;
{
    INTER_DECLAREG(*psrcBase);
    INTER_DECLAREG(*pdstBase);	/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */

    BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew1, pptNew2;
				/* shuffling boxes entails shuffling the
				   source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */

    INTER_DECLAREG(*psrcLine);
    INTER_DECLAREG(*pdstLine); 	/* pointers to line with current src and dst */
    INTER_DECLAREG(*psrc);	/* pointer to current src group */
    INTER_DECLAREG(*pdst);	/* pointer to current dst group */

    INTER_MROP_DECLARE_REG()

				/* following used for looping through a line */
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);	/* masks for writing ends of dst */
    int ngMiddle;		/* whole groups in dst */
    int xoffSrc, xoffDst;
    register int leftShift, rightShift;
    INTER_DECLAREGP(bits);
    INTER_DECLAREGP(bits1);
    INTER_DECLAREGP(bits2);
    register int ng;		/* temp copy of ngMiddle */

				/* place to store full source word */
    int nstart;			/* number of ragged bits at start of dst */
    int nend;			/* number of ragged bits at end of dst */
    int srcStartOver;		/* pulling nstart bits from src
				   overflows into the next word? */
    int careful;
    int tmpSrc;

    INTER_MROP_INITIALIZE(alu,planemask);

    iplGetGroupWidthAndPointer (pSrc, widthSrc, psrcBase)

    iplGetGroupWidthAndPointer (pDst, widthDst, pdstBase)

    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
    careful = ((pSrc == pDst) ||
	       ((pSrc->type == DRAWABLE_WINDOW) &&
		(pDst->type == DRAWABLE_WINDOW)));

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);

    pboxNew1 = NULL;
    pptNew1 = NULL;
    pboxNew2 = NULL;
    pptNew2 = NULL;
    if (careful && (pptSrc->y < pbox->y1))
    {
        /* walk source botttom to top */
	ydir = -1;
	widthSrc = -widthSrc;
	widthDst = -widthDst;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNew1)
		return;
	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNew1)
	    {
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) &&
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
	    pptNew1 -= nbox;
	    pptSrc = pptNew1;
        }
    }
    else
    {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (careful && (pptSrc->x < pbox->x1))
    {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1)
	{
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew2 || !pptNew2)
	    {
		if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
		if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
		if (pboxNew1)
		{
		    DEALLOCATE_LOCAL(pptNew1);
		    DEALLOCATE_LOCAL(pboxNew1);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	    pptNew2 -= nbox;
	    pptSrc = pptNew2;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }

    while(nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	if (ydir == -1) /* start at last scanline of rectangle */
	{
	    psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc);
	    pdstLine = pdstBase + ((pbox->y2-1) * -widthDst);
	}
	else /* start at first scanline */
	{
	    psrcLine = psrcBase + (pptSrc->y * widthSrc);
	    pdstLine = pdstBase + (pbox->y1 * widthDst);
	}
	if ((pbox->x1 & INTER_PIM) + w <= INTER_PPG)
	{
	    INTER_maskpartialbits (pbox->x1, w, endmask);
	    startmask = 0;
	    ngMiddle = 0;
	}
	else
	{
	    INTER_maskbits(pbox->x1, w, startmask, endmask, ngMiddle);
	}

	if (xdir == 1)
	{
	    xoffSrc = pptSrc->x & INTER_PIM;
	    xoffDst = pbox->x1 & INTER_PIM;
	    pdstLine += (pbox->x1 >> INTER_PGSH) * INTER_PLANES;
	    psrcLine += (pptSrc->x >> INTER_PGSH) * INTER_PLANES;
	    ng = xoffSrc - xoffDst;
	    if (xoffSrc == xoffDst)
	    {
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    if (startmask)
		    {
			INTER_MROP_MASK(psrc, pdst, startmask, pdst);
			INTER_NEXT_GROUP(psrc);
			INTER_NEXT_GROUP(pdst);
		    }
		    ng = ngMiddle;

		    DuffL(ng, label1,
			INTER_MROP_SOLID(psrc, pdst, pdst);
			INTER_NEXT_GROUP(psrc);
			INTER_NEXT_GROUP(pdst);
		    )
		    if (endmask)
			INTER_MROP_MASK(psrc, pdst, endmask, pdst);
		}
	    }
	    else
	    {
		if (xoffSrc > xoffDst)
		{
		    leftShift = xoffSrc - xoffDst;
		    rightShift = (INTER_PIM+1) - leftShift;
		}
		else
		{
		    rightShift = xoffDst - xoffSrc;
		    leftShift = (INTER_PIM+1) - rightShift;
		}
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    INTER_CLR(bits);
		    if (xoffSrc > xoffDst) {
			INTER_COPY(psrc, bits);
			INTER_NEXT_GROUP(psrc);
		    }
		    if (startmask)
		    {
			INTER_GETLRC(leftShift, rightShift, psrc, bits, bits1);
			INTER_MROP_MASK(bits1, pdst, startmask, pdst);
			INTER_NEXT_GROUP(psrc);
			INTER_NEXT_GROUP(pdst);
		    }
		    ng = ngMiddle;
		    DuffL (ng,label2,
			INTER_GETLRC(leftShift, rightShift, psrc, bits, bits1);
			INTER_MROP_SOLID(bits1, pdst, pdst);
			INTER_NEXT_GROUP(psrc);
			INTER_NEXT_GROUP(pdst);
		    )
		    if (endmask)
		    {
			if ((endmask << rightShift) & 0xffff) {
			    INTER_GETLRC(leftShift, rightShift, psrc, bits, 
				bits1);
			}
			else {
			    INTER_SCRLEFT(leftShift, bits, bits1);
			}
			INTER_MROP_MASK(bits1, pdst, endmask, pdst);
		    }
		}
	    }
	}
	else	/* xdir == -1 */
	{
	    xoffSrc = (pptSrc->x + w - 1) & INTER_PIM;
	    xoffDst = (pbox->x2 - 1) & INTER_PIM;
	    pdstLine += (((pbox->x2-1) >> INTER_PGSH) + 1) * INTER_PLANES;
	    psrcLine += (((pptSrc->x+w - 1) >> INTER_PGSH) + 1) * INTER_PLANES;
	    if (xoffSrc == xoffDst)
	    {
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    if (endmask)
		    {
			INTER_PREV_GROUP(psrc);
			INTER_PREV_GROUP(pdst);
			INTER_MROP_MASK(psrc, pdst, endmask, pdst);
		    }
		    ng = ngMiddle;

		    DuffL(ng,label3,
			INTER_PREV_GROUP(psrc);
			INTER_PREV_GROUP(pdst);
			INTER_MROP_SOLID(psrc, pdst, pdst);
		    )

		    if (startmask)
		    {
			INTER_PREV_GROUP(psrc);
			INTER_PREV_GROUP(pdst);
			INTER_MROP_MASK(psrc, pdst, startmask, pdst);
		    }
		}
	    }
	    else
	    {
		if (xoffDst > xoffSrc)
		{
		    rightShift = xoffDst - xoffSrc;
		    leftShift = (INTER_PIM + 1) - rightShift;
		}
		else
		{
		    leftShift = xoffSrc - xoffDst;
		    rightShift = (INTER_PIM + 1) - leftShift;
		}
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    INTER_CLR(bits);
		    if (xoffDst > xoffSrc) {
			INTER_PREV_GROUP(psrc);
			INTER_COPY(psrc, bits);
		    }
		    if (endmask)
		    {
			INTER_PREV_GROUP(psrc);
			INTER_PREV_GROUP(pdst);
			INTER_GETRLC(rightShift, leftShift, psrc, bits, bits1);
			INTER_MROP_MASK(bits1, pdst, endmask, pdst);
		    }
		    ng = ngMiddle;
		    DuffL (ng, label4,
			INTER_PREV_GROUP(psrc);
			INTER_PREV_GROUP(pdst);
			INTER_GETRLC(rightShift, leftShift, psrc, bits, bits1);
			INTER_MROP_SOLID(bits1, pdst, pdst);
		    )
		    if (startmask)
		    {
			INTER_PREV_GROUP(psrc);
			INTER_PREV_GROUP(pdst);
			if ((startmask >> leftShift) & 0xffff) {
			    INTER_GETRLC(rightShift, leftShift, psrc, bits, 
				bits1);
			}
			else {
			    INTER_SCRRIGHT(rightShift, bits, bits1);
			}
			INTER_MROP_MASK(bits1, pdst, startmask, pdst);
		    }
		}
	    }
	}
	pbox++;
	pptSrc++;
    }
    if (pboxNew2)
    {
	DEALLOCATE_LOCAL(pptNew2);
	DEALLOCATE_LOCAL(pboxNew2);
    }
    if (pboxNew1)
    {
	DEALLOCATE_LOCAL(pptNew1);
	DEALLOCATE_LOCAL(pboxNew1);
    }
}
