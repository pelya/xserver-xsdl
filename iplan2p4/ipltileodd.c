/* $XFree86: xc/programs/Xserver/iplan2p4/ipltileodd.c,v 3.0 1996/08/18 01:55:11 dawes Exp $ */
/*
 * Fill odd tiled rectangles and spans.
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

/* $XConsortium: ipltileodd.c,v 1.16 94/04/17 20:29:06 dpw Exp $ */

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

#include "iplmskbits.h"
#include "iplmergerop.h"

#define LEFTSHIFT_AMT 0

#define LastTileBits {\
    INTER_COPY(bits, tmp); \
    if (tileEndPart) \
	INTER_MSKINSM(tileEndMask, 0, pSrc, 			\
		         ~0, tileEndLeftShift, pSrcLine, bits) 	\
    else \
	INTER_COPY(pSrc, bits); \
}

#define ResetTileBits {\
    pSrc = pSrcLine; \
    nlwSrc = widthSrc;\
    if (tileEndPart) { \
	if (INTER_PPG - xoff + tileEndPart <= INTER_PPG) {\
	    INTER_COPY(pSrc, bits); INTER_NEXT_GROUP(pSrc); \
	    nlwSrc--; \
	} else \
	    INTER_MSKINSM(~0, tileEndLeftShift, tmp, 		\
			  ~0, tileEndRightShift, bits, bits); 	\
	xoff = (xoff + xoffStep) & INTER_PIM; \
	leftShift = xoff << LEFTSHIFT_AMT; \
	rightShift = INTER_PGSZ - leftShift; \
    }\
}

#define NextTileBits {\
    if (nlwSrc == 1) {\
	LastTileBits\
    } else { \
    	if (nlwSrc == 0) {\
	    ResetTileBits\
    	} \
	if (nlwSrc == 1) {\
	    LastTileBits\
	} else {\
	    INTER_COPY(bits, tmp); \
	    INTER_COPY(pSrc, bits); INTER_NEXT_GROUP(pSrc); \
	}\
    }\
    nlwSrc--; \
}

