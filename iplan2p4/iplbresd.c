/* $XFree86: xc/programs/Xserver/iplan2p4/iplbresd.c,v 3.0 1996/08/18 01:54:38 dawes Exp $ */
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
/* $XConsortium: iplbresd.c,v 1.16 94/04/17 20:28:45 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "misc.h"
#include "ipl.h"
#include "miline.h"
#include "iplmskbits.h"

/* Dashed bresenham line */

void
iplBresD(rrops,
	 pdashIndex, pDash, numInDashList, pdashOffset, isDoubleDash,
	 addrg, ngwidth,
	 signdx, signdy, axis, x1, y1, e, e1, e2, len)
    iplRRopPtr	    rrops;
    int		    *pdashIndex;	/* current dash */
    unsigned char   *pDash;		/* dash list */
    int		    numInDashList;	/* total length of dash list */
    int		    *pdashOffset;	/* offset into current dash */
    int		    isDoubleDash;
    INTER_DECLAREG(*addrg);		/* pointer to base of bitmap */
    int		    ngwidth;		/* width in groups of bitmap */
    int		    signdx, signdy;	/* signs of directions */
    int		    axis;		/* major axis (Y_AXIS or X_AXIS) */
    int		    x1, y1;		/* initial point */
    register int    e;			/* error accumulator */
    register int    e1;			/* bresenham increments */
    int		    e2;
    int		    len;		/* length of line */
{
    register		int e3 = e2-e1;
    int			dashIndex;
    int			dashOffset;
    int			dashRemaining;
    INTER_DECLARERRAX(xorFg);
    INTER_DECLARERRAX(andFg);
    INTER_DECLARERRAX(xorBg);
    INTER_DECLARERRAX(andBg);
    int			thisDash;

    dashOffset = *pdashOffset;
    dashIndex = *pdashIndex;
    xorFg = rrops[0].xorg;
    andFg = rrops[0].andg;
    xorBg = rrops[1].xorg;
    andBg = rrops[1].andg;
    dashRemaining = pDash[dashIndex] - dashOffset;
    if ((thisDash = dashRemaining) >= len)
    {
	thisDash = len;
	dashRemaining -= len;
    }
    e = e-e1;			/* to make looping easier */

#define BresStep(minor,major) {if ((e += e1) >= 0) { e += e3; minor; } major;}

#define NextDash {\
    dashIndex++; \
    if (dashIndex == numInDashList) \
	dashIndex = 0; \
    dashRemaining = pDash[dashIndex]; \
    if ((thisDash = dashRemaining) >= len) \
    { \
	dashRemaining -= len; \
	thisDash = len; \
    } \
}

    {
	INTER_DECLAREG(startbit);
	INTER_DECLAREG(bit);

    	/* point to longword containing first point */
    	addrg = addrg + (y1 * ngwidth) + (x1 >> INTER_PGSH) * INTER_PLANES;
    	signdy = signdy * ngwidth;
	signdx = signdx * INTER_PLANES;

	if (signdx > 0)
	    startbit = iplmask[0];
	else
	    startbit = iplmask[INTER_PPG-1];
    	bit = iplmask[x1 & INTER_PIM];

#define X_Loop(store)	while(thisDash--) {\
			    store; \
		    	    BresStep(addrg += signdy, \
		    	     	     if (signdx > 0) \
		    	     	     	 bit >>= 1; \
		    	     	     else \
		    	     	     	 bit <<= 1; \
		    	     	     if (!bit) \
		    	     	     { \
		    	     	     	 bit = startbit; \
		    	     	     	 addrg += signdx; \
		    	     	     }) \
			}
#define Y_Loop(store)	while(thisDash--) {\
			    store; \
		    	    BresStep(if (signdx > 0) \
					 bit >>= 1; \
		    	     	     else \
		    	     	     	 bit <<= 1; \
		    	     	     if (!bit) \
		    	     	     { \
		    	     	     	 bit = startbit; \
		    	     	     	 addrg += signdx; \
		    	     	     }, \
				     addrg += signdy) \
			}

    	if (axis == X_AXIS)
    	{
	    for (;;)
	    {
	    	len -= thisDash;
	    	if (dashIndex & 1) {
		    if (isDoubleDash) {
		    	X_Loop(
			INTER_DoMaskRRop(addrg, andBg, xorBg, bit, addrg);
			)
		    } else {
		    	X_Loop(;)
		    }
	    	} else {
		    X_Loop(INTER_DoMaskRRop(addrg, andFg, xorFg, bit, addrg));
	    	}
	    	if (!len)
		    break;
	    	NextDash
	    }
    	} /* if X_AXIS */
    	else
    	{
	    for (;;)
	    {
	    	len -= thisDash;
	    	if (dashIndex & 1) {
		    if (isDoubleDash) {
		    	Y_Loop(
			INTER_DoMaskRRop(addrg, andBg, xorBg, bit, addrg);
			)
		    } else {
		    	Y_Loop(;)
		    }
	    	} else {
		    Y_Loop(INTER_DoMaskRRop(addrg, andFg, xorFg, bit, addrg));
	    	}
	    	if (!len)
		    break;
	    	NextDash
	    }
    	} /* else Y_AXIS */
    }
    *pdashIndex = dashIndex;
    *pdashOffset = pDash[dashIndex] - dashRemaining;
}
