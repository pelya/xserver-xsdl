/* $XFree86: xc/programs/Xserver/ilbm/ilbmpolypnt.c,v 3.1 1998/03/20 21:08:03 hohndel Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
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
/* $XConsortium: ilbmpolypnt.c,v 5.6 94/04/17 20:28:30 dpw Exp $ */

/* Modified jun 95 by Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
   to use interleaved bitplanes instead of normal bitplanes */

#include "X.h"
#include "Xprotostr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "miscstruct.h"
#include "regionstr.h"
#include "scrnintstr.h"

#include "ilbm.h"
#include "maskbits.h"

void
ilbmPolyPoint(pDrawable, pGC, mode, npt, pptInit)
	register DrawablePtr pDrawable;
	GCPtr pGC;
	int mode;				/* Origin or Previous */
	int npt;
	xPoint *pptInit;
{

	register BoxPtr pbox;
	register int nbox;
	register int d;

	register PixelType *addrl;
	PixelType *pBase;
	PixelType *pBaseSave;
	int nlwidth;
	int auxDst;
	int depthDst;

	int nptTmp;
	register xPoint *ppt;

	register int x;
	register int y;
	register unsigned char *rrops;
	ilbmPrivGC *pGCPriv;

	pGCPriv = (ilbmPrivGC *) pGC->devPrivates[ilbmGCPrivateIndex].ptr;

	ilbmGetPixelWidthAuxDepthAndPointer(pDrawable, nlwidth, auxDst, depthDst,
													pBaseSave);

	rrops = pGCPriv->rrops;
	if ((mode == CoordModePrevious) && (npt > 1))
		for (ppt = pptInit + 1, nptTmp = npt - 1; --nptTmp >= 0; ppt++) {
			ppt->x += (ppt-1)->x;
			ppt->y += (ppt-1)->y;
		}

	nbox = REGION_NUM_RECTS(pGC->pCompositeClip);
	pbox = REGION_RECTS(pGC->pCompositeClip);
	for (; --nbox >= 0; pbox++)
		for (d = 0, pBase = pBaseSave; d < depthDst; d++, pBase += nlwidth) {	/* @@@ NEXT PLANE @@@ */
			addrl = pBase;

			switch (rrops[d]) {
				case RROP_BLACK:
					for (ppt = pptInit, nptTmp = npt; --nptTmp >= 0; ppt++) {
						x = ppt->x + pDrawable->x;
						y = ppt->y + pDrawable->y;
						if ((x >= pbox->x1) && (x < pbox->x2) &&
							(y >= pbox->y1) && (y < pbox->y2))
							*ilbmScanline(addrl, x, y, auxDst) &= rmask[x & PIM];
					}
					break;

				case RROP_WHITE:
					for (ppt = pptInit, nptTmp = npt; --nptTmp >= 0; ppt++) {
						x = ppt->x + pDrawable->x;
						y = ppt->y + pDrawable->y;
						if ((x >= pbox->x1) && (x < pbox->x2) &&
							(y >= pbox->y1) && (y < pbox->y2))
							*ilbmScanline(addrl, x, y, auxDst) |= mask[x & PIM];
					}
					break;

				case RROP_INVERT:
					for (ppt = pptInit, nptTmp = npt; --nptTmp >= 0; ppt++) {
						x = ppt->x + pDrawable->x;
						y = ppt->y + pDrawable->y;
						if ((x >= pbox->x1) && (x < pbox->x2) &&
							(y >= pbox->y1) && (y < pbox->y2))
							*ilbmScanline(addrl, x, y, auxDst) ^= mask[x & PIM];
					}
					break;

				case RROP_NOP:
					break;
			} /* switch */
		} /* for (d = ...) */
}