void
INTER_MROP_NAME(iplFillBoxTileOdd) (pDrawable, nBox, pBox, tile, xrot, yrot, alu, planemask)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    register BoxPtr pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    tile;	/* tile */
    int		    xrot, yrot;
    int		    alu;
    unsigned long   planemask;
{
    int tileWidth;	/* width of tile in pixels */
    int tileHeight;	/* height of the tile */
    int widthSrc;

    int widthDst;	/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    int h;		/* height of current box */
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    int nlwSrc;		/* number of whole longwords in source */
    
    register int nlw;	/* loop version of nlwMiddle */
    int srcy;		/* current tile y position */
    int srcx;		/* current tile x position */
    int xoffDst, xoffSrc;
    int leftShift, rightShift;

    INTER_MROP_DECLARE_REG()

    INTER_DECLAREG(*pDstBase);	/* pointer to start of dest */
    INTER_DECLAREG(*pDstLine);	/* poitner to start of dest box */
    INTER_DECLAREG(*pSrcBase);	/* pointer to start of source */
    INTER_DECLAREG(*pSrcLine);	/* pointer to start of source line */
    INTER_DECLAREG(*pDst);
    INTER_DECLAREG(*pSrc);
    INTER_DECLAREGP(bits);
    INTER_DECLAREGP(tmp);
    INTER_DECLAREGP(tmp1);
    register int	   nlwPart;
    int xoffStart, xoff;
    int leftShiftStart, rightShiftStart, nlwSrcStart;
    INTER_DECLAREG(tileEndMask);
    int tileEndLeftShift, tileEndRightShift;
    int	xoffStep;
    int tileEndPart;
    int needFirst;
    unsigned short narrow[2 * INTER_PLANES];
    INTER_DECLAREG(narrowMask);
    int	    narrowShift;
    Bool    narrowTile;
    int     narrowRep;

    INTER_MROP_INITIALIZE (alu, planemask)

    tileHeight = tile->drawable.height;
    tileWidth = tile->drawable.width;
    widthSrc = tile->devKind / (INTER_PGSZB * INTER_PLANES);
    narrowTile = FALSE;

    if (widthSrc == 1)
    {
	narrowRep = INTER_PPG / tileWidth;
	narrowMask = iplendpartial [tileWidth];
	tileWidth *= narrowRep;
	narrowShift = tileWidth;
	tileWidth *= 2;
	widthSrc = 2;
	narrowTile = TRUE;
    }
    pSrcBase = (unsigned short *)tile->devPrivate.ptr;

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pDstBase)

    tileEndPart = tileWidth & INTER_PIM;
    tileEndMask = iplendpartial[tileEndPart];
    tileEndLeftShift = (tileEndPart) << LEFTSHIFT_AMT;
    tileEndRightShift = INTER_PGSZ - tileEndLeftShift;
    xoffStep = INTER_PPG - tileEndPart;
    /*
     * current assumptions: tile > 32 bits wide.
     */
    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;
	modulus (pBox->x1 - xrot, tileWidth, srcx);
	modulus (pBox->y1 - yrot, tileHeight, srcy);
	xoffDst = pBox->x1 & INTER_PIM;
	if (xoffDst + w < INTER_PPG)
	{
	    INTER_maskpartialbits(pBox->x1, w, startmask);
	    endmask = 0;
	    nlwMiddle = 0;
	}
	else
	{
	    INTER_maskbits (pBox->x1, w, startmask, endmask, nlwMiddle)
	}
	pDstLine = pDstBase + (pBox->y1 * widthDst) + 
		   (pBox->x1 >> INTER_PGSH) * INTER_PLANES;
	pSrcLine = pSrcBase + (srcy * widthSrc) * INTER_PLANES;
	xoffSrc = srcx & INTER_PIM;
	if (xoffSrc >= xoffDst)
	{
	    xoffStart = xoffSrc - xoffDst;
	    needFirst = 1;
	}
	else
	{
	    xoffStart = INTER_PPG - (xoffDst - xoffSrc);
	    needFirst = 0;
	}
	leftShiftStart = (xoffStart) << LEFTSHIFT_AMT;
	rightShiftStart = INTER_PGSZ - leftShiftStart;
	nlwSrcStart = (widthSrc - (srcx >> INTER_PGSH));
	while (h--)
	{
	    /* XXX only works when narrowShift >= INTER_PPG/2 */
	    if (narrowTile)
	    {
		int tmpnarrowRep;
		int shift=narrowShift/narrowRep;
		INTER_ANDMSK(pSrcBase + srcy * INTER_PLANES, narrowMask, tmp);
                tmpnarrowRep=narrowRep;
		/* copy tile until its nearly a whole group wide */
	        while (--tmpnarrowRep)
		    INTER_MSKINSM(~0,0,tmp,~0,shift,tmp,tmp);
		INTER_MSKINSM(~0, 0, tmp, ~0, narrowShift, tmp, narrow);
		INTER_MSKINSM(~0, INTER_PPG - narrowShift, tmp,
			      ~0, 2 * narrowShift - INTER_PPG, tmp, 
			      narrow + INTER_PLANES); 
		pSrcLine = narrow;
	    }
	    xoff = xoffStart;
	    leftShift = leftShiftStart;
	    rightShift = rightShiftStart;
	    nlwSrc = nlwSrcStart;
	    pSrc = pSrcLine + (srcx >> INTER_PGSH) * INTER_PLANES;
	    pDst = pDstLine;
	    INTER_CLR(bits);
	    if (needFirst)
	    {
		NextTileBits
	    }
	    if (startmask)
	    {
		NextTileBits
		INTER_SCRLEFT(leftShift, tmp, tmp);
 		if (rightShift != INTER_PGSZ)
		    INTER_MSKINSM(~0, 0, tmp, ~0, rightShift, bits, tmp)
		INTER_MROP_MASK (tmp, pDst, startmask, pDst);
		INTER_NEXT_GROUP(pDst);
	    }
	    nlw = nlwMiddle;
	    while (nlw)
	    {
		{
		    NextTileBits
		    if (rightShift != INTER_PGSZ)
		    {
		        INTER_MSKINSM(~0, leftShift, tmp, ~0, rightShift, bits, 
				  tmp1);
		        INTER_MROP_SOLID(tmp1, pDst, pDst);
		    }
		    else
		    {
			INTER_MROP_SOLID (tmp, pDst, pDst);
		    }
		    INTER_NEXT_GROUP(pDst);
		    nlw--;
		}
	    }
	    if (endmask)
	    {
		NextTileBits
		if (rightShift == INTER_PGSZ)
		    INTER_CLR(bits);
	        INTER_MSKINSM(~0, leftShift, tmp, ~0, rightShift, bits, tmp1);
	        INTER_MROP_MASK(tmp1, pDst, endmask, pDst);
	    }
	    pDstLine += widthDst;
	    pSrcLine += widthSrc * INTER_PLANES;
	    if (++srcy == tileHeight)
	    {
		srcy = 0;
		pSrcLine = pSrcBase;
	    }
	}
	pBox++;
    }
}

