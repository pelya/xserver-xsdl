/* $XFree86: xc/programs/Xserver/iplan2p4/iplgetsp.c,v 3.0 1996/08/18 01:54:47 dawes Exp $ */
/* $XConsortium: iplgetsp.c,v 5.14 94/04/17 20:28:50 dpw Exp $ */
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
#include "region.h"
#include "gc.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "ipl.h"
#include "iplmskbits.h"
#include "iplpack.h"

/* GetSpans -- for each span, gets bits from drawable starting at ppt[i]
 * and continuing for pwidth[i] bits
 * Each scanline returned will be server scanline padded, i.e., it will come
 * out to an integral number of words.
 */
void
iplGetSpans(pDrawable, wMax, ppt, pwidth, nspans, pchardstStart)
    DrawablePtr		pDrawable;	/* drawable from which to get bits */
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
    char		*pchardstStart; /* where to put the bits */
{
    unsigned long *pdst = (unsigned long *)pchardstStart;
    INTER_DECLAREG(*psrc);		/* where to get the bits */
    INTER_DECLAREGP(tmpSrc);		/* scratch buffer for bits */
    INTER_DECLAREG(*psrcBase);		/* start of src bitmap */
    int			widthSrc;	/* width of pixmap in bytes */
    register DDXPointPtr pptLast;	/* one past last point to get */
    int         	xEnd;		/* last pixel to copy from */
    register int	nstart; 
    int	 		nend; 
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);
    int			nlMiddle, nl, srcBit;
    int			w,longs;
    INTER_DECLAREG(*tmppdst);
    INTER_DECLAREG(*ipdst);

    switch (pDrawable->bitsPerPixel) {
	case 1:
	    mfbGetSpans(pDrawable, wMax, ppt, pwidth, nspans, pchardstStart);
	    return;
	case INTER_PLANES:
	    break;
	default:
	    FatalError("iplGetSpans: invalid depth\n");
    }

    longs = NUM_LONGS(INTER_PLANES, 0, wMax);
    tmppdst = (unsigned short *) 
	ALLOCATE_LOCAL(NUM_TEMP_BYTES(INTER_PLANES, longs));
    iplGetGroupWidthAndPointer (pDrawable, widthSrc, psrcBase) 

    pptLast = ppt + nspans;
    while(ppt < pptLast)
    {
	xEnd = min(ppt->x + *pwidth, (widthSrc / INTER_PLANES) << INTER_PGSH);
	psrc = psrcBase + ppt->y * widthSrc + 
		(ppt->x >> INTER_PGSH) * INTER_PLANES; 
	w = xEnd - ppt->x;
	srcBit = ppt->x & INTER_PIM;
        ipdst = tmppdst;

	if (srcBit + w <= INTER_PPG) 
	{ 
	    INTER_getbits(psrc, srcBit, w, tmpSrc);
	    INTER_putbits(tmpSrc, 0, w, ipdst, ~((unsigned long)0)); 
	} 
	else 
	{ 
	    INTER_maskbits(ppt->x, w, startmask, endmask, nlMiddle);
	    nstart = 0; 
	    if (startmask) 
	    { 
		nstart = INTER_PPG - srcBit; 
		INTER_getbits(psrc, srcBit, nstart, tmpSrc);
		INTER_putbits(tmpSrc, 0, nstart, ipdst, ~((unsigned long)0));
		if(srcBit + nstart >= INTER_PPG)
		    INTER_NEXT_GROUP(psrc);
	    } 
	    nl = nlMiddle; 
	    while (nl--) 
	    { 
		INTER_putbits(psrc, nstart, INTER_PPG, ipdst, ~((unsigned long)0));
		INTER_NEXT_GROUP(psrc);
		INTER_NEXT_GROUP(ipdst);
	    } 
	    if (endmask) 
	    { 
		nend = xEnd & INTER_PIM; 
		INTER_getbits(psrc, 0, nend, tmpSrc);
		INTER_putbits(tmpSrc, nstart, nend, ipdst, ~((unsigned long)0));
	    } 
	} 
	longs=(w * INTER_PLANES + 31)/32;
	iplPackLine(INTER_PLANES, longs, tmppdst, pdst);
	pdst+=longs;
        ppt++;
	pwidth++;
    }
    DEALLOCATE_LOCAL(tmppdst);
}
