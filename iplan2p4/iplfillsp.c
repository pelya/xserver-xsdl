/* $XFree86: xc/programs/Xserver/iplan2p4/iplfillsp.c,v 3.0 1996/08/18 01:54:43 dawes Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or X Consortium
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and X Consortium make no 
representations about the suitability of this software for 
any purpose. It is provided "as is" without any express or 
implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

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

/* $XConsortium: iplfillsp.c,v 5.24 94/04/17 20:28:48 dpw Exp $ */

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

#include "mergerop.h"

#define MFB_CONSTS_ONLY
#include "maskbits.h"

#include "mi.h"
#include "mispans.h"

#include "iplmskbits.h"

/* scanline filling for color frame buffer
   written by drewry, oct 1986 modified by smarks
   changes for compatibility with Little-endian systems Jul 1987; MIT:yba.

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in iplCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  
Note that for solids, PrivGC.rop == PrivGC.ropOpStip


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedTile, gc.alu} or {RotatedStipple, PrivGC.ropOpStip}
*/

#ifdef	notdef
#include	<stdio.h>
static
dumpspans(n, ppt, pwidth)
    int	n;
    DDXPointPtr ppt;
    int *pwidth;
{
    fprintf(stderr,"%d spans\n", n);
    while (n--) {
	fprintf(stderr, "[%d,%d] %d\n", ppt->x, ppt->y, *pwidth);
	ppt++;
	pwidth++;
    }
    fprintf(stderr, "\n");
}
#endif

/* Fill spans with tiles that aren't 32 bits wide */
void
iplUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    void    (*fill)();
    int	xrot, yrot;

    if (!(pGC->planemask))
	return;

    if (pGC->tile.pixmap->drawable.width & INTER_PIM)
    {
    	fill = iplFillSpanTileOddGeneral;
    	if ((pGC->planemask & INTER_PMSK) == INTER_PMSK)
    	{
	    if (pGC->alu == GXcopy)
	    	fill = iplFillSpanTileOddCopy;
    	}
    }
    else
    {
	fill = iplFillSpanTile32sGeneral;
    	if ((pGC->planemask & INTER_PMSK) == INTER_PMSK)
    	{
	    if (pGC->alu == GXcopy)
		fill = iplFillSpanTile32sCopy;
	}
    }
    n = nInit * miFindMaxBand( iplGetCompositeClip(pGC) );
    if ( n == 0 )
	return;
    pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!ppt || !pwidth)
    {
	if (ppt) DEALLOCATE_LOCAL(ppt);
	if (pwidth) DEALLOCATE_LOCAL(pwidth);
	return;
    }
    n = miClipSpans( iplGetCompositeClip(pGC),
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    xrot = pDrawable->x + pGC->patOrg.x;
    yrot = pDrawable->y + pGC->patOrg.y;

    (*fill) (pDrawable, n, ppt, pwidth, pGC->tile.pixmap, xrot, yrot, pGC->alu, pGC->planemask);

    DEALLOCATE_LOCAL(ppt);
    DEALLOCATE_LOCAL(pwidth);
}

