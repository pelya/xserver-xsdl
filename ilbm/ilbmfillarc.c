/* $XFree86: xc/programs/Xserver/ilbm/ilbmfillarc.c,v 3.1 1998/03/20 21:08:01 hohndel Exp $ */
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

/* $XConsortium: ilbmfillarc.c,v 5.14 94/04/17 20:28:20 dpw Exp $ */

/* Modified jun 95 by Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
   to use interleaved bitplanes instead of normal bitplanes */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "ilbm.h"
#include "maskbits.h"
#include "mifillarc.h"
#include "mi.h"

static void
ilbmFillEllipseSolid(pDraw, arc, rrops)
	DrawablePtr pDraw;
	xArc *arc;
	register unsigned char *rrops;
{
	int x, y, e;
	int yk, xk, ym, xm, dx, dy, xorg, yorg;
	register int slw;
	miFillArcRec info;
	PixelType *addrlt, *addrlb;
	register PixelType *pdst;
	PixelType *addrl;
	register int n;
	register int d;
	int nlwidth;
	register int xpos;
	PixelType startmask, endmask;
	int nlmiddle;
	int depthDst;
	int auxDst;

	ilbmGetPixelWidthAuxDepthAndPointer(pDraw, nlwidth, auxDst, depthDst,
													addrlt);
	miFillArcSetup(arc, &info);
	MIFILLARCSETUP();
	xorg += pDraw->x;
	yorg += pDraw->y;
	addrlb = addrlt;
	addrlt += auxDst * (yorg - y);
	addrlb += auxDst * (yorg + y + dy);
	while (y) {
		addrlt += auxDst;
		addrlb -= auxDst;
		MIFILLARCSTEP(slw);
		if (!slw)
			continue;
		xpos = xorg - x;
		pdst = addrl = ilbmScanlineOffset(addrlt, (xpos >> PWSH));
		if (((xpos & PIM) + slw) < PPW) {
			maskpartialbits(xpos, slw, startmask);
			for (d = 0; d < depthDst; d++, pdst += nlwidth) {	/* @@@ NEXT PLANE @@@ */
				switch (rrops[d]) {
					case RROP_BLACK:
						*pdst &= ~startmask;
						break;
					case RROP_WHITE:
						*pdst |= startmask;
						break;
					case RROP_INVERT:
						*pdst ^= startmask;
						break;
					case RROP_NOP:
						break;
				}
			}
			if (miFillArcLower(slw)) {
				pdst = ilbmScanlineOffset(addrlb, (xpos >> PWSH));

				for (d = 0; d < depthDst; d++, pdst += nlwidth) {	/* @@@ NEXT PLANE @@@ */
					switch (rrops[d]) {
						case RROP_BLACK:
							*pdst &= ~startmask;
							break;
						case RROP_WHITE:
							*pdst |= startmask;
							break;
						case RROP_INVERT:
							*pdst ^= startmask;
							break;
						case RROP_NOP:
							break;
					}
				}
			}
			continue;
		}
		maskbits(xpos, slw, startmask, endmask, nlmiddle);
		for (d = 0; d < depthDst; d++, addrl += nlwidth) {	/* @@@ NEXT PLANE @@@ */
			n = nlmiddle;
			pdst = addrl;

			switch (rrops[d]) {
				case RROP_BLACK:
					if (startmask)
						*pdst++ &= ~startmask;
					while (n--)
						*pdst++ = 0;
					if (endmask)
						*pdst &= ~endmask;
					break;

				case RROP_WHITE:
					if (startmask)
						*pdst++ |= startmask;
					while (n--)
						*pdst++ = ~0;
					if (endmask)
						*pdst |= endmask;
					break;

				case RROP_INVERT:
					if (startmask)
						*pdst++ ^= startmask;
					while (n--)
						*pdst++ ^= ~0;
					if (endmask)
						*pdst ^= endmask;
					break;

				case RROP_NOP:
					break;
			}
		}
		if (!miFillArcLower(slw))
			continue;
		addrl = ilbmScanlineOffset(addrlb, (xpos >> PWSH));
		for (d = 0; d < depthDst; d++, addrl += nlwidth) {	/* @@@ NEXT PLANE @@@ */
			n = nlmiddle;
			pdst = addrl;

			switch (rrops[d]) {
				case RROP_BLACK:
					if (startmask)
						*pdst++ &= ~startmask;
					while (n--)
						*pdst++ = 0;
					if (endmask)
						*pdst &= ~endmask;
					break;

				case RROP_WHITE:
					if (startmask)
						*pdst++ |= startmask;
					while (n--)
						*pdst++ = ~0;
					if (endmask)
						*pdst |= endmask;
					break;

				case RROP_INVERT:
					if (startmask)
						*pdst++ ^= startmask;
					while (n--)
						*pdst++ ^= ~0;
					if (endmask)
						*pdst ^= endmask;
					break;

				case RROP_NOP:
					break;
			}
		}
	}
}