void
INTER_MROP_NAME(iplFillSpanTileOdd) (pDrawable, n, ppt, pwidth, tile, xrot, yrot, alu, planemask)
    DrawablePtr	pDrawable;
    int		n;
    DDXPointPtr	ppt;
    int		*pwidth;
    PixmapPtr	tile;
    int		xrot, yrot;
    int		alu;
    unsigned long   planemask;
{
    int tileWidth;	/* width of tile in pixels */
    int tileHeight;	/* height of the tile */
    int widthSrc;

    int widthDst;		/* width in longwords of the dest pixmap */
    int w;		/* width of current span */
    INTER_DECLAREG(startmask);
    INTER_DECLAREG (endmask);	/* masks for reggedy bits at either end of line */
    int nlwSrc;		/* number of whole longwords in source */
    
    register int nlw;	/* loop version of nlwMiddle */
    int srcy;		/* current tile y position */
    int srcx;		/* current tile x position */
    int xoffDst, xoffSrc;
    int leftShift, rightShift;

    INTER_MROP_DECLARE_REG()

    INTER_DECLAREG(*pDstBase);	/* pointer to start of dest */
    INTER_DECLAREG(*pDstLine);	/* poitner to start of dest box */
    INTER_DECLAREG(*pSrcBase);	/* pointer to start of source */
    INTER_DECLAREG(*pSrcLine);	/* pointer to start of source line */
    INTER_DECLAREG(*pDst);
    INTER_DECLAREG(*pSrc);
    INTER_DECLAREGP(bits);
    INTER_DECLAREGP(tmp);
    INTER_DECLAREGP(tmp1);
    register int	   nlwPart;
    int xoffStart, xoff;
    int leftShiftStart, rightShiftStart, nlwSrcStart;
    INTER_DECLAREG(tileEndMask);
    int tileEndLeftShift, tileEndRightShift;
    int	xoffStep;
    int tileEndPart;
    int needFirst;
    unsigned short narrow[2 * INTER_PLANES];
    INTER_DECLAREG(narrowMask);
    int	    narrowShift;
    Bool    narrowTile;
    int     narrowRep;

    INTER_MROP_INITIALIZE (alu, planemask)

    tileHeight = tile->drawable.height;
    tileWidth = tile->drawable.width;
    widthSrc = tile->devKind / (INTER_PGSZB * INTER_PLANES);
    narrowTile = FALSE;
    if (widthSrc == 1)
    {
	narrowRep = INTER_PPG / tileWidth;
	narrowMask = iplendpartial [tileWidth];
	tileWidth *= narrowRep;
	narrowShift = tileWidth;
	tileWidth *= 2;
	widthSrc = 2;
	narrowTile = TRUE;
    }
    pSrcBase = (unsigned short *)tile->devPrivate.ptr;

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pDstBase)

    tileEndPart = tileWidth & INTER_PIM;
    tileEndMask = iplendpartial[tileEndPart];
    tileEndLeftShift = (tileEndPart) << LEFTSHIFT_AMT;
    tileEndRightShift = INTER_PGSZ - tileEndLeftShift;
    xoffStep = INTER_PPG - tileEndPart;
    while (n--)
    {
	w = *pwidth++;
	modulus (ppt->x - xrot, tileWidth, srcx);
	modulus (ppt->y - yrot, tileHeight, srcy);
	xoffDst = ppt->x & INTER_PIM;
	if (xoffDst + w < INTER_PPG)
	{
	    INTER_maskpartialbits(ppt->x, w, startmask);
	    endmask = 0;
	    nlw = 0;
	}
	else
	{
	    INTER_maskbits (ppt->x, w, startmask, endmask, nlw)
	}
	pDstLine = pDstBase + (ppt->y * widthDst) + 
		   (ppt->x >> INTER_PGSH) * INTER_PLANES;
	pSrcLine = pSrcBase + (srcy * widthSrc) * INTER_PLANES;
	xoffSrc = srcx & INTER_PIM;
	if (xoffSrc >= xoffDst)
	{
	    xoffStart = xoffSrc - xoffDst;
	    needFirst = 1;
	}
	else
	{
	    xoffStart = INTER_PPG - (xoffDst - xoffSrc);
	    needFirst = 0;
	}
	leftShiftStart = (xoffStart) << LEFTSHIFT_AMT;
	rightShiftStart = INTER_PGSZ - leftShiftStart;
	nlwSrcStart = widthSrc - (srcx >> INTER_PGSH);
	/* XXX only works when narrowShift >= INTER_PPG/2 */
	if (narrowTile)
	{
	    int tmpnarrowRep;
	    int shift=narrowShift/narrowRep;
	    INTER_ANDMSK(pSrcBase + srcy * INTER_PLANES, narrowMask, tmp);
            tmpnarrowRep=narrowRep;
	    /* copy tile until its nearly a whole group wide */
	    while (--tmpnarrowRep)
		INTER_MSKINSM(~0,0,tmp,~0,shift,tmp,tmp);
	    INTER_MSKINSM(~0, 0, tmp, ~0, narrowShift, tmp, narrow);
	    INTER_MSKINSM(~0, INTER_PPG - narrowShift, tmp,
			      ~0, 2 * narrowShift - INTER_PPG, tmp, 
			      narrow + INTER_PLANES); 
	    pSrcLine = narrow;
	}
	xoff = xoffStart;
	leftShift = leftShiftStart;
	rightShift = rightShiftStart;
	nlwSrc = nlwSrcStart;
	pSrc = pSrcLine + (srcx >> INTER_PGSH) * INTER_PLANES;
	pDst = pDstLine;
	INTER_CLR(bits);
	if (needFirst)
	{
	    NextTileBits
	}
	if (startmask)
	{
	    NextTileBits
	    INTER_SCRLEFT(leftShift, tmp, tmp);
	    if (rightShift != INTER_PGSZ)
		INTER_MSKINSM(~0, 0, tmp, ~0, rightShift, bits, tmp);
	    INTER_MROP_MASK (tmp, pDst, startmask, pDst);
	    INTER_NEXT_GROUP(pDst);
	}
	while (nlw)
	{
	    {
		NextTileBits
		if (rightShift != INTER_PGSZ)
		{
		    INTER_MSKINSM(~0, leftShift, tmp, ~0, rightShift, bits, 
				  tmp1);
		    INTER_MROP_SOLID(tmp1, pDst, pDst);
		    INTER_NEXT_GROUP(pDst);
		}
		else
		{
		    INTER_MROP_SOLID (tmp, pDst, pDst);
		    INTER_NEXT_GROUP(pDst);
		}
		nlw--;
	    }
	}
	if (endmask)
	{
	    NextTileBits
	    if (rightShift == INTER_PGSZ)
		INTER_CLR(bits);
	    
	    INTER_MSKINSM(~0, leftShift, tmp, ~0, rightShift, bits, tmp1);
	    INTER_MROP_MASK(tmp1, pDst, endmask, pDst);
	}
	ppt++;
    }
}