/* Fill spans with stipples that aren't 32 bits wide */
void
iplUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
				/* next three parameters are post-clip */
    int			    n;		/* number of spans to fill */
    register DDXPointPtr    ppt;	/* pointer to list of start points */
    register int	    *pwidth;	/* pointer to list of n widths */
    int			    iline;	/* first line of tile to use */
    INTER_DECLAREG(*addrgBase);		/* pointer to start of bitmap */
    int			    ngwidth;	/* width in groups of bitmap */
    INTER_DECLAREG(*pdst);		/* pointer to current group in bitmap */
    PixmapPtr		    pStipple;	/* pointer to stipple we want to fill with */
    register int	    w;
    int			    width,  x, xrem, xSrc, ySrc;
    INTER_DECLAREGP(tmpSrc); 
    INTER_DECLAREGP(tmpDst1);
    INTER_DECLAREGP(tmpDst2);
    int			    stwidth, stippleWidth;
    unsigned long	    *psrcS;
    int			    rop, stiprop;
    int			    stippleHeight;
    int			    *pwidthFree;    /* copies of the pointers to free */
    DDXPointPtr		    pptFree;
    INTER_DECLARERRAXP(bgfill);
    INTER_DECLARERRAXP(fgfill);

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand( iplGetCompositeClip(pGC) );
    if ( n == 0 )
	return;
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
    rop = pGC->alu;
    if (pGC->fillStyle == FillStippled) {
	switch (rop) {
	    case GXand:
	    case GXcopy:
	    case GXnoop:
	    case GXor:
		stiprop = rop;
		break;
	    default:
		stiprop = rop;
		rop = GXcopy;
	}
    }
    INTER_PFILL(pGC->fgPixel, fgfill);
    INTER_PFILL(pGC->bgPixel, bgfill);

    /*
     *  OK,  so what's going on here?  We have two Drawables:
     *
     *  The Stipple:
     *		Depth = 1
     *		Width = stippleWidth
     *		Words per scanline = stwidth
     *		Pointer to pixels = pStipple->devPrivate.ptr
     */
    pStipple = pGC->stipple;

    stwidth = pStipple->devKind / MFB_PGSZB;
    stippleWidth = pStipple->drawable.width;
    stippleHeight = pStipple->drawable.height;

    /*
     *	The Target:
     *		Depth = INTER_PLANES
     *		Width = determined from *pwidth
     *		Groups per scanline = ngwidth
     *		Pointer to pixels = addrgBase
     */

    iplGetGroupWidthAndPointer (pDrawable, ngwidth, addrgBase)

    /* this replaces rotating the stipple. Instead we just adjust the offset
     * at which we start grabbing bits from the stipple.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the stipple bounds.
     */
    modulus (pGC->patOrg.x, stippleWidth, xSrc);
    xSrc += pDrawable->x - stippleWidth;
    modulus (pGC->patOrg.y, stippleHeight, ySrc);
    ySrc += pDrawable->y - stippleHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % stippleHeight;
	x = ppt->x;
	pdst = addrgBase + (ppt->y * ngwidth);
        psrcS = (unsigned long *) pStipple->devPrivate.ptr + (iline * stwidth);

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
	        int xtemp, tmpx;
		register unsigned long *ptemp;
		INTER_DECLAREG(*pdsttmp);
		/*
		 *  Do a stripe through the stipple & destination w pixels
		 *  wide.  w is not more than:
		 *	-	the width of the destination
		 *	-	the width of the stipple
		 *	-	the distance between x and the next word 
		 *		boundary in the destination
		 *	-	the distance between x and the next word
		 *		boundary in the stipple
		 */

		/* width of dest/stipple */
                xrem = (x - xSrc) % stippleWidth;
	        w = min((stippleWidth - xrem), width);
		/* dist to word bound in dest */
		w = min(w, INTER_PPG - (x & INTER_PIM));
		/* dist to word bound in stip */
		w = min(w, MFB_PPW - (x & MFB_PIM));

	        xtemp = (xrem & MFB_PIM);
	        ptemp = (unsigned long *)(psrcS + (xrem >> MFB_PWSH));
		tmpx = x & INTER_PIM;
		pdsttmp = pdst + (x >> INTER_PGSH) * INTER_PLANES;
		switch ( pGC->fillStyle ) {
		    case FillOpaqueStippled:
			INTER_getstipplepixelsb(ptemp,xtemp,w,bgfill,fgfill,
				tmpDst1);
			INTER_putbitsrop(tmpDst1, tmpx, w, pdsttmp, 
				pGC->planemask, rop);
			break;
		    case FillStippled:
			/* Fill tmpSrc with the source pixels */
			INTER_getbits(pdsttmp, tmpx, w, tmpSrc);
			INTER_getstipplepixels(ptemp, xtemp, w, 0, tmpSrc, 
				tmpDst1);
			if (rop != stiprop) {
			    INTER_putbitsrop(fgfill, 0, w, tmpSrc, pGC->planemask, stiprop);
			} else {
			    INTER_COPY(fgfill, tmpSrc);
			}
			INTER_getstipplepixels(ptemp, xtemp, w, 1, tmpSrc, tmpDst2);
			INTER_OR(tmpDst1,tmpDst2,tmpDst2);
			INTER_putbitsrop(tmpDst2, tmpx, w, pdsttmp, 
				pGC->planemask, rop);
		}
		x += w;
		width -= w;
	    }
	}
	ppt++;
	pwidth++;
    }

    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}

