/* $XFree86: xc/programs/Xserver/iplan2p4/iplsolid.c,v 3.1 1998/03/20 21:08:09 hohndel Exp $ */
/*
 * $XConsortium: iplsolid.c,v 1.9 94/04/17 20:29:02 dpw Exp $
 *
Copyright (c) 1990  X Consortium

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */

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
#include "iplrrop.h"

#include "mi.h"
#include "mispans.h"

#include "iplmskbits.h"

#  define Expand(left, right, leftAdjust) { \
    while (h--) { \
	pdst = pdstRect; \
	left \
	m = nmiddle; \
	INTER_RROP_SPAN(pdst, m); \
	right \
	pdstRect += widthDst; \
    } \
}
	

void
INTER_RROP_NAME(iplFillRectSolid) (pDrawable, pGC, nBox, pBox)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    nBox;
    BoxPtr	    pBox;
{
    register int    m;
    INTER_DECLAREG(*pdst);
    INTER_RROP_DECLARE
    INTER_DECLAREG(leftMask);
    INTER_DECLAREG(rightMask);
    INTER_DECLAREG(*pdstBase);
    INTER_DECLAREG(*pdstRect);
    int		    nmiddle;
    int		    h;
    int		    w;
    int		    widthDst;

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pdstBase)

    INTER_RROP_FETCH_GC(pGC)
    
    for (; nBox; nBox--, pBox++)
    {
    	pdstRect = pdstBase + pBox->y1 * widthDst;
    	h = pBox->y2 - pBox->y1;
	w = pBox->x2 - pBox->x1;
	pdstRect += (pBox->x1 >> INTER_PGSH) * INTER_PLANES;
	if ((pBox->x1 & INTER_PIM) + w <= INTER_PPG)
	{
	    INTER_maskpartialbits(pBox->x1, w, leftMask);
	    pdst = pdstRect;
	    while (h--) {
		INTER_RROP_SOLID_MASK (pdst, leftMask);
		pdst += widthDst;
	    }
	}
	else
	{
	    INTER_maskbits (pBox->x1, w, leftMask, rightMask, nmiddle);
	    if (leftMask)
	    {
		if (rightMask)	/* left mask and right mask */
		{
		    Expand(INTER_RROP_SOLID_MASK (pdst, leftMask); 
			   INTER_NEXT_GROUP(pdst);,
			   INTER_RROP_SOLID_MASK (pdst, rightMask);, 1)
		}
		else	/* left mask and no right mask */
		{
		    Expand(INTER_RROP_SOLID_MASK (pdst, leftMask); 
			   INTER_NEXT_GROUP(pdst);,
			   ;, 1)
		}
	    }
	    else
	    {
		if (rightMask)	/* no left mask and right mask */
		{
		    Expand(;,
			   INTER_RROP_SOLID_MASK (pdst, rightMask);, 0)
		}
		else	/* no left mask and no right mask */
		{
		    Expand(;,
			    ;, 0)
		}
	    }
	}
    }
}

void
INTER_RROP_NAME(iplSolidSpans) (pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
    INTER_DECLAREG(*pdstBase);
    int		    widthDst;

    INTER_RROP_DECLARE
    
    INTER_DECLAREG(*pdst);
    register int	    ngmiddle;
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);
    register int	    w;
    int			    x;
    
				/* next three parameters are post-clip */
    int		    n;		/* number of spans to fill */
    DDXPointPtr	    ppt;	/* pointer to list of start points */
    int		    *pwidthFree;/* copies of the pointers to free */
    DDXPointPtr	    pptFree;
    int		    *pwidth;
    iplPrivGCPtr    devPriv;

    devPriv = iplGetGCPrivate(pGC);
    INTER_RROP_FETCH_GCPRIV(devPriv)
    n = nInit * miFindMaxBand(pGC->pCompositeClip);
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
    n = miClipSpans(pGC->pCompositeClip, pptInit, pwidthInit, nInit,
		     ppt, pwidth, fSorted);

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pdstBase)

    while (n--)
    {
	x = ppt->x;
	pdst = pdstBase + (ppt->y * widthDst);
	++ppt;
	w = *pwidth++;
	if (!w)
	    continue;
	if ((x & INTER_PIM) + w <= INTER_PPG)
	{
	    pdst += (x >> INTER_PGSH) * INTER_PLANES;
	    INTER_maskpartialbits (x, w, startmask);
	    INTER_RROP_SOLID_MASK (pdst, startmask);
	}
	else
	{
	    pdst += (x >> INTER_PGSH) * INTER_PLANES;
	    INTER_maskbits (x, w, startmask, endmask, ngmiddle);
	    if (startmask)
	    {
		INTER_RROP_SOLID_MASK (pdst, startmask);
		INTER_NEXT_GROUP(pdst);
	    }
	    
	    INTER_RROP_SPAN(pdst,ngmiddle);

	    if (endmask)
	    {
		INTER_RROP_SOLID_MASK (pdst, endmask);
	    }
	}
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
