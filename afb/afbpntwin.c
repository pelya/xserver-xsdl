/* $XFree86$ */
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "afb.h"
#include "maskbits.h"
#include "mi.h"

void
afbPaintWindow(pWin, pRegion, what)
	WindowPtr		pWin;
	RegionPtr		pRegion;
	int				what;
{
	register afbPrivWin		*pPrivWin;
	unsigned char rrops[AFB_MAX_DEPTH];

	pPrivWin = (afbPrivWin *)(pWin->devPrivates[afbWindowPrivateIndex].ptr);

	switch (what) {
		case PW_BACKGROUND:
			switch (pWin->backgroundState) {
				case None:
					return;
				case ParentRelative:
					do {
						pWin = pWin->parent;
					} while (pWin->backgroundState == ParentRelative);
					(*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
																					 what);
					return;
				case BackgroundPixmap:
					if (pPrivWin->fastBackground) {
						afbTileAreaPPWCopy((DrawablePtr)pWin,
												  REGION_NUM_RECTS(pRegion),
												  REGION_RECTS(pRegion), GXcopy,
												  pPrivWin->pRotatedBackground, ~0);
						return;
					} else {
						afbTileAreaCopy((DrawablePtr)pWin, REGION_NUM_RECTS(pRegion),
											  REGION_RECTS(pRegion), GXcopy,
											  pWin->background.pixmap, 0, 0, ~0);
						return;
					}
					break;
				case BackgroundPixel:
					afbReduceRop(GXcopy, pWin->background.pixel, ~0,
									  pWin->drawable.depth, rrops);
					afbSolidFillArea((DrawablePtr)pWin, REGION_NUM_RECTS(pRegion),
											REGION_RECTS(pRegion), rrops);
					return;
			}
			break;
		case PW_BORDER:
			if (pWin->borderIsPixel) {
				afbReduceRop(GXcopy, pWin->border.pixel, ~0, pWin->drawable.depth,
								  rrops);
				afbSolidFillArea((DrawablePtr)pWin, REGION_NUM_RECTS(pRegion),
										REGION_RECTS(pRegion), rrops);
				return;
			} else if (pPrivWin->fastBorder) {
				afbTileAreaPPWCopy((DrawablePtr)pWin, REGION_NUM_RECTS(pRegion),
										  REGION_RECTS(pRegion), GXcopy,
										  pPrivWin->pRotatedBorder, ~0);
				return;
			}
			break;
	}
	miPaintWindow(pWin, pRegion, what);
}