# include "fastblt.h"

#define IncSrcPtr   INTER_NEXT_GROUP(psrc); if (!--srcRemaining) { srcRemaining = widthSrc; psrc = psrcStart; }

void
INTER_MROP_NAME(iplFillBoxTile32s) (pDrawable, nBox, pBox, tile, xrot, yrot, alu, planemask)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    register BoxPtr pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    tile;	/* tile */
    int		    xrot, yrot;
    int		    alu;
    unsigned long   planemask;
{
    int	tileWidth;	/* width of tile */
    int tileHeight;	/* height of the tile */
    int	widthSrc;	/* width in longwords of the source tile */

    int widthDst;	/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    int h;		/* height of current box */
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask); /* masks for reggedy bits at either end of line */
    int nlMiddle;	/* number of longwords between sides of boxes */
    
    register int nl;	/* loop version of nlMiddle */
    int srcy;		/* current tile y position */
    int srcx;		/* current tile x position */
    int	srcRemaining;	/* number of longwords remaining in source */
    int xoffDst, xoffSrc;
    int	srcStart;	/* number of longwords source offset at left of box */
    int	leftShift, rightShift;

    INTER_MROP_DECLARE_REG()

    INTER_DECLAREG(*pdstBase);	/* pointer to start of dest */
    INTER_DECLAREG(*pdstLine);	/* poitner to start of dest box */
    INTER_DECLAREG(*psrcBase);	/* pointer to start of source */
    INTER_DECLAREG(*psrcLine);	/* pointer to fetch point of source */
    INTER_DECLAREG(*psrcStart);	/* pointer to start of source line */
    INTER_DECLAREG(*pdst);
    INTER_DECLAREG(*psrc);
    INTER_DECLAREGP(bits);
    INTER_DECLAREGP(bits1);
    register int	    nlTemp;

    INTER_MROP_INITIALIZE (alu, planemask)

    psrcBase = (unsigned short *)tile->devPrivate.ptr;
    tileHeight = tile->drawable.height;
    tileWidth = tile->drawable.width;
    widthSrc = tile->devKind / (INTER_PGSZB * INTER_PLANES);

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pdstBase)

    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;

	/* set up source */
	modulus (pBox->x1 - xrot, tileWidth, srcx);
	modulus (pBox->y1 - yrot, tileHeight, srcy);
	xoffSrc = srcx & INTER_PIM;
	srcStart = srcx >> INTER_PGSH;
	psrcStart = psrcBase + (srcy * widthSrc) * INTER_PLANES;
	psrcLine = psrcStart + srcStart * INTER_PLANES;

	/* set up dest */
	xoffDst = pBox->x1 & INTER_PIM;
	pdstLine = pdstBase + (pBox->y1 * widthDst) + 
		(pBox->x1 >> INTER_PGSH) * INTER_PLANES;
	/* set up masks */
	if (xoffDst + w < INTER_PPG)
	{
	    INTER_maskpartialbits(pBox->x1, w, startmask);
	    endmask = 0;
	    nlMiddle = 0;
	}
	else
	{
	    INTER_maskbits (pBox->x1, w, startmask, endmask, nlMiddle)
	}
	if (xoffSrc == xoffDst)
	{
	    while (h--)
	    {
		psrc = psrcLine;
		pdst = pdstLine;
		srcRemaining = widthSrc - srcStart;
		if (startmask)
		{
		    INTER_MROP_MASK (psrc, pdst, startmask, pdst);
		    INTER_NEXT_GROUP(pdst);
		    IncSrcPtr
		}
		nlTemp = nlMiddle;
		while (nlTemp)
		{
		    nl = nlTemp;
		    if (nl > srcRemaining)
			nl = srcRemaining;

		    nlTemp -= nl;
		    srcRemaining -= nl;

		    while (nl--) {
			    INTER_MROP_SOLID (psrc, pdst, pdst);
			    INTER_NEXT_GROUP(pdst); INTER_NEXT_GROUP(psrc);
		    }

		    if (!srcRemaining)
		    {
			srcRemaining = widthSrc;
			psrc = psrcStart;
		    }
		}
		if (endmask)
		{
		    INTER_MROP_MASK (psrc, pdst, endmask, pdst);
		}
		pdstLine += widthDst;
		psrcLine += widthSrc * INTER_PLANES;
		psrcStart += widthSrc * INTER_PLANES;
		if (++srcy == tileHeight)
		{
		    psrcStart = psrcBase;
		    psrcLine = psrcStart + srcStart * INTER_PLANES;
		    srcy = 0;
		}
	    }
	}
	else
	{
	    if (xoffSrc > xoffDst)
	    {
		leftShift = (xoffSrc - xoffDst) << LEFTSHIFT_AMT;
		rightShift = INTER_PGSZ - leftShift;
	    }
	    else
	    {
		rightShift = (xoffDst - xoffSrc) << LEFTSHIFT_AMT;
		leftShift = INTER_PGSZ - rightShift;
	    }
	    while (h--)
	    {
		psrc = psrcLine;
		pdst = pdstLine;
		INTER_CLR(bits);
		srcRemaining = widthSrc - srcStart;
		if (xoffSrc > xoffDst)
		{
		    INTER_COPY(psrc, bits);
		    IncSrcPtr
		}
		if (startmask)
		{
		    INTER_SCRLEFT(leftShift, bits, bits1);
		    INTER_COPY(psrc, bits);
		    IncSrcPtr
		    INTER_MSKINSM(~0, 0, bits1, ~0, rightShift, bits, bits1);
		    INTER_MROP_MASK(bits1, pdst, startmask, pdst);
		    INTER_NEXT_GROUP(pdst);
		}
		nlTemp = nlMiddle;
		while (nlTemp)
		{
		    nl = nlTemp;
		    if (nl > srcRemaining)
			nl = srcRemaining;

		    nlTemp -= nl;
		    srcRemaining -= nl;
    
		    while (nl--) {
		    	INTER_SCRLEFT(leftShift, bits, bits1);
		    	INTER_COPY(psrc, bits); INTER_NEXT_GROUP(psrc);
		        INTER_MSKINSM(~0, 0, bits1, ~0, rightShift, bits, bits1);
		    	INTER_MROP_SOLID (bits1, pdst, pdst);
		    	INTER_NEXT_GROUP(pdst);
		    }

		    if (!srcRemaining)
		    {
			srcRemaining = widthSrc;
			psrc = psrcStart;
		    }
		}

		if (endmask)
		{
		    INTER_SCRLEFT(leftShift, bits, bits1);
		    if (endmask << rightShift)
		    {
			INTER_COPY(psrc, bits);
		        INTER_MSKINSM(~0, 0, bits1, ~0, rightShift, bits, bits1);
		    }
		    INTER_MROP_MASK (bits1, pdst, endmask, pdst);
		}
		pdstLine += widthDst;
		psrcLine += widthSrc * INTER_PLANES;
		psrcStart += widthSrc * INTER_PLANES;
		if (++srcy == tileHeight)
		{
		    psrcStart = psrcBase;
		    psrcLine = psrcStart + srcStart * INTER_PLANES;
		    srcy = 0;
		}
	    }
	}
	pBox++;
    }
}