#define FILLSPAN(xl,xr,addr) \
	if (xr >= xl) { \
		width = xr - xl + 1; \
		addrl = ilbmScanlineOffset(addr, (xl >> PWSH)); \
		if (((xl & PIM) + width) < PPW) { \
			maskpartialbits(xl, width, startmask); \
			for (pdst = addrl, d = 0; d < depthDst; d++, pdst += nlwidth) { /* @@@ NEXT PLANE @@@ */ \
				switch (rrops[d]) { \
					case RROP_BLACK: \
						*pdst &= ~startmask; \
						break; \
					case RROP_WHITE: \
						*pdst |= startmask; \
						break; \
					case RROP_INVERT: \
						*pdst ^= startmask; \
						break; \
					case RROP_NOP: \
						break; \
				} \
			} \
		} else { \
			maskbits(xl, width, startmask, endmask, nlmiddle); \
			for (d = 0; d < depthDst; d++, addrl += nlwidth) {	/* @@@ NEXT PLANE @@@ */ \
				n = nlmiddle; \
				pdst = addrl; \
				switch (rrops[d]) { \
					case RROP_BLACK: \
						if (startmask) \
							*pdst++ &= ~startmask; \
						while (n--) \
							*pdst++ = 0; \
						if (endmask) \
							*pdst &= ~endmask; \
						break; \
					case RROP_WHITE: \
						if (startmask) \
							*pdst++ |= startmask; \
						while (n--) \
							*pdst++ = ~0; \
						if (endmask) \
							*pdst |= endmask; \
						break; \
					case RROP_INVERT: \
						if (startmask) \
							*pdst++ ^= startmask; \
						while (n--) \
							*pdst++ ^= ~0; \
						if (endmask) \
							*pdst ^= endmask; \
						break; \
					case RROP_NOP: \
						break; \
				} \
			} \
		} \
	}

#define FILLSLICESPANS(flip,addr) \
	if (!flip) { \
		FILLSPAN(xl, xr, addr); \
	} else { \
		xc = xorg - x; \
		FILLSPAN(xc, xr, addr); \
		xc += slw - 1; \
		FILLSPAN(xl, xc, addr); \
	}

static void
ilbmFillArcSliceSolidCopy(pDraw, pGC, arc, rrops)
	DrawablePtr pDraw;
	GCPtr pGC;
	xArc *arc;
	register unsigned char *rrops;
{
	PixelType *addrl;
	register PixelType *pdst;
	register int n;
	register int d;
	int yk, xk, ym, xm, dx, dy, xorg, yorg, slw;
	register int x, y, e;
	miFillArcRec info;
	miArcSliceRec slice;
	int xl, xr, xc;
	PixelType *addrlt, *addrlb;
	int nlwidth;
	int width;
	PixelType startmask, endmask;
	int nlmiddle;
	int auxDst;
	int depthDst;

	ilbmGetPixelWidthAuxDepthAndPointer(pDraw, nlwidth, auxDst, depthDst,
													addrlt);
	miFillArcSetup(arc, &info);
	miFillArcSliceSetup(arc, &slice, pGC);
	MIFILLARCSETUP();
	xorg += pDraw->x;
	yorg += pDraw->y;
	addrlb = addrlt;
	addrlt = ilbmScanlineDeltaNoBankSwitch(addrlt, yorg - y, auxDst);
	addrlb = ilbmScanlineDeltaNoBankSwitch(addrlb, yorg + y + dy, auxDst);
	slice.edge1.x += pDraw->x;
	slice.edge2.x += pDraw->x;
	while (y > 0) {
		ilbmScanlineIncNoBankSwitch(addrlt, auxDst);
		ilbmScanlineIncNoBankSwitch(addrlb, -auxDst);
		MIFILLARCSTEP(slw);
		MIARCSLICESTEP(slice.edge1);
		MIARCSLICESTEP(slice.edge2);
		if (miFillSliceUpper(slice)) {
			MIARCSLICEUPPER(xl, xr, slice, slw);
			FILLSLICESPANS(slice.flip_top, addrlt);
		}
		if (miFillSliceLower(slice)) {
			MIARCSLICELOWER(xl, xr, slice, slw);
			FILLSLICESPANS(slice.flip_bot, addrlb);
		}
	}
}

void
ilbmPolyFillArcSolid(pDraw, pGC, narcs, parcs)
	register DrawablePtr pDraw;
	GCPtr		pGC;
	int				narcs;
	xArc		*parcs;
{
	ilbmPrivGC *priv;
	register xArc *arc;
	register int i;
	int x2, y2;
	BoxRec box;
	RegionPtr cclip;
	unsigned char *rrops;

	priv = (ilbmPrivGC *) pGC->devPrivates[ilbmGCPrivateIndex].ptr;
	rrops = priv->rrops;
	cclip = pGC->pCompositeClip;
	for (arc = parcs, i = narcs; --i >= 0; arc++) {
		if (miFillArcEmpty(arc))
			continue;
		if (miCanFillArc(arc)) {
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
				(RECT_IN_REGION(pDraw->pScreen, cclip, &box) == rgnIN) ) {
				if ((arc->angle2 >= FULLCIRCLE) ||
					(arc->angle2 <= -FULLCIRCLE))
					ilbmFillEllipseSolid(pDraw, arc, rrops);
				else
					ilbmFillArcSliceSolidCopy(pDraw, pGC, arc, rrops);
				continue;
			}
		}
		miPolyFillArc(pDraw, pGC, 1, arc);
	}
}
