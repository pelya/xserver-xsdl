/* $XFree86: xc/programs/Xserver/iplan2p4/iplfillarc.c,v 3.0 1996/08/18 01:54:41 dawes Exp $ */
/************************************************************

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

********************************************************/

/* $XConsortium: iplfillarc.c,v 5.15 94/04/17 20:28:47 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "ipl.h"
#include "mifillarc.h"
#include "iplrrop.h"
#include "mi.h"
#include "iplmskbits.h"

static void
INTER_RROP_NAME(iplFillEllipseSolid) (pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    int x, y, e;
    int yk, xk, ym, xm, dx, dy, xorg, yorg;
    miFillArcRec info;
    INTER_DECLAREG(*addrgt);
    INTER_DECLAREG(*addrgb);
    INTER_DECLAREG(*addrg);
    register int n;
    int ngwidth;
    INTER_RROP_DECLARE
    register int xpos;
    register int slw;
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);
    int	nlmiddle;

    iplGetGroupWidthAndPointer (pDraw, ngwidth, addrgt);

    INTER_RROP_FETCH_GC(pGC);
    miFillArcSetup(arc, &info);
    MIFILLARCSETUP();
    xorg += pDraw->x;
    yorg += pDraw->y;
    addrgb = addrgt;
    addrgt += ngwidth * (yorg - y);
    addrgb += ngwidth * (yorg + y + dy);
    while (y)
    {
	addrgt += ngwidth;
	addrgb -= ngwidth;
	MIFILLARCSTEP(slw);
	if (!slw)
	    continue;
	xpos = xorg - x;
	addrg = addrgt + (xpos >> INTER_PGSH) * INTER_PLANES;
	if (((xpos & INTER_PIM) + slw) <= INTER_PPG)
	{
	    INTER_maskpartialbits(xpos, slw, startmask);
	    INTER_RROP_SOLID_MASK(addrg,startmask);
	    if (miFillArcLower(slw))
	    {
		addrg = addrgb + (xpos >> INTER_PGSH) * INTER_PLANES;
		INTER_RROP_SOLID_MASK(addrg, startmask);
	    }
	    continue;
	}
	INTER_maskbits(xpos, slw, startmask, endmask, nlmiddle);
	if (startmask)
	{
	    INTER_RROP_SOLID_MASK(addrg, startmask);
	    INTER_NEXT_GROUP(addrg);
	}
	n = nlmiddle;
	INTER_RROP_SPAN(addrg,n)

	if (endmask)
	    INTER_RROP_SOLID_MASK(addrg, endmask);
	if (!miFillArcLower(slw))
	    continue;
	addrg = addrgb + (xpos >> INTER_PGSH) * INTER_PLANES;
	if (startmask)
	{
	    INTER_RROP_SOLID_MASK(addrg, startmask);
	    INTER_NEXT_GROUP(addrg);
	}
	n = nlmiddle;
	INTER_RROP_SPAN(addrg, n);
	if (endmask)
	    INTER_RROP_SOLID_MASK(addrg, endmask);
    }
}

#define FILLSPAN(xl,xr,addr) \
    if (xr >= xl) \
    { \
	n = xr - xl + 1; \
	addrg = addr + (xl >> INTER_PGSH) * INTER_PLANES; \
	if (((xl & INTER_PIM) + n) <= INTER_PPG) \
	{ \
	    INTER_maskpartialbits(xl, n, startmask); \
	    INTER_RROP_SOLID_MASK(addrg, startmask); \
	} \
	else \
	{ \
	    INTER_maskbits(xl, n, startmask, endmask, n); \
	    if (startmask) \
	    { \
		INTER_RROP_SOLID_MASK(addrg, startmask); \
		INTER_NEXT_GROUP(addrg); \
	    } \
	    while (n--) \
	    { \
		INTER_RROP_SOLID(addrg); \
		INTER_NEXT_GROUP(addrg); \
	    } \
	    if (endmask) \
		INTER_RROP_SOLID_MASK(addrg, endmask); \
	} \
    }

#define FILLSLICESPANS(flip,addr) \
    if (!flip) \
    { \
	FILLSPAN(xl, xr, addr); \
    } \
    else \
    { \
	xc = xorg - x; \
	FILLSPAN(xc, xr, addr); \
	xc += slw - 1; \
	FILLSPAN(xl, xc, addr); \
    }

static void
INTER_RROP_NAME(iplFillArcSliceSolid)(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    int yk, xk, ym, xm, dx, dy, xorg, yorg, slw;
    register int x, y, e;
    miFillArcRec info;
    miArcSliceRec slice;
    int xl, xr, xc;
    INTER_DECLAREG(*addrgt);
    INTER_DECLAREG(*addrgb);
    INTER_DECLAREG(*addrg);
    register int n;
    int ngwidth;
    INTER_RROP_DECLARE
    INTER_DECLAREG(startmask);
    INTER_DECLAREG(endmask);

    iplGetGroupWidthAndPointer (pDraw, ngwidth, addrgt);

    INTER_RROP_FETCH_GC(pGC);
    miFillArcSetup(arc, &info);
    miFillArcSliceSetup(arc, &slice, pGC);
    MIFILLARCSETUP();
    xorg += pDraw->x;
    yorg += pDraw->y;
    addrgb = addrgt;
    addrgt += ngwidth * (yorg - y);
    addrgb += ngwidth * (yorg + y + dy);
    slice.edge1.x += pDraw->x;
    slice.edge2.x += pDraw->x;
    while (y > 0)
    {
	addrgt += ngwidth;
	addrgb -= ngwidth;
	MIFILLARCSTEP(slw);
	MIARCSLICESTEP(slice.edge1);
	MIARCSLICESTEP(slice.edge2);
	if (miFillSliceUpper(slice))
	{
	    MIARCSLICEUPPER(xl, xr, slice, slw);
	    FILLSLICESPANS(slice.flip_top, addrgt);
	}
	if (miFillSliceLower(slice))
	{
	    MIARCSLICELOWER(xl, xr, slice, slw);
	    FILLSLICESPANS(slice.flip_bot, addrgb);
	}
    }
}

void
INTER_RROP_NAME(iplPolyFillArcSolid) (pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register xArc *arc;
    register int i;
    int x2, y2;
    BoxRec box;
    RegionPtr cclip;

    cclip = iplGetCompositeClip(pGC);
    for (arc = parcs, i = narcs; --i >= 0; arc++)
    {
	if (miFillArcEmpty(arc))
	    continue;
	if (miCanFillArc(arc))
	{
	    box.x1 = arc->x + pDraw->x;
	    box.y1 = arc->y + pDraw->y;
 	    /*
 	     * Because box.x2 and box.y2 get truncated to 16 bits, and the
 	     * RECT_IN_REGION test treats the resulting number as a signed
 	     * integer, the RECT_IN_REGION test alone can go the wrong way.
 	     * This can result in a server crash because the rendering
 	     * routines in this file deal directly with cpu addresses
 	     * of pixels to be stored, and do not clip or otherwise check
 	     * that all such addresses are within their respective pixmaps.
 	     * So we only allow the RECT_IN_REGION test to be used for
 	     * values that can be expressed correctly in a signed short.
 	     */
 	    x2 = box.x1 + (int)arc->width + 1;
 	    box.x2 = x2;
 	    y2 = box.y1 + (int)arc->height + 1;
 	    box.y2 = y2;
 	    if ( (x2 <= MAXSHORT) && (y2 <= MAXSHORT) &&
 		    (RECT_IN_REGION(pDraw->pScreen, cclip, &box) == rgnIN) )
	    {
		if ((arc->angle2 >= FULLCIRCLE) ||
		    (arc->angle2 <= -FULLCIRCLE))
		    INTER_RROP_NAME(iplFillEllipseSolid)(pDraw, pGC, arc);
		else
		    INTER_RROP_NAME(iplFillArcSliceSolid)(pDraw, pGC, arc);
		continue;
	    }
	}
	miPolyFillArc(pDraw, pGC, 1, arc);
    }
}
