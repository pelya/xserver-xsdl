/* $XFree86: xc/programs/Xserver/iplan2p4/iplbres.c,v 3.0 1996/08/18 01:54:36 dawes Exp $ */
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
/* $XConsortium: iplbres.c,v 1.15 94/04/17 20:28:45 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "misc.h"
#include "ipl.h"
#include "servermd.h"
#include "miline.h"

#include "iplmskbits.h"

/* Solid bresenham line */
/* NOTES
   e2 is used less often than e1, so it's not in a register
*/

void
iplBresS(rop, andp, xorp, addrg, ngwidth, signdx, signdy, axis, x1, y1, e, e1,
	 e2, len)
    int		    rop;
    INTER_DECLARERRAX(andp);
    INTER_DECLARERRAX(xorp);
    INTER_DECLAREG(*addrg);		/* pointer to base of bitmap */
    int		    ngwidth;		/* width in longwords of bitmap */
    register int    signdx;
    int		    signdy;		/* signs of directions */
    int		    axis;		/* major axis (Y_AXIS or X_AXIS) */
    int		    x1, y1;		/* initial point */
    register int    e;			/* error accumulator */
    register int    e1;			/* bresenham increments */
    int		    e2;
    int		    len;		/* length of line */
{
    register int	e3 = e2-e1;
    INTER_DECLAREG(bit);
    INTER_DECLAREG(leftbit);
    INTER_DECLAREG(rightbit);

    /* point to longword containing first point */
    addrg = addrg + y1 * ngwidth + (x1 >> INTER_PGSH) * INTER_PLANES; 

    if (signdy < 0)
	    ngwidth = -ngwidth;
    e = e-e1;			/* to make looping easier */

    leftbit = iplmask[0];
    rightbit = iplmask[INTER_PPG-1];
    bit = iplmask[x1 & INTER_PIM];

    if (axis == X_AXIS)
    {
	if (signdx > 0)
	{
	    while (len--)
	    { 
		INTER_DoMaskRRop(addrg, andp, xorp, bit, addrg);
		bit = bit >> 1;
		e += e1;
		if (e >= 0)
		{
		    addrg += ngwidth;
		    e += e3;
		}
		if (!bit)
		{
		    bit = leftbit;
		    addrg += INTER_PLANES;
		}
	    }
	}
	else
	{
	    while (len--)
	    { 
		INTER_DoMaskRRop(addrg, andp, xorp, bit, addrg);
		e += e1;
		bit = bit << 1;
		if (e >= 0)
		{
		    addrg += ngwidth;
		    e += e3;
		}
		if (!bit)
		{
		    bit = rightbit;
		    addrg -= INTER_PLANES;
		}
	    }
	}
    } /* if X_AXIS */
    else
    {
	if (signdx > 0)
	{
	    while(len--)
	    {
		INTER_DoMaskRRop(addrg, andp, xorp, bit, addrg);
		e += e1;
		if (e >= 0)
		{
		    bit = bit >> 1;
		    if (!bit)
		    {
			bit = leftbit;
			addrg += INTER_PLANES;
		    }
		    e += e3;
		}
		addrg += ngwidth;
	    }
	}
	else
	{
	    while(len--)
	    {
		INTER_DoMaskRRop(addrg, andp, xorp, bit, addrg);
		e += e1;
		if (e >= 0)
		{
		    bit = bit << 1;
		    if (!bit)
		    {
			bit = rightbit;
			addrg -= INTER_PLANES;
		    }
		    e += e3;
		}
		addrg += ngwidth;
	    }
	}
    } /* else Y_AXIS */
}
