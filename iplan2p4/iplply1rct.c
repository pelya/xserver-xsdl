/* $XFree86: xc/programs/Xserver/iplan2p4/iplply1rct.c,v 3.1 1998/03/20 21:08:09 hohndel Exp $ */
/*
 * $XConsortium: iplply1rct.c,v 1.14 94/04/17 20:28:56 dpw Exp $
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

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "mistruct.h"

#include "ipl.h"
#include "iplrrop.h"

#include "iplmskbits.h"

void
INTER_RROP_NAME(iplFillPoly1Rect) (pDrawable, pGC, shape, mode, count, ptsIn)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		count;
    DDXPointPtr	ptsIn;
{
    iplPrivGCPtr    devPriv;
    int		    ngwidth;
    unsigned short  *addrl, *addr;
    int		    maxy;
    int		    origin;
    register int    vertex1, vertex2;
    int		    c;
    BoxPtr	    extents;
    int		    clip;
    int		    y;
    int		    *vertex1p, *vertex2p;
    int		    *endp;
    int		    x1, x2;
    int		    dx1, dx2;
    int		    dy1, dy2;
    int		    e1, e2;
    int		    step1, step2;
    int		    sign1, sign2;
    int		    h;
    int		    l, r;
    INTER_DECLAREG(mask); 
    INTER_DECLAREG(bits);
    int		    nmiddle;
    INTER_RROP_DECLARE
    bits=~0;

    if (mode == CoordModePrevious)
    {
	miFillPolygon (pDrawable, pGC, shape, mode, count, ptsIn);
	return;
    }
    
    devPriv = iplGetGCPrivate(pGC);
#ifdef NO_ONE_RECT
    if (REGION_NUM_RECTS(pGC->pCompositeClip) != 1)
    {
	miFillPolygon (pDrawable, pGC, shape, mode, count, ptsIn);
	return;
    }
#endif
    origin = *((int *) &pDrawable->x);
    origin -= (origin & 0x8000) << 1;
    extents = &pGC->pCompositeClip->extents;
    INTER_RROP_FETCH_GCPRIV(devPriv);
    vertex1 = *((int *) &extents->x1) - origin;
    vertex2 = *((int *) &extents->x2) - origin - 0x00010001;
    clip = 0;
    y = 32767;
    maxy = 0;
    vertex2p = (int *) ptsIn;
    endp = vertex2p + count;
    if (shape == Convex)
    {
    	while (count--)
    	{
	    c = *vertex2p;
	    clip |= (c - vertex1) | (vertex2 - c);
	    c = intToY(c);
	    if (c < y) 
	    {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
    	}
    }
    else
    {
	int yFlip = 0;
	dx1 = 1;
	x2 = -1;
	x1 = -1;
    	while (count--)
    	{
	    c = *vertex2p;
	    clip |= (c - vertex1) | (vertex2 - c);
	    c = intToY(c);
	    if (c < y) 
	    {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
	    if (c == x1)
		continue;
	    if (dx1 > 0)
	    {
		if (x2 < 0)
		    x2 = c;
		else
		    dx2 = dx1 = (c - x1) >> 31;
	    }
	    else
		if ((c - x1) >> 31 != dx1) 
		{
		    dx1 = ~dx1;
		    yFlip++;
		}
	    x1 = c;
       	}
	x1 = (x2 - c) >> 31;
	if (x1 != dx1)
	    yFlip++;
	if (x1 != dx2)
	    yFlip++;
	if (yFlip != 2) 
	    clip = 0x8000;
    }
    if (y == maxy)
	return;

    if (clip & 0x80008000)
    {
	miFillPolygon (pDrawable, pGC, shape, mode, vertex2p - (int *) ptsIn, ptsIn);
	return;
    }

#define AddrYPlus(a,y)  ((a) + (y) * ngwidth)

    iplGetGroupWidthAndPointer(pDrawable, ngwidth, addrl);
    addrl = AddrYPlus(addrl,y + pDrawable->y);
    origin = intToX(origin);
    vertex2p = vertex1p;
    vertex2 = vertex1 = *vertex2p++;
    if (vertex2p == endp)
	vertex2p = (int *) ptsIn;
#define Setup(c,x,vertex,dx,dy,e,sign,step) {\
    x = intToX(vertex); \
    if (dy = intToY(c) - y) { \
    	dx = intToX(c) - x; \
	step = 0; \
    	if (dx >= 0) \
    	{ \
	    e = 0; \
	    sign = 1; \
	    if (dx >= dy) {\
	    	step = dx / dy; \
	    	dx = dx % dy; \
	    } \
    	} \
    	else \
    	{ \
	    e = 1 - dy; \
	    sign = -1; \
	    dx = -dx; \
	    if (dx >= dy) { \
		step = - (dx / dy); \
		dx = dx % dy; \
	    } \
    	} \
    } \
    x += origin; \
    vertex = c; \
}

#define Step(x,dx,dy,e,sign,step) {\
    x += step; \
    if ((e += dx) > 0) \
    { \
	x += sign; \
	e -= dy; \
    } \
}
    for (;;)
    {
	if (y == intToY(vertex1))
	{
	    do
	    {
	    	if (vertex1p == (int *) ptsIn)
		    vertex1p = endp;
	    	c = *--vertex1p;
	    	Setup (c,x1,vertex1,dx1,dy1,e1,sign1,step1)
	    } while (y >= intToY(vertex1));
	    h = dy1;
	}
	else
	{
	    Step(x1,dx1,dy1,e1,sign1,step1)
	    h = intToY(vertex1) - y;
	}
	if (y == intToY(vertex2))
	{
	    do
	    {
	    	c = *vertex2p++;
	    	if (vertex2p == endp)
		    vertex2p = (int *) ptsIn;
	    	Setup (c,x2,vertex2,dx2,dy2,e2,sign2,step2)
	    } while (y >= intToY(vertex2));
	    if (dy2 < h)
		h = dy2;
	}
	else
	{
	    Step(x2,dx2,dy2,e2,sign2,step2)
	    if ((c = (intToY(vertex2) - y)) < h)
		h = c;
	}
	/* fill spans for this segment */
	y += h;
	for (;;)
	{
	    l = x1;
	    r = x2;
	    nmiddle = x2 - x1;
    	    if (nmiddle < 0)
	    {
	    	nmiddle = -nmiddle;
	    	l = x2;
	    	r = x1;
    	    }
	    c = l & INTER_PIM;
	    l -= c;

	    addr = addrl + (l >> INTER_PGSH) * INTER_PLANES;
	    if (c + nmiddle < INTER_PPG)
	    {
	    	mask = (bits >> c) ^ (bits >> (c+nmiddle));
	    	INTER_RROP_SOLID_MASK(addr,mask);
	    }
	    else
	    {
	    	if (c)
	    	{
	    	    mask = bits >> c;
	    	    INTER_RROP_SOLID_MASK(addr,mask);
	    	    nmiddle += c - INTER_PPG;
	    	    INTER_NEXT_GROUP(addr);
	    	}
	    	nmiddle >>= INTER_PGSH;
		while (--nmiddle >= 0) {
		    INTER_RROP_SOLID(addr); INTER_NEXT_GROUP(addr);
		}
	    	if (mask = ~(bits >> (r & INTER_PIM)))
	    	    INTER_RROP_SOLID_MASK(addr,mask);
	    }
	    if (!--h)
		break;
	    addrl = AddrYPlus (addrl, 1);
	    Step(x1,dx1,dy1,e1,sign1,step1)
	    Step(x2,dx2,dy2,e2,sign2,step2)
	}
	if (y == maxy)
	    break;
	addrl = AddrYPlus (addrl, 1);
    }
}
