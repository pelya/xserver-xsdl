/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/
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
/* $XConsortium: ppcPolyRec.c /main/4 1996/02/21 17:58:11 kaleb $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf4bpp.h"

void
xf4bppPolyRectangle(pDraw, pGC, nrects, pRects)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    int i;
    xRectangle *pR = pRects;
    xRectangle *tmprects, *tmprectsinit;
    int lw, fs, ss;

    if ( ! ( tmprectsinit = tmprects = (xRectangle *)ALLOCATE_LOCAL( ( sizeof ( xRectangle ) * nrects ) << 2 ) ) )
	return;

    lw = pGC->lineWidth;
    ss = lw >> 1;		/* skinny side of line */
    fs = ( lw + 1 ) >> 1;	/* fat side of line */

    for (i=0; i<nrects; i++)
    {
	tmprects->x = pR->x - ss;
	tmprects->y = pR->y - ss;
	tmprects->width = pR->width + lw;
	tmprects->height = lw;
	tmprects++;

	tmprects->x = pR->x - ss;
	tmprects->y = pR->y + fs;
	tmprects->width = lw;
	tmprects->height = pR->height - lw;
	tmprects++;

	tmprects->x = pR->x + pR->width - ss;
	tmprects->y = pR->y + fs;
	tmprects->width = lw;
	tmprects->height = pR->height - lw;
	tmprects++;

	tmprects->x = pR->x - ss;
	tmprects->y = pR->y + pR->height - ss;
	tmprects->width = pR->width + lw;
	tmprects->height = lw;
	tmprects++;

	pR++;
    }

    (* pGC->ops->PolyFillRect)( pDraw, pGC, nrects << 2, tmprectsinit );

    DEALLOCATE_LOCAL( tmprectsinit );
    return ;
}
