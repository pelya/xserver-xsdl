/* $XFree86: xc/programs/Xserver/iplan2p4/iplhrzvert.c,v 3.0 1996/08/18 01:54:48 dawes Exp $ */
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
/* $XConsortium: iplhrzvert.c,v 1.8 94/04/17 20:28:51 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"

#include "gc.h"
#include "window.h"
#include "pixmap.h"
#include "region.h"

#include "ipl.h"

#include "iplmskbits.h"

/* horizontal solid line
   abs(len) > 1
*/
iplHorzS(rop, andp, xorp, addrg, ngwidth, x1, y1, len)
register int rop;
INTER_DECLARERRAX(andp);
INTER_DECLARERRAX(xorp);
INTER_DECLAREG(*addrg); /* pointer to base of bitmap */
int ngwidth;		/* width in groups of bitmap */
int x1;			/* initial point */ 
int y1;
int len;		/* length of line */
{
    register int ngmiddle;
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);

    addrg = addrg + y1 * ngwidth + (x1 >> INTER_PGSH) * INTER_PLANES; 
    /* all bits inside same group */
    if ( ((x1 & INTER_PIM) + len) < INTER_PPG)
    {
	INTER_maskpartialbits(x1, len, startmask);
	INTER_DoMaskRRop(addrg, andp, xorp, startmask, addrg);
    }
    else
    {
	INTER_maskbits(x1, len, startmask, endmask, ngmiddle);
	    if (startmask)
	    {
		INTER_DoMaskRRop(addrg, andp, xorp, startmask, addrg);
		addrg += INTER_PLANES;
	    }
		while (ngmiddle--)
		{
		    INTER_DoRRop(addrg, andp, xorp, addrg);
		    addrg += INTER_PLANES;
		}
	    if (endmask)
		INTER_DoMaskRRop(addrg, andp, xorp, endmask, addrg);
    }
}

/* vertical solid line */

iplVertS(rop, andp, xorp, addrg, ngwidth, x1, y1, len)
int rop;
INTER_DECLARERRAX(andp);
INTER_DECLARERRAX(xorp);
INTER_DECLAREG(*addrg);	/* pointer to base of bitmap */
register int ngwidth;	/* width in groups of bitmap */
int x1, y1;		/* initial point */
register int len;	/* length of line */
{
    addrg = addrg + (y1 * ngwidth) + (x1 >> INTER_PGSH) * INTER_PLANES;
    while (len--)
    {
	INTER_DoMaskRRop(addrg, andp, xorp, iplmask[x1 & INTER_PIM], addrg);
	addrg += ngwidth;
    }
}
