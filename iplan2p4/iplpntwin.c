/* $XFree86: xc/programs/Xserver/iplan2p4/iplpntwin.c,v 3.0 1996/08/18 01:55:01 dawes Exp $ */
/* $XConsortium: iplpntwin.c,v 5.18 94/04/17 20:28:57 dpw Exp $ */
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

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "ipl.h"
#include "mi.h"

void
iplPaintWindow(pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    register iplPrivWin	*pPrivWin;
    WindowPtr	pBgWin;

    pPrivWin = iplGetWindowPrivate(pWin);

    switch (what) {
    case PW_BACKGROUND:
	switch (pWin->backgroundState) {
	case None:
	    break;
	case ParentRelative:
	    do {
		pWin = pWin->parent;
	    } while (pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
							     what);
	    break;
	case BackgroundPixmap:
	    if (pPrivWin->fastBackground)
	    {
		iplFillBoxTile32 ((DrawablePtr)pWin,
				  (int)REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion),
				  pPrivWin->pRotatedBackground);
	    }
	    else
	    {
		iplFillBoxTileOdd ((DrawablePtr)pWin,
				   (int)REGION_NUM_RECTS(pRegion),
				   REGION_RECTS(pRegion),
				   pWin->background.pixmap,
				   (int) pWin->drawable.x, (int) pWin->drawable.y);
	    }
	    break;
	case BackgroundPixel:
	    iplFillBoxSolid ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel);
	    break;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel)
	{
	    iplFillBoxSolid ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel);
	}
	else if (pPrivWin->fastBorder)
	{
	    iplFillBoxTile32 ((DrawablePtr)pWin,
			      (int)REGION_NUM_RECTS(pRegion),
			      REGION_RECTS(pRegion),
			      pPrivWin->pRotatedBorder);
	}
	else
	{
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);

	    iplFillBoxTileOdd ((DrawablePtr)pWin,
			       (int)REGION_NUM_RECTS(pRegion),
			       REGION_RECTS(pRegion),
			       pWin->border.pixmap,
			       (int) pBgWin->drawable.x,
 			       (int) pBgWin->drawable.y);
	}
	break;
    }
}

/*
 * Use the RROP macros in copy mode
 */

#define RROP GXcopy
#include "iplrrop.h"
#include "iplmskbits.h"


# define Expand(left, right, leftAdjust) { \
    int widthStep; \
    widthStep = widthDst - (nmiddle + leftAdjust) * INTER_PLANES; \
    while (h--) { \
	left \
	m = nmiddle; \
	INTER_RROP_SPAN(pdst, m); \
	right \
	pdst += widthStep; \
    } \
}

void
iplFillBoxSolid (pDrawable, nBox, pBox, pixel)
    DrawablePtr	    pDrawable;
    int		    nBox;
    BoxPtr	    pBox;
    unsigned long   pixel;
{
    INTER_DECLAREG(*pdstBase);
    int		    widthDst;
    register int    h;
    INTER_DECLAREGP(rrop_xor);
    INTER_DECLAREG(*pdst);
    INTER_DECLAREG(leftMask);
    INTER_DECLAREG(rightMask);
    int		    nmiddle;
    register int    m;
    int		    w;

    iplGetGroupWidthAndPointer(pDrawable, widthDst, pdstBase);

    INTER_PFILL(pixel, rrop_xor);
    for (; nBox; nBox--, pBox++)
    {
    	pdst = pdstBase + pBox->y1 * widthDst;
    	h = pBox->y2 - pBox->y1;
	w = pBox->x2 - pBox->x1;
	pdst += (pBox->x1 >> INTER_PGSH) * INTER_PLANES;
	if ((pBox->x1 & INTER_PIM) + w <= INTER_PPG)
	{
	    INTER_maskpartialbits(pBox->x1, w, leftMask);
	    while (h--) {
		INTER_COPYM(rrop_xor, pdst, leftMask, pdst);
		pdst += widthDst;
	    }
	}
	else
	{
	    INTER_maskbits (pBox->x1, w, leftMask, rightMask, nmiddle);
	    if (leftMask)
	    {
		if (rightMask)
		{
		    Expand (INTER_RROP_SOLID_MASK (pdst, leftMask); 
			    INTER_NEXT_GROUP(pdst);,
			    INTER_RROP_SOLID_MASK (pdst, rightMask); ,
			    1)
		}
		else
		{
		    Expand (INTER_RROP_SOLID_MASK (pdst, leftMask); 
			    INTER_NEXT_GROUP(pdst);,
			    ;,
			    1)
		}
	    }
	    else
	    {
		if (rightMask)
		{
		    Expand (;,
			    INTER_RROP_SOLID_MASK (pdst, rightMask);,
			    0)
		}
		else
		{
		    Expand (;,
			    ;,
			    0)
		}
	    }
	}
    }
}

void
iplFillBoxTile32 (pDrawable, nBox, pBox, tile)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    BoxPtr 	    pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    tile;	/* rotated, expanded tile */
{
    INTER_DECLAREGP(rrop_xor);	
    INTER_DECLAREG(*pdst);
    register int	    m;
    INTER_DECLAREG(*psrc);
    int			    tileHeight;

    int			    widthDst;
    int			    w;
    int			    h;
    INTER_DECLAREG(leftMask);
    INTER_DECLAREG(rightMask);
    int			    nmiddle;
    int			    y;
    int			    srcy;

    INTER_DECLAREG(*pdstBase);

    tileHeight = tile->drawable.height;
    psrc = (unsigned short *)tile->devPrivate.ptr;


    iplGetGroupWidthAndPointer (pDrawable, widthDst, pdstBase);

    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;
	y = pBox->y1;
	pdst = pdstBase + (pBox->y1 * widthDst) + 
		(pBox->x1 >> INTER_PGSH) * INTER_PLANES;
	srcy = y % tileHeight;

#define StepTile    INTER_COPY(psrc + srcy * INTER_PLANES, rrop_xor); \
		    ++srcy; \
		    if (srcy == tileHeight) \
		        srcy = 0;

	if ( ((pBox->x1 & INTER_PIM) + w) < INTER_PPG)
	{
	    INTER_maskpartialbits(pBox->x1, w, leftMask);
	    rightMask = ~leftMask;
	    while (h--)
	    {
		StepTile
		INTER_MSKINSM(rightMask, 0, pdst, leftMask, 0, rrop_xor, pdst);
		pdst += widthDst;
	    }
	}
	else
	{
	    INTER_maskbits(pBox->x1, w, leftMask, rightMask, nmiddle);

	    if (leftMask)
	    {
		if (rightMask)
		{
		    Expand (StepTile
			    INTER_RROP_SOLID_MASK(pdst, leftMask); 
			    INTER_NEXT_GROUP(pdst);,
			    INTER_RROP_SOLID_MASK(pdst, rightMask);,
			    1)
		}
		else
		{
		    Expand (StepTile
			    INTER_RROP_SOLID_MASK(pdst, leftMask); 
			    INTER_NEXT_GROUP(pdst);,
			    ;,
			    1)
		}
	    }
	    else
	    {
		if (rightMask)
		{
		    Expand (StepTile
			    ,
			    INTER_RROP_SOLID_MASK(pdst, rightMask);,
			    0)
		}
		else
		{
		    Expand (StepTile
			    ,
			    ;,
			    0)
		}
	    }
	}
        pBox++;
    }
}
