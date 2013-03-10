/*
 *
 * Copyright © 2013 Geert Uytterhoeven
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Geert Uytterhoeven not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Geert Uytterhoeven makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * GEERT UYTTERHOEVEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL GEERT UYTTERHOEVEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Based on shpacked.c, which is Copyright © 2000 Keith Packard
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    <X11/fonts/font.h>
#include    "dixfontstr.h"
#include    <X11/fonts/fontstruct.h>
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"
#include    "fb.h"
#include    "c2p_core.h"


    /*
     *  Perform a full C2P step on 16 8-bit pixels, stored in 4 32-bit words
     *  containing
     *    - 16 8-bit chunky pixels on input
     *    - permutated planar data (2 planes per 32-bit word) on output
     */

static void c2p_16x8(CARD32 d[4])
{
    transp4(d, 8, 2);
    transp4(d, 1, 2);
    transp4x(d, 16, 2);
    transp4x(d, 2, 2);
    transp4(d, 4, 1);
}


    /*
     *  Store a full block of permutated iplan2p8 data after c2p conversion
     */

static inline void store_iplan2p8(void *dst, const CARD32 d[4])
{
    CARD32 *p = dst;

    *p++ = d[1];
    *p++ = d[3];
    *p++ = d[0];
    *p++ = d[2];
}


void
shadowUpdateIplan2p8(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    RegionPtr damage = shadowDamage(pBuf);
    PixmapPtr pShadow = pBuf->pPixmap;
    int nbox = RegionNumRects(damage);
    BoxPtr pbox = RegionRects(damage);
    FbBits *shaBase;
    CARD16 *shaLine, *sha;
    FbStride shaStride;
    int scrLine;
    _X_UNUSED int shaBpp, shaXoff, shaYoff;
    int x, y, w, h;
    int i, n;
    CARD16 *win;
    _X_UNUSED CARD32 winSize;
    union {
        CARD8 bytes[16];
        CARD32 words[4];
    } d;

    fbGetDrawable(&pShadow->drawable, shaBase, shaStride, shaBpp, shaXoff,
                  shaYoff);
    shaStride *= sizeof(FbBits) / sizeof(CARD16);

    while (nbox--) {
        x = pbox->x1;
        y = pbox->y1;
        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;

        scrLine = x & -16;
        shaLine = (CARD16 *)shaBase + y * shaStride + scrLine / sizeof(CARD16);

        n = ((x & 15) + w + 15) / 16;   /* number of c2p units in scanline */

        while (h--) {
            sha = shaLine;
            win = (CARD16 *) (*pBuf->window) (pScreen,
                                              y,
                                              scrLine,
                                              SHADOW_WINDOW_WRITE,
                                              &winSize,
                                              pBuf->closure);
            if (!win)
                return;
            for (i = 0; i < n; i++) {
                memcpy(d.bytes, sha, sizeof(d.bytes));
                c2p_16x8(d.words);
                store_iplan2p8(win, d.words);
                sha += sizeof(d.bytes) / sizeof(*sha);
                win += sizeof(d.bytes) / sizeof(*win);
            }
            shaLine += shaStride;
            y++;
        }
        pbox++;
    }
}