void
INTER_MROP_NAME(iplFillSpanTile32s) (pDrawable, n, ppt, pwidth, tile, xrot, yrot, alu, planemask)
    DrawablePtr	pDrawable;
    int		n;
    DDXPointPtr	ppt;
    int		*pwidth;
    PixmapPtr	tile;
    int		xrot, yrot;
    int		alu;
    unsigned long   planemask;
{
    int	tileWidth;	/* width of tile */
    int tileHeight;	/* height of the tile */
    int	widthSrc;	/* width in longwords of the source tile */

    int widthDst;	/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);/* masks for reggedy bits at either end of line */
    int nlMiddle;	/* number of longwords between sides of boxes */
    
    register int nl;	/* loop version of nlMiddle */
    int srcy;		/* current tile y position */
    int srcx;		/* current tile x position */
    int	srcRemaining;	/* number of longwords remaining in source */
    int xoffDst, xoffSrc;
    int	srcStart;	/* number of longwords source offset at left of box */
    int	leftShift, rightShift;

    INTER_MROP_DECLARE_REG()

    INTER_DECLAREG(*pdstBase);	/* pointer to start of dest */
    INTER_DECLAREG(*pdstLine);	/* poitner to start of dest box */
    INTER_DECLAREG(*psrcBase);	/* pointer to start of source */
    INTER_DECLAREG(*psrcLine);	/* pointer to fetch point of source */
    INTER_DECLAREG(*psrcStart);	/* pointer to start of source line */
    INTER_DECLAREG(*pdst);
    INTER_DECLAREG(*psrc);
    INTER_DECLAREGP(bits);
    INTER_DECLAREGP(bits1);
    register int	    nlTemp;

    INTER_MROP_INITIALIZE (alu, planemask)

    psrcBase = (unsigned short *)tile->devPrivate.ptr;
    tileHeight = tile->drawable.height;
    tileWidth = tile->drawable.width;
    widthSrc = tile->devKind / (INTER_PGSZB * INTER_PLANES);

    iplGetGroupWidthAndPointer (pDrawable, widthDst, pdstBase)

    while (n--)
    {
	w = *pwidth++;

	/* set up source */
	modulus (ppt->x - xrot, tileWidth, srcx);
	modulus (ppt->y - yrot, tileHeight, srcy);
	xoffSrc = srcx & INTER_PIM;
	srcStart = srcx >> INTER_PGSH;
	psrcStart = psrcBase + (srcy * widthSrc) * INTER_PLANES;
	psrcLine = psrcStart + srcStart * INTER_PLANES;

	/* set up dest */
	xoffDst = ppt->x & INTER_PIM;
	pdstLine = pdstBase + (ppt->y * widthDst) + 
		(ppt->x >> INTER_PGSH) * INTER_PLANES;
	/* set up masks */
	if (xoffDst + w < INTER_PPG)
	{
	    INTER_maskpartialbits(ppt->x, w, startmask);
	    endmask = 0;
	    nlMiddle = 0;
	}
	else
	{
	    INTER_maskbits (ppt->x, w, startmask, endmask, nlMiddle)
	}

	if (xoffSrc == xoffDst)
	{
	    psrc = psrcLine;
	    pdst = pdstLine;
	    srcRemaining = widthSrc - srcStart;
	    if (startmask)
	    {
		INTER_MROP_MASK (psrc, pdst, startmask, pdst);
		INTER_NEXT_GROUP(pdst);
		IncSrcPtr
	    }
	    nlTemp = nlMiddle;
	    while (nlTemp)
	    {
		nl = nlTemp;
		if (nl > srcRemaining)
		    nl = srcRemaining;

		nlTemp -= nl;
		srcRemaining -= nl;

		while (nl--) {
			INTER_MROP_SOLID (psrc, pdst, pdst);
			INTER_NEXT_GROUP(pdst); INTER_NEXT_GROUP(psrc);
		}

		if (!srcRemaining)
		{
		    srcRemaining = widthSrc;
		    psrc = psrcStart;
		}
	    }
	    if (endmask)
	    {
		INTER_MROP_MASK (psrc, pdst, endmask, pdst);
	    }
	}
	else
	{
	    if (xoffSrc > xoffDst)
	    {
		leftShift = (xoffSrc - xoffDst) << LEFTSHIFT_AMT;
		rightShift = INTER_PGSZ - leftShift;
	    }
	    else
	    {
		rightShift = (xoffDst - xoffSrc) << LEFTSHIFT_AMT;
		leftShift = INTER_PGSZ - rightShift;
	    }
	    psrc = psrcLine;
	    pdst = pdstLine;
	    INTER_CLR(bits);
	    srcRemaining = widthSrc - srcStart;
	    if (xoffSrc > xoffDst)
	    {
		INTER_COPY(psrc, bits);
		IncSrcPtr
	    }
	    if (startmask)
	    {
		INTER_SCRLEFT(leftShift, bits, bits1);
		INTER_COPY(psrc, bits);
		IncSrcPtr
		INTER_MSKINSM(~0, 0, bits1, ~0, rightShift, bits, bits1);
		INTER_MROP_MASK(bits1, pdst, startmask, pdst);
		INTER_NEXT_GROUP(pdst);
	    }
	    nlTemp = nlMiddle;
	    while (nlTemp)
	    {
		nl = nlTemp;
		if (nl > srcRemaining)
		    nl = srcRemaining;

		nlTemp -= nl;
		srcRemaining -= nl;

		while (nl--) {
		    INTER_SCRLEFT(leftShift, bits, bits1);
		    INTER_COPY(psrc, bits); INTER_NEXT_GROUP(psrc);
		    INTER_MSKINSM(~0, 0, bits1, ~0, rightShift, bits, bits1);
		    INTER_MROP_SOLID(bits1, pdst, pdst);
		    INTER_NEXT_GROUP(pdst);
		}

		if (!srcRemaining)
		{
		    srcRemaining = widthSrc;
		    psrc = psrcStart;
		}
	    }

	    if (endmask)
	    {
		INTER_SCRLEFT(leftShift, bits, bits1);
		if (endmask << rightShift)
		{
		    INTER_COPY(psrc, bits);
		    INTER_MSKINSM(~0, 0, bits1, ~0, rightShift, bits, bits1);
		}
		INTER_MROP_MASK (bits1, pdst, endmask, pdst);
	    }
	}
	ppt++;
    }
}
