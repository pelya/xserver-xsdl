/*
 * Specialized window functions to protect the alpha channel
 */
/*
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* Portions of this file are based on fbwindow.c, which contains the
 * following copyright:
 *
 * Copyright © 1998 Keith Packard
 */
/* $XFree86: xc/programs/Xserver/miext/rootless/safeAlpha/safeAlphaWindow.c,v 1.2 2004/01/19 01:22:48 torrey Exp $ */

#include "fb.h"
#include "safeAlpha.h"
#include "rootlessCommon.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

/*
 * SafeAlphaFillRegionTiled
 *  Fill using a tile while leaving the alpha channel untouched.
 *  Based on fbfillRegionTiled.
 */
void
SafeAlphaFillRegionTiled(
    DrawablePtr pDrawable,
    RegionPtr   pRegion,
    PixmapPtr   pTile)
{
    FbBits      *dst;
    FbStride    dstStride;
    int         dstBpp;
    int         dstXoff, dstYoff;
    FbBits      *tile;
    FbStride    tileStride;
    int         tileBpp;
    int         tileXoff, tileYoff; /* XXX assumed to be zero */
    int         tileWidth, tileHeight;
    int         n = REGION_NUM_RECTS(pRegion);
    BoxPtr      pbox = REGION_RECTS(pRegion);
    int         xRot = pDrawable->x;
    int         yRot = pDrawable->y;
    FbBits      planeMask;

#ifdef PANORAMIX
    if(!noPanoramiXExtension)
    {
        int index = pDrawable->pScreen->myNum;
        if(&WindowTable[index]->drawable == pDrawable)
        {
            xRot -= panoramiXdataPtr[index].x;
            yRot -= panoramiXdataPtr[index].y;
        }
    }
#endif
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    fbGetDrawable (&pTile->drawable, tile, tileStride, tileBpp,
                   tileXoff, tileYoff);
    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;
    xRot += dstXoff;
    yRot += dstYoff;
    planeMask = FB_ALLONES & ~RootlessAlphaMask(dstBpp);

    while (n--)
    {
        fbTile (dst + (pbox->y1 + dstYoff) * dstStride,
                dstStride,
                (pbox->x1 + dstXoff) * dstBpp,
                (pbox->x2 - pbox->x1) * dstBpp,
                pbox->y2 - pbox->y1,
                tile,
                tileStride,
                tileWidth * dstBpp,
                tileHeight,
                GXcopy,
                planeMask,
                dstBpp,
                xRot * dstBpp,
                yRot - pbox->y1);
        pbox++;
    }
}


/*
 * SafeAlphaPaintWindow
 *  Paint the window while filling in the alpha channel with all on.
 *  We can't use fbPaintWindow because it zeros the alpha channel.
 */
void
SafeAlphaPaintWindow(
    WindowPtr pWin,
    RegionPtr pRegion,
    int what)
{
    switch (what) {
      case PW_BACKGROUND:

        switch (pWin->backgroundState) {
            case None:
                break;
            case ParentRelative:
                do {
                    pWin = pWin->parent;
                } while (pWin->backgroundState == ParentRelative);
                (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
                                                                 what);
                break;
            case BackgroundPixmap:
                SafeAlphaFillRegionTiled (&pWin->drawable,
                                          pRegion,
                                          pWin->background.pixmap);
                break;
            case BackgroundPixel:
            {
                Pixel pixel = pWin->background.pixel |
                              RootlessAlphaMask(pWin->drawable.bitsPerPixel);
                fbFillRegionSolid (&pWin->drawable, pRegion, 0,
                                   fbReplicatePixel (pixel,
                                        pWin->drawable.bitsPerPixel));
                break;
            }
        }
    	break;
      case PW_BORDER:
        if (pWin->borderIsPixel)
        {
            Pixel pixel = pWin->border.pixel |
                          RootlessAlphaMask(pWin->drawable.bitsPerPixel);
            fbFillRegionSolid (&pWin->drawable, pRegion, 0,
                               fbReplicatePixel (pixel,
                                    pWin->drawable.bitsPerPixel));
        }
        else
        {
            WindowPtr pBgWin;
            for (pBgWin = pWin; pBgWin->backgroundState == ParentRelative;
                 pBgWin = pBgWin->parent);
    
            SafeAlphaFillRegionTiled (&pBgWin->drawable,
                                      pRegion,
                                      pWin->border.pixmap);
        }
        break;
    }
    fbValidateDrawable (&pWin->drawable);
}
