/*
 * $RCSId: $
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"
#include <picturestr.h>
#include <mipict.h>
#include <fbpict.h>
#include "kaa.h"

void
KdCheckComposite (CARD8      op,
		  PicturePtr pSrc,
		  PicturePtr pMask,
		  PicturePtr pDst,
		  INT16      xSrc,
		  INT16      ySrc,
		  INT16      xMask,
		  INT16      yMask,
		  INT16      xDst,
		  INT16      yDst,
		  CARD16     width,
		  CARD16     height)
{
    KdCheckSync (pDst->pDrawable->pScreen);
    kaaDrawableDirty (pDst->pDrawable);
    fbComposite (op,
		 pSrc,
		 pMask,
		 pDst,
		 xSrc,
		 ySrc,
		 xMask,
		 yMask,
		 xDst,
		 yDst,
		 width,
		 height);
}

void
KdCheckRasterizeTrapezoid(PicturePtr	pMask,
			  xTrapezoid	*trap,
			  int		x_off,
			  int		y_off)
{
    KdCheckSync (pMask->pDrawable->pScreen);
    kaaDrawableDirty (pMask->pDrawable);
    fbRasterizeTrapezoid (pMask, trap, x_off, y_off);
}

void
KdPictureInitAsync (ScreenPtr pScreen)
{
    PictureScreenPtr    ps;

    ps = GetPictureScreen(pScreen);
    ps->Composite = KdCheckComposite;
    ps->RasterizeTrapezoid = KdCheckRasterizeTrapezoid;
}
