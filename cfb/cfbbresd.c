/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


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
/* $Xorg: cfbbresd.c,v 1.4 2001/02/09 02:04:37 xorgcvs Exp $ */
#include "X.h"
#include "misc.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "miline.h"

/* Dashed bresenham line */

void
cfbBresD(rrops,
	 pdashIndex, pDash, numInDashList, pdashOffset, isDoubleDash,
	 addrl, nlwidth,
	 signdx, signdy, axis, x1, y1, e, e1, e2, len)
    cfbRRopPtr	    rrops;
    int		    *pdashIndex;	/* current dash */
    unsigned char   *pDash;		/* dash list */
    int		    numInDashList;	/* total length of dash list */
    int		    *pdashOffset;	/* offset into current dash */
    int		    isDoubleDash;
    unsigned long   *addrl;		/* pointer to base of bitmap */
    int		    nlwidth;		/* width in longwords of bitmap */
    int		    signdx, signdy;	/* signs of directions */
    int		    axis;		/* major axis (Y_AXIS or X_AXIS) */
    int		    x1, y1;		/* initial point */
    register int    e;			/* error accumulator */
    register int    e1;			/* bresenham increments */
    int		    e2;
    int		    len;		/* length of line */
{
#ifdef PIXEL_ADDR
    register PixelType	*addrp;
#endif
    register		int e3 = e2-e1;
    int			dashIndex;
    int			dashOffset;
    int			dashRemaining;
    unsigned long	xorFg, andFg, xorBg, andBg;
    Bool		isCopy;
    int			thisDash;

    dashOffset = *pdashOffset;
    dashIndex = *pdashIndex;
    isCopy = (rrops[0].rop == GXcopy && rrops[1].rop == GXcopy);
    xorFg = rrops[0].xor;
    andFg = rrops[0].and;
    xorBg = rrops[1].xor;
    andBg = rrops[1].and;
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

#ifdef PIXEL_ADDR

#define Loop(store) while (thisDash--) {\
			store; \
 			BresStep(addrp+=signdy,addrp+=signdx) \
		    }
    /* point to first point */
    nlwidth <<= PWSH;
    addrp = (PixelType *)(addrl) + (y1 * nlwidth) + x1;
    signdy *= nlwidth;
    if (axis == Y_AXIS)
    {
	int t;

	t = signdx;
	signdx = signdy;
	signdy = t;
    }

    if (isCopy)
    {
	for (;;)
	{ 
	    len -= thisDash;
	    if (dashIndex & 1) {
		if (isDoubleDash) {
		    Loop(*addrp = xorBg)
		} else {
		    Loop(;)
		}
	    } else {
		Loop(*addrp = xorFg)
	    }
	    if (!len)
		break;
	    NextDash
	}
    }
    else
    {
	for (;;)
	{ 
	    len -= thisDash;
	    if (dashIndex & 1) {
		if (isDoubleDash) {
		    Loop(*addrp = DoRRop(*addrp,andBg, xorBg))
		} else {
		    Loop(;)
		}
	    } else {
		Loop(*addrp = DoRRop(*addrp,andFg, xorFg))
	    }
	    if (!len)
		break;
	    NextDash
	}
    }
#else /* !PIXEL_ADDR */
    {
    	register unsigned long	tmp;
	unsigned long		startbit, bit;

    	/* point to longword containing first point */
    	addrl = (addrl + (y1 * nlwidth) + (x1 >> PWSH));
    	signdy = signdy * nlwidth;

	if (signdx > 0)
	    startbit = cfbmask[0];
	else
	    startbit = cfbmask[PPW-1];
    	bit = cfbmask[x1 & PIM];

#define X_Loop(store)	while(thisDash--) {\
			    store; \
		    	    BresStep(addrl += signdy, \
		    	     	     if (signdx > 0) \
		    	     	     	 bit = SCRRIGHT(bit,1); \
		    	     	     else \
		    	     	     	 bit = SCRLEFT(bit,1); \
		    	     	     if (!bit) \
		    	     	     { \
		    	     	     	 bit = startbit; \
		    	     	     	 addrl += signdx; \
		    	     	     }) \
			}
#define Y_Loop(store)	while(thisDash--) {\
			    store; \
		    	    BresStep(if (signdx > 0) \
		    	     	     	 bit = SCRRIGHT(bit,1); \
		    	     	     else \
		    	     	     	 bit = SCRLEFT(bit,1); \
		    	     	     if (!bit) \
		    	     	     { \
		    	     	     	 bit = startbit; \
		    	     	     	 addrl += signdx; \
		    	     	     }, \
				     addrl += signdy) \
			}

    	if (axis == X_AXIS)
    	{
	    for (;;)
	    {
	    	len -= thisDash;
	    	if (dashIndex & 1) {
		    if (isDoubleDash) {
		    	X_Loop(*addrl = DoMaskRRop(*addrl, andBg, xorBg, bit));
		    } else {
		    	X_Loop(;)
		    }
	    	} else {
		    X_Loop(*addrl = DoMaskRRop(*addrl, andFg, xorFg, bit));
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
		    	Y_Loop(*addrl = DoMaskRRop(*addrl, andBg, xorBg, bit));
		    } else {
		    	Y_Loop(;)
		    }
	    	} else {
		    Y_Loop(*addrl = DoMaskRRop(*addrl, andFg, xorFg, bit));
	    	}
	    	if (!len)
		    break;
	    	NextDash
	    }
    	} /* else Y_AXIS */
    }
#endif
    *pdashIndex = dashIndex;
    *pdashOffset = pDash[dashIndex] - dashRemaining;
}
